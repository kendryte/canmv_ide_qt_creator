#ifndef K210BURNFLASH_H
#define K210BURNFLASH_H

#include <QtCore>
#include <QObject>

#include "util.h"

class K210FlashBurner : public QObject
{
    Q_OBJECT
public:
    explicit K210FlashBurner(QObject *parent = nullptr) : QObject(parent) { }
    ~K210FlashBurner();

    burnResp_t handShake(OpenMVPluginSerialPort_thing *port);
    void change_baudrate(OpenMVPluginSerialPort_thing *port, int baud);

    burnResp_t flashInit(OpenMVPluginSerialPort_thing *port, int chip_type = 1);
    burnResp_t flashWrite(OpenMVPluginSerialPort_thing *port, int addr, int len, const QByteArray &data);

    burnResp_t flashErase(OpenMVPluginSerialPort_thing *port, int addr, int size);
    burnResp_t flashEraseStatus(OpenMVPluginSerialPort_thing *port);

    burnResp_t flashReboot(OpenMVPluginSerialPort_thing *port);
signals:

public slots:

private:
    int timeout_ms = 1500; // wait resp timeout
};

#endif // K210BURNFLASH_H
