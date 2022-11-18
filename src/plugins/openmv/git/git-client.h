#pragma once

// #include "vcs.h"

#include <coreplugin/shellcommand.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>

QT_BEGIN_NAMESPACE
class QProcessEnvironment;
QT_END_NAMESPACE

namespace Utils {
    class ExitCodeInterpreter;
    struct SynchronousProcessResponse;
} // namespace Utils

namespace Git {

class GitClient : public QObject
{
    Q_OBJECT

public:
    explicit GitClient();

    Utils::FileName gitBinary() const;

    bool cloneRepository(const QString &directory, const QByteArray &url, const QStringList &args);

public slots:

private:
    Utils::SynchronousProcessResponse runGitCommand(const QString &workingDir,
                                                        const QStringList &args,
                                                        unsigned flags) const;
};

} // namespace Git
