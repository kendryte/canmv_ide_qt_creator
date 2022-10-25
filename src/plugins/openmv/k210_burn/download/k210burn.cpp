#include "k210burn.h"

#include <QMetaType>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private ////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool K210BurnPrivate::resetToISP(int mode)
{
    switch (mode)
    {
    default:
    case 0:
        { // dan
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(true))) {
                return false;
            }
            QThread::msleep(100);
            if((!m_port->setDataTerminalReady(true)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);

            ispMode = 0;
        }
        break;
    case 1:
        { // kd233
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);
            if((!m_port->setDataTerminalReady(true)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(true))) {
                return false;
            }
            QThread::msleep(100);

            ispMode = 1;
        }
        break;
    }
    return true;
}

bool K210BurnPrivate::resetToBOOT()
{
    switch (ispMode)
    {
    default:
    case 0:
        { // dan
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(true))) {
                return false;
            }
            QThread::msleep(100);
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);
        }
        break;
    case 1:
        { // kd233
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);
            if((!m_port->setDataTerminalReady(true)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);
            if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(false))) {
                return false;
            }
            QThread::msleep(100);
        }
        break;
    }
    return true;
}

bool K210BurnPrivate::burnRamHandShakeSpecBaud(int mode, int baud)
{
    burnResp_t resp;

    if(!m_port->setBaudRate(baud))
    {
        emit burnRamResult(ERROR_PORT_OP_FAIL, m_port->errorString());

        delete m_port;
        m_port = Q_NULLPTR;

        return false;
    }

    m_port->readAll();

    for(int i = 0; i < (retry_times * 2); i++)
    {
        if(forceStop)
        {
            emit burnStoped();
            return false;
        }

        if(false == resetToISP(i % 2))
        {
            emit burnRamResult(ERROR_PORT_OP_FAIL, tr("StubISP Geeeting Failed."));
            return false;
        }

        resp = m_ramBurner.handShake(m_port);
        if((ISP_NOP == resp.op) && (ISP_RET_OK == resp.reason)) {
            break;
        }
        QThread::msleep(100);
    }

    if((ISP_NOP != resp.op) || (ISP_RET_OK != resp.reason))
    {
        emit burnRamResult(ERROR_GREETING_FAIL, resp.text);
        return false;
    }

    return true;
}

bool K210BurnPrivate::burnFlashHandShakeSpecBaud(int baud)
{
    burnResp_t resp;

    if(!m_port->setBaudRate(baud))
    {
        emit burnFlashResult(ERROR_PORT_OP_FAIL, m_port->errorString());

        delete m_port;
        m_port = Q_NULLPTR;

        return false;
    }

    m_port->readAll();

    for(int i = 0; i < retry_times; i++)
    {
        if(forceStop)
        {
            emit burnStoped();
            return false;
        }

        resp = m_flashBurner.handShake(m_port);

        if((FLASH_ISP_NOP == resp.op) && (ISP_RET_OK == resp.reason)) {
            break;
        }
        QThread::msleep(100);
    }

    if((FLASH_ISP_NOP != resp.op) || (ISP_RET_OK != resp.reason))
    {
        emit burnFlashResult(ERROR_FLASH_GREETING_FAIL, resp.text);
        return false;
    }

    return true;
}

bool K210BurnPrivate::burnFlashInitFlash(void)
{
    burnResp_t resp;

    for(int i = 0; i < retry_times; i++)
    {
        if(forceStop)
        {
            emit burnStoped();
            return false;
        }

        resp = m_flashBurner.flashInit(m_port);

        if((FLASH_ISP_FLASH_INIT == resp.op) && (ISP_RET_OK == resp.reason)) {
            break;
        }
        QThread::msleep(100);
    }

    if((FLASH_ISP_FLASH_INIT != resp.op) || (ISP_RET_OK != resp.reason))
    {
        emit burnFlashResult(ERROR_FLASH_INIT_FAIL, resp.text);
        return false;
    }

    return true;
}

bool K210BurnPrivate::burnFlashReboot(void)
{
    burnResp_t resp;

    for(int i = 0; i < retry_times; i++)
    {
        if(forceStop)
        {
            emit burnStoped();
            return true;
        }

        resp = m_flashBurner.flashReboot(m_port);

        if((FLASH_ISP_REBOOT == resp.op) && (ISP_RET_OK == resp.reason))
        {
            break;
        }
        else if((FLASH_ISP_REBOOT == resp.op) && (ISP_RET_FLASH_FLASH_BUSY == resp.reason))
        {
            QEventLoop loop;
            QTimer::singleShot(1000 * 5, &loop, &QEventLoop::quit);
            loop.exec();
        }
        else
        {
            QThread::msleep(100);
        }
    }

    if((FLASH_ISP_REBOOT != resp.op) || (ISP_RET_OK != resp.reason)) {
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public /////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void K210BurnPrivate::K210BurnStop(void)
{
    // qDebug() << __FUNCTION__ << __LINE__;

    forceStop = true;
}

void K210BurnPrivate::K210BurnOpenPort(const QString &portName)
{
    forceStop = false;

    if(m_port)
    {
        delete m_port;
        m_port = Q_NULLPTR;
    }

    m_port = new OpenMVPluginSerialPort_thing(portName, this);
    // QSerialPort is buggy unless this is set.
    m_port->setReadBufferSize(1000000);

    if(!m_port->open(QIODevice::ReadWrite))
    {
        emit openResult(ERROR_OPENPORT_FAIL, m_port->errorString());

        delete m_port;
        m_port = Q_NULLPTR;

        return;
    }

    emit openResult(ERROR_NONE, tr("Open Port Succ."));
}

void K210BurnPrivate::K210BurnReleasePort(void)
{
    // qDebug() << __FUNCTION__ << __LINE__;

    if(m_port)
    {
        delete m_port;
        m_port = Q_NULLPTR;
    }
}

void K210BurnPrivate::K210BurnRam(int mode, const QString &loader)
{
    burnResp_t resp;

    if(false == burnRamHandShakeSpecBaud(mode, 115200)) {
        return;
    }

    QByteArray loaderData = readFile(loader);

    if(!loaderData.isEmpty())
    {
        int totalLen = loaderData.size();
        int chunkSize = 1024;
        int writeLen = 0, perLen = 0;
        int address = 0x80000000;
        int retry = 0;

        emit burnRamProcess(totalLen, 0);

        do
        {
            if(forceStop)
            {
                emit burnStoped();
                return;
            }

            perLen = (totalLen - writeLen) > chunkSize ? chunkSize : (totalLen - writeLen);

            resp = m_ramBurner.memoryWrite(m_port, address, perLen, loaderData);

            if((resp.op == ISP_MEMORY_WRITE) && ((resp.reason == ISP_RET_DEFAULT) || (resp.reason == ISP_RET_OK)))
            {
                retry = 0;

                address += perLen;
                writeLen += perLen;

                loaderData.remove(0, perLen);
            }
            else
            {
                if(++retry > retry_times)
                {
                    emit burnRamResult(ERROR_MEMORY_WRITE_FAIL, tr("StubISP Write Memory Failed."));
                    return;
                }
                QThread::msleep(300);
            }

            emit burnRamProcess(totalLen, writeLen);
        } while (totalLen > writeLen);

        m_ramBurner.memoryBoot(m_port, 0x80000000);
    }

    emit burnRamResult(ERROR_NONE, tr("StubISP Write Memory Succ."));
}

void K210BurnPrivate::K210BurnFlash(int baud, const QList <burnFirmwareInfo_t> &files)
{
    burnResp_t resp;

    if(false == burnFlashHandShakeSpecBaud(115200)) {
        return;
    }

    if(115200 != baud)
    {
        m_flashBurner.change_baudrate(m_port, baud);

        if(false == burnFlashHandShakeSpecBaud(baud)) {
            return;
        }
    }

    if(false == burnFlashInitFlash()) {
        return;
    }

    if(!files.isEmpty())
    {
        const int chunkSize = 64 * 1024;

        int totalLen, writeLen, perLen;
        int retry, address;

        QList<burnFirmwareInfo_t>::const_iterator it;
        for(it = files.begin(); it != files.end(); it++)
        // for(int i = 0; i < files.size(); i++)
        {
            // burnFirmwareInfo_t info = files.at(i);
            burnFirmwareInfo_t info = *it;

            QByteArray data = info.data;

            retry = 0;
            perLen = 0;
            writeLen = 0;

            totalLen = data.size();
            address = info.address;

// qDebug() << __FUNCTION__ << __LINE__ << "write" << info.fileName << "size" << data.size() << "to" << info.address;

            emit burnFlashProcess(totalLen, 0);

            do
            {
                if(forceStop)
                {
                    emit burnStoped();
                    return;
                }

                perLen = (totalLen - writeLen) > chunkSize ? chunkSize : (totalLen - writeLen);

                resp = m_flashBurner.flashWrite(m_port, address, perLen, data);

                if((resp.op == FLASH_ISP_FLASH_WRITE) && (resp.reason == ISP_RET_OK))
                {
                    retry = 0;

                    address += perLen;
                    writeLen += perLen;

                    data.remove(0, perLen);
                }
                else
                {
                    if(++retry > retry_times)
                    {
                        emit burnFlashResult(ERROR_FLASH_WRITE_FAIL, tr("FlashISP Write Flash Failed."));
                        return;
                    }
                    // TODO: judge the reason
                    QThread::msleep(500);
                }

                emit burnFlashProcess(totalLen, writeLen);
            } while (totalLen > writeLen);
        }
    }
    else
    {
        emit burnFlashResult(ERROR_FLASH_PARSE_FILES_FAIL, tr("FlashISP Parse File Failed."));
        return;
    }

    if(false == burnFlashReboot()) {
        resetToBOOT();
    }

    emit burnFlashResult(ERROR_NONE, tr("FlashISP Write Flash Succ."));
}

void K210BurnPrivate::K210BurnErase(burneraseType_t type, int addr, int size)
{
    burnResp_t resp;

    if(false == burnFlashHandShakeSpecBaud(115200)) {
        return;
    }

    if(false == burnFlashInitFlash()) {
        return;
    }

    if(eraseFullChip == type) {
        addr = 0;
        size = 16 * 1024 * 1024;
    }

    // TODO: erase flash.

    if(0x00 >= size)
    {
        emit burnEraseResult(ERROR_FLASH_ERASE_INVAILD_PARAM, tr("Invaild size"));
        return;
    }

    for(int i = 0; i < retry_times; i++)
    {
        if(forceStop)
        {
            emit burnStoped();
            return;
        }

        resp = m_flashBurner.flashErase(m_port, addr, size);
        
        if((resp.op == FLASH_ISP_FLASH_ERASE_NONBLOCKING) && (resp.reason == ISP_RET_FLASH_FLASH_BUSY)) {
            QThread::msleep(3 * 1000);
        } else {
            break;
        }
    }

    if((resp.op != FLASH_ISP_FLASH_ERASE_NONBLOCKING) || (resp.reason != ISP_RET_OK)) {
        emit burnEraseResult(ERROR_FLASH_ERASE_FAIL, resp.text);
        return;
    }

    forever
    {
        if(forceStop)
        {
            emit burnStoped();
            return;
        }

        resp = m_flashBurner.flashEraseStatus(m_port);

        if((resp.op == FLASH_ISP_FLASH_STATUS) && (resp.reason == ISP_RET_FLASH_FLASH_BUSY)) {
            QThread::msleep(3 * 1000);
        } else {
            break;
        }
    }

    if((resp.op != FLASH_ISP_FLASH_STATUS) || (resp.reason != ISP_RET_OK)) {
        emit burnEraseResult(ERROR_FLASH_ERASE_FAIL, resp.text);
        return;
    }

    if(false == burnFlashReboot()) {
        resetToBOOT();
    }

    emit burnEraseResult(ERROR_NONE, tr("FlashISP Erase Flash Succ."));
}

K210BurnPrivate::K210BurnPrivate(QObject *parent) : QObject(parent)
{
    m_port = Q_NULLPTR;
}

K210BurnPrivate::~K210BurnPrivate()
{
    // qDebug() << __FUNCTION__ << __LINE__;
}

K210Burn::K210Burn(QObject *parent) : QObject(parent)
{
    QThread *burnThread = new QThread;
    K210BurnPrivate *burn = new K210BurnPrivate();
    burn->moveToThread(burnThread);

    qRegisterMetaType<burnErrcode_t>("burnErrcode_t");
    qRegisterMetaType<burneraseType_t>("burneraseType_t");
    qRegisterMetaType<QList<burnFirmwareInfo_t>>("QList<burnFirmwareInfo_t>");

    connect(this, &K210Burn::open,
            burn, &K210BurnPrivate::K210BurnOpenPort);
    connect(burn, &K210BurnPrivate::openResult,
            this, &K210Burn::openResult);
    connect(this, &K210Burn::burnReleasePort,
            burn, &K210BurnPrivate::K210BurnReleasePort);

    connect(this, &K210Burn::burnStop,
            burn, &K210BurnPrivate::K210BurnStop, Qt::DirectConnection);
    connect(burn, &K210BurnPrivate::burnStoped,
            this, &K210Burn::burnStoped);


    connect(this, &K210Burn::burnRam,
            burn, &K210BurnPrivate::K210BurnRam);
    connect(burn, &K210BurnPrivate::burnRamProcess,
            this, &K210Burn::burnRamProcess);
    connect(burn, &K210BurnPrivate::burnRamResult,
            this, &K210Burn::burnRamResult);


    connect(this, &K210Burn::burnFlash,
            burn, &K210BurnPrivate::K210BurnFlash);
    connect(burn, &K210BurnPrivate::burnFlashProcess,
            this, &K210Burn::burnFlashProcess);
    connect(burn, &K210BurnPrivate::burnFlashResult,
            this, &K210Burn::burnFlashResult);


    connect(this, &K210Burn::burnErase,
            burn, &K210BurnPrivate::K210BurnErase);
    connect(burn, &K210BurnPrivate::burnEraseResult,
            this, &K210Burn::burnEraseResult);


    connect(this, &K210Burn::destroyed,
            burn, &K210BurnPrivate::deleteLater);

    connect(burn, &K210BurnPrivate::destroyed,
            burnThread, &QThread::quit);

    connect(burnThread, &QThread::finished,
            burnThread, &QThread::deleteLater);

    burnThread->setObjectName(QString(QStringLiteral("K210BurnThread")));
    burnThread->start();
}

K210Burn::~K210Burn()
{
    // qDebug() << __FUNCTION__ << __LINE__;
}
