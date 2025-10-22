/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-28 14:27:35
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-04-20 22:56:47
 * @FilePath: \hass_mqtt_xiaozhi\src\private\myconfig.cpp
 * @Description: 
 * 
 */
#include "private/myconfig.h"

config devconfig; // 全局变量，用于存储设备配置信息

//
String verifycode;  
String mqttinfo;
udpconfig udpinfo;
volatile uint8_t recvhello; 
volatile uint8_t recvstart; 
volatile uint8_t recvstop;
volatile uint8_t userover;