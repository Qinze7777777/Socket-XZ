#include "private/BL0942.h"

HardwareSerial S1(1);

uint8_t SerialTemps[30];

void initBL0942(void)
{
    S1.begin(4800, SERIAL_8N1, 25, 26);       //初始化25、26为串口1引脚
    enableWrite();      //使能写
    setCntClear();      //设置每次读取完清空功率计数
}



void sendCommandForRead(void)
{
    S1.write((byte)0x58);
    S1.write((byte)0xAA);
}

void writeReg(byte reg, uint32_t value)
{
    uint8_t checknum;//校验和
    char buf_send[10];  //发送缓冲区
    memset(buf_send, 0, sizeof(buf_send));
    //发送命令字节 A8 参考手册中发送时序
    //构建发送包
    buf_send[0] = 0xA8;
    buf_send[1] = reg;
    buf_send[2] = value & 0xFF;
    buf_send[3] = (value >> 8) & 0xFF;
    buf_send[4] = (value >> 16) & 0xFF;
    checknum = ~((buf_send[0] + buf_send[1] + buf_send[2] + buf_send[3] + buf_send[4]) & 0XFF);
    buf_send[5] = checknum;
    S1.write(buf_send, 6);
    delay(20);
}

uint32_t readReg(byte reg)
{
    uint8_t checknum;//校验和
    uint8_t rdata[5]; //所接收的数据  
    uint8_t buf_send[2];  //发送缓冲区
    uint8_t i = 0, rnum;
    memset(buf_send, 0, sizeof(buf_send));
    //构建发送包
    buf_send[0] = 0x58;
    buf_send[1] = reg;

    //发送命令字节 58 参考手册中接收时序
    S1.flush(true);         //清空串口接收缓冲区
    S1.write(buf_send, 2);
    delay(20);        //延时20ms
    rnum = S1.available();
    if (rnum > 4)
    {
        for (size_t i = 0; i < rnum - 4; i++)
        {
            S1.read();
        }
    }
    
    //接收发送来的4字节
    S1.readBytes(rdata, 4);
    Serial.printf("0X%X 0X%X 0X%X 0X%X\n", rdata[0], rdata[1], rdata[2], rdata[3]);
    
    checknum = ~((buf_send[0] + buf_send[1] + rdata[0] + rdata[1] + rdata[2]) & 0XFF);
    Serial.printf("cal checknum:0X%X\n", checknum);
    delay(20);        //延时20ms
    if (checknum != rdata[3])
    {
        Serial.println("read checknum error");
        return 0;
    }
    else
    {
        return ((uint32_t)rdata[2] << 16) + ((uint32_t)rdata[1] << 8) + (uint32_t)rdata[0];
    }
}

void getBL0942(void)
{
    uint8_t cnt = 0;
    for (size_t i = 0; i < S1.available(); i++)
    {
        S1.read();
    }

    sendCommandForRead();
    delay(50);
    // Serial.printf("length %d", S1.available());
    S1.readBytes(SerialTemps, S1.available());
    while (1)
    {
        if (SerialTemps[cnt] != 0X55)
        {
            cnt++;
        }
        else
        {
            break;
        }
    }
    for (size_t i = cnt; i < 30; i++)
    {
        SerialTemps[i - cnt] = SerialTemps[i];
    }
    // Serial.printf("0:%X\n", SerialTemps[0]);
    // Serial.printf("1:%X\n", SerialTemps[1]);
    // Serial.printf("2:%X\n", SerialTemps[2]);
    devstate.I = getCurrent();
    // Serial.println("I:" + devstate.I);
    devstate.V = getVoltage();
    // Serial.println("V:" + devstate.V);
    devstate.P = getActivePower();
    // Serial.println("P:" + devstate.P);
    devstate.E = String(devstate.E.toFloat() + getEnergy().toFloat());
    // Serial.println("E:" + devstate.E);
    delay(20);
}

void enableWrite(void)      //写使能
{
    writeReg(0X1D, 0X55);
}

void setCntClear(void)      //设置电能读后清零
{
    writeReg(0X19, 0XC7);
    if (readReg(0X19) == 0XC7)
    {
        Serial.println("set energy cnt clear success");
    }
}


String getCurrent(void)
{
    uint32_t parm = 0;
    parm = ((uint32_t)SerialTemps[3] << 16) + ((uint32_t)SerialTemps[2] << 8) + SerialTemps[1];
    float current = (float)parm * V_REF * 1000 / (305978 * RL_CURRENT); // mA

    return String(current);
}

String getVoltage(void)
{
    uint32_t parm = 0;
    parm = ((uint32_t)SerialTemps[6] << 16) + ((uint32_t)SerialTemps[5] << 8) + SerialTemps[4];
    float voltage = (float)parm * V_REF * (R2_VOLTAGE + R1_VOLTAGE) / (73989 * R1_VOLTAGE * 1000);

    return String(voltage);
}

// String getFastCurrent()
// {
//     uint32_t parm = 0;
//     parm = ((uint32_t)SerialTemps[9] << 16) + ((uint32_t)SerialTemps[8] << 8) + SerialTemps[7];
//     float fcurrent = (float)parm * V_REF * 1000 / (305978 * RL_CURRENT); // mA

//     return String(fcurrent);
// }

String getActivePower(void)
{
    uint32_t parm = 0;
    parm = ((uint32_t)SerialTemps[12] << 16) + ((uint32_t)SerialTemps[11] << 8) + SerialTemps[10];
    byte flag = bitRead(SerialTemps[12], 7);
    if (1 == flag)
    {
        parm = 0xFFFFFF - parm + 1; // 取补码
    }
    float power = (float)parm  * V_REF * V_REF * (R2_VOLTAGE + R1_VOLTAGE) / (3537 * RL_CURRENT * R1_VOLTAGE * 1000);

    return String(power);
}

String getEnergy(void)
{
    uint32_t parm = 0;
    parm = ((uint32_t)SerialTemps[15] << 16) + ((uint32_t)SerialTemps[14] << 8) + SerialTemps[13];
    float energy = (float)parm * 1638.4 * 256 * V_REF * V_REF * (R2_VOLTAGE + R1_VOLTAGE) / (3600000.0 * 3537 * RL_CURRENT * R1_VOLTAGE * 1000);

    return String(energy);
}

String getFREQ()
{
    uint32_t parm = 0;
    parm = ((uint32_t)SerialTemps[18] << 16) + ((uint32_t)SerialTemps[17] << 8) + SerialTemps[16];
    float FREQ = 0;
    if (parm > 0)
    {
        FREQ = 500 * 1000 * 2 / (float)parm;
    }

    return String(FREQ);
}