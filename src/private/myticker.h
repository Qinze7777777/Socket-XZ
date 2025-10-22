/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-28 14:27:35
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-03-31 18:01:38
 * @FilePath: \hass_mqtt_xiaozhi\src\private\myticker.h
 * @Description: 
 * 
 */
#ifndef __MYTICKER_H__
#define __MYTICKER_H__

#include <Arduino.h>
#include <Ticker.h>
#include "private/mystate.h"
#include "private/myNTP.h"
#include "private/BL0942.h"
#include "private/mymqtt.h"
#include "private/AHT20.h"
#include "private/relay.h"

void timerInit(void);
void setSocketTimer(int seconds, bool open);

#endif