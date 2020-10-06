#include "differ.h"
#include <QtPlugin>
class FileDiffController : public DiffEditorController
    FileDiffController(Core::IDocument *document, const QString &leftFileName,
    void reload();
    QString m_leftFileName;
    QString m_rightFileName;
FileDiffController::FileDiffController(Core::IDocument *document, const QString &leftFileName,
    DiffEditorController(document), m_leftFileName(leftFileName), m_rightFileName(rightFileName)
void FileDiffController::reload()
    format.codec = Core::EditorManager::defaultTextCodec();
    if (Utils::TextFileFormat::readFile(m_leftFileName,
                                    format.codec,
                                    &leftText, &format, &errorString)
            != Utils::TextFileFormat::ReadSuccess) {
    }
    QString rightText;
    if (Utils::TextFileFormat::readFile(m_rightFileName,
                                    format.codec,
                                    &rightText, &format, &errorString)
            != Utils::TextFileFormat::ReadSuccess) {
    }
    Differ differ;
    QList<Diff> diffList = differ.cleanupSemantics(differ.diff(leftText, rightText));

    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
    QList<Diff> outputLeftDiffList;
    QList<Diff> outputRightDiffList;

    if (ignoreWhitespace()) {
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
    FileData fileData = DiffUtils::calculateContextData(chunkData, contextLineCount(), 0);
    fileData.leftFileInfo.fileName = m_leftFileName;
    fileData.rightFileInfo.fileName = m_rightFileName;
    QList<FileData> fileDataList;
    fileDataList << fileData;
    setDiffFiles(fileDataList);
    reloadFinished(true);
/////////////////
bool DiffEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    //register actions
    Core::ActionContainer *toolsContainer
            = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_OPTIONS, Constants::G_TOOLS_DIFF);
    QAction *diffAction = new QAction(tr("Diff..."), this);
    Core::Command *diffCommand = Core::ActionManager::registerAction(diffAction, "DiffEditor.Diff");
    connect(diffAction, &QAction::triggered, this, &DiffEditorPlugin::diff);
    toolsContainer->addAction(diffCommand, Constants::G_TOOLS_DIFF);
    addAutoReleasedObject(new DiffEditorFactory(this));
    return true;
void DiffEditorPlugin::extensionsInitialized()
{ }
void DiffEditorPlugin::diff()
    QString fileName1 = QFileDialog::getOpenFileName(Core::ICore::dialogParent(),
    QString fileName2 = QFileDialog::getOpenFileName(Core::ICore::dialogParent(),

    const QString documentId = QLatin1String("Diff ") + fileName1 + QLatin1String(", ") + fileName2;
    QString title = tr("Diff \"%1\", \"%2\"").arg(fileName1).arg(fileName2);
        new FileDiffController(document, fileName1, fileName2);
    Core::EditorManager::activateEditorForDocument(document);
    QTest::addColumn<QString>("leftFileName");
    QTest::addColumn<QString>("rightFileName");
    const QString fileName = _("a.txt");
    const QString header = _("--- ") + fileName + _("\n+++ ") + fileName + _("\n");
    QString patchText = header + _("@@ -1,2 +1,1 @@\n"
                                   "-ABCD\n"
                                   " EFGH\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,2 +1,1 @@\n"
                           "-ABCD\n"
                           " EFGH\n"
                           "\\ No newline at end of file\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "+ABCD\n"
                           "\\ No newline at end of file\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,2 +1,1 @@\n"
                           " ABCD\n"
                           "-\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,2 +1,1 @@\n"
                           "-ABCD\n"
                           "-\n"
                           "+ABCD\n"
                           "\\ No newline at end of file\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "\\ No newline at end of file\n"
                           "+ABCD\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,1 +1,2 @@\n"
                           " ABCD\n"
                           "+\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "+EFGH\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,2 +1,2 @@\n"
                           "-ABCD\n"
                           "+EFGH\n"
                           " \n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "\\ No newline at end of file\n"
                           "+EFGH\n"
                           "\\ No newline at end of file\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "+EFGH\n");
                            << fileName
                            << fileName
    patchText = header + _("@@ -1,2 +1,2 @@\n"
                           "-ABCD\n"
                           "+EFGH\n"
                           " IJKL\n"
                           "\\ No newline at end of file\n");
            << fileName
            << fileName
    patchText = header + _("@@ -1,2 +1,2 @@\n"
                           "-ABCD\n"
                           "+EFGH\n"
                           " IJKL\n");
            << fileName
            << fileName
    patchText = header + _("@@ -1,1 +1,3 @@\n"
                           " ABCD\n"
                           "+\n"
                           "+EFGH\n"
                           "\\ No newline at end of file\n");
            << fileName
            << fileName
    QFETCH(QString, leftFileName);
    QFETCH(QString, rightFileName);
    QString result = DiffUtils::makePatch(sourceChunk, leftFileName, rightFileName, lastChunk);
        QCOMPARE(resultFileData.leftFileInfo.fileName, leftFileName);
        QCOMPARE(resultFileData.rightFileInfo.fileName, rightFileName);
    QString patch = _("diff --git a/src/plugins/diffeditor/diffeditor.cpp b/src/plugins/diffeditor/diffeditor.cpp\n"
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
                      );
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/diffeditor/diffeditor.cpp"), _("eab9e9b"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/diffeditor/diffeditor.cpp"), _("082c135"));
    fileData3.leftFileInfo = DiffFileInfo(_("new"), _("0000000"));
    fileData3.rightFileInfo = DiffFileInfo(_("new"), _("257cc56"));
    fileData4.leftFileInfo = DiffFileInfo(_("deleted"), _("257cc56"));
    fileData4.rightFileInfo = DiffFileInfo(_("deleted"), _("0000000"));
    fileData5.leftFileInfo = DiffFileInfo(_("empty"), _("0000000"));
    fileData5.rightFileInfo = DiffFileInfo(_("empty"), _("e69de29"));
    fileData6.leftFileInfo = DiffFileInfo(_("empty"), _("e69de29"));
    fileData6.rightFileInfo = DiffFileInfo(_("empty"), _("0000000"));
    fileData7.leftFileInfo = DiffFileInfo(_("file a.txt"), _("1234567"));
    fileData7.rightFileInfo = DiffFileInfo(_("file b.txt"), _("9876543"));
    fileData8.leftFileInfo = DiffFileInfo(_("file a.txt"));
    fileData8.rightFileInfo = DiffFileInfo(_("file b.txt"));
    fileData9.leftFileInfo = DiffFileInfo(_("file.txt"), _("1234567"));
    fileData9.rightFileInfo = DiffFileInfo(_("file.txt"), _("9876543"));
    patch = _("diff --git a/file foo.txt b/file foo.txt\n"
              "index 1234567..9876543 100644\n"
              "--- a/file foo.txt\n"
              "+++ b/file foo.txt\n"
              "@@ -50,4 +50,5 @@ void DiffEditor::ctor()\n"
              " A\n"
              " B\n"
              " C\n"
              "+\n");

    fileData1.leftFileInfo = DiffFileInfo(_("file foo.txt"), _("1234567"));
    fileData1.rightFileInfo = DiffFileInfo(_("file foo.txt"), _("9876543"));
    patch = _("diff --git a/file foo.txt b/file foo.txt\n"
              "index 1234567..9876543 100644\n"
              "--- a/file foo.txt\n"
              "+++ b/file foo.txt\n"
              "@@ -1,1 +1,1 @@\n"
              "-ABCD\n"
              "\\ No newline at end of file\n"
              "+ABCD\n");

    fileData1.leftFileInfo = DiffFileInfo(_("file foo.txt"), _("1234567"));
    fileData1.rightFileInfo = DiffFileInfo(_("file foo.txt"), _("9876543"));
    patch = _("diff --git a/difftest.txt b/difftest.txt\n"
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
              );

    fileData1.leftFileInfo = DiffFileInfo(_("difftest.txt"), _("1234567"));
    fileData1.rightFileInfo = DiffFileInfo(_("difftest.txt"), _("9876543"));
    patch = _("diff --git a/file foo.txt b/file foo.txt\n"
              "index 1234567..9876543 100644\n"
              "--- a/file foo.txt\n"
              "+++ b/file foo.txt\n"
              "@@ -1,1 +1,3 @@ void DiffEditor::ctor()\n"
              " ABCD\n"
              "+\n"
              "+EFGH\n"
              "\\ No newline at end of file\n");

    fileData1.leftFileInfo = DiffFileInfo(_("file foo.txt"), _("1234567"));
    fileData1.rightFileInfo = DiffFileInfo(_("file foo.txt"), _("9876543"));
    patch = _("diff --git a/src/plugins/texteditor/basetextdocument.h b/src/plugins/texteditor/textdocument.h\n"
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
              );
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/texteditor/basetextdocument.h"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/texteditor/textdocument.h"));
    fileData2.leftFileInfo = DiffFileInfo(_("src/plugins/texteditor/basetextdocumentlayout.cpp"), _("0121933"));
    fileData2.rightFileInfo = DiffFileInfo(_("src/plugins/texteditor/textdocumentlayout.cpp"), _("01cc3a0"));
    patch = _("diff --git a/src/shared/qbs b/src/shared/qbs\n"
              "--- a/src/shared/qbs\n"
              "+++ b/src/shared/qbs\n"
              "@@ -1 +1 @@\n"
              "-Subproject commit eda76354077a427d692fee05479910de31040d3f\n"
              "+Subproject commit eda76354077a427d692fee05479910de31040d3f-dirty\n"
              );
    fileData1.leftFileInfo = DiffFileInfo(_("src/shared/qbs"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/shared/qbs"));
    patch = _("Index: src/plugins/subversion/subversioneditor.cpp\n"
              "===================================================================\n"
              "--- src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
              "+++ src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
              "@@ -0,0 +125 @@\n\n");
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    QList<FileData> fileDataList8;
    fileDataList8 << fileData1;
                                    << fileDataList8;
    patch = _("Index: src/plugins/subversion/subversioneditor.cpp\n"
              "===================================================================\n"
              "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
              "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
              "@@ -1,125 +0,0 @@\n\n");
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    QList<FileData> fileDataList9;
    fileDataList9 << fileData1;
                                        << fileDataList9;
    patch = _("Index: src/plugins/subversion/subversioneditor.cpp\n"
              "===================================================================\n"
              "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
              "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
              "@@ -120,7 +120,7 @@\n\n");
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    QList<FileData> fileDataList10;
    fileDataList10 << fileData1;
                                       << fileDataList10;
    QList<FileData> result = DiffUtils::readPatch(sourcePatch, &ok);
    QCOMPARE(fileDataList.count(), result.count());