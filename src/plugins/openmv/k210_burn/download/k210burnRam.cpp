#include "k210burnRam.h"

K210RamBurner::~K210RamBurner()
{
    // qDebug() << __FUNCTION__ << __LINE__;
}

burnResp_t K210RamBurner::handShake(OpenMVPluginSerialPort_thing *port)
{
    QByteArray cmd = QByteArray::fromHex("c0c2000000000000000000000000c0");

    port->write(cmd);
    QByteArray resp = slip_recv_one_return(port, timeout_ms);

    return parseResp(resp);
}

burnResp_t K210RamBurner::memoryWrite(OpenMVPluginSerialPort_thing *port, int addr, int len, const QByteArray &data)
{
    QByteArray hdr, hdr_info, cmd;

    serializeLong(hdr_info, addr);
    serializeLong(hdr_info, len);

    hdr.append(hdr_info);
    hdr.append(data.left(len));

    int crc = crc32(hdr);

    serializeWord(cmd, ISP_MEMORY_WRITE);
    serializeWord(cmd, 0x00);
    serializeLong(cmd, crc);

    cmd.append(hdr);

    slip_write_packet(port, cmd);

    QByteArray resp = slip_recv_one_return(port, timeout_ms);

    return parseResp(resp);
}

void K210RamBurner::memoryBoot(OpenMVPluginSerialPort_thing *port, int addr)
{
    QByteArray hdr, hdr_info, cmd;

    serializeLong(hdr_info, addr);
    serializeLong(hdr_info, 0);

    hdr.append(hdr_info);

    int crc = crc32(hdr);

    serializeWord(cmd, ISP_MEMORY_BOOT);
    serializeWord(cmd, 0x00);
    serializeLong(cmd, crc);

    cmd.append(hdr);

    slip_write_packet(port, cmd);
}
