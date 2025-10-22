#ifndef __MYUDP_H__
#define __MYUDP_H__

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include "private/audio.h"
#include "private/myconfig.h"

int connectUdp(String server, int port);
int connectAudioUdp(void);
void sendUdpData(uint8_t *data, int len);
int getUdpData(uint8_t *addr_data, uint16_t *len);
void closeUdp(void);
int checkUdpConnect(void);
bool getFromBuffer(uint8_t **data, uint16_t *length);
int getLengthBuffer(void);

extern int selectAudio;

#endif