/*获取时间 网络对时*/
#include "private/myNTP.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "cn.ntp.org.cn", 3600, 60000);

int tryNtpServers(void)
{
    // 定义一个字符数组，包含多个NTP服务器地址
    char *arr[] = {(char *)"cn.ntp.org.cn", (char *)"cn.pool.ntp.org", (char *)"ntp.aliyun.com", (char *)"ntp.ntsc.ac.cn", (char *)"ntp.tencent.com", (char *)"time.edu.cn"};
    unsigned char count = 0;
  //  Serial.print("N...");

    // 遍历字符数组，依次尝试连接NTP服务器
    for (char *server : arr)
    {
     //   Serial.print("NTP Server:");
    //    Serial.print(server);

        // 设置NTP服务器地址
        timeClient.setPoolServerName(server);
        // 设置时区偏移量
        timeClient.setTimeOffset(8 * 3600);
        // 开始连接NTP服务器
        timeClient.begin();
        // 更新时间
        timeClient.update();

        // 如果成功获取到时间，则返回当前服务器索引
        if (timeClient.isTimeSet())
        {
       //     Serial.println(",Done.");
            timeClient.end();
            return count;
        }

        count++;
     //   Serial.println(",Change NTP Server.");
        delay(500);
    };

        timeClient.end();
    return -1;
}

int initNTP(void)
{
    if (0 > tryNtpServers())
    {
        Serial.println("NTP can not Init!");
        return 0;
    }
    Serial.println("============NTP Init============");
    return 1;
}

String getDateTime(void)
{
    return timeClient.getFormattedTime();
}

String getTime(void)
{
    return timeClient.getFormattedTime();
}

String getHMTime(void)
{
    unsigned long rawTime = timeClient.getEpochTime();
    unsigned long hours = (rawTime % 86400L) / 3600;
    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

    unsigned long minutes = (rawTime % 3600) / 60;
    String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

    unsigned long seconds = rawTime % 60;
    String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

    return hoursStr + ":" + minuteStr;
}

String getMSTime(void)
{
    unsigned long rawTime = timeClient.getEpochTime();
    unsigned long hours = (rawTime % 86400L) / 3600;
    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

    unsigned long minutes = (rawTime % 3600) / 60;
    String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

    unsigned long seconds = rawTime % 60;
    String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

    return minuteStr + ":" + secondStr;
}


int hour(void)
{
    unsigned long rawTime = timeClient.getEpochTime();
    unsigned long hh = (rawTime % 86400L) / 3600;

    return hh;
}

int minute(void)
{
    unsigned long rawTime = timeClient.getEpochTime();
    unsigned long mm = (rawTime % 3600) / 60;

    return mm;
}

int second(void)
{
    unsigned long rawTime = timeClient.getEpochTime();
    int ss = rawTime % 60;

    return ss;
}

unsigned long rawTime(void)
{
    return timeClient.getEpochTime();
}