#ifndef K210_DOWNLOAD_UTIL_H
#define K210_DOWNLOAD_UTIL_H

#include <QtCore>
#include <QObject>

#include "../../openmvpluginserialport.h"

enum ISPOperation_t {
    ISP_INVAILD                             = 0xC0,

    ISP_ECHO                                = 0xC1,
    ISP_NOP                                 = 0xC2,
    ISP_MEMORY_WRITE                        = 0xC3,
    ISP_MEMORY_READ                         = 0xC4,
    ISP_MEMORY_BOOT                         = 0xC5,
    ISP_CHANGE_BAUDRATE                     = 0xC6,
    ISP_DEBUG_INFO                          = 0xD1,
    
    // FLASH Add.
    FLASH_ISP_NOP                           = 0xD2,
    FLASH_ISP_FLASH_ERASE                   = 0xD3,
    FLASH_ISP_FLASH_WRITE                   = 0xD4,
    FLASH_ISP_REBOOT                        = 0xD5,
    FLASH_ISP_UARTHS_BAUDRATE_SET           = 0xD6,
    FLASH_ISP_FLASH_INIT                    = 0xD7,
    FLASH_ISP_FLASH_ERASE_NONBLOCKING       = 0xD8,
    FLASH_ISP_FLASH_STATUS                  = 0xD9,
};

enum ISPErrCode_t {
    ISP_RET_INVAILD                         = 0xC0,

    ISP_RET_DEFAULT                         = 0x00,
    ISP_RET_OK                              = 0xE0,
    ISP_RET_BAD_DATA_LEN                    = 0xE1,
    ISP_RET_BAD_DATA_CHECKSUM               = 0xE2,
    ISP_RET_INVALID_COMMAND                 = 0xE3,

    // FLASH Add.
    ISP_RET_FLASH_BAD_INIT                  = 0xE4,
    ISP_RET_FLASH_BAD_ERASE                 = 0xE5,
    ISP_RET_FLASH_BAD_WRITE                 = 0xE6,
    ISP_RET_FLASH_FLASH_BUSY                = 0xE7,
};

typedef struct burnResp {
    ISPOperation_t op;
    ISPErrCode_t reason;
    QString text;
} burnResp_t;

typedef struct burnFirmwareInfo {
    int address;
    QString fileName;
    QByteArray data;
} burnFirmwareInfo_t;

QByteArray readFile(const QString &fileName);
QList <burnFirmwareInfo_t> parseFirmware(const QString &fileName, int binAddr, bool aesEncryption, const QByteArray &aesKey, bool dioMode);

burnResp_t parseResp(QByteArray &data);
QByteArray slip_recv_one_return(OpenMVPluginSerialPort_thing *port, int timeout_s);
qint64 slip_write_packet(OpenMVPluginSerialPort_thing *port, QByteArray &data);

quint32 crc32(const QByteArray &data);

#endif // K210_DOWNLOAD_UTIL_H
