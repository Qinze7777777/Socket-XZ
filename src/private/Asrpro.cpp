#include "private/Asrpro.h"

void initAsrpro(void)
{
    pinMode(ASRPRO_SIGNAL_PIN, INPUT);
}

int getAsrproSignal(void)
{
    if (digitalRead(ASRPRO_SIGNAL_PIN) == HIGH)
    {
        return HIGH;
    }
    else
    {
        return LOW;
    }
    
}