#include "mercurialcontrol.h"
#include <coreplugin/id.h>
#include <coreplugin/documentmanager.h>
#include <QtPlugin>
static const VcsBaseEditorParameters editorParameters[] = {
    Constants::LOGAPP},
{   AnnotateOutput,
    Constants::ANNOTATEAPP},
{   DiffOutput,
    Constants::DIFFAPP}
static const VcsBaseSubmitEditorParameters submitEditorParameters = {
MercurialPlugin *MercurialPlugin::m_instance = 0;
MercurialPlugin::MercurialPlugin()
{
    m_instance = this;
}
    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }

    m_instance = nullptr;
    Core::Context context(Constants::MERCURIAL_CONTEXT);
    m_client = new MercurialClient;
    initializeVcs(new MercurialControl(m_client), context);
    addAutoReleasedObject(new OptionsPage(versionControl()));
    connect(m_client, SIGNAL(changed(QVariant)), versionControl(), SLOT(changed(QVariant)));
    connect(m_client, &MercurialClient::needUpdate, this, &MercurialPlugin::update);
    static const char *describeSlot = SLOT(view(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(editorParameters[0]);
    const auto widgetCreator = []() { return new MercurialEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, m_client, describeSlot));
    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitEditorParameters,
        []() { return new CommitEditor(&submitEditorParameters); }));
    m_commandLocator = new Core::CommandLocator("Mercurial", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    createSubmitEditorActions();

    return true;
void MercurialPlugin::createMenu(const Core::Context &context)
    createRepositoryManagementActions(context);
    Core::ActionContainer *toolsMenu = Core::ActionManager::actionContainer(Core::Id(Core::Constants::M_TOOLS));
void MercurialPlugin::createFileActions(const Core::Context &context)
    command = Core::ActionManager::registerAction(annotateFile, Core::Id(Constants::ANNOTATE), context);
    connect(annotateFile, &QAction::triggered, this, &MercurialPlugin::annotateCurrentFile);
    command = Core::ActionManager::registerAction(diffFile, Core::Id(Constants::DIFF), context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+D") : tr("Alt+G,Alt+D")));
    connect(diffFile, &QAction::triggered, this, &MercurialPlugin::diffCurrentFile);
    command = Core::ActionManager::registerAction(logFile, Core::Id(Constants::LOG), context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+L") : tr("Alt+G,Alt+L")));
    connect(logFile, &QAction::triggered, this, &MercurialPlugin::logCurrentFile);
    command = Core::ActionManager::registerAction(statusFile, Core::Id(Constants::STATUS), context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+S") : tr("Alt+G,Alt+S")));
    connect(statusFile, &QAction::triggered, this, &MercurialPlugin::statusCurrentFile);
    command = Core::ActionManager::registerAction(m_addAction, Core::Id(Constants::ADD), context);
    connect(m_addAction, &QAction::triggered, this, &MercurialPlugin::addCurrentFile);
    command = Core::ActionManager::registerAction(m_deleteAction, Core::Id(Constants::DELETE), context);
    connect(m_deleteAction, &QAction::triggered, this, &MercurialPlugin::promptToDeleteCurrentFile);
    command = Core::ActionManager::registerAction(revertFile, Core::Id(Constants::REVERT), context);
    connect(revertFile, &QAction::triggered, this, &MercurialPlugin::revertCurrentFile);
void MercurialPlugin::addCurrentFile()
    m_client->synchronousAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
void MercurialPlugin::annotateCurrentFile()
    m_client->annotate(state.currentFileTopLevel(), state.relativeCurrentFile(), QString(), currentLine);
void MercurialPlugin::diffCurrentFile()
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
void MercurialPlugin::logCurrentFile()
    m_client->log(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()),
                  QStringList(), true);
void MercurialPlugin::revertCurrentFile()
    m_client->revertFile(state.currentFileTopLevel(), state.relativeCurrentFile(), reverter.revision());
void MercurialPlugin::statusCurrentFile()
    m_client->status(state.currentFileTopLevel(), state.relativeCurrentFile());
void MercurialPlugin::createDirectoryActions(const Core::Context &context)
    Core::Command *command = Core::ActionManager::registerAction(action, Core::Id(Constants::DIFFMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::diffRepository);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::LOGMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::logRepository);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::REVERTMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::revertMulti);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::STATUSMULTI), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::statusMulti);
void MercurialPlugin::diffRepository()
    m_client->diff(state.topLevel());
void MercurialPlugin::logRepository()
    m_client->log(state.topLevel());
void MercurialPlugin::revertMulti()
    m_client->revertAll(state.topLevel(), reverter.revision());
void MercurialPlugin::statusMulti()
    m_client->status(state.topLevel());
void MercurialPlugin::createRepositoryActions(const Core::Context &context)
    Core::Command *command = Core::ActionManager::registerAction(action, Core::Id(Constants::PULL), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::pull);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::PUSH), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::push);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::UPDATE), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::update);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::IMPORT), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::import);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::INCOMING), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::incoming);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::OUTGOING), context);
    connect(action, &QAction::triggered, this, &MercurialPlugin::outgoing);
    command = Core::ActionManager::registerAction(action, Core::Id(Constants::COMMIT), context);
    command->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+H,Meta+C") : tr("Alt+G,Alt+C")));
    connect(action, &QAction::triggered, this, &MercurialPlugin::commit);
    command = Core::ActionManager::registerAction(m_createRepositoryAction, Core::Id(Constants::CREATE_REPOSITORY), context);
    connect(m_createRepositoryAction, &QAction::triggered, this, &MercurialPlugin::createRepository);
void MercurialPlugin::pull()
    SrcDestDialog dialog(SrcDestDialog::incoming, Core::ICore::dialogParent());
    m_client->synchronousPull(dialog.workingDir(), dialog.getRepositoryString());
void MercurialPlugin::push()
    SrcDestDialog dialog(SrcDestDialog::outgoing, Core::ICore::dialogParent());
    m_client->synchronousPush(dialog.workingDir(), dialog.getRepositoryString());
void MercurialPlugin::update()
    m_client->update(state.topLevel(), updateDialog.revision());
void MercurialPlugin::import()
    m_client->import(state.topLevel(), fileNames);
void MercurialPlugin::incoming()
    SrcDestDialog dialog(SrcDestDialog::incoming, Core::ICore::dialogParent());
    m_client->incoming(state.topLevel(), dialog.getRepositoryString());
void MercurialPlugin::outgoing()
    m_client->outgoing(state.topLevel());
void MercurialPlugin::createSubmitEditorActions()
    Core::Context context(Constants::COMMIT_ID);

    editorCommit = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    Core::Command *command = Core::ActionManager::registerAction(editorCommit, Core::Id(Constants::COMMIT), context);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(editorCommit, &QAction::triggered, this, &MercurialPlugin::commitFromEditor);

    editorDiff = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    Core::ActionManager::registerAction(editorDiff, Core::Id(Constants::DIFFEDITOR), context);
    editorUndo = new QAction(tr("&Undo"), this);
    Core::ActionManager::registerAction(editorUndo, Core::Id(Core::Constants::UNDO), context);

    editorRedo = new QAction(tr("&Redo"), this);
    Core::ActionManager::registerAction(editorRedo, Core::Id(Core::Constants::REDO), context);
}

void MercurialPlugin::commit()
{
    connect(m_client, &MercurialClient::parsedStatus, this, &MercurialPlugin::showCommitWidget);
    m_client->emitParsedStatus(m_submitRepository);
void MercurialPlugin::showCommitWidget(const QList<VcsBaseClient::StatusItem> &status)
    disconnect(m_client, &MercurialClient::parsedStatus, this, &MercurialPlugin::showCommitWidget);
    CommitEditor *commitEditor = static_cast<CommitEditor *>(editor);
    commitEditor->registerActions(editorUndo, editorRedo, editorCommit, editorDiff);
            this, &MercurialPlugin::diffFromEditorSelected);
    QString branch = versionControl()->vcsTopic(m_submitRepository);
                            m_client->settings().stringValue(MercurialSettings::userNameKey),
                            m_client->settings().stringValue(MercurialSettings::userEmailKey), status);
void MercurialPlugin::diffFromEditorSelected(const QStringList &files)
    m_client->diff(m_submitRepository, files);
void MercurialPlugin::commitFromEditor()
bool MercurialPlugin::submitEditorAboutToClose()
    CommitEditor *commitEditor = qobject_cast<CommitEditor *>(submitEditor());
    bool dummyPrompt = false;
            commitEditor->promptSubmit(tr("Close Commit Editor"), tr("Do you want to commit the changes?"),
                                       tr("Message check failed. Do you want to proceed?"),
                                       &dummyPrompt, !m_submitActionTriggered);
        m_client->commit(m_submitRepository, files, editorFile->filePath().toString(),
                         extraOptions);
void MercurialPlugin::createRepositoryManagementActions(const Core::Context &context)
{
    //TODO create menu for these options
    Q_UNUSED(context);
    return;
    //    auto action = new QAction(tr("Branch"), this);
    //    actionList.append(action);
    //    Core::Command *command = Core::ActionManager::registerAction(action, Constants::BRANCH, context);
    //    //    connect(action, SIGNAL(triggered()), this, SLOT(branch()));
    //    m_mercurialContainer->addAction(command);
}

void MercurialPlugin::updateActions(VcsBasePlugin::ActionState as)
    VcsBaseEditorWidget::testDiffFileResolving(editorParameters[2].id);
    VcsBaseEditorWidget::testLogResolving(editorParameters[0].id, data, "18473:692cbda1eb50", "18472:37100f30590f");