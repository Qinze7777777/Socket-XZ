#include <Arduino.h>
#include "private/mywifi.h"
#include "private/myfile.h"
#include "private/myconfig.h"
#include "private/mystate.h"
#include "private/myNTP.h"
#include "private/mymqtt.h"
#include "private/BL0942.h"
#include "private/AHT20.h"
#include "private/ota.h"
#include "private/myudp.h"
#include "private/audio.h"
#include "private/codec.h"
#include "private/xzmqtt.h"
#include "private/Asrpro.h"

int selectAudio;
bool init_success;
int soundplay;
// put function declarations here:
void Audio_opus_task(void *pvParameters);
void Update_task(void *pvParameters);
void MQTT_loop_task(void *pvParameters);
void setup() {
  // put your setup code here, to run once:
  	Serial.begin(115200);

	//初始化外设
	initBL0942();
	// initAHT20();
    initAsrpro();
    
	// initRelay();
    initMicI2SVad();
    initSoundI2S();
    LittleFS_init();

    //加载配置、状态
    loadSound();
	loadDevState();
	loadDevConfig();
    // 配置网络选项
	connectWifi();      //连接WIFI
    
	initNTP();          //配置NTP，同步设备时间
	initHassMQTT();         //初始化hassMQTT
    getOTA();           //获取xiaozhi端OTA信息
    initXZMQTT();       //初始化xiaozhi端mqtt

    //等待MQTT连接
    while (1)
    {
        
        if (!checkHassMQTT())
        {
            Serial.println("HASS MQTT CONNECTING");
        }
        if (!checkXZMQTT())
        {
            Serial.println("XZ MQTT CONNECTING");
            loopXZMQTT();
        }
        if (checkHassMQTT() && checkXZMQTT())
        {
            break;
        }
        delay(500);
    }
	// configDev();		//配置设备
    // 初始化定时器
    timerInit();
    
    // 创建任务
	xTaskCreate(
        Audio_opus_task,   // 任务函数
        "Audio_opus_task", // 任务名称
        42000,    // 堆栈大小
        NULL,    // 任务参数
        10,       // 任务优先级
        NULL     // 任务句柄
    );
    xTaskCreate(
        MQTT_loop_task,   // 任务函数
        "MQTT_loop_task", // 任务名称
        4096,    // 堆栈大小
        NULL,    // 任务参数
        11,       // 任务优先级
        NULL     // 任务句柄
    );
    init_success = true;
}

void loop() 
{		
    loopXZMQTT();
}


void MQTT_loop_task(void *pvParameters)
{
    
    while (1)
    {
        saveDevState();
        //播放初始化提示音
        if (init_success)
        {
            soundplay = 1;
            addSoundToBuffer(SOUND_INIT);
            soundplay = -1;
            init_success = false;
            vTaskDelay(pdMS_TO_TICKS(1000));
            publishHelloMsg();
            Serial.println("send hello");
        }
        
        
        if (getAsrproSignal() == HIGH)
        {
            soundplay = 1;
            addSoundToBuffer(SOUND_WELCOME);
            soundplay = -1;
            vTaskDelay(pdMS_TO_TICKS(1000));
            publishHelloMsg();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Audio_opus_task(void *pvParameters)
{
    uint8_t I2Sbuf[FRAME_OUT_SIZE_8];
    uint8_t opusbuf[512];
    uint8_t aesbuf[512];
    uint8_t sendbuf[512];       //发送给服务器的加密音频数据
    unsigned char nonce_bytes[16];
    uint8_t *recvbuf;       //接受服务器返回的加密音频数据
    uint16_t length_recv;
	int ret = 0;
    int encodelength;
    uint8_t encoder; //编码器初始化标志位
    uint8_t decoder; //编码器初始化标志位
    uint8_t aes;    //aes初始化标志位
    uint8_t starttalk; //开始说话标志位

	while (1)
	{
        if (recvhello == 1)         //如果收到hello信息
        {
            Serial.println("========recv hello========");
            //初始化aes
            initAES(udpinfo.key);
            aes = 1;
            starttalk = 1;
            if (connectAudioUdp())
            {
                Serial.println("========connect UDP========");
                publishIOTMsg();
                publishIOTUpdateMsg();
                publishListenMsg();
                Serial.printf("Free heap: %u\n", esp_get_free_heap_size());
            }
            else
            {
                Serial.println("connect UDP fail");
            }
            //创建编码器
            if (initOpusEncoder() == 0)
            {
                Serial.println("========init opus encoder========");
                encoder = 1;
                selectAudio = AUDIO_STATE_RECORDSOUND;
                Serial.println("open mic");
            }
            else
            {
                Serial.printf("Free heap: %u\n", esp_get_free_heap_size());
                Serial.println("init encoder fail");
                encoder = 0; 
                selectAudio = AUDIO_STATE_IDLE;
            }
            recvhello = 0;
        }
        if (recvstart == 1)         //如果收到tts start信息
        {
            if (checkUdpConnect())      //如果udp已连接
            {
                Serial.println("start decode");
                selectAudio = AUDIO_STATE_DECODESOUND;
            }
            recvstart = 0;
        }
        if (recvstop == 1)         //如果收到tts stop信息
        {
            if (getLengthBuffer() == 0) 
            {
                vTaskDelay(100);//等待看看是否会收到goodbye,继续对话的前提是用户没有结束
                if (userover)
                {
                    recvstop = 0;//用户主动结束
                }
                else
                {
                    destoryAES();
                    aes = 0;
                    //销毁解码器
                    if (destoryOpusDecoder() == 0)
                    {
                        Serial.println("========destory decoder========");
                        decoder = 0;
                    }
                    Serial.println("stop play");
                    //清理标志位
                    recvstop = 0;
                    initAES(udpinfo.key);
                    aes = 1;
                    
                    Serial.println("========connect UDP========");
                    publishIOTMsg();
                    publishIOTUpdateMsg();
                    publishListenMsg();
                    if (initOpusEncoder() == 0)
                    {
                        Serial.println("========init opus encoder========");
                        encoder = 1;
                        selectAudio = AUDIO_STATE_RECORDSOUND;
                        starttalk = 1;
                        Serial.println("open mic");
                    }
                    else
                    {
                        Serial.println("init encoder fail");
                        encoder = 0; 
                        selectAudio = AUDIO_STATE_IDLE;
                    }
                }
                
            }
        }                     
        if (soundplay == 1)
        {
            //创建解码器
            if (initOpusDecoder() == 0)
            {
                Serial.println("========init decoder========");
                decoder = 1;
            }
            else
            {
                Serial.println("init decoder fail");
                decoder = 0; 
            }
            selectAudio = AUDIO_STATE_PLAYSOUND;
            soundplay = 0;
        }
        if (soundplay == -1)
        {
            //停止播放
            if (getLengthBuffer() == 0)
            {
                if (destoryOpusDecoder() == 0)
                {
                    Serial.println("========destory decoder========");
                    decoder = 0;
                }
                selectAudio = AUDIO_STATE_IDLE;
                soundplay = 0;
            }

        }
        
        
        if (userover)
        {
            if (starttalk)  //说明正在说话，不应该停止，标志位被错误置1
            {
                Serial.println("userover error");
                userover = 0;
            }
            else
            {
                // vTaskDelay(2000);   //延时等待，防止清理完标志位又被置位
                Serial.println("=============userover=============");
                selectAudio = AUDIO_STATE_IDLE;
                encoder = 0;
                decoder = 0;
                recvhello = 0;
                recvstart = 0;
                recvstop = 0;
                soundplay = 0;
                if (aes == 1)
                {
                    destoryAES();
                    aes = 0;
                }
                resetOpusSequence();
                destoryOpusDecoder();
                destoryOpusEncoder();
                userover = 0;
                //关闭udp连接
                closeUdp();
            }
            
        }
        if (selectAudio == AUDIO_STATE_PLAYSOUND && decoder == 1)       //播放声音
        {
            if (getFromBuffer(&recvbuf, &length_recv))
            {
                //opus解码
                ret = decodeOpus(recvbuf, length_recv, I2Sbuf);
                if (ret == -1)
                {
                    Serial.println("opus decode error");
                }
                i2s_write_with_volume(I2Sbuf, ret * 2, devstate.volume.toFloat() / 100);
            }
            // else
            // {
            //     vTaskDelay(pdMS_TO_TICKS(10));
            // }
        }    
        if (selectAudio == AUDIO_STATE_RECORDSOUND && encoder == 1)       //录音
        {
            ret = read60msMic(I2Sbuf);
            if (ret == FRAME_IN_SIZE_8)
            {
                encodelength = encodeOpus(I2Sbuf, FRAME_IN_SIZE_8, opusbuf);
                if (encodelength == -1)
                {
                    Serial.println("encode error");
                }
                //处理nonce
                processNonce(udpinfo.nonce, encodelength, nonce_bytes);
                //aes-ctr 128加密
                if (encryptAESCTR128(nonce_bytes, opusbuf, encodelength, aesbuf) == -1)
                {
                    Serial.println("AESCTR128 encrypt error");
                }
                //拼接数据
                memcpy(sendbuf, nonce_bytes, 16);
                memcpy(sendbuf + 16, aesbuf, encodelength); //aes加密完长度不变
                //发送
                sendUdpData(sendbuf, encodelength + 16);
                ret = 0;                
            }
            if (ret == -1)      //表示检测不到人声
            {
                selectAudio = AUDIO_STATE_IDLE;
                starttalk = 0;
                destoryAES();
                initAES(udpinfo.key); //重新初始化aes
                aes = 1;
                //销毁编码器
                if (destoryOpusEncoder() == 0)
                {
                    Serial.println("========destory encoder========");
                    encoder = 0;
                }
                //创建解码器
                if (initOpusDecoder() == 0)
                {
                    Serial.println("========init decoder========");
                    decoder = 1;
                }
                else
                {
                    Serial.println("init decoder fail");
                    decoder = 0; 
                }
                Serial.println("end talk");
                publishStopMsg();
                Serial.println("stop listen");
                ret = 0;
            }
        }
        if (selectAudio == AUDIO_STATE_DECODESOUND && decoder == 1)     //解码并播放
        {
            if (getFromBuffer(&recvbuf, &length_recv))
            {
                unsigned char nonce_recv[16];
                int length_data = length_recv - 16;
                //拆分数据
                //从recvbuf中提取nonce
                memcpy(nonce_recv, recvbuf, 16);
                //从recvbuf中提取data
                memcpy(aesbuf, (recvbuf + 16), length_data);
                //aes解密
                ret = decryptAESCTR128(nonce_recv, aesbuf, length_data, opusbuf);
                if (ret == -1)
                {
                    Serial.println("AESCTR128 decrypt error");
                }
                //opus解码
                ret = decodeOpus(opusbuf, length_data, I2Sbuf);
                if (ret == -1)
                {
                    Serial.println("opus decode error");
                }
                //写入缓冲区
                // addToAudioBuffer(I2Sbuf, ret * 2);
                i2s_write_with_volume(I2Sbuf, ret * 2,  devstate.volume.toFloat() / 100);
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
		vTaskDelay(pdMS_TO_TICKS(1));
    }
}