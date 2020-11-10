#include <QFutureWatcher>
#include <QMenu>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include "texteditor/texteditoractionhandler.h"

#include <utils/algorithm.h>
#include <utils/differ.h>
#include <utils/mapreduce.h>
using namespace Core;
using namespace TextEditor;
using namespace Utils;

class ReloadInput {
public:
    QString leftText;
    QString rightText;
    DiffFileInfo leftFileInfo;
    DiffFileInfo rightFileInfo;
    FileData::FileOperation fileOperation = FileData::ChangeFile;
    bool binaryFiles = false;
};

class DiffFile
{
public:
    DiffFile(bool ignoreWhitespace, int contextLineCount)
        : m_contextLineCount(contextLineCount),
          m_ignoreWhitespace(ignoreWhitespace)
    {}

    void operator()(QFutureInterface<FileData> &futureInterface,
                    const ReloadInput &reloadInfo) const
    {
        if (reloadInfo.leftText == reloadInfo.rightText)
            return; // We show "No difference" in this case, regardless if it's binary or not

        Differ differ(&futureInterface);

        FileData fileData;
        if (!reloadInfo.binaryFiles) {
            const QList<Diff> diffList = differ.cleanupSemantics(
                        differ.diff(reloadInfo.leftText, reloadInfo.rightText));

            QList<Diff> leftDiffList;
            QList<Diff> rightDiffList;
            Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
            QList<Diff> outputLeftDiffList;
            QList<Diff> outputRightDiffList;

            if (m_ignoreWhitespace) {
                const QList<Diff> leftIntermediate
                        = Differ::moveWhitespaceIntoEqualities(leftDiffList);
                const QList<Diff> rightIntermediate
                        = Differ::moveWhitespaceIntoEqualities(rightDiffList);
                Differ::ignoreWhitespaceBetweenEqualities(leftIntermediate, rightIntermediate,
                                                          &outputLeftDiffList, &outputRightDiffList);
            } else {
                outputLeftDiffList = leftDiffList;
                outputRightDiffList = rightDiffList;
            }

            const ChunkData chunkData = DiffUtils::calculateOriginalData(
                        outputLeftDiffList, outputRightDiffList);
            fileData = DiffUtils::calculateContextData(chunkData, m_contextLineCount, 0);
        }
        fileData.leftFileInfo = reloadInfo.leftFileInfo;
        fileData.rightFileInfo = reloadInfo.rightFileInfo;
        fileData.fileOperation = reloadInfo.fileOperation;
        fileData.binaryFiles = reloadInfo.binaryFiles;
        futureInterface.reportResult(fileData);
    }

private:
    const int m_contextLineCount;
    const bool m_ignoreWhitespace;
};

class DiffFilesController : public DiffEditorController
{
    Q_OBJECT
public:
    DiffFilesController(IDocument *document);
    ~DiffFilesController() override { cancelReload(); }

protected:
    virtual QList<ReloadInput> reloadInputList() const = 0;

private:
    void reloaded();
    void cancelReload();

    QFutureWatcher<FileData> m_futureWatcher;
};

DiffFilesController::DiffFilesController(IDocument *document)
    : DiffEditorController(document)
{
    connect(&m_futureWatcher, &QFutureWatcher<FileData>::finished,
            this, &DiffFilesController::reloaded);

    setReloader([this] {
        cancelReload();
        m_futureWatcher.setFuture(Utils::map(reloadInputList(),
                                             DiffFile(ignoreWhitespace(),
                                                      contextLineCount())));

        Core::ProgressManager::addTask(m_futureWatcher.future(),
                                       tr("Calculating diff"), "DiffEditor");
    });
}

void DiffFilesController::reloaded()
{
    const bool success = !m_futureWatcher.future().isCanceled();
    const QList<FileData> fileDataList = success
            ? m_futureWatcher.future().results() : QList<FileData>();

    setDiffFiles(fileDataList);
    reloadFinished(success);
}

void DiffFilesController::cancelReload()
{
    if (m_futureWatcher.future().isRunning()) {
        m_futureWatcher.future().cancel();
        m_futureWatcher.setFuture(QFuture<FileData>());
    }
}

class DiffCurrentFileController : public DiffFilesController
    DiffCurrentFileController(IDocument *document, const QString &fileName);

protected:
    QList<ReloadInput> reloadInputList() const final;

private:
    const QString m_fileName;
};

DiffCurrentFileController::DiffCurrentFileController(IDocument *document, const QString &fileName) :
    DiffFilesController(document), m_fileName(fileName)
{ }

QList<ReloadInput> DiffCurrentFileController::reloadInputList() const
{
    QList<ReloadInput> result;

    auto textDocument = qobject_cast<TextEditor::TextDocument *>(
        DocumentModel::documentForFilePath(Utils::FilePath::fromString(m_fileName)));

    if (textDocument && textDocument->isModified()) {
        QString errorString;
        Utils::TextFileFormat format = textDocument->format();

        QString leftText;
        const Utils::TextFileFormat::ReadResult leftResult
                = Utils::TextFileFormat::readFile(m_fileName, format.codec,
                                        &leftText, &format, &errorString);

        const QString rightText = textDocument->plainText();

        ReloadInput reloadInput;
        reloadInput.leftText = leftText;
        reloadInput.rightText = rightText;
        reloadInput.leftFileInfo.fileName = m_fileName;
        reloadInput.rightFileInfo.fileName = m_fileName;
        reloadInput.leftFileInfo.typeInfo = tr("Saved");
        reloadInput.rightFileInfo.typeInfo = tr("Modified");
        reloadInput.rightFileInfo.patchBehaviour = DiffFileInfo::PatchEditor;
        reloadInput.binaryFiles = (leftResult == Utils::TextFileFormat::ReadEncodingError);

        if (leftResult == Utils::TextFileFormat::ReadIOError)
            reloadInput.fileOperation = FileData::NewFile;

        result << reloadInput;
    }

    return result;
}

/////////////////

class DiffOpenFilesController : public DiffFilesController
{
    Q_OBJECT
public:
    DiffOpenFilesController(IDocument *document);

protected:
    QList<ReloadInput> reloadInputList() const final;
};

DiffOpenFilesController::DiffOpenFilesController(IDocument *document) :
    DiffFilesController(document)
{ }

QList<ReloadInput> DiffOpenFilesController::reloadInputList() const
{
    QList<ReloadInput> result;

    const QList<IDocument *> openedDocuments = DocumentModel::openedDocuments();

    for (IDocument *doc : openedDocuments) {
        auto textDocument = qobject_cast<TextEditor::TextDocument *>(doc);

        if (textDocument && textDocument->isModified()) {
            QString errorString;
            Utils::TextFileFormat format = textDocument->format();

            QString leftText;
            const QString fileName = textDocument->filePath().toString();
            const Utils::TextFileFormat::ReadResult leftResult
                    = Utils::TextFileFormat::readFile(fileName, format.codec,
                                            &leftText, &format, &errorString);

            const QString rightText = textDocument->plainText();

            ReloadInput reloadInput;
            reloadInput.leftText = leftText;
            reloadInput.rightText = rightText;
            reloadInput.leftFileInfo.fileName = fileName;
            reloadInput.rightFileInfo.fileName = fileName;
            reloadInput.leftFileInfo.typeInfo = tr("Saved");
            reloadInput.rightFileInfo.typeInfo = tr("Modified");
            reloadInput.rightFileInfo.patchBehaviour = DiffFileInfo::PatchEditor;
            reloadInput.binaryFiles = (leftResult == Utils::TextFileFormat::ReadEncodingError);

            if (leftResult == Utils::TextFileFormat::ReadIOError)
                reloadInput.fileOperation = FileData::NewFile;

            result << reloadInput;
        }
    }

    return result;
}

/////////////////

class DiffModifiedFilesController : public DiffFilesController
{
    Q_OBJECT
public:
    DiffModifiedFilesController(IDocument *document, const QStringList &fileNames);

protected:
    QList<ReloadInput> reloadInputList() const final;

private:
    const QStringList m_fileNames;
};

DiffModifiedFilesController::DiffModifiedFilesController(IDocument *document, const QStringList &fileNames) :
    DiffFilesController(document), m_fileNames(fileNames)
{ }

QList<ReloadInput> DiffModifiedFilesController::reloadInputList() const
{
    QList<ReloadInput> result;

    for (const QString &fileName : m_fileNames) {
        auto textDocument = qobject_cast<TextEditor::TextDocument *>(
            DocumentModel::documentForFilePath(Utils::FilePath::fromString(fileName)));

        if (textDocument && textDocument->isModified()) {
            QString errorString;
            Utils::TextFileFormat format = textDocument->format();

            QString leftText;
            const QString fileName = textDocument->filePath().toString();
            const Utils::TextFileFormat::ReadResult leftResult
                    = Utils::TextFileFormat::readFile(fileName, format.codec,
                                            &leftText, &format, &errorString);

            const QString rightText = textDocument->plainText();

            ReloadInput reloadInput;
            reloadInput.leftText = leftText;
            reloadInput.rightText = rightText;
            reloadInput.leftFileInfo.fileName = fileName;
            reloadInput.rightFileInfo.fileName = fileName;
            reloadInput.leftFileInfo.typeInfo = tr("Saved");
            reloadInput.rightFileInfo.typeInfo = tr("Modified");
            reloadInput.rightFileInfo.patchBehaviour = DiffFileInfo::PatchEditor;
            reloadInput.binaryFiles = (leftResult == Utils::TextFileFormat::ReadEncodingError);

            if (leftResult == Utils::TextFileFormat::ReadIOError)
                reloadInput.fileOperation = FileData::NewFile;

            result << reloadInput;
        }
    }

    return result;
}

/////////////////

class DiffExternalFilesController : public DiffFilesController
{
    Q_OBJECT
public:
    DiffExternalFilesController(IDocument *document, const QString &leftFileName,
    QList<ReloadInput> reloadInputList() const final;
    const QString m_leftFileName;
    const QString m_rightFileName;
DiffExternalFilesController::DiffExternalFilesController(IDocument *document, const QString &leftFileName,
    DiffFilesController(document), m_leftFileName(leftFileName), m_rightFileName(rightFileName)
QList<ReloadInput> DiffExternalFilesController::reloadInputList() const
    format.codec = EditorManager::defaultTextCodec();
    const Utils::TextFileFormat::ReadResult leftResult
            = Utils::TextFileFormat::readFile(m_leftFileName, format.codec,
                                    &leftText, &format, &errorString);
    const Utils::TextFileFormat::ReadResult rightResult
            = Utils::TextFileFormat::readFile(m_rightFileName, format.codec,
                                    &rightText, &format, &errorString);

    ReloadInput reloadInput;
    reloadInput.leftText = leftText;
    reloadInput.rightText = rightText;
    reloadInput.leftFileInfo.fileName = m_leftFileName;
    reloadInput.rightFileInfo.fileName = m_rightFileName;
    reloadInput.binaryFiles = (leftResult == Utils::TextFileFormat::ReadEncodingError
            || rightResult == Utils::TextFileFormat::ReadEncodingError);

    const bool leftFileExists = (leftResult != Utils::TextFileFormat::ReadIOError);
    const bool rightFileExists = (rightResult != Utils::TextFileFormat::ReadIOError);
    if (!leftFileExists && rightFileExists)
        reloadInput.fileOperation = FileData::NewFile;
    else if (leftFileExists && !rightFileExists)
        reloadInput.fileOperation = FileData::DeleteFile;

    QList<ReloadInput> result;
    if (leftFileExists || rightFileExists)
        result << reloadInput;

    return result;
}
/////////////////
static TextEditor::TextDocument *currentTextDocument()
{
    return qobject_cast<TextEditor::TextDocument *>(
                EditorManager::currentDocument());
DiffEditorServiceImpl::DiffEditorServiceImpl() = default;
void DiffEditorServiceImpl::diffFiles(const QString &leftFileName, const QString &rightFileName)
    const QString documentId = Constants::DIFF_EDITOR_PLUGIN
            + QLatin1String(".DiffFiles.") + leftFileName + QLatin1Char('.') + rightFileName;
    const QString title = tr("Diff Files");
    auto const document = qobject_cast<DiffEditorDocument *>(
                DiffEditorController::findOrCreateDocument(documentId, title));
    if (!document)
        return;

    if (!DiffEditorController::controller(document))
        new DiffExternalFilesController(document, leftFileName, rightFileName);
    EditorManager::activateEditorForDocument(document);
    document->reload();
}

void DiffEditorServiceImpl::diffModifiedFiles(const QStringList &fileNames)
{
    const QString documentId = Constants::DIFF_EDITOR_PLUGIN
            + QLatin1String(".DiffModifiedFiles");
    const QString title = tr("Diff Modified Files");
    auto const document = qobject_cast<DiffEditorDocument *>(
                DiffEditorController::findOrCreateDocument(documentId, title));
    if (!document)
        return;

    if (!DiffEditorController::controller(document))
        new DiffModifiedFilesController(document, fileNames);
    EditorManager::activateEditorForDocument(document);
    document->reload();
}

class DiffEditorPluginPrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(DiffEditor::Internal::DiffEditorPlugin)

public:
    DiffEditorPluginPrivate();
    void updateDiffCurrentFileAction();
    void updateDiffOpenFilesAction();
    void diffCurrentFile();
    void diffOpenFiles();
    void diffExternalFiles();

    QAction *m_diffCurrentFileAction = nullptr;
    QAction *m_diffOpenFilesAction = nullptr;

    DiffEditorFactory editorFactory;
    DiffEditorServiceImpl service;
};

DiffEditorPluginPrivate::DiffEditorPluginPrivate()
{
    ActionContainer *toolsContainer
            = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *diffContainer = ActionManager::createMenu("Diff");
    diffContainer->menu()->setTitle(tr("&Diff"));
    toolsContainer->addMenu(diffContainer, Constants::G_TOOLS_DIFF);

    m_diffCurrentFileAction = new QAction(tr("Diff Current File"), this);
    Command *diffCurrentFileCommand = ActionManager::registerAction(m_diffCurrentFileAction, "DiffEditor.DiffCurrentFile");
    diffCurrentFileCommand->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+H") : tr("Ctrl+H")));
    connect(m_diffCurrentFileAction, &QAction::triggered, this, &DiffEditorPluginPrivate::diffCurrentFile);
    diffContainer->addAction(diffCurrentFileCommand);

    m_diffOpenFilesAction = new QAction(tr("Diff Open Files"), this);
    Command *diffOpenFilesCommand = ActionManager::registerAction(m_diffOpenFilesAction, "DiffEditor.DiffOpenFiles");
    diffOpenFilesCommand->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+Shift+H") : tr("Ctrl+Shift+H")));
    connect(m_diffOpenFilesAction, &QAction::triggered, this, &DiffEditorPluginPrivate::diffOpenFiles);
    diffContainer->addAction(diffOpenFilesCommand);

    QAction *diffExternalFilesAction = new QAction(tr("Diff External Files..."), this);
    Command *diffExternalFilesCommand = ActionManager::registerAction(diffExternalFilesAction, "DiffEditor.DiffExternalFiles");
    connect(diffExternalFilesAction, &QAction::triggered, this, &DiffEditorPluginPrivate::diffExternalFiles);
    diffContainer->addAction(diffExternalFilesCommand);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &DiffEditorPluginPrivate::updateDiffCurrentFileAction);
    connect(EditorManager::instance(), &EditorManager::currentDocumentStateChanged,
        this, &DiffEditorPluginPrivate::updateDiffCurrentFileAction);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
        this, &DiffEditorPluginPrivate::updateDiffOpenFilesAction);
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
        this, &DiffEditorPluginPrivate::updateDiffOpenFilesAction);
    connect(EditorManager::instance(), &EditorManager::documentStateChanged,
        this, &DiffEditorPluginPrivate::updateDiffOpenFilesAction);

    updateDiffCurrentFileAction();
    updateDiffOpenFilesAction();
}

void DiffEditorPluginPrivate::updateDiffCurrentFileAction()
{
    auto textDocument = currentTextDocument();
    const bool enabled = textDocument && textDocument->isModified();
    m_diffCurrentFileAction->setEnabled(enabled);
}
void DiffEditorPluginPrivate::updateDiffOpenFilesAction()
{
    const bool enabled = Utils::anyOf(DocumentModel::openedDocuments(), [](IDocument *doc) {
            return doc->isModified() && qobject_cast<TextEditor::TextDocument *>(doc);
        });
    m_diffOpenFilesAction->setEnabled(enabled);
}
void DiffEditorPluginPrivate::diffCurrentFile()
{
    auto textDocument = currentTextDocument();
    if (!textDocument)
        return;
    const QString fileName = textDocument->filePath().toString();

    if (fileName.isEmpty())
        return;

    const QString documentId = Constants::DIFF_EDITOR_PLUGIN
            + QLatin1String(".Diff.") + fileName;
    const QString title = tr("Diff \"%1\"").arg(fileName);
    auto const document = qobject_cast<DiffEditorDocument *>(
                DiffEditorController::findOrCreateDocument(documentId, title));
    if (!document)
        return;

    if (!DiffEditorController::controller(document))
        new DiffCurrentFileController(document, fileName);
    EditorManager::activateEditorForDocument(document);
    document->reload();
void DiffEditorPluginPrivate::diffOpenFiles()
{
    const QString documentId = Constants::DIFF_EDITOR_PLUGIN
            + QLatin1String(".DiffOpenFiles");
    const QString title = tr("Diff Open Files");
    auto const document = qobject_cast<DiffEditorDocument *>(
                DiffEditorController::findOrCreateDocument(documentId, title));
    if (!document)
        return;

    if (!DiffEditorController::controller(document))
        new DiffOpenFilesController(document);
    EditorManager::activateEditorForDocument(document);
    document->reload();
}
void DiffEditorPluginPrivate::diffExternalFiles()
    const QString fileName1 = QFileDialog::getOpenFileName(ICore::dialogParent(),
    if (EditorManager::skipOpeningBigTextFile(fileName1))
        return;
    const QString fileName2 = QFileDialog::getOpenFileName(ICore::dialogParent(),
    if (EditorManager::skipOpeningBigTextFile(fileName2))
        return;
    const QString documentId = Constants::DIFF_EDITOR_PLUGIN
            + QLatin1String(".DiffExternalFiles.") + fileName1 + QLatin1Char('.') + fileName2;
    const QString title = tr("Diff \"%1\", \"%2\"").arg(fileName1, fileName2);
        new DiffExternalFilesController(document, fileName1, fileName2);
    EditorManager::activateEditorForDocument(document);
DiffEditorPlugin::~DiffEditorPlugin()
{
    delete d;
}

bool DiffEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    d = new DiffEditorPluginPrivate;

    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    return true;
}

Q_DECLARE_METATYPE(DiffEditor::ChunkSelection)
    const QString fileName = "a.txt";
    const QString header = "--- " + fileName + "\n+++ " + fileName + '\n';
    QString patchText = header + "@@ -1,2 +1,1 @@\n"
                                 "-ABCD\n"
                                 " EFGH\n";
    patchText = header + "@@ -1,2 +1,1 @@\n"
                         "-ABCD\n"
                         " EFGH\n"
                         "\\ No newline at end of file\n";
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "+ABCD\n"
                         "\\ No newline at end of file\n";
    patchText = header + "@@ -1,2 +1,1 @@\n"
                         " ABCD\n"
                         "-\n";
    patchText = header + "@@ -1,2 +1,1 @@\n"
                         "-ABCD\n"
                         "-\n"
                         "+ABCD\n"
                         "\\ No newline at end of file\n";
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "\\ No newline at end of file\n"
                         "+ABCD\n";
    patchText = header + "@@ -1,1 +1,2 @@\n"
                         " ABCD\n"
                         "+\n";
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "+EFGH\n";
    patchText = header + "@@ -1,2 +1,2 @@\n"
                         "-ABCD\n"
                         "+EFGH\n"
                         " \n";
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "\\ No newline at end of file\n"
                         "+EFGH\n"
                         "\\ No newline at end of file\n";
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "+EFGH\n";
    patchText = header + "@@ -1,2 +1,2 @@\n"
                         "-ABCD\n"
                         "+EFGH\n"
                         " IJKL\n"
                         "\\ No newline at end of file\n";
    patchText = header + "@@ -1,2 +1,2 @@\n"
                         "-ABCD\n"
                         "+EFGH\n"
                         " IJKL\n";
    patchText = header + "@@ -1,1 +1,3 @@\n"
                         " ABCD\n"
                         "+\n"
                         "+EFGH\n"
                         "\\ No newline at end of file\n";
    const QString fileName = "a.txt";
    const QString result = DiffUtils::makePatch(sourceChunk, fileName, fileName, lastChunk);
        QCOMPARE(resultFileData.leftFileInfo.fileName, fileName);
        QCOMPARE(resultFileData.rightFileInfo.fileName, fileName);
    QString patch = "diff --git a/src/plugins/diffeditor/diffeditor.cpp b/src/plugins/diffeditor/diffeditor.cpp\n"
                    "index eab9e9b..082c135 100644\n"
                    "--- a/src/plugins/diffeditor/diffeditor.cpp\n"
                    "+++ b/src/plugins/diffeditor/diffeditor.cpp\n"
                    "@@ -187,9 +187,6 @@ void DiffEditor::ctor()\n"
                    "     m_controller = m_document->controller();\n"
                    "     m_guiController = new DiffEditorGuiController(m_controller, this);\n"
                    " \n"
                    "-//    m_sideBySideEditor->setDiffEditorGuiController(m_guiController);\n"
                    "-//    m_unifiedEditor->setDiffEditorGuiController(m_guiController);\n"
                    "-\n"
                    "     connect(m_controller, SIGNAL(cleared(QString)),\n"
                    "             this, SLOT(slotCleared(QString)));\n"
                    "     connect(m_controller, SIGNAL(diffContentsChanged(QList<DiffEditorController::DiffFilesContents>,QString)),\n"
                    "diff --git a/src/plugins/diffeditor/diffutils.cpp b/src/plugins/diffeditor/diffutils.cpp\n"
                    "index 2f641c9..f8ff795 100644\n"
                    "--- a/src/plugins/diffeditor/diffutils.cpp\n"
                    "+++ b/src/plugins/diffeditor/diffutils.cpp\n"
                    "@@ -464,5 +464,12 @@ QString DiffUtils::makePatch(const ChunkData &chunkData,\n"
                    "     return diffText;\n"
                    " }\n"
                    " \n"
                    "+FileData DiffUtils::makeFileData(const QString &patch)\n"
                    "+{\n"
                    "+    FileData fileData;\n"
                    "+\n"
                    "+    return fileData;\n"
                    "+}\n"
                    "+\n"
                    " } // namespace Internal\n"
                    " } // namespace DiffEditor\n"
                    "diff --git a/new b/new\n"
                    "new file mode 100644\n"
                    "index 0000000..257cc56\n"
                    "--- /dev/null\n"
                    "+++ b/new\n"
                    "@@ -0,0 +1 @@\n"
                    "+foo\n"
                    "diff --git a/deleted b/deleted\n"
                    "deleted file mode 100644\n"
                    "index 257cc56..0000000\n"
                    "--- a/deleted\n"
                    "+++ /dev/null\n"
                    "@@ -1 +0,0 @@\n"
                    "-foo\n"
                    "diff --git a/empty b/empty\n"
                    "new file mode 100644\n"
                    "index 0000000..e69de29\n"
                    "diff --git a/empty b/empty\n"
                    "deleted file mode 100644\n"
                    "index e69de29..0000000\n"
                    "diff --git a/file a.txt b/file b.txt\n"
                    "similarity index 99%\n"
                    "copy from file a.txt\n"
                    "copy to file b.txt\n"
                    "index 1234567..9876543\n"
                    "--- a/file a.txt\n"
                    "+++ b/file b.txt\n"
                    "@@ -20,3 +20,3 @@\n"
                    " A\n"
                    "-B\n"
                    "+C\n"
                    " D\n"
                    "diff --git a/file a.txt b/file b.txt\n"
                    "similarity index 99%\n"
                    "rename from file a.txt\n"
                    "rename to file b.txt\n"
                    "diff --git a/file.txt b/file.txt\n"
                    "old mode 100644\n"
                    "new mode 100755\n"
                    "index 1234567..9876543\n"
                    "--- a/file.txt\n"
                    "+++ b/file.txt\n"
                    "@@ -20,3 +20,3 @@\n"
                    " A\n"
                    "-B\n"
                    "+C\n"
                    " D\n"
                    ;
    fileData1.leftFileInfo = DiffFileInfo("src/plugins/diffeditor/diffeditor.cpp", "eab9e9b");
    fileData1.rightFileInfo = DiffFileInfo("src/plugins/diffeditor/diffeditor.cpp", "082c135");
    fileData3.leftFileInfo = DiffFileInfo("new", "0000000");
    fileData3.rightFileInfo = DiffFileInfo("new", "257cc56");
    fileData4.leftFileInfo = DiffFileInfo("deleted", "257cc56");
    fileData4.rightFileInfo = DiffFileInfo("deleted", "0000000");
    fileData5.leftFileInfo = DiffFileInfo("empty", "0000000");
    fileData5.rightFileInfo = DiffFileInfo("empty", "e69de29");
    fileData6.leftFileInfo = DiffFileInfo("empty", "e69de29");
    fileData6.rightFileInfo = DiffFileInfo("empty", "0000000");
    fileData7.leftFileInfo = DiffFileInfo("file a.txt", "1234567");
    fileData7.rightFileInfo = DiffFileInfo("file b.txt", "9876543");
    fileData8.leftFileInfo = DiffFileInfo("file a.txt");
    fileData8.rightFileInfo = DiffFileInfo("file b.txt");
    fileData9.leftFileInfo = DiffFileInfo("file.txt", "1234567");
    fileData9.rightFileInfo = DiffFileInfo("file.txt", "9876543");
    patch = "diff --git a/file foo.txt b/file foo.txt\n"
            "index 1234567..9876543 100644\n"
            "--- a/file foo.txt\n"
            "+++ b/file foo.txt\n"
            "@@ -50,4 +50,5 @@ void DiffEditor::ctor()\n"
            " A\n"
            " B\n"
            " C\n"
            "+\n";

    fileData1.leftFileInfo = DiffFileInfo("file foo.txt", "1234567");
    fileData1.rightFileInfo = DiffFileInfo("file foo.txt", "9876543");
    patch = "diff --git a/file foo.txt b/file foo.txt\n"
            "index 1234567..9876543 100644\n"
            "--- a/file foo.txt\n"
            "+++ b/file foo.txt\n"
            "@@ -1,1 +1,1 @@\n"
            "-ABCD\n"
            "\\ No newline at end of file\n"
            "+ABCD\n";

    fileData1.leftFileInfo = DiffFileInfo("file foo.txt", "1234567");
    fileData1.rightFileInfo = DiffFileInfo("file foo.txt", "9876543");
    patch = "diff --git a/difftest.txt b/difftest.txt\n"
            "index 1234567..9876543 100644\n"
            "--- a/difftest.txt\n"
            "+++ b/difftest.txt\n"
            "@@ -2,5 +2,5 @@ void func()\n"
            " A\n"
            " B\n"
            "-C\n"
            "+Z\n"
            " D\n"
            " \n"
            "@@ -9,2 +9,4 @@ void OtherFunc()\n"
            " \n"
            " D\n"
            "+E\n"
            "+F\n"
            ;

    fileData1.leftFileInfo = DiffFileInfo("difftest.txt", "1234567");
    fileData1.rightFileInfo = DiffFileInfo("difftest.txt", "9876543");
    patch = "diff --git a/file foo.txt b/file foo.txt\n"
            "index 1234567..9876543 100644\n"
            "--- a/file foo.txt\n"
            "+++ b/file foo.txt\n"
            "@@ -1,1 +1,3 @@ void DiffEditor::ctor()\n"
            " ABCD\n"
            "+\n"
            "+EFGH\n"
            "\\ No newline at end of file\n";

    fileData1.leftFileInfo = DiffFileInfo("file foo.txt", "1234567");
    fileData1.rightFileInfo = DiffFileInfo("file foo.txt", "9876543");
    patch = "diff --git a/src/plugins/texteditor/basetextdocument.h b/src/plugins/texteditor/textdocument.h\n"
            "similarity index 100%\n"
            "rename from src/plugins/texteditor/basetextdocument.h\n"
            "rename to src/plugins/texteditor/textdocument.h\n"
            "diff --git a/src/plugins/texteditor/basetextdocumentlayout.cpp b/src/plugins/texteditor/textdocumentlayout.cpp\n"
            "similarity index 79%\n"
            "rename from src/plugins/texteditor/basetextdocumentlayout.cpp\n"
            "rename to src/plugins/texteditor/textdocumentlayout.cpp\n"
            "index 0121933..01cc3a0 100644\n"
            "--- a/src/plugins/texteditor/basetextdocumentlayout.cpp\n"
            "+++ b/src/plugins/texteditor/textdocumentlayout.cpp\n"
            "@@ -2,5 +2,5 @@ void func()\n"
            " A\n"
            " B\n"
            "-C\n"
            "+Z\n"
            " D\n"
            " \n"
            ;
    fileData1.leftFileInfo = DiffFileInfo("src/plugins/texteditor/basetextdocument.h");
    fileData1.rightFileInfo = DiffFileInfo("src/plugins/texteditor/textdocument.h");
    fileData2.leftFileInfo = DiffFileInfo("src/plugins/texteditor/basetextdocumentlayout.cpp", "0121933");
    fileData2.rightFileInfo = DiffFileInfo("src/plugins/texteditor/textdocumentlayout.cpp", "01cc3a0");
    patch = "diff --git a/src/shared/qbs b/src/shared/qbs\n"
            "--- a/src/shared/qbs\n"
            "+++ b/src/shared/qbs\n"
            "@@ -1 +1 @@\n"
            "-Subproject commit eda76354077a427d692fee05479910de31040d3f\n"
            "+Subproject commit eda76354077a427d692fee05479910de31040d3f-dirty\n"
            ;
    fileData1.leftFileInfo = DiffFileInfo("src/shared/qbs");
    fileData1.rightFileInfo = DiffFileInfo("src/shared/qbs");
    //////////////
    patch = "diff --git a/demos/arthurplugin/arthurplugin.pro b/demos/arthurplugin/arthurplugin.pro\n"
            "new file mode 100644\n"
            "index 0000000..c5132b4\n"
            "--- /dev/null\n"
            "+++ b/demos/arthurplugin/arthurplugin.pro\n"
            "@@ -0,0 +1 @@\n"
            "+XXX\n"
            "diff --git a/demos/arthurplugin/bg1.jpg b/demos/arthurplugin/bg1.jpg\n"
            "new file mode 100644\n"
            "index 0000000..dfc7cee\n"
            "Binary files /dev/null and b/demos/arthurplugin/bg1.jpg differ\n"
            "diff --git a/demos/arthurplugin/flower.jpg b/demos/arthurplugin/flower.jpg\n"
            "new file mode 100644\n"
            "index 0000000..f8e022c\n"
            "Binary files /dev/null and b/demos/arthurplugin/flower.jpg differ\n"
            ;

    fileData1 = FileData();
    fileData1.leftFileInfo = DiffFileInfo("demos/arthurplugin/arthurplugin.pro", "0000000");
    fileData1.rightFileInfo = DiffFileInfo("demos/arthurplugin/arthurplugin.pro", "c5132b4");
    fileData1.fileOperation = FileData::NewFile;
    chunkData1 = ChunkData();
    chunkData1.leftStartingLineNumber = -1;
    chunkData1.rightStartingLineNumber = 0;
    rows1.clear();
    rows1 << RowData(TextLineData::Separator, _("XXX"));
    rows1 << RowData(TextLineData::Separator, TextLineData(TextLineData::TextLine));
    chunkData1.rows = rows1;
    fileData1.chunks << chunkData1;
    fileData2 = FileData();
    fileData2.leftFileInfo = DiffFileInfo("demos/arthurplugin/bg1.jpg", "0000000");
    fileData2.rightFileInfo = DiffFileInfo("demos/arthurplugin/bg1.jpg", "dfc7cee");
    fileData2.fileOperation = FileData::NewFile;
    fileData2.binaryFiles = true;
    fileData3 = FileData();
    fileData3.leftFileInfo = DiffFileInfo("demos/arthurplugin/flower.jpg", "0000000");
    fileData3.rightFileInfo = DiffFileInfo("demos/arthurplugin/flower.jpg", "f8e022c");
    fileData3.fileOperation = FileData::NewFile;
    fileData3.binaryFiles = true;

    QList<FileData> fileDataList8;
    fileDataList8 << fileData1 << fileData2 << fileData3;

    QTest::newRow("Binary files") << patch
                                  << fileDataList8;

    //////////////
    patch = "diff --git a/script.sh b/script.sh\n"
            "old mode 100644\n"
            "new mode 100755\n"
            ;

    fileData1 = FileData();
    fileData1.leftFileInfo = DiffFileInfo("script.sh");
    fileData1.rightFileInfo = DiffFileInfo("script.sh");
    fileData1.fileOperation = FileData::ChangeMode;

    QList<FileData> fileDataList9;
    fileDataList9 << fileData1;

    QTest::newRow("Mode change") << patch << fileDataList9;

    patch = "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
            "@@ -0,0 +125 @@\n\n";
    fileData1.leftFileInfo = DiffFileInfo("src/plugins/subversion/subversioneditor.cpp");
    fileData1.rightFileInfo = DiffFileInfo("src/plugins/subversion/subversioneditor.cpp");
    QList<FileData> fileDataList21;
    fileDataList21 << fileData1;
                                    << fileDataList21;
    patch = "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
            "@@ -1,125 +0,0 @@\n\n";
    fileData1.leftFileInfo = DiffFileInfo("src/plugins/subversion/subversioneditor.cpp");
    fileData1.rightFileInfo = DiffFileInfo("src/plugins/subversion/subversioneditor.cpp");
    QList<FileData> fileDataList22;
    fileDataList22 << fileData1;
                                        << fileDataList22;
    patch = "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
            "@@ -120,7 +120,7 @@\n\n";
    fileData1.leftFileInfo = DiffFileInfo("src/plugins/subversion/subversioneditor.cpp");
    fileData1.rightFileInfo = DiffFileInfo("src/plugins/subversion/subversioneditor.cpp");
    QList<FileData> fileDataList23;
    fileDataList23 << fileData1;
                                       << fileDataList23;
    const QList<FileData> &result = DiffUtils::readPatch(sourcePatch, &ok);
    QCOMPARE(result.count(), fileDataList.count());
using ListOfStringPairs = QList<QPair<QString, QString>>;

void DiffEditor::Internal::DiffEditorPlugin::testFilterPatch_data()
{
    QTest::addColumn<ChunkData>("chunk");
    QTest::addColumn<ListOfStringPairs>("rows");
    QTest::addColumn<ChunkSelection>("selection");
    QTest::addColumn<bool>("revert");

    auto createChunk = []() {
        ChunkData chunk;
        chunk.contextInfo = "void DiffEditor::ctor()";
        chunk.contextChunk = false;
        chunk.leftStartingLineNumber = 49;
        chunk.rightStartingLineNumber = 49;
        return chunk;
    };
    auto appendRow = [](ChunkData *chunk, const QString &left, const QString &right) {
        RowData row;
        row.equal = (left == right);
        row.leftLine.text = left;
        row.leftLine.textLineType = left.isEmpty() ? TextLineData::Separator : TextLineData::TextLine;
        row.rightLine.text = right;
        row.rightLine.textLineType = right.isEmpty() ? TextLineData::Separator : TextLineData::TextLine;
        chunk->rows.append(row);
    };
    ChunkData chunk;
    ListOfStringPairs rows;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "",  "B"); // 51 +
    appendRow(&chunk, "C", "C"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"", "B"},
        {"C", "C"}
    };
    QTest::newRow("one added") << chunk << rows << ChunkSelection() << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "");  // 51 -
    appendRow(&chunk, "C", "C"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", ""},
        {"C", "C"}
    };
    QTest::newRow("one removed") << chunk << rows << ChunkSelection() << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "",  "B"); // 51
    appendRow(&chunk, "",  "C"); // 52 +
    appendRow(&chunk, "",  "D"); // 53 +
    appendRow(&chunk, "",  "E"); // 54
    appendRow(&chunk, "F", "F"); // 55
    rows = ListOfStringPairs {
        {"A", "A"},
        {"", "C"},
        {"", "D"},
        {"F", "F"}
    };
    QTest::newRow("stage selected added") << chunk << rows << ChunkSelection({2, 3}, {2, 3}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "",  "B"); // 51 +
    appendRow(&chunk, "C", "D"); // 52
    appendRow(&chunk, "E", "E"); // 53
    rows = ListOfStringPairs {
        {"A", "A"},
        {"", "B"},
        {"C", "C"},
        {"E", "E"}
    };
    QTest::newRow("stage selected added keep changed") << chunk << rows << ChunkSelection({1}, {1}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "");  // 51
    appendRow(&chunk, "C", "");  // 52 -
    appendRow(&chunk, "D", "");  // 53 -
    appendRow(&chunk, "E", "");  // 54
    appendRow(&chunk, "F", "F"); // 55
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "B"},
        {"C", ""},
        {"D", ""},
        {"E", "E"},
        {"F", "F"}
    };
    QTest::newRow("stage selected removed") << chunk << rows << ChunkSelection({2, 3}, {2, 3}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "");  // 51
    appendRow(&chunk, "C", "");  // 52 -
    appendRow(&chunk, "",  "D"); // 53 +
    appendRow(&chunk, "",  "E"); // 54
    appendRow(&chunk, "F", "F"); // 55
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "B"},
        {"C", ""},
        {"", "D"},
        {"F", "F"}
    };
    QTest::newRow("stage selected added/removed") << chunk << rows << ChunkSelection({2, 3}, {2, 3}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage modified row") << chunk << rows << ChunkSelection({1}, {1}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage modified and unmodified rows") << chunk << rows << ChunkSelection({0, 1, 2}, {0, 1, 2}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage unmodified left rows") << chunk << rows << ChunkSelection({0, 1, 2}, {1}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage unmodified right rows") << chunk << rows << ChunkSelection({1}, {0, 1, 2}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", ""},
        {"D", "D"}
    };
    QTest::newRow("stage left only") << chunk << rows << ChunkSelection({1}, {}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "B"},
        {"", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage right only") << chunk << rows << ChunkSelection({}, {1}) << false;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage modified row and revert") << chunk << rows << ChunkSelection({1}, {1}) << true;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", ""},
        {"C", "C"},
        {"D", "D"}
    };
    // symmetric to: "stage right only"
    QTest::newRow("stage left only and revert") << chunk << rows << ChunkSelection({1}, {}) << true;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"", "C"},
        {"D", "D"}
    };
    // symmetric to: "stage left only"
    QTest::newRow("stage right only and revert") << chunk << rows << ChunkSelection({}, {1}) << true;

}

void DiffEditor::Internal::DiffEditorPlugin::testFilterPatch()
{
    QFETCH(ChunkData, chunk);
    QFETCH(ListOfStringPairs, rows);
    QFETCH(ChunkSelection, selection);
    QFETCH(bool, revert);

    const ChunkData result = DiffEditorDocument::filterChunk(chunk, selection, revert);
    QCOMPARE(result.rows.size(), rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        QCOMPARE(result.rows.at(i).leftLine.text, rows.at(i).first);
        QCOMPARE(result.rows.at(i).rightLine.text, rows.at(i).second);
    }
}
