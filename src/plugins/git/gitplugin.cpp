#include "branchview.h"
#include <coreplugin/messagebox.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <aggregation/aggregate.h>

#include <texteditor/texteditor.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseplugin.h>
#include <QApplication>
#include <QClipboard>
#include <QVBoxLayout>
using namespace TextEditor;
using namespace std::placeholders;
using GitClientMemberFunc = void (GitClient::*)(const QString &) const;

class GitTopicCache : public Core::IVersionControl::TopicCache
{
public:
    GitTopicCache(GitClient *client) :
        m_client(client)
    { }

protected:
    QString trackFile(const QString &repository) override
    {
        const QString gitDir = m_client->findGitDirForRepository(repository);
        return gitDir.isEmpty() ? QString() : (gitDir + "/HEAD");
    }

    QString refreshTopic(const QString &repository) override
    {
        return m_client->synchronousTopic(repository);
    }

private:
    GitClient *m_client;
};
class GitReflogEditorWidget : public GitEditorWidget
public:
    GitReflogEditorWidget()
    {
        setLogEntryPattern("^([0-9a-f]{8,}) [^}]*\\}: .*$");
    }

    QString revisionSubject(const QTextBlock &inBlock) const override
    {
        const QString text = inBlock.text();
        return text.mid(text.indexOf(' ') + 1);
    }
};

class GitLogEditorWidget : public QWidget
{
public:
    GitLogEditorWidget(GitEditorWidget *gitEditor)
    {
        auto vlayout = new QVBoxLayout;
        vlayout->setSpacing(0);
        vlayout->setContentsMargins(0, 0, 0, 0);
        vlayout->addWidget(gitEditor->addFilterWidget());
        vlayout->addWidget(gitEditor);
        setLayout(vlayout);

        auto textAgg = Aggregation::Aggregate::parentAggregate(gitEditor);
        auto agg = textAgg ? textAgg : new Aggregation::Aggregate;
        agg->add(this);
        agg->add(gitEditor);
        setFocusProxy(gitEditor);
    }
};

template<class Editor>
class GitLogEditorWidgetT : public GitLogEditorWidget
{
public:
    GitLogEditorWidgetT() : GitLogEditorWidget(new Editor) {}
};

const unsigned minimumRequiredVersion = 0x010900;

const VcsBaseSubmitEditorParameters submitParameters {
    Git::Constants::SUBMIT_MIMETYPE,
    Git::Constants::GITSUBMITEDITOR_ID,
    Git::Constants::GITSUBMITEDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffRows
};

const VcsBaseEditorParameters svnLogEditorParameters {
    Git::Constants::GIT_SVN_LOG_EDITOR_ID,
    Git::Constants::GIT_SVN_LOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.svnlog"
};

const VcsBaseEditorParameters logEditorParameters {
    LogOutput,
    "text/vnd.qtcreator.git.log"
};

const VcsBaseEditorParameters reflogEditorParameters {
    LogOutput,
    Git::Constants::GIT_REFLOG_EDITOR_ID,
    Git::Constants::GIT_REFLOG_EDITOR_DISPLAY_NAME,
    "text/vnd.qtcreator.git.reflog"
};

const VcsBaseEditorParameters blameEditorParameters {
    AnnotateOutput,
    "text/vnd.qtcreator.git.annotation"
};

const VcsBaseEditorParameters commitTextEditorParameters {
    OtherContent,
    "text/vnd.qtcreator.git.commit"
};

const VcsBaseEditorParameters rebaseEditorParameters {
    OtherContent,
    "text/vnd.qtcreator.git.rebase"
class GitPluginPrivate final : public VcsBase::VcsBasePluginPrivate
{
    Q_OBJECT

public:
    GitPluginPrivate();
    ~GitPluginPrivate() final;

    // IVersionControl
    QString displayName() const final;
    Utils::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FilePath &fileName) const final;

    bool managesDirectory(const QString &directory, QString *topLevel) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;
    QStringList unmanagedFiles(const QString &workingDir, const QStringList &filePaths) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const QString &fileName) final;
    bool vcsAdd(const QString &fileName) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;

    void vcsAnnotate(const QString &file, int line) final;
    void vcsDescribe(const QString &source, const QString &id) final { m_gitClient.show(source, id); };
    QString vcsTopic(const QString &directory) final;

    Core::ShellCommand *createInitialCheckoutCommand(const QString &url,
                                                     const Utils::FilePath &baseDirectory,
                                                     const QString &localName,
                                                     const QStringList &extraArgs) final;

    void fillLinkContextMenu(QMenu *menu,
                             const QString &workingDirectory,
                             const QString &reference) final
    {
        menu->addAction(tr("&Copy \"%1\"").arg(reference),
                        [reference] { QApplication::clipboard()->setText(reference); });
        QAction *action = menu->addAction(tr("&Describe Change %1").arg(reference),
                                          [=] { vcsDescribe(workingDirectory, reference); });
        menu->setDefaultAction(action);
        GitClient::addChangeActions(menu, workingDirectory, reference);
    }

    bool handleLink(const QString &workingDirectory, const QString &reference) final
    {
        if (reference.contains(".."))
            GitClient::instance()->log(workingDirectory, {}, false, {reference});
        else
            GitClient::instance()->show(workingDirectory, reference);
        return true;
    }

    RepoUrl getRepoUrl(const QString &location) const override;

    QStringList additionalToolsPath() const final;

    bool isCommitEditorOpen() const;
    void startCommit(CommitType commitType = SimpleCommit);
    void updateBranches(const QString &repository);
    void updateCurrentBranch();

    void manageRemotes();
    void initRepository();
    void startRebaseFromCommit(const QString &workingDirectory, QString commit);

    void updateActions(VcsBase::VcsBasePluginPrivate::ActionState) override;
    bool submitEditorAboutToClose() override;

    void diffCurrentFile();
    void diffCurrentProject();
    void commitFromEditor() override;
    void logFile();
    void blameFile();
    void logProject();
    void logRepository();
    void reflogRepository();
    void undoFileChanges(bool revertStaging);
    void resetRepository();
    void recoverDeletedFiles();
    void startRebase();
    void startChangeRelatedAction(const Utils::Id &id);
    void stageFile();
    void unstageFile();
    void gitkForCurrentFile();
    void gitkForCurrentFolder();
    void gitGui();
    void gitBash();
    void cleanProject();
    void cleanRepository();
    void updateSubmodules();
    void applyCurrentFilePatch();
    void promptApplyPatch();

    void stash(bool unstagedOnly = false);
    void stashUnstaged();
    void stashSnapshot();
    void stashPop();
    void branchList();
    void stashList();
    void fetch();
    void pull();
    void push();
    void startMergeTool();
    void continueOrAbortCommand();
    void updateContinueAndAbortCommands();
    void delayedPushToGerrit();

    Core::Command *createCommand(QAction *action, Core::ActionContainer *ac, Utils::Id id,
                                 const Core::Context &context, bool addToLocator,
                                 const std::function<void()> &callback, const QKeySequence &keys);

    Utils::ParameterAction *createParameterAction(Core::ActionContainer *ac,
                                                  const QString &defaultText, const QString &parameterText,
                                                  Utils::Id id, const Core::Context &context, bool addToLocator,
                                                  const std::function<void()> &callback,
                                                  const QKeySequence &keys = QKeySequence());

    QAction *createFileAction(Core::ActionContainer *ac,
                              const QString &defaultText, const QString &parameterText,
                              Utils::Id id, const Core::Context &context, bool addToLocator,
                              const std::function<void()> &callback,
                              const QKeySequence &keys = QKeySequence());

    QAction *createProjectAction(Core::ActionContainer *ac,
                                 const QString &defaultText, const QString &parameterText,
                                 Utils::Id id, const Core::Context &context, bool addToLocator,
                                 void (GitPluginPrivate::*func)(),
                                 const QKeySequence &keys = QKeySequence());

    QAction *createRepositoryAction(Core::ActionContainer *ac, const QString &text, Utils::Id id,
                                    const Core::Context &context, bool addToLocator,
                                    const std::function<void()> &callback,
                                    const QKeySequence &keys = QKeySequence());
    QAction *createRepositoryAction(Core::ActionContainer *ac, const QString &text, Utils::Id id,
                                    const Core::Context &context, bool addToLocator,
                                    GitClientMemberFunc, const QKeySequence &keys = QKeySequence());

    QAction *createChangeRelatedRepositoryAction(const QString &text, Utils::Id id,
                                                 const Core::Context &context);

    void updateRepositoryBrowserAction();
    Core::IEditor *openSubmitEditor(const QString &fileName, const CommitData &cd);
    void cleanCommitMessageFile();
    void cleanRepository(const QString &directory);
    void applyPatch(const QString &workingDirectory, QString file = QString());
    void updateVersionWarning();


    void onApplySettings();;

    Core::CommandLocator *m_commandLocator = nullptr;

    QAction *m_menuAction = nullptr;
    QAction *m_repositoryBrowserAction = nullptr;
    QAction *m_mergeToolAction = nullptr;
    QAction *m_submoduleUpdateAction = nullptr;
    QAction *m_abortMergeAction = nullptr;
    QAction *m_abortRebaseAction = nullptr;
    QAction *m_abortCherryPickAction = nullptr;
    QAction *m_abortRevertAction = nullptr;
    QAction *m_skipRebaseAction = nullptr;
    QAction *m_continueRebaseAction = nullptr;
    QAction *m_continueCherryPickAction = nullptr;
    QAction *m_continueRevertAction = nullptr;
    QAction *m_fixupCommitAction = nullptr;
    QAction *m_interactiveRebaseAction = nullptr;

    QVector<Utils::ParameterAction *> m_fileActions;
    QVector<Utils::ParameterAction *> m_projectActions;
    QVector<QAction *> m_repositoryActions;
    Utils::ParameterAction *m_applyCurrentFilePatchAction = nullptr;
    Gerrit::Internal::GerritPlugin *m_gerritPlugin = nullptr;

    GitSettings m_settings;
    GitClient m_gitClient{&m_settings};
    QPointer<StashDialog> m_stashDialog;
    BranchViewFactory m_branchViewFactory;
    QPointer<RemoteDialog> m_remoteDialog;
    QString m_submitRepository;
    QString m_commitMessageFileName;
    bool m_submitActionTriggered = false;

    GitSettingsPage settingPage{&m_settings, std::bind(&GitPluginPrivate::onApplySettings, this)};

    GitGrep gitGrep{&m_gitClient};

    VcsEditorFactory svnLogEditorFactory {
        &svnLogEditorParameters,
        [] { return new GitEditorWidget; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory logEditorFactory {
        &logEditorParameters,
        [] { return new GitLogEditorWidgetT<GitEditorWidget>; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory reflogEditorFactory {
        &reflogEditorParameters,
                [] { return new GitLogEditorWidgetT<GitReflogEditorWidget>; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory blameEditorFactory {
        &blameEditorParameters,
        [] { return new GitEditorWidget; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory commitTextEditorFactory {
        &commitTextEditorParameters,
        [] { return new GitEditorWidget; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory rebaseEditorFactory {
        &rebaseEditorParameters,
        [] { return new GitEditorWidget; },
        std::bind(&GitPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsSubmitEditorFactory submitEditorFactory {
        submitParameters,
        [] { return new GitSubmitEditor; },
        this
    };
};

static GitPluginPrivate *dd = nullptr;
GitPluginPrivate::~GitPluginPrivate()
    cleanCommitMessageFile();
    delete dd;
    dd = nullptr;
void GitPluginPrivate::onApplySettings()
{
    configurationChanged();
    updateRepositoryBrowserAction();
    bool gitFoundOk;
    QString errorMessage;
    m_settings.gitExecutable(&gitFoundOk, &errorMessage);
    if (!gitFoundOk)
        Core::AsynchronousMessageBox::warning(tr("Git Settings"), errorMessage);
}

void GitPluginPrivate::cleanCommitMessageFile()
bool GitPluginPrivate::isCommitEditorOpen() const
GitClient *GitPlugin::client()
    return &dd->m_gitClient;
IVersionControl *GitPlugin::versionControl()
    return dd;
const GitSettings &GitPlugin::settings()
{
    return dd->m_settings;
}

const VcsBasePluginState &GitPlugin::currentState()
{
    return dd->currentState();
}

QString GitPlugin::msgRepositoryLabel(const QString &repository)
{
    return repository.isEmpty() ?
            tr("<No repository>")  :
            tr("Repository: %1").arg(QDir::toNativeSeparators(repository));
}
// Returns a regular expression pattern with characters not allowed
// in branch and remote names.
QString GitPlugin::invalidBranchAndRemoteNamePattern()
{
    return QLatin1String(
        "\\s"     // no whitespace
        "|~"      // no "~"
        "|\\^"    // no "^"
        "|\\["    // no "["
        "|\\.\\." // no ".."
        "|/\\."   // no slashdot
        "|:"      // no ":"
        "|@\\{"   // no "@{" sequence
        "|\\\\"   // no backslash
        "|//"     // no double slash
        "|^[/-]"  // no leading slash or dash
        "|\""     // no quotes
        "|\\*"    // no asterisk
        "|(^|[A-Z]+_)HEAD" // no HEAD, FETCH_HEAD etc.
    );
}

Command *GitPluginPrivate::createCommand(QAction *action, ActionContainer *ac, Id id,
ParameterAction *GitPluginPrivate::createParameterAction(ActionContainer *ac,
QAction *GitPluginPrivate::createFileAction(ActionContainer *ac,
QAction *GitPluginPrivate::createProjectAction(ActionContainer *ac, const QString &defaultText,
                                        bool addToLocator, void (GitPluginPrivate::*func)(),
                                                    addToLocator, std::bind(func, this), keys);
QAction *GitPluginPrivate::createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
QAction *GitPluginPrivate::createChangeRelatedRepositoryAction(const QString &text, Id id,
                                  std::bind(&GitPluginPrivate::startChangeRelatedAction, this, id),
                                  QKeySequence());
QAction *GitPluginPrivate::createRepositoryAction(ActionContainer *ac, const QString &text, Id id,
        (m_gitClient.*func)(currentState().topLevel());
    dd = new GitPluginPrivate;
    auto cmdContext = new QObject(this);
    connect(Core::ICore::instance(), &Core::ICore::coreOpened, cmdContext, [this, cmdContext, arguments] {
        remoteCommand(arguments, QDir::currentPath(), {});
        cmdContext->deleteLater();
    });
    return true;
}

void GitPlugin::extensionsInitialized()
{
    dd->extensionsInitialized()    ;
}

GitPluginPrivate::GitPluginPrivate()
    : VcsBase::VcsBasePluginPrivate(Context(Constants::GIT_CONTEXT))
{
    dd = this;
    setTopicCache(new GitTopicCache(&m_gitClient));
    m_fileActions.reserve(10);
    m_projectActions.reserve(10);
    m_repositoryActions.reserve(50);
    Context context(Constants::GIT_CONTEXT);
    const QString prefix = "git";
    m_commandLocator = new CommandLocator("Git", prefix, prefix, this);
                     "Git.Diff", context, true, std::bind(&GitPluginPrivate::diffCurrentFile, this),
                      QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+D") : tr("Alt+G,Alt+D")));
                     "Git.Log", context, true, std::bind(&GitPluginPrivate::logFile, this),
                     QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+L") : tr("Alt+G,Alt+L")));
                     "Git.Blame", context, true, std::bind(&GitPluginPrivate::blameFile, this),
                     QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+B") : tr("Alt+G,Alt+B")));
                     "Git.Stage", context, true, std::bind(&GitPluginPrivate::stageFile, this),
                     QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+A") : tr("Alt+G,Alt+A")));
                     "Git.Unstage", context, true, std::bind(&GitPluginPrivate::unstageFile, this));
                     true, std::bind(&GitPluginPrivate::undoFileChanges, this, false));
                     true, std::bind(&GitPluginPrivate::undoFileChanges, this, true),
                     QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+U") : tr("Alt+G,Alt+U")));
                        "Git.DiffProject", context, true, &GitPluginPrivate::diffCurrentProject,
                        QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+Shift+D") : tr("Alt+G,Alt+Shift+D")));
                        "Git.LogProject", context, true, &GitPluginPrivate::logProject,
                        QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+K") : tr("Alt+G,Alt+K")));
                        "Git.CleanProject", context, true, &GitPluginPrivate::cleanProject);
                           context, true, std::bind(&GitPluginPrivate::logRepository, this));
                           context, true, std::bind(&GitPluginPrivate::reflogRepository, this));
                           context, true, std::bind(&GitPluginPrivate::startCommit, this, SimpleCommit),
                           QKeySequence(useMacShortcuts ? tr("Meta+G,Meta+C") : tr("Alt+G,Alt+C")));
                           context, true, std::bind(&GitPluginPrivate::startCommit, this, AmendCommit));
                                     std::bind(&GitPluginPrivate::startCommit, this, FixupCommit));
                           context, true, std::bind(&GitPluginPrivate::resetRepository, this));

    createRepositoryAction(localRepositoryMenu, tr("Recover Deleted Files"), "Git.RecoverDeleted",
                           context, true, std::bind(&GitPluginPrivate::recoverDeletedFiles, this));
                                     context, true, std::bind(&GitPluginPrivate::startRebase, this));
                                     context, true, std::bind(&GitPluginPrivate::updateSubmodules, this));
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));

    m_skipRebaseAction
            = createRepositoryAction(localRepositoryMenu,
                                     tr("Skip Rebase"), "Git.RebaseSkip",
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));
                                     context, true,
                                     std::bind(&GitPluginPrivate::continueOrAbortCommand, this));
                           context, true, std::bind(&GitPluginPrivate::branchList, this));
                                    context, true, std::bind(&GitPluginPrivate::applyCurrentFilePatch, this));
                           context, true, std::bind(&GitPluginPrivate::promptApplyPatch, this));
                           context, false, std::bind(&GitPluginPrivate::stashList, this));
                                             context, true, std::bind(&GitPluginPrivate::stash, this, false));
                                    context, true, std::bind(&GitPluginPrivate::stashUnstaged, this));
                                    context, true, std::bind(&GitPluginPrivate::stashSnapshot, this));
                                    context, true, std::bind(&GitPluginPrivate::stashPop, this));
                           context, true, std::bind(&GitPluginPrivate::fetch, this));
                           context, true, std::bind(&GitPluginPrivate::pull, this));
                           context, true, std::bind(&GitPluginPrivate::push, this));
    createRepositoryAction(subversionMenu, tr("DCommit"), "Git.Subversion.DCommit",
                           context, false, &GitClient::subversionDeltaCommit);

                           context, false, std::bind(&GitPluginPrivate::manageRemotes, this));
    createChangeRelatedRepositoryAction(tr("Archive..."), "Git.Archive", context);
    createRepositoryAction(nullptr, tr("Rebase..."), "Git.Rebase", context, true,
                           std::bind(&GitPluginPrivate::branchList, this));
    createRepositoryAction(nullptr, tr("Merge..."), "Git.Merge", context, true,
                           std::bind(&GitPluginPrivate::branchList, this));
                     "Git.GitkFile", context, true, std::bind(&GitPluginPrivate::gitkForCurrentFile, this));
                     "Git.GitkFolder", context, true, std::bind(&GitPluginPrivate::gitkForCurrentFolder, this));
                           context, true, std::bind(&GitPluginPrivate::gitGui, this));
                                     context, true, std::bind(&GitPluginPrivate::startMergeTool, this));

    // --------------
    if (Utils::HostOsInfo::isWindowsHost()) {
        gitToolsMenu->addSeparator(context);

        createRepositoryAction(gitToolsMenu, tr("Git Bash"), "Git.GitBash",
                               context, true, std::bind(&GitPluginPrivate::gitBash, this));
    }
    connect(createRepositoryAction, &QAction::triggered, this, &GitPluginPrivate::createRepository);
            this, &GitPluginPrivate::updateContinueAndAbortCommands);
            this, &GitPluginPrivate::updateBranches, Qt::QueuedConnection);
    m_gerritPlugin->initialize(remoteRepositoryMenu);
    m_gerritPlugin->updateActions(currentState());
void GitPluginPrivate::diffCurrentFile()
    m_gitClient.diffFile(state.currentFileTopLevel(), state.relativeCurrentFile());
void GitPluginPrivate::diffCurrentProject()
        m_gitClient.diffRepository(state.currentProjectTopLevel());
        m_gitClient.diffProject(state.currentProjectTopLevel(), relativeProject);
void GitPluginPrivate::logFile()
    m_gitClient.log(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
void GitPluginPrivate::blameFile()
    QStringList extraOptions;
    int firstLine = -1;
    if (BaseTextEditor *textEditor = BaseTextEditor::currentTextEditor()) {
        QTextCursor cursor = textEditor->textCursor();
        if (cursor.hasSelection()) {
            QString argument = "-L ";
            int selectionStart = cursor.selectionStart();
            int selectionEnd = cursor.selectionEnd();
            cursor.setPosition(selectionStart);
            const int startBlock = cursor.blockNumber();
            cursor.setPosition(selectionEnd);
            int endBlock = cursor.blockNumber();
            if (startBlock != endBlock) {
                firstLine = startBlock + 1;
                if (cursor.atBlockStart())
                    --endBlock;
                if (auto widget = qobject_cast<VcsBaseEditorWidget *>(textEditor->widget())) {
                    const int previousFirstLine = widget->firstLineNumber();
                    if (previousFirstLine > 0)
                        firstLine = previousFirstLine;
                }
                argument += QString::number(firstLine) + ',';
                argument += QString::number(endBlock + firstLine - startBlock);
                extraOptions << argument;
            }
        }
    }
    VcsBaseEditorWidget *editor = m_gitClient.annotate(
                state.currentFileTopLevel(), state.relativeCurrentFile(), QString(),
                lineNumber, extraOptions);
    if (firstLine > 0)
        editor->setFirstLineNumber(firstLine);
void GitPluginPrivate::logProject()
    m_gitClient.log(state.currentProjectTopLevel(), state.relativeCurrentProject());
void GitPluginPrivate::logRepository()
    m_gitClient.log(state.topLevel());
void GitPluginPrivate::reflogRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.reflog(state.topLevel());
}

void GitPluginPrivate::undoFileChanges(bool revertStaging)
        if (!DocumentManager::saveModifiedDocumentSilently(document))
    m_gitClient.revert({state.currentFile()}, revertStaging);
        : IconItemDelegate(widget, Utils::Icons::UNDO)
void GitPluginPrivate::resetRepository()
    LogChangeDialog dialog(true, ICore::dialogParent());
        m_gitClient.reset(topLevel, dialog.resetFlag(), dialog.commit());
void GitPluginPrivate::recoverDeletedFiles()
    m_gitClient.recoverDeletedFiles(state.topLevel());
}

void GitPluginPrivate::startRebase()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    startRebaseFromCommit(topLevel, QString());
}

void GitPluginPrivate::startRebaseFromCommit(const QString &workingDirectory, QString commit)
{
    if (!DocumentManager::saveAllModifiedDocuments())
    if (workingDirectory.isEmpty() || !m_gitClient.canRebase(workingDirectory))

    if (commit.isEmpty()) {
        LogChangeDialog dialog(false, ICore::dialogParent());
        RebaseItemDelegate delegate(dialog.widget());
        dialog.setWindowTitle(tr("Interactive Rebase"));
        if (!dialog.runDialog(workingDirectory))
            return;
        commit = dialog.commit();
    }

    if (m_gitClient.beginStashScope(workingDirectory, "Rebase-i"))
        m_gitClient.interactiveRebase(workingDirectory, commit, false);
void GitPluginPrivate::startChangeRelatedAction(const Id &id)
                                 id, ICore::dialogParent());
        m_gitClient.show(workingDirectory, change);
        return;
    }

    if (dialog.command() == Archive) {
        m_gitClient.archive(workingDirectory, change);
        m_gitClient.synchronousCherryPick(workingDirectory, change);
        m_gitClient.synchronousRevert(workingDirectory, change);
        m_gitClient.checkout(workingDirectory, change);
void GitPluginPrivate::stageFile()
    m_gitClient.addFile(state.currentFileTopLevel(), state.relativeCurrentFile());
void GitPluginPrivate::unstageFile()
    m_gitClient.synchronousReset(state.currentFileTopLevel(), {state.relativeCurrentFile()});
void GitPluginPrivate::gitkForCurrentFile()
    m_gitClient.launchGitK(state.currentFileTopLevel(), state.relativeCurrentFile());
void GitPluginPrivate::gitkForCurrentFolder()
     *  m_gitClient.launchGitK(dir.currentFileDirectory(), ".");
    if (QFileInfo(dir,".git").exists() || dir.cd(".git")) {
        m_gitClient.launchGitK(state.currentFileDirectory());
        m_gitClient.launchGitK(dir.absolutePath(), folderName);
void GitPluginPrivate::gitGui()
    m_gitClient.launchGitGui(state.topLevel());
void GitPluginPrivate::gitBash()
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_gitClient.launchGitBash(state.topLevel());
void GitPluginPrivate::startCommit(CommitType commitType)
    if (!promptBeforeCommit())
        return;
    if (!m_gitClient.getCommitData(state.topLevel(), &commitTemplate, data, &errorMessage)) {
void GitPluginPrivate::updateVersionWarning()
    unsigned version = m_gitClient.gitVersion();
                        InfoBarEntry::GlobalSuppression::Enabled));
IEditor *GitPluginPrivate::openSubmitEditor(const QString &fileName, const CommitData &cd)
    auto submitEditor = qobject_cast<GitSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return nullptr);
    VcsBase::setSource(document, m_submitRepository);
void GitPluginPrivate::commitFromEditor()
bool GitPluginPrivate::submitEditorAboutToClose()
    auto editor = qobject_cast<GitSubmitEditor *>(submitEditor());
            = editor->promptSubmit(this, nullptr, !m_submitActionTriggered, false);
    auto model = qobject_cast<SubmitFileModel *>(editor->fileModel());
        if (!m_gitClient.addAndCommit(m_submitRepository, editor->panelData(), commitType,
                                       amendSHA1, m_commitMessageFileName, model)) {
            editor->updateFileModel();
            return false;
        }
        if (!m_gitClient.beginStashScope(m_submitRepository, "Rebase-fixup",
        m_gitClient.interactiveRebase(m_submitRepository, amendSHA1, true);
        m_gitClient.continueCommandIfNeeded(m_submitRepository);
        if (editor->panelData().pushAction == NormalPush) {
            m_gitClient.push(m_submitRepository);
        } else if (editor->panelData().pushAction == PushToGerrit) {
            connect(editor, &QObject::destroyed, this, &GitPluginPrivate::delayedPushToGerrit,
                    Qt::QueuedConnection);
        }
void GitPluginPrivate::fetch()
    m_gitClient.fetch(currentState().topLevel(), QString());
void GitPluginPrivate::pull()
    bool rebase = m_settings.boolValue(GitSettings::pullRebaseKey);
        QString currentBranch = m_gitClient.synchronousCurrentLocalBranch(topLevel);
            currentBranch.prepend("branch.");
            currentBranch.append(".rebase");
            rebase = (m_gitClient.readConfigValue(topLevel, currentBranch) == "true");
    if (!m_gitClient.beginStashScope(topLevel, "Pull", rebase ? Default : AllowUnstashed))
    m_gitClient.pull(topLevel, rebase);
void GitPluginPrivate::push()
    m_gitClient.push(state.topLevel());
void GitPluginPrivate::startMergeTool()
    m_gitClient.merge(state.topLevel());
void GitPluginPrivate::continueOrAbortCommand()
        m_gitClient.synchronousMerge(state.topLevel(), "--abort");
        m_gitClient.rebase(state.topLevel(), "--abort");
        m_gitClient.synchronousCherryPick(state.topLevel(), "--abort");
        m_gitClient.synchronousRevert(state.topLevel(), "--abort");
    else if (action == m_skipRebaseAction)
        m_gitClient.rebase(state.topLevel(), "--skip");
        m_gitClient.rebase(state.topLevel(), "--continue");
        m_gitClient.cherryPick(state.topLevel(), "--continue");
        m_gitClient.revert(state.topLevel(), "--continue");
void GitPluginPrivate::cleanProject()
void GitPluginPrivate::cleanRepository()
void GitPluginPrivate::cleanRepository(const QString &directory)
    const bool gotFiles = m_gitClient.synchronousCleanList(directory, QString(), &files, &ignoredFiles, &errorMessage);
        Core::AsynchronousMessageBox::warning(tr("Unable to Retrieve File List"), errorMessage);
void GitPluginPrivate::updateSubmodules()
    m_gitClient.updateSubmodulesIfNeeded(state.topLevel(), false);
void GitPluginPrivate::applyCurrentFilePatch()
void GitPluginPrivate::promptApplyPatch()
void GitPluginPrivate::applyPatch(const QString &workingDirectory, QString file)
    if (!m_gitClient.beginStashScope(workingDirectory, "Apply-Patch", AllowUnstashed))
        file = QFileDialog::getOpenFileName(ICore::dialogParent(), tr("Choose Patch"), QString(), filter);
            m_gitClient.endStashScope(workingDirectory);
    if (m_gitClient.synchronousApplyPatch(workingDirectory, file, &errorMessage)) {
    m_gitClient.endStashScope(workingDirectory);
void GitPluginPrivate::stash(bool unstagedOnly)
    m_gitClient.executeSynchronousStash(topLevel, QString(), unstagedOnly);
void GitPluginPrivate::stashUnstaged()
void GitPluginPrivate::stashSnapshot()
    const QString id = m_gitClient.synchronousStash(state.topLevel(), QString(),
void GitPluginPrivate::stashPop()
    m_gitClient.stashPop(repository);
        dialog = new NonModalDialog(ICore::dialogParent());
void GitPluginPrivate::branchList()
    ModeManager::activateMode(Core::Constants::MODE_EDIT);
    NavigationWidget::activateSubWidget(Constants::GIT_BRANCH_VIEW_ID, Side::Right);
void GitPluginPrivate::manageRemotes()
void GitPluginPrivate::initRepository()
{
    createRepository();
}

void GitPluginPrivate::stashList()
void GitPluginPrivate::updateActions(VcsBasePluginPrivate::ActionState as)
    const VcsBasePluginState state = currentState();
    const bool repositoryEnabled = state.hasTopLevel();
        m_stashDialog->refresh(state.topLevel(), false);
    if (m_branchViewFactory.view())
        m_branchViewFactory.view()->refresh(state.topLevel(), false);
        m_remoteDialog->refresh(state.topLevel(), false);
    const QString fileName = Utils::quoteAmpersands(state.currentFileName());
    for (ParameterAction *fileAction : qAsConst(m_fileActions))
    m_applyCurrentFilePatchAction->setParameter(state.currentPatchFileDisplayName());
    const QString projectName = state.currentProjectName();
    for (ParameterAction *projectAction : qAsConst(m_projectActions))
    for (QAction *repositoryAction : qAsConst(m_repositoryActions))
            && !m_gitClient.submoduleList(state.topLevel()).isEmpty());
    m_gerritPlugin->updateActions(state);
void GitPluginPrivate::updateContinueAndAbortCommands()
                m_gitClient.checkCommandInProgress(currentState().topLevel());
        m_skipRebaseAction->setVisible(gitCommandInProgress == GitClient::Rebase
                                        || gitCommandInProgress == GitClient::RebaseMerge);
        m_skipRebaseAction->setVisible(false);
void GitPluginPrivate::delayedPushToGerrit()
void GitPluginPrivate::updateBranches(const QString &repository)
    if (m_branchViewFactory.view())
        m_branchViewFactory.view()->refreshIfSame(repository);
void GitPluginPrivate::updateCurrentBranch()
{
    if (m_branchViewFactory.view())
        m_branchViewFactory.view()->refreshCurrentBranch();
}

QObject *GitPlugin::remoteCommand(const QStringList &options, const QString &workingDirectory,
                                  const QStringList &)
{
    if (options.size() < 2)
        return nullptr;

    if (options.first() == "-git-show")
        dd->m_gitClient.show(workingDirectory, options.at(1));
    return nullptr;
}

void GitPluginPrivate::updateRepositoryBrowserAction()
            = !m_settings.stringValue(GitSettings::repositoryBrowserCmd).isEmpty();
QString GitPluginPrivate::displayName() const
{
    return QLatin1String("Git");
}

Utils::Id GitPluginPrivate::id() const
{
    return Utils::Id(VcsBase::Constants::VCS_ID_GIT);
}

bool GitPluginPrivate::isVcsFileOrDirectory(const Utils::FilePath &fileName) const
{
    if (fileName.fileName().compare(".git", Utils::HostOsInfo::fileNameCaseSensitivity()))
        return false;
    if (fileName.isDir())
        return true;
    QFile file(fileName.toString());
    if (!file.open(QFile::ReadOnly))
        return false;
    return file.read(8) == "gitdir: ";
}

bool GitPluginPrivate::isConfigured() const
{
    return !m_gitClient.vcsBinary().isEmpty();
}

bool GitPluginPrivate::supportsOperation(Operation operation) const
{
    if (!isConfigured())
        return false;

    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case AnnotateOperation:
    case InitialCheckoutOperation:
        return true;
    }
    return false;
}

bool GitPluginPrivate::vcsOpen(const QString & /*fileName*/)
{
    return false;
}

bool GitPluginPrivate::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_gitClient.synchronousAdd(fi.absolutePath(), {fi.fileName()});
}

bool GitPluginPrivate::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return m_gitClient.synchronousDelete(fi.absolutePath(), true, {fi.fileName()});
}

bool GitPluginPrivate::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo fromInfo(from);
    const QFileInfo toInfo(to);
    return m_gitClient.synchronousMove(fromInfo.absolutePath(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool GitPluginPrivate::vcsCreateRepository(const QString &directory)
{
    return m_gitClient.synchronousInit(directory);
}

QString GitPluginPrivate::vcsTopic(const QString &directory)
    QString topic = Core::IVersionControl::vcsTopic(directory);
    const QString commandInProgress = m_gitClient.commandInProgressDescription(directory);
    if (!commandInProgress.isEmpty())
        topic += " (" + commandInProgress + ')';
    return topic;
}

Core::ShellCommand *GitPluginPrivate::createInitialCheckoutCommand(const QString &url,
                                                                    const Utils::FilePath &baseDirectory,
                                                                    const QString &localName,
                                                                    const QStringList &extraArgs)
{
    QStringList args = {"clone", "--progress"};
    args << extraArgs << url << localName;

    auto command = new VcsBase::VcsCommand(baseDirectory.toString(), m_gitClient.processEnvironment());
    command->addFlags(VcsBase::VcsCommand::SuppressStdErr);
    command->addJob({m_gitClient.vcsBinary(), args}, -1);
    return command;
}

GitPluginPrivate::RepoUrl GitPluginPrivate::getRepoUrl(const QString &location) const
{
    return GitRemote(location);
}

QStringList GitPluginPrivate::additionalToolsPath() const
{
    QStringList res = m_gitClient.settings().searchPathList();
    const QString binaryPath = m_gitClient.gitBinDirectory().toString();
    if (!binaryPath.isEmpty() && !res.contains(binaryPath))
        res << binaryPath;
    return res;
}

bool GitPluginPrivate::managesDirectory(const QString &directory, QString *topLevel) const
{
    const QString topLevelFound = m_gitClient.findRepositoryForDirectory(directory);
    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

bool GitPluginPrivate::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    return m_gitClient.managesFile(workingDirectory, fileName);
}

QStringList GitPluginPrivate::unmanagedFiles(const QString &workingDir,
                                              const QStringList &filePaths) const
{
    return m_gitClient.unmanagedFiles(workingDir, filePaths);
}

void GitPluginPrivate::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    m_gitClient.annotate(fi.absolutePath(), fi.fileName(), QString(), line);
}

void GitPlugin::emitFilesChanged(const QStringList &l)
{
    emit dd->filesChanged(l);
}

void GitPlugin::emitRepositoryChanged(const QString &r)
{
    emit dd->repositoryChanged(r);
}

void GitPlugin::startRebaseFromCommit(const QString &workingDirectory, const QString &commit)
{
    dd->startRebaseFromCommit(workingDirectory, commit);
}

void GitPlugin::manageRemotes()
{
    dd->manageRemotes();
}

void GitPlugin::initRepository()
{
    dd->initRepository();
}

void GitPlugin::startCommit()
{
    dd->startCommit();
}

void GitPlugin::updateCurrentBranch()
{
    dd->updateCurrentBranch();
}

void GitPlugin::updateBranches(const QString &repository)
{
    dd->updateBranches(repository);
}

void GitPlugin::gerritPush(const QString &topLevel)
{
    dd->m_gerritPlugin->push(topLevel);
}

bool GitPlugin::isCommitEditorOpen()
{
    return dd->isCommitEditorOpen();
    QTest::newRow(" T") << FileStates(TypeChangedFile) << FileStates(UnknownFileState);
    QTest::newRow("T ") << (TypeChangedFile | StagedFile) << FileStates(UnknownFileState);
    QTest::newRow("TM") << (TypeChangedFile | StagedFile) << FileStates(ModifiedFile);
    QTest::newRow("MT") << (ModifiedFile | StagedFile) << FileStates(TypeChangedFile);
    QString output = "## master...origin/master [ahead 1]\n";
    output += QString::fromLatin1(QTest::currentDataTag()) + " main.cpp\n";
    VcsBaseEditorWidget::testDiffFileResolving(dd->commitTextEditorFactory);
    VcsBaseEditorWidget::testLogResolving(dd->logEditorFactory, data,

class RemoteTest {
public:
    RemoteTest() = default;
    explicit RemoteTest(const QString &url): m_url(url) {}

    inline RemoteTest &protocol(const QString &p) { m_protocol = p; return *this; }
    inline RemoteTest &userName(const QString &u) { m_userName = u; return *this; }
    inline RemoteTest &host(const QString &h) { m_host = h; return *this; }
    inline RemoteTest &port(quint16 p) { m_port = p; return *this; }
    inline RemoteTest &path(const QString &p) { m_path = p; return *this; }
    inline RemoteTest &isLocal(bool l) { m_isLocal = l; return *this; }
    inline RemoteTest &isValid(bool v) { m_isValid = v; return *this; }

    const QString m_url;
    QString m_protocol;
    QString m_userName;
    QString m_host;
    QString m_path;
    quint16 m_port = 0;
    bool    m_isLocal = false;
    bool    m_isValid = true;
};

} // namespace Internal
} // namespace Git

Q_DECLARE_METATYPE(Git::Internal::RemoteTest)

namespace Git {
namespace Internal {

void GitPlugin::testGitRemote_data()
{
    QTest::addColumn<RemoteTest>("rt");

    QTest::newRow("http-no-user")
            << RemoteTest("http://code.qt.io/qt-creator/qt-creator.git")
               .protocol("http")
               .host("code.qt.io")
               .path("/qt-creator/qt-creator.git");
    QTest::newRow("https-with-port")
            << RemoteTest("https://code.qt.io:80/qt-creator/qt-creator.git")
               .protocol("https")
               .host("code.qt.io")
               .port(80)
               .path("/qt-creator/qt-creator.git");
    QTest::newRow("invalid-port")
            << RemoteTest("https://code.qt.io:99999/qt-creator/qt-creator.git")
               .protocol("https")
               .host("code.qt.io")
               .path("/qt-creator/qt-creator.git")
               .isValid(false);
    QTest::newRow("ssh-user-foo")
            << RemoteTest("ssh://foo@codereview.qt-project.org:29418/qt-creator/qt-creator.git")
               .protocol("ssh")
               .userName("foo")
               .host("codereview.qt-project.org")
               .port(29418)
               .path("/qt-creator/qt-creator.git");
    QTest::newRow("ssh-github")
            << RemoteTest("git@github.com:qt-creator/qt-creator.git")
               .userName("git")
               .host("github.com")
               .path("qt-creator/qt-creator.git");
    QTest::newRow("local-file-protocol")
            << RemoteTest("file:///tmp/myrepo.git")
               .protocol("file")
               .path("/tmp/myrepo.git")
               .isLocal(true);
    QTest::newRow("local-absolute-path-unix")
            << RemoteTest("/tmp/myrepo.git")
               .protocol("file")
               .path("/tmp/myrepo.git")
               .isLocal(true);
    if (Utils::HostOsInfo::isWindowsHost()) {
        QTest::newRow("local-absolute-path-unix")
                << RemoteTest("c:/git/myrepo.git")
                   .protocol("file")
                   .path("c:/git/myrepo.git")
                   .isLocal(true);
    }
    QTest::newRow("local-relative-path-children")
            << RemoteTest("./git/myrepo.git")
               .protocol("file")
               .path("./git/myrepo.git")
               .isLocal(true);
    QTest::newRow("local-relative-path-parent")
            << RemoteTest("../myrepo.git")
               .protocol("file")
               .path("../myrepo.git")
               .isLocal(true);
}

void GitPlugin::testGitRemote()
{
    QFETCH(RemoteTest, rt);

    const GitRemote remote = GitRemote(rt.m_url);

    if (!rt.m_isLocal) {
        // local repositories must exist to be valid, so skip the test
        QCOMPARE(remote.isValid, rt.m_isValid);
    }

    QCOMPARE(remote.protocol, rt.m_protocol);
    QCOMPARE(remote.userName, rt.m_userName);
    QCOMPARE(remote.host,     rt.m_host);
    QCOMPARE(remote.port,     rt.m_port);
    QCOMPARE(remote.path,     rt.m_path);
}


#include "gitplugin.moc"