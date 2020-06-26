#include "gitversioncontrol.h"
#include "branchdialog.h"
#include <coreplugin/coreicons.h>
#include <coreplugin/infobar.h>
#include <coreplugin/messagebox.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <QtPlugin>
#include <QScopedPointer>
const unsigned minimumRequiredVersion = 0x010800;
const char RC_GIT_MIME_XML[] = ":/git/Git.mimetypes.xml";
const VcsBaseEditorParameters editorParameters[] = {
    Git::Constants::GIT_COMMAND_LOG_EDITOR_ID,
    Git::Constants::GIT_COMMAND_LOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.commandlog"},
{   LogOutput,
    "text/vnd.qtcreator.git.log"},
{   AnnotateOutput,
    "text/vnd.qtcreator.git.annotation"},
{   OtherContent,
    "text/vnd.qtcreator.git.commit"},
{   OtherContent,
    "text/vnd.qtcreator.git.rebase"},
// GitPlugin
static GitPlugin *m_instance = 0;
GitPlugin::GitPlugin()
    m_instance = this;
    m_fileActions.reserve(10);
    m_projectActions.reserve(10);
    m_repositoryActions.reserve(50);
    cleanCommitMessageFile();
    delete m_gitClient;
    m_instance = 0;
void GitPlugin::cleanCommitMessageFile()
bool GitPlugin::isCommitEditorOpen() const
GitPlugin *GitPlugin::instance()
    return m_instance;
GitClient *GitPlugin::client()
    return m_instance->m_gitClient;
const VcsBaseSubmitEditorParameters submitParameters = {
    Git::Constants::SUBMIT_MIMETYPE,
    Git::Constants::GITSUBMITEDITOR_ID,
    Git::Constants::GITSUBMITEDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffRows
};
Command *GitPlugin::createCommand(QAction *action, ActionContainer *ac, Id id,
ParameterAction *GitPlugin::createParameterAction(ActionContainer *ac,
QAction *GitPlugin::createFileAction(ActionContainer *ac,
QAction *GitPlugin::createFileAction(ActionContainer *ac, const QString &defaultText,
                                     const QString &parameterText, Id id, const Context &context,
                                     bool addToLocator, void (GitPlugin::*func)(),
                                     const QKeySequence &keys)
{
    return createFileAction(ac, defaultText, parameterText, id, context, addToLocator,
                            [this, func]() { return (this->*func)(); }, keys);
}

QAction *GitPlugin::createProjectAction(ActionContainer *ac, const QString &defaultText,
                                        bool addToLocator, void (GitPlugin::*func)(),
                                                    addToLocator,
                                                    [this, func]() { return (this->*func)(); },
                                                    keys);
QAction *GitPlugin::createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
QAction *GitPlugin::createChangeRelatedRepositoryAction(const QString &text, Id id,
                                  [this, id] { startChangeRelatedAction(id); }, QKeySequence());
QAction *GitPlugin::createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
        (m_gitClient->*func)(currentState().topLevel());
    Q_UNUSED(arguments)
    Context context(Constants::GIT_CONTEXT);
    m_gitClient = new GitClient;
    initializeVcs(new GitVersionControl(m_gitClient), context);
    // Create the settings Page
    addAutoReleasedObject(new SettingsPage(versionControl()));
    addAutoReleasedObject(new GitGrep);
    static const char *describeSlot = SLOT(show(QString,QString));
    const int editorCount = sizeof(editorParameters) / sizeof(editorParameters[0]);
    const auto widgetCreator = []() { return new GitEditorWidget; };
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new VcsEditorFactory(editorParameters + i, widgetCreator, m_gitClient, describeSlot));
    addAutoReleasedObject(new VcsSubmitEditorFactory(&submitParameters,
        []() { return new GitSubmitEditor(&submitParameters); }));
    const QString prefix = QLatin1String("git");
    m_commandLocator = new CommandLocator("Git", prefix, prefix);
    addAutoReleasedObject(m_commandLocator);
                     "Git.Diff", context, true, &GitPlugin::diffCurrentFile,
                      QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+D") : tr("Alt+G,Alt+D")));
                     "Git.Log", context, true, &GitPlugin::logFile,
                     QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+L") : tr("Alt+G,Alt+L")));
                     "Git.Blame", context, true, &GitPlugin::blameFile,
                     QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+B") : tr("Alt+G,Alt+B")));
                     "Git.Stage", context, true, &GitPlugin::stageFile,
                     QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+A") : tr("Alt+G,Alt+A")));
                     "Git.Unstage", context, true, &GitPlugin::unstageFile);
                     true, [this]() { return undoFileChanges(false); });
                     true, [this]() { return undoFileChanges(true); },
                     QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+U") : tr("Alt+G,Alt+U")));
                        "Git.DiffProject", context, true, &GitPlugin::diffCurrentProject,
                        QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+Shift+D") : tr("Alt+G,Alt+Shift+D")));
                        "Git.LogProject", context, true, &GitPlugin::logProject,
                        QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+K") : tr("Alt+G,Alt+K")));
                        "Git.CleanProject", context, true, &GitPlugin::cleanProject);
                           context, true, [this] { logRepository(); });
                           context, true, &GitClient::reflog);
                           context, true, [this] { startCommit(); },
                           QKeySequence(UseMacShortcuts ? tr("Meta+G,Meta+C") : tr("Alt+G,Alt+C")));
                           context, true, [this] { startAmendCommit(); });
                                     [this] { startFixupCommit(); });
                           context, true, [this] { resetRepository(); });
                                     context, true, [this] { startRebase(); });
                                     context, true, [this] { updateSubmodules(); });
                                     context, true, [this] { continueOrAbortCommand(); });
                                     context, true, [this] { continueOrAbortCommand(); });
                                     context, true, [this] { continueOrAbortCommand(); });
                                     context, true, [this] { continueOrAbortCommand(); });
                                     context, true, [this] { continueOrAbortCommand(); });
                                     context, true, [this] { continueOrAbortCommand(); });
                                     context, true, [this] { continueOrAbortCommand(); });
                           context, true, [this] { branchList(); });
                                    context, true, [this] { applyCurrentFilePatch(); });
                           context, true, [this] { promptApplyPatch(); });
                           context, false, [this] { stashList(); });
                                             context, true, [this] { stash(); });
                                    context, true, [this] { stashUnstaged(); });
                                    context, true, [this] { stashSnapshot(); });
                                    context, true, [this] { stashPop(); });
                           context, true, [this] { fetch(); });
                           context, true, [this] { pull(); });
                           context, true, [this] { push(); });
                           context, false, [this] { remoteList(); });
    createRepositoryAction(0, tr("Rebase..."), "Git.Rebase", context, true, [this] { branchList(); });
    createRepositoryAction(0, tr("Merge..."), "Git.Merge", context, true, [this] { branchList(); });
                     "Git.GitkFile", context, true, &GitPlugin::gitkForCurrentFile);
                     "Git.GitkFolder", context, true, &GitPlugin::gitkForCurrentFolder);
                           context, true, [this] { gitGui(); });
                                     context, true, [this] { startMergeTool(); });
    connect(createRepositoryAction, &QAction::triggered, this, &GitPlugin::createRepository);
    // Submit editor
    Context submitContext(Constants::GITSUBMITEDITOR_ID);
    m_submitCurrentAction = new QAction(VcsBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    Command *command = ActionManager::registerAction(m_submitCurrentAction, Constants::SUBMIT_CURRENT, submitContext);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_submitCurrentAction, &QAction::triggered, this, &GitPlugin::submitCurrentLog);

    m_diffSelectedFilesAction = new QAction(VcsBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    ActionManager::registerAction(m_diffSelectedFilesAction, Constants::DIFF_SELECTED, submitContext);

    m_undoAction = new QAction(tr("&Undo"), this);
    ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, submitContext);

    m_redoAction = new QAction(tr("&Redo"), this);
    ActionManager::registerAction(m_redoAction, Core::Constants::REDO, submitContext);

            this, &GitPlugin::updateContinueAndAbortCommands);
            this, &GitPlugin::updateBranches, Qt::QueuedConnection);

    Utils::MimeDatabase::addMimeTypes(QLatin1String(RC_GIT_MIME_XML));
    const bool ok = m_gerritPlugin->initialize(remoteRepositoryMenu);
    m_gerritPlugin->updateActions(currentState().hasTopLevel());
    return ok;
}
GitVersionControl *GitPlugin::gitVersionControl() const
{
    return static_cast<GitVersionControl *>(versionControl());
void GitPlugin::diffCurrentFile()
    m_gitClient->diffFile(state.currentFileTopLevel(), state.relativeCurrentFile());
void GitPlugin::diffCurrentProject()
        m_gitClient->diffRepository(state.currentProjectTopLevel());
        m_gitClient->diffProject(state.currentProjectTopLevel(), relativeProject);
void GitPlugin::logFile()
    m_gitClient->log(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
void GitPlugin::blameFile()
    m_gitClient->annotate(state.currentFileTopLevel(), state.relativeCurrentFile(), QString(), lineNumber);
void GitPlugin::logProject()
    m_gitClient->log(state.currentProjectTopLevel(), state.relativeCurrentProject());
void GitPlugin::logRepository()
    m_gitClient->log(state.topLevel());
void GitPlugin::undoFileChanges(bool revertStaging)
        if (!DocumentManager::saveModifiedDocument(document))
    m_gitClient->revert(QStringList(state.currentFile()), revertStaging);
        : IconItemDelegate(widget, Core::Icons::UNDO.imageFileName())
void GitPlugin::resetRepository()
    LogChangeDialog dialog(true, ICore::mainWindow());
        m_gitClient->reset(topLevel, dialog.resetFlag(), dialog.commit());
void GitPlugin::startRebase()
    if (topLevel.isEmpty() || !m_gitClient->canRebase(topLevel))
    LogChangeDialog dialog(false, ICore::mainWindow());
    RebaseItemDelegate delegate(dialog.widget());
    dialog.setWindowTitle(tr("Interactive Rebase"));
    if (!dialog.runDialog(topLevel))
    if (m_gitClient->beginStashScope(topLevel, QLatin1String("Rebase-i")))
        m_gitClient->interactiveRebase(topLevel, dialog.commit(), false);
void GitPlugin::startChangeRelatedAction(const Id &id)
                                 id, ICore::mainWindow());
        m_gitClient->show(workingDirectory, change);
        m_gitClient->synchronousCherryPick(workingDirectory, change);
        m_gitClient->synchronousRevert(workingDirectory, change);
        m_gitClient->stashAndCheckout(workingDirectory, change);
void GitPlugin::stageFile()
    m_gitClient->addFile(state.currentFileTopLevel(), state.relativeCurrentFile());
void GitPlugin::unstageFile()
    m_gitClient->synchronousReset(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
void GitPlugin::gitkForCurrentFile()
    m_gitClient->launchGitK(state.currentFileTopLevel(), state.relativeCurrentFile());
void GitPlugin::gitkForCurrentFolder()
     *  m_gitClient->launchGitK(dir.currentFileDirectory(), QLatin1String("."));
    if (QFileInfo(dir,QLatin1String(".git")).exists() || dir.cd(QLatin1String(".git"))) {
        m_gitClient->launchGitK(state.currentFileDirectory());
        m_gitClient->launchGitK(dir.absolutePath(), folderName);
void GitPlugin::gitGui()
    m_gitClient->launchGitGui(state.topLevel());
void GitPlugin::startAmendCommit()
    startCommit(AmendCommit);
void GitPlugin::startFixupCommit()
    startCommit(FixupCommit);
}
void GitPlugin::startCommit()
{
    startCommit(SimpleCommit);
}

void GitPlugin::startCommit(CommitType commitType)
{
    if (!m_gitClient->getCommitData(state.topLevel(), &commitTemplate, data, &errorMessage)) {
void GitPlugin::updateVersionWarning()
    unsigned version = m_gitClient->gitVersion();
                        InfoBarEntry::GlobalSuppressionEnabled));
IEditor *GitPlugin::openSubmitEditor(const QString &fileName, const CommitData &cd)
    GitSubmitEditor *submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return 0);
    // The actions are for some reason enabled by the context switching
    // mechanism. Disable them correctly.
    submitEditor->registerActions(m_undoAction, m_redoAction, m_submitCurrentAction, m_diffSelectedFilesAction);
    VcsBasePlugin::setSource(document, m_submitRepository);
void GitPlugin::submitCurrentLog()
bool GitPlugin::submitEditorAboutToClose()
    GitSubmitEditor *editor = qobject_cast<GitSubmitEditor *>(submitEditor());
    bool promptData = false;
            = editor->promptSubmit(tr("Closing Git Editor"),
                 tr("Do you want to commit the change?"),
                 tr("Git will not accept this commit. Do you want to continue to edit it?"),
                 &promptData, !m_submitActionTriggered, false);
    SubmitFileModel *model = qobject_cast<SubmitFileModel *>(editor->fileModel());
    bool closeEditor = true;
        closeEditor = m_gitClient->addAndCommit(m_submitRepository, editor->panelData(),
                                                commitType, amendSHA1,
                                                m_commitMessageFileName, model);
    if (!closeEditor)
        return false;
        if (!m_gitClient->beginStashScope(m_submitRepository, QLatin1String("Rebase-fixup"),
        m_gitClient->interactiveRebase(m_submitRepository, amendSHA1, true);
        m_gitClient->continueCommandIfNeeded(m_submitRepository);
        if (editor->panelData().pushAction == NormalPush)
            m_gitClient->push(m_submitRepository);
        else if (editor->panelData().pushAction == PushToGerrit)
            connect(editor, &QObject::destroyed, this, &GitPlugin::delayedPushToGerrit);
void GitPlugin::fetch()
    m_gitClient->fetch(currentState().topLevel(), QString());
void GitPlugin::pull()
    bool rebase = client()->settings().boolValue(GitSettings::pullRebaseKey);
        QString currentBranch = m_gitClient->synchronousCurrentLocalBranch(topLevel);
            currentBranch.prepend(QLatin1String("branch."));
            currentBranch.append(QLatin1String(".rebase"));
            rebase = (m_gitClient->readConfigValue(topLevel, currentBranch) == QLatin1String("true"));
    if (!m_gitClient->beginStashScope(topLevel, QLatin1String("Pull"), rebase ? Default : AllowUnstashed))
    m_gitClient->synchronousPull(topLevel, rebase);
void GitPlugin::push()
    m_gitClient->push(state.topLevel());
void GitPlugin::startMergeTool()
    m_gitClient->merge(state.topLevel());
void GitPlugin::continueOrAbortCommand()
        m_gitClient->synchronousMerge(state.topLevel(), QLatin1String("--abort"));
        m_gitClient->rebase(state.topLevel(), QLatin1String("--abort"));
        m_gitClient->synchronousCherryPick(state.topLevel(), QLatin1String("--abort"));
        m_gitClient->synchronousRevert(state.topLevel(), QLatin1String("--abort"));
        m_gitClient->rebase(state.topLevel(), QLatin1String("--continue"));
        m_gitClient->cherryPick(state.topLevel(), QLatin1String("--continue"));
        m_gitClient->revert(state.topLevel(), QLatin1String("--continue"));
void GitPlugin::cleanProject()
void GitPlugin::cleanRepository()
void GitPlugin::cleanRepository(const QString &directory)
    const bool gotFiles = m_gitClient->synchronousCleanList(directory, QString(), &files, &ignoredFiles, &errorMessage);
        Core::AsynchronousMessageBox::warning(tr("Unable to retrieve file list"), errorMessage);
void GitPlugin::updateSubmodules()
    m_gitClient->updateSubmodulesIfNeeded(state.topLevel(), false);
void GitPlugin::applyCurrentFilePatch()
void GitPlugin::promptApplyPatch()
void GitPlugin::applyPatch(const QString &workingDirectory, QString file)
    if (!m_gitClient->beginStashScope(workingDirectory, QLatin1String("Apply-Patch"), AllowUnstashed))
        file = QFileDialog::getOpenFileName(ICore::mainWindow(), tr("Choose Patch"), QString(), filter);
            m_gitClient->endStashScope(workingDirectory);
    if (m_gitClient->synchronousApplyPatch(workingDirectory, file, &errorMessage)) {
    m_gitClient->endStashScope(workingDirectory);
void GitPlugin::stash(bool unstagedOnly)
    m_gitClient->executeSynchronousStash(topLevel, QString(), unstagedOnly);
void GitPlugin::stashUnstaged()
void GitPlugin::stashSnapshot()
    const QString id = m_gitClient->synchronousStash(state.topLevel(), QString(),
void GitPlugin::stashPop()
    m_gitClient->stashPop(repository);
        dialog = new NonModalDialog(ICore::mainWindow());
void GitPlugin::branchList()
    showNonModalDialog(currentState().topLevel(), m_branchDialog);
void GitPlugin::remoteList()
void GitPlugin::stashList()
void GitPlugin::updateActions(VcsBasePlugin::ActionState as)
    const bool repositoryEnabled = currentState().hasTopLevel();
        m_stashDialog->refresh(currentState().topLevel(), false);
    if (m_branchDialog)
        m_branchDialog->refresh(currentState().topLevel(), false);
        m_remoteDialog->refresh(currentState().topLevel(), false);
    const QString fileName = currentState().currentFileName();
    foreach (ParameterAction *fileAction, m_fileActions)
    m_applyCurrentFilePatchAction->setParameter(currentState().currentPatchFileDisplayName());
    const QString projectName = currentState().currentProjectName();
    foreach (ParameterAction *projectAction, m_projectActions)
    foreach (QAction *repositoryAction, m_repositoryActions)
            && !m_gitClient->submoduleList(currentState().topLevel()).isEmpty());
    m_gerritPlugin->updateActions(repositoryEnabled);
void GitPlugin::updateContinueAndAbortCommands()
                m_gitClient->checkCommandInProgress(currentState().topLevel());
void GitPlugin::delayedPushToGerrit()
void GitPlugin::updateBranches(const QString &repository)
    if (m_branchDialog && m_branchDialog->isVisible())
        m_branchDialog->refreshIfSame(repository);
void GitPlugin::updateRepositoryBrowserAction()
            = !client()->settings().stringValue(GitSettings::repositoryBrowserCmd).isEmpty();
Gerrit::Internal::GerritPlugin *GitPlugin::gerritPlugin() const
    return m_gerritPlugin;
    QString output = QLatin1String("## master...origin/master [ahead 1]\n");
    output += QString::fromLatin1(QTest::currentDataTag()) + QLatin1String(" main.cpp\n");
    VcsBaseEditorWidget::testDiffFileResolving(editorParameters[3].id);
    VcsBaseEditorWidget::testLogResolving(editorParameters[1].id, data,