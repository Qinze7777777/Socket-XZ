#include "AHT20.h"

AHT20 myAHT20;

void initAHT20(void)
{
    myAHT20.begin();
	myAHT20.available();	//上电启动测量
}

void getAHT20(void)
{
    if (myAHT20.available() == true)
	{
		devstate.humidity = myAHT20.getHumidity();
        devstate.temperature = myAHT20.getTemperature();
	}
}
