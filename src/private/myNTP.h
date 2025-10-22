#ifndef __MYNTP_h__
#define __MYNTP_h__

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <string>
#include "private/mystate.h"

int tryNtpServers();
int initNTP();
String getDateTime();
String getTime();
String getHMTime();
String getMSTime();
int hour();
int minute();
int second();
unsigned long rawTime();

#endif 
