/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-19 22:30:53
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-04-22 10:49:00
 * @FilePath: \hass_mqtt_xiaozhi\src\private\xzmqtt.h
 * @Description: 
 * 
 */
#ifndef __XZMQTT_H__
#define __XZMQTT_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include "private/myconfig.h"
#include "private/mystate.h"


void initXZMQTT(void);
int checkXZMQTT(void);
void reconnectXZMQTT(void);
void loopXZMQTT(void);
void onXZConnectHandler(bool sessionPresent);
void onXZMessageHandler(char* topic, byte* payload, unsigned int length);
// void onXZDisconnectHandler(AsyncMqttClientDisconnectReason reason);
// void onMessageHandler(char* topic, byte* payload, unsigned int length);
void publishXZMsg(String msg);
void publishHelloMsg(void);
void publishAbortMsg(void);
void publishListenMsg(void);
void publishAutoListenMsg(void);
void publishStopMsg(void);
void publishGoodbyeMsg(void);
void publishIOTMsg(void);
void publishIOTUpdateMsg();
void controlXiaozhiIOT(const char *name, const char *method, JsonObject &params);

#endif