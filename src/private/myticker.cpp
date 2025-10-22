/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-28 14:27:35
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-04-11 16:25:36
 * @FilePath: \hass_mqtt_xiaozhi\src\private\myticker.cpp
 * @Description: 从ticker更新为freertos定时器
 */
/*定时任务*/
#include "private/myticker.h"

TimerHandle_t xSocketTimer, xUpdateTimer;
unsigned long starttime = 0;
int countdown;  //计时器倒计时

// Ticker ticker_updateState, ticker_timer, ticker_updateBL0942; 
// TimerHandle_t myTimer;
// void updateData(void)
// {
//     getBL0942();
//     getAHT20();
//     updateRelay();
//     saveDevState();
// }


// void setTickers(void)
// {
//     ticker_updateBL0942.attach(1, updateData); //每1秒更新一次设备数据

//     ticker_updateState.attach(3, publishDevState);
// }

// void setTimer(int seconds, bool open)
// {
//     if (open)
//     {
//         starttime = millis();
//         ticker_timer.once(seconds, timerHandler); //设置单次触发定时器
//         Serial.println("timer set" + getTime() + "   senconds:" + String(seconds) + 's');
//     }
//     else
//     {
//         ticker_timer.detach(); //关闭定时器
//         starttime = 0;
//     }
// }

void updateCountdown(void)
{
    //计算定时器倒计时
	if (starttime != 0)
	{
		unsigned long endtime = millis();
		countdown = devstate.timernum.toInt() - ((endtime - starttime) / 1000);
        devstate.countdown = String(countdown);
	}
	else
	{
		devstate.countdown = "0";
	}
}

void socketTimerCallback(TimerHandle_t xTimer)
{
    //定时器响应
    Serial.println("timer out" + getTime());
    //反转插座
    if (devstate.socket == "1")
    {
        devstate.socket = "0"; //插座关闭
    }
    else
    {
        devstate.socket = "1"; //插座打开
    }
    starttime = 0;
    devstate.timer = "0";
    xTimerStop(xTimer, 0);
}

void updateTimerCallback(TimerHandle_t xTimer)
{
    static u8_t nums = 0;
    getBL0942();
    // // getAHT20();
    // // updateRelay();
    updateCountdown();
    nums++;
    //每2次向hass更新一次设备状态
    if (nums % 3 == 0)
    {
        nums = 0;
        // Serial.println("publishDevState");
        publishDevState();
    }
}

void timerInit(void)
{
    // 创建定时器，周期为 1000ms，自动重载
    xUpdateTimer = xTimerCreate("updatetimer", 
                          pdMS_TO_TICKS(1000),  
                          pdTRUE,          
                          (void *)0,      
                          updateTimerCallback);
    if (xUpdateTimer == NULL)
    {
        // 创建失败
        Serial.println("CREATE UPDATETIMER FAILED");
    }
    else
    {
        Serial.println("=========TIMER INIT=======");
        xTimerStart(xUpdateTimer, 0);
    }
}

void setSocketTimer(int seconds, bool open)
{
    if (open)
    {
        starttime = millis();
        xSocketTimer = xTimerCreate("sockettimer", 
                          pdMS_TO_TICKS(1000 * seconds),  
                          pdTRUE,          
                          (void *)0,      
                          socketTimerCallback);
        if (xSocketTimer == NULL)
        {
            // 创建失败
            Serial.println("CREATE UPDATETIMER FAILED");
        }
        else
        {
            xTimerStart(xSocketTimer, 0);
        }
        Serial.println("timer set" + getTime() + "   senconds:" + String(seconds) + 's');
    }
    else
    {
        xTimerStop(xSocketTimer, 0);
        starttime = 0;
    }
}



