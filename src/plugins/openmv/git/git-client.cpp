#include "git-client.h"

#include <QDebug>

#include <QDir>
#include <QProcessEnvironment>

using namespace Utils;

namespace Git {

///////////////////////////////////////////////////////////////////////////////
// GitClient
///////////////////////////////////////////////////////////////////////////////
GitClient::GitClient()
{

}

Utils::FileName GitClient::gitBinary() const
{
    return Utils::Environment::systemEnvironment().searchInPath(QLatin1String("git"), QStringList());
}

Utils::SynchronousProcessResponse GitClient::runGitCommand(const QString &workingDir,
                                                            const QStringList &args,
                                                            unsigned flags) const
{
    Utils::ShellCommand command(workingDir, QProcessEnvironment::systemEnvironment());

    command.addFlags(flags);

    return command.runCommand(gitBinary(), args, 30);
}

bool GitClient::cloneRepository(const QString &directory, const QByteArray &url, const QStringList &args)
{
    const unsigned flags = Utils::ShellCommand::MergeOutputChannels;

    QDir workingDirectory(directory);

    QStringList arguments(QLatin1String("clone"));
    arguments << QLatin1String(url) << workingDirectory.dirName();
    arguments.append(args);
    workingDirectory.cdUp();

    const SynchronousProcessResponse resp = runGitCommand(workingDirectory.path(), arguments, flags);

    return (resp.result == SynchronousProcessResponse::Finished);
}

} // namespace Git
