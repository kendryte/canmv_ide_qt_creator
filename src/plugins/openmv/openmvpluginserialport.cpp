#include "openmvpluginserialport.h"
#include <QDebug>
#define OPENMVCAM_BAUD_RATE 12000000
#define OPENMVCAM_BAUD_RATE_2 921600

#define ARDUINO_TTR_BAUD_RATE 1200

#define WRITE_LOOPS 1 // disabled
#define WRITE_DELAY 0 // disabled
#define WRITE_TIMEOUT 6000
#define SERIAL_READ_TIMEOUT 10000
#define WIFI_READ_TIMEOUT 10000
#define SERIAL_READ_STALL_TIMEOUT 1000
#define WIFI_READ_STALL_TIMEOUT 3000
#define BOOTLOADER_WRITE_TIMEOUT 6
#define BOOTLOADER_READ_TIMEOUT 10
#define BOOTLOADER_READ_STALL_TIMEOUT 2
#define LEARN_MTU_WRITE_TIMEOUT 30
#define LEATN_MTU_READ_TIMEOUT 50

#define LEARN_MTU_MAX 4096
#define LEARN_MTU_MIN 64

void serializeByte(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 1);
}

void serializeWord(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 2);
}

void serializeLong(QByteArray &buffer, int value) // LittleEndian
{
    buffer.append(reinterpret_cast<const char *>(&value), 4);
}

int deserializeByte(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 1);
    buffer = buffer.mid(1);
    return r;
}

int deserializeWord(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 2);
    buffer = buffer.mid(2);
    return r;
}

int deserializeLong(QByteArray &buffer) // LittleEndian
{
    int r = int();
    memcpy(&r, buffer.data(), 4);
    buffer = buffer.mid(4);
    return r;
}

OpenMVPluginSerialPort_thing::OpenMVPluginSerialPort_thing(const QString &name, QObject *parent) : QObject(parent)
{
    if(QSerialPortInfo(name).isValid())
    {
        m_serialPort = new QSerialPort(name, this);
        m_tcpSocket = Q_NULLPTR;
    }
    else
    {
        m_serialPort = Q_NULLPTR;
        m_tcpSocket = new QTcpSocket(this);
        m_tcpSocket->setProperty("name", name);
    }
}

QString OpenMVPluginSerialPort_thing::portName()
{
    if(m_serialPort)
    {
        return m_serialPort->portName();
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->property("name").toString();
    }

    return QString();
}

void OpenMVPluginSerialPort_thing::setReadBufferSize(qint64 size)
{
    if(m_serialPort)
    {
        m_serialPort->setReadBufferSize(size);
    }

    if(m_tcpSocket)
    {
        m_tcpSocket->setReadBufferSize(size);
    }
}

bool OpenMVPluginSerialPort_thing::setBaudRate(qint32 baudRate)
{
    if(m_serialPort)
    {
        return m_serialPort->setBaudRate(baudRate);
    }

    if(m_tcpSocket)
    {
        return true;
    }

    return bool();
}

bool OpenMVPluginSerialPort_thing::open(QIODevice::OpenMode mode)
{
    if(m_serialPort)
    {
        return m_serialPort->open(mode);
    }

    if(m_tcpSocket)
    {
        QStringList list = m_tcpSocket->property("name").toString().split(QLatin1Char(':'));

        if(list.size() != 3)
        {
            return false;
        }

        QString hostName = list.at(1);
        QString port = list.at(2);

        bool portNumberOkay;
        quint16 portNumber = port.toUInt(&portNumberOkay);

        if(!portNumberOkay)
        {
            return false;
        }

        m_tcpSocket->connectToHost(hostName, portNumber, mode);
        return m_tcpSocket->waitForConnected(3000);
    }

    return bool();
}

bool OpenMVPluginSerialPort_thing::flush()
{
    if(m_serialPort)
    {
        return m_serialPort->flush();
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->flush();
    }

    return bool();
}

QString OpenMVPluginSerialPort_thing::errorString()
{
    if(m_serialPort)
    {
        return m_serialPort->errorString();
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->errorString();
    }

    return QString();
}

void OpenMVPluginSerialPort_thing::clearError()
{
    if(m_serialPort)
    {
        m_serialPort->clearError();
    }
}

qint64 OpenMVPluginSerialPort_thing::read(char *data, qint64 maxSize)
{
    if(m_serialPort)
    {
        return m_serialPort->read(data, maxSize);
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->read(data, maxSize);
    }

    return qint64();
}

QByteArray OpenMVPluginSerialPort_thing::readAll()
{
    if(m_serialPort)
    {
        return m_serialPort->readAll();
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->readAll();
    }

    return QByteArray();
}

qint64 OpenMVPluginSerialPort_thing::write(const QByteArray &data)
{
    if(m_serialPort)
    {
        return m_serialPort->write(data);
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->write(data);
    }

    return qint64();
}

qint64 OpenMVPluginSerialPort_thing::bytesAvailable()
{
    if(m_serialPort)
    {
        return m_serialPort->bytesAvailable();
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->bytesAvailable();
    }

    return qint64();
}

qint64 OpenMVPluginSerialPort_thing::bytesToWrite()
{
    if(m_serialPort)
    {
        return m_serialPort->bytesToWrite();
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->bytesToWrite();
    }

    return qint64();
}

bool OpenMVPluginSerialPort_thing::waitForReadyRead(int msecs)
{
    if(m_serialPort)
    {
        return m_serialPort->waitForReadyRead(msecs);
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->waitForReadyRead(msecs);
    }

    return bool();
}

bool OpenMVPluginSerialPort_thing::waitForBytesWritten(int msecs)
{
    if(m_serialPort)
    {
        return m_serialPort->waitForBytesWritten(msecs);
    }

    if(m_tcpSocket)
    {
        return m_tcpSocket->waitForBytesWritten(msecs);
    }

    return bool();
}

bool OpenMVPluginSerialPort_thing::setDataTerminalReady(bool set)
{
    if(m_serialPort)
    {
        return m_serialPort->setDataTerminalReady(set);
    }

    if(m_tcpSocket)
    {
        return true;
    }

    return bool();
}

bool OpenMVPluginSerialPort_thing::setRequestToSend(bool set)
{
    if(m_serialPort)
    {
        return m_serialPort->setRequestToSend(set);
    }

    if(m_tcpSocket)
    {
        return true;
    }

    return bool();
}

OpenMVPluginSerialPort_private::OpenMVPluginSerialPort_private(int override_read_timeout, int override_read_stall_timeout, QObject *parent) : QObject(parent)
{
    m_port = Q_NULLPTR;
    m_bootloaderStop = false;
    m_override_read_timeout = override_read_timeout;
    m_override_read_stall_timeout = override_read_stall_timeout;
}

void OpenMVPluginSerialPort_private::changeBoardBaud(int baud)
{
    QByteArray cmd_ctrl_c(1, 0x03);
    write(cmd_ctrl_c, 0, 0, WRITE_TIMEOUT);
    QThread::msleep(200);

    write(cmd_ctrl_c, 0, 0, WRITE_TIMEOUT);
    QThread::msleep(200);

    QByteArray cmd0("\r");
    write(cmd0, 0, 10, WRITE_TIMEOUT);
    write(cmd0, 0, 10, WRITE_TIMEOUT);

    QByteArray cmd1("from machine import UART\r");
    write(cmd1, 0, 10, WRITE_TIMEOUT);

    QByteArray cmd2("repl = UART.repl_uart()\r");
    write(cmd2, 0, 10, WRITE_TIMEOUT);

    // QByteArray cmd3("repl.init(1500000, 8, None, 1, read_buf_len=2048, ide=True)\r\n\r\n");
    QString strCmd3 = QString(QStringLiteral("repl.init(%1, 8, None, 1, read_buf_len=2048, ide=True)\r")).arg(baud);
    QByteArray cmd3 = strCmd3.toLatin1();

    write(cmd3, 0, 25, WRITE_TIMEOUT);
}

int OpenMVPluginSerialPort_private::handshakeBoard(int timeouts)
{
    if(m_port)
    {
        int responseLen = 4;
        QByteArray response;
        QElapsedTimer elaspedTimer;
        QElapsedTimer elaspedTimer2;
        QElapsedTimer waitAckTimer;

        waitAckTimer.start();
        elaspedTimer.start();
        elaspedTimer2.start();

        m_port->readAll();

        // first send handshake cmd
        {
            QByteArray data;
            serializeByte(data, __USBDBG_CMD);
            serializeByte(data, 0x8D);
            serializeLong(data, 4);

            write(data, 2, 10, WRITE_TIMEOUT);
        }

        do
        {
            do
            {
                m_port->waitForReadyRead(1);
                response.append(m_port->readAll());

                if((response.size() < responseLen) && elaspedTimer2.hasExpired(150))
                {
                    QByteArray data;
                    serializeByte(data, __USBDBG_CMD);
                    serializeByte(data, 0x8D);
                    serializeLong(data, 4);

                    write(data, 2, 10, WRITE_TIMEOUT);

                    if(m_port)
                    {
                        response = response.mid(response.size());
                        elaspedTimer2.restart();
                    }
                    else
                    {
                        break;
                    }
                }
            }
            while((response.size() < responseLen) && (!elaspedTimer.hasExpired(300)));

            // if(response.size())
            //     qDebug() << response.toHex();

            if(response.size() >= responseLen)
            {
                int result = deserializeLong(response);

                if((uint32_t)result == 0xFFEEBBAA)
                {
                    return 0;
                }
                else
                {
                    response = response.mid(response.size());
                    elaspedTimer.restart();
                    elaspedTimer2.restart();
                }
            }
        } while(!waitAckTimer.hasExpired(timeouts));

        if(waitAckTimer.hasExpired(timeouts)) {
            return -1;
        }
    }

    return -2;
}

/**
 *          DTR         RTS
 * Mode-0   Boot(0)     Rst(0)
 * Mode-1   Boot(0)     Rst(1)
 * Mode-2   Boot(1)     Rst(0)
 * Mode-3   Boot(1)     Rst(1)
 */

int OpenMVPluginSerialPort_private::resetboard(int mode)
{
    if(!m_port) {
        return -1;
    }

    switch (mode)
    {
    default:
    case 0:
        // set boot to vaild, rst to invaild
        if((!m_port->setDataTerminalReady(true)) || (!m_port->setRequestToSend(false))) {
            return -1;
        }
        QThread::msleep(100); // set rst to 0 and wait 100ms
        if((!m_port->setDataTerminalReady(true)) || (!m_port->setRequestToSend(true))) {
            return -1;
        }
        break;
    case 1:
        // set boot to vaild, rst to invaild
        if((!m_port->setDataTerminalReady(true)) || (!m_port->setRequestToSend(true))) {
            return -1;
        }
        QThread::msleep(100); // set rst to 0 and wait 100ms
        if((!m_port->setDataTerminalReady(true)) || (!m_port->setRequestToSend(false))) {
            return -1;
        }
        break;
    case 2:
        // set boot to vaild, rst to invaild
        if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(false))) {
            return -1;
        }
        QThread::msleep(100); // set rst to 0 and wait 100ms
        if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(true))) {
            return -1;
        }
        break;
    case 3:
        // set boot to vaild, rst to invaild
        if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(true))) {
            return -1;
        }
        QThread::msleep(100); // set rst to 0 and wait 100ms
        if((!m_port->setDataTerminalReady(false)) || (!m_port->setRequestToSend(false))) {
            return -1;
        }
        break;
    }
    return 0;
}

void OpenMVPluginSerialPort_private::open(const QString &portName, int mode, int baud)
{
    if(m_port)
    {
        delete m_port;
    }

    m_port = new OpenMVPluginSerialPort_thing(portName, this);
    // QSerialPort is buggy unless this is set.
    m_port->setReadBufferSize(1000000);

    QSerialPortInfo arduinoPort(m_port->portName());

    bool isTouchToReset = arduinoPort.hasVendorIdentifier() &&
                          arduinoPort.hasProductIdentifier() &&
                         (arduinoPort.vendorIdentifier() == ARDUINOCAM_VID) && ((arduinoPort.productIdentifier() == PORTENTA_TTR_1_PID) ||
                                                                                (arduinoPort.productIdentifier() == PORTENTA_TTR_2_PID) ||
                                                                                (arduinoPort.productIdentifier() == NICLA_TTR_1_PID) ||
                                                                                (arduinoPort.productIdentifier() == NICLA_TTR_2_PID));

    int baudRate = OPENMVCAM_BAUD_RATE;
    int baudRate2 = OPENMVCAM_BAUD_RATE_2;

    if(isTouchToReset)
    {
        baudRate = ARDUINO_TTR_BAUD_RATE;
        baudRate2 = ARDUINO_TTR_BAUD_RATE;
    }

    baudRate = 115200;

    bool dtr, rts;

    // judge default dtr and rts setting
    if(0x00 == mode) {
        dtr = true;
        rts = true;
    } else if(0x01 == mode) {
        dtr = true;
        rts = false;
    } else if(0x02 == mode) {
        dtr = false;
        rts = true;
    } else {
        dtr = false;
        rts = false;
    }

    // if((!m_port->setBaudRate(baudRate))
    // || (!m_port->open(QIODevice::ReadWrite))
    // || (!m_port->setDataTerminalReady(dtr))
    // || (!m_port->setRequestToSend(rts)))
    if(!m_port->open(QIODevice::ReadWrite))
    {
        // delete m_port;
        // m_port = new OpenMVPluginSerialPort_thing(portName, this);
        // // QSerialPort is buggy unless this is set.
        // m_port->setReadBufferSize(1000000);

        // if((!m_port->setBaudRate(baudRate2))
        // || (!m_port->open(QIODevice::ReadWrite))
        // || (!m_port->setDataTerminalReady(dtr))
        // || (!m_port->setRequestToSend(rts)))
        // {
            emit openResult(m_port->errorString());
            delete m_port;
            m_port = Q_NULLPTR;
        // }
    }

    if(!m_port)
    {
        return;
    }

    {
        // qDebug() << "reset board";
        // if failed, reset the board
        if(0x00 != resetboard(mode)) {
            emit openResult(m_port->errorString());
            delete m_port;
            m_port = Q_NULLPTR;
            return;
        }

        QThread::msleep(3000);

        // chane baud to 115200
        if(!m_port->setBaudRate(115200)) {
            emit openResult(m_port->errorString());
            delete m_port;
            m_port = Q_NULLPTR;
            return;
        }
        QThread::msleep(10);

        // qDebug() << "change baud";
        // send script to change uart baud
        changeBoardBaud(baud);

        // change baud to 1500000
        if(!m_port->setBaudRate(baud)) {
            emit openResult(m_port->errorString());
            delete m_port;
            m_port = Q_NULLPTR;
            return;
        }
        QThread::msleep(10); // sleep 200ms for change

        // qDebug() << "handshake";
        // handshake
        if(0x00 == handshakeBoard(3000)) {
            emit openResult(QString());
            return;
        }
    }

    if(m_port)
    {
        delete m_port;
        m_port = Q_NULLPTR;
    }

    emit openResult(QString(QStringLiteral("Handshake Failed")));
}

void OpenMVPluginSerialPort_private::write(const QByteArray &data, int startWait, int stopWait, int timeout)
{
    if(m_port)
    {
        QString portName = m_port->portName();

        for(int i = 0; i < WRITE_LOOPS; i++)
        {
            if(!m_port)
            {
                m_port = new OpenMVPluginSerialPort_thing(portName, this);
                // QSerialPort is buggy unless this is set.
                m_port->setReadBufferSize(1000000);

                if((!m_port->setBaudRate(OPENMVCAM_BAUD_RATE))
                || (!m_port->open(QIODevice::ReadWrite))
                || (!m_port->setDataTerminalReady(true)))
                {
                    delete m_port;
                    m_port = new OpenMVPluginSerialPort_thing(portName, this);
                    // QSerialPort is buggy unless this is set.
                    m_port->setReadBufferSize(1000000);

                    if((!m_port->setBaudRate(OPENMVCAM_BAUD_RATE_2))
                    || (!m_port->open(QIODevice::ReadWrite))
                    || (!m_port->setDataTerminalReady(true)))
                    {
                        delete m_port;
                        m_port = Q_NULLPTR;
                    }
                }
            }

            if(m_port)
            {
                if(startWait)
                {
                    QThread::msleep(startWait);
                }

                m_port->clearError();

                if((m_port->write(data) != data.size()) || (!m_port->flush()))
                {
                    delete m_port;
                    m_port = Q_NULLPTR;
                }
                else
                {
                    QElapsedTimer elaspedTimer;
                    elaspedTimer.start();

                    while(m_port->bytesToWrite())
                    {
                        m_port->waitForBytesWritten(1);

                        if(m_port->bytesToWrite() && elaspedTimer.hasExpired(timeout))
                        {
                            break;
                        }
                    }

                    if(m_port->bytesToWrite())
                    {
                        delete m_port;
                        m_port = Q_NULLPTR;
                    }
                    else if(stopWait)
                    {
                        QThread::msleep(stopWait);
                    }
                }
            }

            if(m_port)
            {
                break;
            }

            QThread::msleep(WRITE_DELAY);
        }
    }
}

void OpenMVPluginSerialPort_private::command(const OpenMVPluginSerialPortCommand &command)
{
    if(command.m_data.isEmpty())
    {
        if(!command.m_responseLen) // close
        {
            if(m_port)
            {
                delete m_port;
                m_port = Q_NULLPTR;
            }

            emit commandResult(OpenMVPluginSerialPortCommandResult(true, QByteArray()));
        }
        else if(m_port) // learn
        {
            bool ok = false;

            for(int i = LEARN_MTU_MAX; i >= LEARN_MTU_MIN; i /= 2)
            {
                QByteArray learnMTU;
                serializeByte(learnMTU, __USBDBG_CMD);
                serializeByte(learnMTU, __USBDBG_LEARN_MTU);
                serializeLong(learnMTU, i - 1);

                write(learnMTU, LEARN_MTU_START_DELAY, LEARN_MTU_END_DELAY, LEARN_MTU_WRITE_TIMEOUT);

                if(!m_port)
                {
                    break;
                }
                else
                {
                    QByteArray response;
                    QElapsedTimer elaspedTimer;
                    elaspedTimer.start();

                    do
                    {
                        m_port->waitForReadyRead(1);
                        response.append(m_port->readAll());
                    }
                    while((response.size() < (i - 1)) && (!elaspedTimer.hasExpired(LEATN_MTU_READ_TIMEOUT)));

                    if(response.size() >= (i - 1))
                    {
                        QByteArray temp;
                        serializeLong(temp, (i - 1));
                        emit commandResult(OpenMVPluginSerialPortCommandResult(true, temp));
                        ok = true;
                        break;
                    }
                }
            }

            if(!ok)
            {
                if(m_port)
                {
                    delete m_port;
                    m_port = Q_NULLPTR;
                }

                emit commandResult(OpenMVPluginSerialPortCommandResult(false, QByteArray()));
            }
        }
        else
        {
            emit commandResult(OpenMVPluginSerialPortCommandResult(false, QByteArray()));
        }
    }
    else if(m_port)
    {
        static int noRespCnt = 0;

        QByteArray tempData = command.m_data;

        const int flag = deserializeByte(tempData);
        const int cmd = deserializeByte(tempData);

        QByteArray drop = m_port->readAll();

        write(command.m_data, command.m_startWait, command.m_endWait, WRITE_TIMEOUT);

        if((!m_port) || (!command.m_responseLen))
        {
            emit commandResult(OpenMVPluginSerialPortCommandResult(m_port, QByteArray()));
        }
        else
        {
            int read_timeout = m_port->isSerialPort() ? SERIAL_READ_TIMEOUT : WIFI_READ_TIMEOUT;

            if(m_override_read_timeout > 0)
            {
                read_timeout = m_override_read_timeout;
            }

            int read_stall_timeout = m_port->isSerialPort() ? SERIAL_READ_STALL_TIMEOUT : WIFI_READ_STALL_TIMEOUT;

            if(m_override_read_stall_timeout > 0)
            {
                read_stall_timeout = m_override_read_stall_timeout;
            }

            QByteArray response;
            int responseLen = command.m_responseLen;
            QElapsedTimer elaspedTimer;
            QElapsedTimer elaspedTimer2;
            elaspedTimer.start();

            if((__USBDBG_CMD == flag) && (__USBDBG_FRAME_DUMP == cmd))
            {
                read_timeout = responseLen / 30;
                read_stall_timeout = responseLen / 60;
            }

            if((__USBDBG_CMD == flag) && (__USBDBG_QUERY_FILE_STAT != cmd)) {
                elaspedTimer2.start();
            }

            do
            {
                m_port->waitForReadyRead(1);

                QByteArray data = m_port->readAll();
                response.append(data);

                if((responseLen == command.m_responseLen) && (!data.isEmpty()))
                {
                    elaspedTimer.restart();
                    if((__USBDBG_CMD == flag) && (__USBDBG_QUERY_FILE_STAT != cmd)) {
                        elaspedTimer2.start();
                    }
                }

                if((__USBDBG_CMD == flag) && \
                    ((__USBDBG_QUERY_FILE_STAT != cmd) || \
                        (__USBDBG_FRAME_DUMP != cmd)))
                {
                    if(m_port->isSerialPort() && (response.size() < responseLen) && elaspedTimer2.hasExpired(read_stall_timeout))
                    {
                        QByteArray data;
                        serializeByte(data, __USBDBG_CMD);
                        serializeByte(data, __USBDBG_SCRIPT_RUNNING);
                        serializeLong(data, SCRIPT_RUNNING_RESPONSE_LEN);
                        write(data, SCRIPT_RUNNING_START_DELAY, SCRIPT_RUNNING_END_DELAY, WRITE_TIMEOUT);

                        if(m_port)
                        {
                            responseLen += SCRIPT_RUNNING_RESPONSE_LEN;
                            elaspedTimer2.restart();
                        }
                        else
                        {
                            break;
                        }
                    }

                    if(m_port->isTCPPort() && (response.size() < responseLen) && elaspedTimer2.hasExpired(read_stall_timeout))
                    {
                        write(command.m_data, 0, 0, WRITE_TIMEOUT);

                        if(!m_port)
                        {
                            break;
                        }
                    }
                }
                QThread::msleep(Utils::HostOsInfo::isMacHost() ? 2 : 1);
            }
            while((response.size() < responseLen) && (!elaspedTimer.hasExpired(read_timeout)));

            if(response.size() >= responseLen)
            {
                if((flag == __USBDBG_CMD) && ((__USBDBG_FRAME_DUMP == cmd) || (__USBDBG_QUERY_FILE_STAT == cmd)))
                {
                    noRespCnt = 0;
                }
                emit commandResult(OpenMVPluginSerialPortCommandResult(true, response.left(command.m_responseLen)));
            }
            else
            {
                noRespCnt ++;

                if((flag == __USBDBG_CMD) && (noRespCnt < 3) && (cmd == __USBDBG_FRAME_DUMP))
                {
                    // if is wait frame dump, maybe lost some data, we ignore the error.
                    emit commandResult(OpenMVPluginSerialPortCommandResult(true, QByteArray()));
                }
                else if((flag == __USBDBG_CMD) && (noRespCnt <= 50) && (cmd == __USBDBG_QUERY_FILE_STAT))
                {
                    // if is wait query write file stat, maybe busying write
                    emit commandResult(OpenMVPluginSerialPortCommandResult(false, QByteArray()));
                }
                else
                {
                    if(m_port)
                    {
                        delete m_port;
                        m_port = Q_NULLPTR;
                    }

                    emit commandResult(OpenMVPluginSerialPortCommandResult(false, QByteArray()));
                }
            }
        }
    }
    else
    {
        emit commandResult(OpenMVPluginSerialPortCommandResult(false, QByteArray()));
    }

    // Execute commands slowly so as to not overload the OpenMV Cam board.

    QThread::msleep(Utils::HostOsInfo::isMacHost() ? 2 : 1);
}

void OpenMVPluginSerialPort_private::bootloaderStart(const QString &selectedPort)
{
    if(m_port)
    {
        int command = __USBDBG_SYS_RESET;
        QByteArray buffer;
        serializeByte(buffer, __USBDBG_CMD);
        serializeByte(buffer, command);
        serializeLong(buffer, int());
        write(buffer, SYS_RESET_START_DELAY, SYS_RESET_END_DELAY, WRITE_TIMEOUT);

        if(m_port)
        {
            delete m_port;
            m_port = Q_NULLPTR;
        }
    }

    forever
    {
        QStringList stringList;

        foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts())
        {
            if(port.hasVendorIdentifier() && (port.vendorIdentifier() == OPENMVCAM_VID)
            && port.hasProductIdentifier() && (port.productIdentifier() == OPENMVCAM_PID) && (port.serialNumber() == QStringLiteral("000000000011")))
            {
                stringList.append(port.portName());
            }
        }

        if(Utils::HostOsInfo::isMacHost())
        {
            stringList = stringList.filter(QStringLiteral("cu"), Qt::CaseInsensitive);
        }

        if(!stringList.isEmpty())
        {
            const QString portName = ((!selectedPort.isEmpty()) && stringList.contains(selectedPort)) ? selectedPort : stringList.first();

            if(Q_UNLIKELY(m_port))
            {
                delete m_port;
            }

            m_port = new OpenMVPluginSerialPort_thing(portName, this);
            // QSerialPort is buggy unless this is set.
            m_port->setReadBufferSize(1000000);

            if((!m_port->setBaudRate(OPENMVCAM_BAUD_RATE))
            || (!m_port->open(QIODevice::ReadWrite))
            || (!m_port->setDataTerminalReady(true)))
            {
                delete m_port;
                m_port = new OpenMVPluginSerialPort_thing(portName, this);
                // QSerialPort is buggy unless this is set.
                m_port->setReadBufferSize(1000000);

                if((!m_port->setBaudRate(OPENMVCAM_BAUD_RATE_2))
                || (!m_port->open(QIODevice::ReadWrite))
                || (!m_port->setDataTerminalReady(true)))
                {
                    delete m_port;
                    m_port = Q_NULLPTR;
                }
            }

            if(m_port)
            {
                QByteArray buffer;
                serializeLong(buffer, __BOOTLDR_START);
                write(buffer, BOOTLDR_START_START_DELAY, BOOTLDR_START_END_DELAY, BOOTLOADER_WRITE_TIMEOUT);

                if(m_port)
                {
                    QByteArray response;
                    int responseLen = BOOTLDR_START_RESPONSE_LEN;
                    QElapsedTimer elaspedTimer;
                    QElapsedTimer elaspedTimer2;
                    elaspedTimer.start();
                    elaspedTimer2.start();

                    do
                    {
                        m_port->waitForReadyRead(1);
                        response.append(m_port->readAll());

                        if((response.size() < responseLen) && elaspedTimer2.hasExpired(BOOTLOADER_READ_STALL_TIMEOUT))
                        {
                            QByteArray data;
                            serializeLong(data, __BOOTLDR_START);
                            write(data, BOOTLDR_START_START_DELAY, BOOTLDR_START_END_DELAY, BOOTLOADER_WRITE_TIMEOUT);

                            if(m_port)
                            {
                                responseLen += BOOTLDR_START_RESPONSE_LEN;
                                elaspedTimer2.restart();
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    while((response.size() < responseLen) && (!elaspedTimer.hasExpired(BOOTLOADER_READ_TIMEOUT)));

                    if(response.size() >= responseLen)
                    {
                        int result = deserializeLong(response);

                        if((result == V1_BOOTLDR)
                        || (result == V2_BOOTLDR)
                        || (result == V3_BOOTLDR))
                        {
                            emit bootloaderStartResponse(true, result);
                            return;
                        }
                    }

                    if(m_port)
                    {
                        delete m_port;
                        m_port = Q_NULLPTR;
                    }
                }
            }
        }

        QCoreApplication::processEvents();

        if(m_bootloaderStop)
        {
            emit bootloaderStartResponse(false, int());
            return;
        }
    }
}

void OpenMVPluginSerialPort_private::bootloaderStop()
{
    m_bootloaderStop = true;
    emit bootloaderStopResponse();
}

void OpenMVPluginSerialPort_private::bootloaderReset()
{
    m_bootloaderStop = false;
    emit bootloaderResetResponse();
}

OpenMVPluginSerialPort::OpenMVPluginSerialPort(int override_read_timeout, int override_read_stall_timeout, QObject *parent) : QObject(parent)
{
    QThread *thread = new QThread;
    OpenMVPluginSerialPort_private *port = new OpenMVPluginSerialPort_private(override_read_timeout, override_read_stall_timeout);
    port->moveToThread(thread);

    connect(this, &OpenMVPluginSerialPort::open,
            port, &OpenMVPluginSerialPort_private::open);

    connect(port, &OpenMVPluginSerialPort_private::openResult,
            this, &OpenMVPluginSerialPort::openResult);

    connect(this, &OpenMVPluginSerialPort::command,
            port, &OpenMVPluginSerialPort_private::command);

    connect(port, &OpenMVPluginSerialPort_private::commandResult,
            this, &OpenMVPluginSerialPort::commandResult);

    connect(this, &OpenMVPluginSerialPort::bootloaderStart,
            port, &OpenMVPluginSerialPort_private::bootloaderStart);

    connect(this, &OpenMVPluginSerialPort::bootloaderStop,
            port, &OpenMVPluginSerialPort_private::bootloaderStop);

    connect(this, &OpenMVPluginSerialPort::bootloaderReset,
            port, &OpenMVPluginSerialPort_private::bootloaderReset);

    connect(port, &OpenMVPluginSerialPort_private::bootloaderStartResponse,
            this, &OpenMVPluginSerialPort::bootloaderStartResponse);

    connect(port, &OpenMVPluginSerialPort_private::bootloaderStopResponse,
            this, &OpenMVPluginSerialPort::bootloaderStopResponse);

    connect(port, &OpenMVPluginSerialPort_private::bootloaderResetResponse,
            this, &OpenMVPluginSerialPort::bootloaderResetResponse);

    connect(this, &OpenMVPluginSerialPort::destroyed,
            port, &OpenMVPluginSerialPort_private::deleteLater);

    connect(port, &OpenMVPluginSerialPort_private::destroyed,
            thread, &QThread::quit);

    connect(thread, &QThread::finished,
            thread, &QThread::deleteLater);

    thread->start();
}
