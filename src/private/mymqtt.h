/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-28 14:27:35
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-04-21 18:05:52
 * @FilePath: \hass_mqtt_xiaozhi\src\private\mymqtt.h
 * @Description: 
 * 
 */
#ifndef __MQTT_H__
#define __MQTT_H__

#include <Arduino.h>
#include <ArduinoJson.h>

#include <string.h>
#include <AsyncMqttClient.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include "private/myconfig.h"
#include "private/mywifi.h"
#include "private/myticker.h"
#include "private/myNTP.h"


void initHassMQTT(void);
int checkHassMQTT(void);
void onConnectHandler(bool sessionPresent);
void onDisconnectHandler(AsyncMqttClientDisconnectReason reason);
void onMessageHandler(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
void publishHassMQTTMessage(String topic, DynamicJsonDocument &doc);
void publishHassMQTTdiscoverys(void);
void publishDevState(void);

#endif