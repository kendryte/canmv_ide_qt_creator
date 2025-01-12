#ifndef BOSSAC_H
#define BOSSAC_H

#include <QtCore>
#include <QtWidgets>

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/hostosinfo.h>
#include <utils/synchronousprocess.h>

#define BOSSAC_SETTINGS_GROUP "OpenMVBOSSAC"
#define LAST_BOSSAC_TERMINAL_WINDOW_GEOMETRY "LastBOSSACTerminalWindowGeometry"

void bossacRunBootloader(Utils::SynchronousProcess &process, Utils::SynchronousProcessResponse &response, const QString &device);
void bossacDownloadFirmware(QString &command, Utils::SynchronousProcess &process, Utils::SynchronousProcessResponse &response, const QString &path, const QString &device, const QString &moreArgs = QString());

#endif // BOSSAC_H
