#include "k210burnFlash.h"

K210FlashBurner::~K210FlashBurner()
{
    // qDebug() << __FUNCTION__ << __LINE__;
}

burnResp_t K210FlashBurner::handShake(OpenMVPluginSerialPort_thing *port)
{
    QByteArray cmd = QByteArray::fromHex("c0d2000000000000000000000000c0");

    port->write(cmd);
    QByteArray resp = slip_recv_one_return(port, timeout_ms);

    return parseResp(resp);
}

void K210FlashBurner::change_baudrate(OpenMVPluginSerialPort_thing *port, int baud)
{
    QByteArray hdr, hdr_info, cmd;

    serializeLong(hdr_info, 0);
    serializeLong(hdr_info, 4);
    serializeLong(hdr_info, baud);

    hdr.append(hdr_info);

    int crc = crc32(hdr);

    serializeWord(cmd, FLASH_ISP_UARTHS_BAUDRATE_SET);
    serializeWord(cmd, 0x00);
    serializeLong(cmd, crc);

    cmd.append(hdr);

    slip_write_packet(port, cmd);

    // magic, dont remove this.
    slip_recv_one_return(port, timeout_ms);
}

burnResp_t K210FlashBurner::flashInit(OpenMVPluginSerialPort_thing *port, int chip_type)
{
    QByteArray hdr, hdr_info, cmd;

    serializeLong(hdr_info, chip_type);
    serializeLong(hdr_info, 0);

    hdr.append(hdr_info);

    int crc = crc32(hdr);

    serializeWord(cmd, FLASH_ISP_FLASH_INIT);
    serializeWord(cmd, 0x00);
    serializeLong(cmd, crc);

    cmd.append(hdr);

    slip_write_packet(port, cmd);

    QByteArray resp = slip_recv_one_return(port, timeout_ms);

    return parseResp(resp);
}

burnResp_t K210FlashBurner::flashWrite(OpenMVPluginSerialPort_thing *port, int addr, int len, const QByteArray &data)
{
    QByteArray hdr, hdr_info, cmd;

    serializeLong(hdr_info, addr);
    serializeLong(hdr_info, len);

    hdr.append(hdr_info);
    hdr.append(data.left(len));

    int crc = crc32(hdr);

    serializeWord(cmd, FLASH_ISP_FLASH_WRITE);
    serializeWord(cmd, 0x00);
    serializeLong(cmd, crc);

    cmd.append(hdr);

    slip_write_packet(port, cmd);

    QByteArray resp = slip_recv_one_return(port, 90 * 1000);

    return parseResp(resp);
}

burnResp_t K210FlashBurner::flashErase(OpenMVPluginSerialPort_thing *port, int addr, int size)
{
    QByteArray hdr, cmd;

    serializeLong(hdr, addr);
    serializeLong(hdr, size);

    int crc = crc32(hdr);

    serializeWord(cmd, FLASH_ISP_FLASH_ERASE_NONBLOCKING);
    serializeWord(cmd, 0x00);
    serializeLong(cmd, crc);

    cmd.append(hdr);

    slip_write_packet(port, cmd);

    QByteArray resp = slip_recv_one_return(port, 90 * 1000); // timeout 90s.

    return parseResp(resp);
}

burnResp_t K210FlashBurner::flashEraseStatus(OpenMVPluginSerialPort_thing *port)
{
    QByteArray cmd = QByteArray::fromHex("c0d9000000000000000000000000c0");

    port->write(cmd);
    QByteArray resp = slip_recv_one_return(port, timeout_ms);

    return parseResp(resp);
}

burnResp_t K210FlashBurner::flashReboot(OpenMVPluginSerialPort_thing *port)
{
    QByteArray cmd = QByteArray::fromHex("c0d5000000000000000000000000c0");

    port->write(cmd);
    QByteArray resp = slip_recv_one_return(port, timeout_ms);

    return parseResp(resp);
}
