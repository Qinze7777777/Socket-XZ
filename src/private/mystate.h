/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-28 14:27:35
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-03-31 17:52:47
 * @FilePath: \hass_mqtt_xiaozhi\src\private\mystate.h
 * @Description: 
 * 
 */
#ifndef __MYSTATE_H__
#define __MYSTATE_H__


#include <Arduino.h>
#include "private/mywifi.h"

typedef struct state
{
    String state;           //设备状态
    String V;               //电压
    String I;               //电流
    String P;               //功率
    String E;               //电能
    String socket;          //开关状态
    String timernum;        //定时器的值
    String timer;           //定时器开关
    String screen;          //屏幕背光开关
    String brightness;      //亮度
    String IR_node;         //红外索引号
    String temperature;     //温度
    String humidity;        //湿度
    String volume;          //音量
    String countdown;       //倒计时
    //红外自学习码库
}state;

extern state devstate;

#endif