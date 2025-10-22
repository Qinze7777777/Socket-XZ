#ifndef __MYWIFI_H__
#define __MYWIFI_H__

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include "./private/myconfig.h"
#include "./private/myfile.h"


int connectWifi(void);
void configDev(void);
void handleNotFound(void);
void getConfigFromIndex(void);
void showIndex(void);
bool testInternet(void);


#endif