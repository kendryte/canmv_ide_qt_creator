#ifndef K210BURNRAM_H
#define K210BURNRAM_H

#include <QtCore>
#include <QObject>

#include "util.h"

class K210RamBurner : public QObject
{
    Q_OBJECT
public:
    explicit K210RamBurner(QObject *parent = nullptr) : QObject(parent) { }
    ~K210RamBurner();

    burnResp_t handShake(OpenMVPluginSerialPort_thing *port);

    burnResp_t memoryWrite(OpenMVPluginSerialPort_thing *port, int addr, int len, const QByteArray &data);
    void memoryBoot(OpenMVPluginSerialPort_thing *port, int addr);

signals:

public slots:

private:
    int timeout_ms = 500; // wait resp timeout
};

#endif // K210BURNRAM_H
