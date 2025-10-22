#include "private/relay.h"

void initRelay(void)
{
    pinMode(CN8023_INA, OUTPUT);
    pinMode(CN8023_INB, OUTPUT);
}
void setRelay(int state)
{
    if (state == 1)
    {
        digitalWrite(CN8023_INA, LOW);
        digitalWrite(CN8023_INB, HIGH);
    }
    else
    {
        digitalWrite(CN8023_INA, HIGH);
        digitalWrite(CN8023_INB, LOW);
    }
    delay(100);
    keepRelay();
}

void keepRelay(void)
{
    digitalWrite(CN8023_INA, LOW);
    digitalWrite(CN8023_INB, LOW);
}

void updateRelay(void)
{
    static String laststate;

    if (laststate != devstate.socket)
    {
        setRelay(devstate.socket.toInt());
    }
    laststate = devstate.socket;
    
}