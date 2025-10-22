#ifndef __AHT20_H__
#define __AHT20_H__
#include <Arduino.h>

#include "lib/SparkFun_Qwiic_Humidity_AHT20.h"
#include "private/mystate.h"

void initAHT20(void);
void getAHT20(void);

#endif