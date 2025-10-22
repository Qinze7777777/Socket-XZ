#ifndef __BL0942_H__
#define __BL0942_H__

#include <Arduino.h>
#include <HardwareSerial.h>
#include "private/mystate.h"

#define V_REF 1.218
#define RL_CURRENT 1    // 0.001欧,单位为毫欧
#define R2_VOLTAGE 1950 // 390K*5,单位为K欧
#define R1_VOLTAGE 0.51 // 0.51K,单位为K欧

void initBL0942(void);
void getBL0942(void);
void enableWrite(void);      //写使能
void setCntClear(void);
String getCurrent(void);
String getVoltage(void);
String getActivePower(void);
String getEnergy(void);


#endif