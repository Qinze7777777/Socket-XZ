/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-28 14:27:35
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-04-20 22:56:55
 * @FilePath: \hass_mqtt_xiaozhi\src\private\myconfig.h
 * @Description: 
 * 
 */
#ifndef __MYCONFIG_H__
#define __MYCONFIG_H__

#include <Arduino.h>

#define OTA_URL "https://api.tenclass.net/xiaozhi/ota/"

typedef struct config
{
    String ssid;
    String password;
    String mqtt_server;
    String mqtt_port;
    String mqtt_user;
    String mqtt_password;
    String mqtt_topic;
    String matt_clientId;
}config;

typedef struct 
{
    String server;
    int port;
    String key;
    String nonce;
    String session_id;
    int sample_rate;
    int frame_duration;
}udpconfig;

extern config devconfig;
extern bool haveInternet;

extern udpconfig udpinfo;    //udp及音频配置
extern String verifycode;    //验证码
extern String mqttinfo;     //mqtt配置
extern volatile uint8_t recvhello;   //是否已经接收hello 1：已接收 0：未接收
extern volatile uint8_t recvstart;   //是否已经接收start 1：已接收 0：未接收
extern volatile uint8_t recvstop;    //是否已经接收stop 1：已接收 0：未接收
extern volatile uint8_t userover;    //主动结束

#endif