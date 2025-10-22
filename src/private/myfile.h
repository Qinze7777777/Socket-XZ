#ifndef __MYFILE_H__
#define __MYFILE_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <LITTLEFS.h>
#include "private/myconfig.h"
#include "private/mywifi.h"
#include "private/mystate.h"
#include "private/relay.h"

//String readFile(const char * path);
void LittleFS_init(void);
String readFile(const String filepath);
int readFileBytes(const String filepath, char *bytes);
void writeFile(const char * path, const char * message);
void delFile(const char * path);
void listDir(const char * dirname, uint8_t levels);
void saveDevConfig(void);
void loadDevConfig(void);
void saveDevState(void);
void loadDevState(void);

#endif