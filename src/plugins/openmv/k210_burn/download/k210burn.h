#ifndef K210BURN_H
#define K210BURN_H

#include <QtCore>
#include <QObject>
#include <QtSerialPort>
#include <QThread>

#include "k210burnRam.h"
#include "k210burnFlash.h"

typedef enum {
    ERROR_NONE = 0,

    ERROR_PORT_OP_FAIL,
    ERROR_OPENPORT_FAIL,

    ERROR_GREETING_FAIL,
    ERROR_MEMORY_WRITE_FAIL,

    ERROR_FLASH_GREETING_FAIL,
    ERROR_FLASH_INIT_FAIL,
    ERROR_FLASH_WRITE_FAIL,

    ERROR_FLASH_PARSE_FILES_FAIL,

    ERROR_FLASH_ERASE_INVAILD_PARAM,
    ERROR_FLASH_ERASE_FAIL,

    ERROR_MAX = 255,
} burnErrcode_t;

typedef enum {
    eraseSection,
    eraseFullChip,
} burneraseType_t;

class K210BurnPrivate : public QObject
{
    Q_OBJECT
public:
    explicit K210BurnPrivate(QObject *parent = nullptr);
    ~K210BurnPrivate();

    void K210BurnStop(void);

    void K210BurnOpenPort(const QString &portName);
    void K210BurnReleasePort(void);

    void K210BurnRam(int mode, const QString &loader);
    void K210BurnFlash(int baud, const QList <burnFirmwareInfo_t> &files);

    void K210BurnErase(burneraseType_t type, int addr, int size);

signals:
    void openResult(burnErrcode_t errCode, const QString &errorMessage);

    void burnStoped(void);

    void burnRamProcess(int total, int current);
    void burnRamResult(burnErrcode_t errCode, const QString &errMessage);

    void burnFlashProcess(int total, int current);
    void burnFlashResult(burnErrcode_t errCode, const QString &errMessage);

    void burnEraseResult(burnErrcode_t errCode, const QString &errMessage);

public slots:

private:
    bool resetToISP(int mode);
    bool resetToBOOT(void);

    bool burnRamHandShakeSpecBaud(int mode, int baud);

    bool burnFlashReboot(void);
    bool burnFlashInitFlash(void);
    bool burnFlashHandShakeSpecBaud(int baud);

    int ispMode = 0;

    bool forceStop = false;
    int retry_times = 3;

    OpenMVPluginSerialPort_thing *m_port;

    K210RamBurner m_ramBurner;
    K210FlashBurner m_flashBurner;
};

class K210Burn : public QObject
{
    Q_OBJECT
public:
    explicit K210Burn(QObject *parent = nullptr);
    ~K210Burn();

signals:
    void open(const QString &portName);
    void openResult(burnErrcode_t errCode, const QString &errorMessage);

    void burnReleasePort(void);

    void burnStop(void);
    void burnStoped(void);

    void burnRam(int mode, const QString &loader);
    void burnRamProcess(int total, int current);
    void burnRamResult(burnErrcode_t errCode, const QString &errMessage);

    void burnFlash(int baud, const QList <burnFirmwareInfo_t> &files);
    void burnFlashProcess(int total, int current);
    void burnFlashResult(burnErrcode_t errCode, const QString &errMessage);

    void burnErase(burneraseType_t type, int addr, int size);
    void burnEraseResult(burnErrcode_t errCode, const QString &errMessage);

private:

};

#endif // K210BURN_H
