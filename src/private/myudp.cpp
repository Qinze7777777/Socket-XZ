/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-19 22:30:53
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-04-10 11:45:50
 * @FilePath: \hass_mqtt_xiaozhi\src\private\myudp.cpp
 * @Description: 
 * 
 */
#include "private/myudp.h"

AsyncUDP udp;
IPAddress ip;


void onUdpPacket(AsyncUDPPacket& packet)
{
    // Serial.printf("UDP packet received %d!\n", packet.length());
    addToBuffer(packet.data(), packet.length());
}


int connectUdp(String server, int port)
{
    int ret = 0;
    ip.fromString(server);
    if (!udp.connected())
    {
        ret = udp.connect(ip, port);
        udp.onPacket(onUdpPacket);
    }
    return ret;
}

int connectAudioUdp(void)
{
    Serial.printf("Connecting to %s:%d\n", udpinfo.server.c_str(), udpinfo.port);
    return connectUdp(udpinfo.server, udpinfo.port);
}

void closeUdp(void)
{
    udp.close();
}

void sendUdpData(uint8_t *data, int len)
{
    udp.writeTo(data, len, ip, udpinfo.port);
}

int checkUdpConnect(void)
{
    return udp.connected();
}