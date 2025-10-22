#ifndef __ASRPRO_H__
#define __ASRPRO_H__
#include <Arduino.h>

#define ASRPRO_SIGNAL_PIN 34

void initAsrpro(void);
int getAsrproSignal(void);

#endif