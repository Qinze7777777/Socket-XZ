#ifndef __RELAY_H__
#define __RELAY_H__

#include <Arduino.h>
#include "private/mystate.h"

#define CN8023_INA 35
#define CN8023_INB 32

void initRelay(void);
void setRelay(int state);
void updateRelay(void);
void keepRelay(void);


#endif