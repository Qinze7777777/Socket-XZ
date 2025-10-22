/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-28 14:29:10
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-04-20 15:50:11
 * @FilePath: \hass_mqtt_xiaozhi\src\private\audio.h
 * @Description: 
 * 
 */
#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <Arduino.h>
#include <driver/i2s.h>
#include "vad/include/vad.h"
#include "private/myfile.h"

#define FRAME_IN_SIZE_8 1920
#define SAMPLES_IN 960
#define FRAME_OUT_SIZE_8 2880
#define SAMPLES_OUT 1440
#define FRAME_DURATION 30
#define SAMPLE_IN_RATE 16000
#define SAMPLE_OUT_RATE 24000

#define IN_BCK_IO 12
#define IN_WS_IO 13
#define IN_DATAIN_IO 14

#define OUT_BCK_IO 33
#define OUT_WS_IO 27
#define OUT_DATAOUT_IO 32


#define BUFFER_SIZE 40  // 缓冲区最多存 40 个数据包
#define PACKET_SIZE 300 // 每个数据包最大 300 字节

struct AudioPacket
{
    uint8_t data[PACKET_SIZE];
    uint16_t len;
};

struct Sound
{
    uint8_t data[4096];
    uint16_t len;
};

typedef enum AudioState
{
    AUDIO_STATE_IDLE = 0,
    AUDIO_STATE_RECORDSOUND,
    AUDIO_STATE_DECODESOUND,
    AUDIO_STATE_PLAYSOUND,
}AudioState;

typedef enum SoundKind
{
    SOUND_WELCOME = 0,
    SOUND_INIT,
    SOUND_NONE
}SoundKind;


void initMicI2SVad(void);
void initSoundI2S(void);
int initVAD(void);
void recordAudio(void);
int read60msMic(uint8_t* buf);
int read30msMic(uint8_t* buf);
size_t i2s_write_sound(uint8_t* data, size_t size);
size_t i2s_write_with_volume(uint8_t *data, size_t length, float volume);
void changeRate(int rate);
void addToBuffer(const uint8_t *data, uint16_t length);
bool getFromBuffer(uint8_t **data, uint16_t *length);
int getLengthBuffer(void);
void loadSound(void);
void addSoundToBuffer(SoundKind sound);

#endif