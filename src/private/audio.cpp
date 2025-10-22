/*
 * @Author: Qinze7 2598799271@qq.com
 * @Date: 2025-03-21 12:15:38
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-04-20 17:44:38
 * @FilePath: \hass_mqtt_xiaozhi\src\private\audio.cpp
 * @Description: 
 * 
 */
#include "private/audio.h"

VadInst *vadInst;
int count = 0;      //人说话检测计数,当连续检测到30次无人说话时，认为对话结束

AudioPacket ringBuffer[BUFFER_SIZE]; // 环形缓冲区
volatile int head = 0; // 指向下一个写入的位置
volatile int tail = 0; // 指向下一个读取的位置
volatile bool bufferFull = false; // 标记缓冲区是否满了

Sound sound_welcome;
Sound sound_init;

void initMicI2SVad(void)
{
    i2s_config_t i2s_mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_IN_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 6,
    .dma_buf_len = 1024,
    .use_apll=0,
    .tx_desc_auto_clear= true, 
    .fixed_mclk=-1    
  };
    i2s_pin_config_t pin_config = {
        .bck_io_num = IN_BCK_IO,       // 比特时钟
        .ws_io_num = IN_WS_IO,        // 左右声道选择
        .data_out_num = I2S_PIN_NO_CHANGE,     // 数据输出
        .data_in_num = IN_DATAIN_IO       // 数据输入
    };
    i2s_driver_install(I2S_NUM_0, &i2s_mic_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);      
    if (initVAD())
    {
        Serial.println("Audio I2S init success");
    }
}

void initSoundI2S(void)
{
    i2s_config_t i2s_sound_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_OUT_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 6,
    .dma_buf_len = 1024,
    .use_apll = 0,
    .tx_desc_auto_clear=true,
    .fixed_mclk = -1
  };
      // .use_apll = 0,             
    // .fixed_mclk=-1  
    i2s_pin_config_t pin_config = {
        .bck_io_num = OUT_BCK_IO,       // 比特时钟
        .ws_io_num = OUT_WS_IO,        // 左右声道选择
        .data_out_num = OUT_DATAOUT_IO,     // 数据输出
        .data_in_num = I2S_PIN_NO_CHANGE       // 数据输入
    };
    i2s_driver_install(I2S_NUM_1, &i2s_sound_config, 0, NULL);
    i2s_set_pin(I2S_NUM_1, &pin_config);    
    i2s_zero_dma_buffer(I2S_NUM_1);                                                                                              
}

void changeRate(int rate)
{
    i2s_set_sample_rates(I2S_NUM_0, rate);
}
int initVAD(void)
{
    vadInst = WebRtcVad_Create();
    if (vadInst == NULL) return -1;
    int status = WebRtcVad_Init(vadInst);
    if (status != 0) {
        printf("WebRtcVad_Init fail\n");
        WebRtcVad_Free(vadInst);
        return -1;
    }
    status = WebRtcVad_set_mode(vadInst, 2);        //高灵敏
    if (status != 0) {
        printf("WebRtcVad_set_mode fail\n");
        WebRtcVad_Free(vadInst);
        return -1;
    }
    Serial.println("VAD init success");
    return 1;
}

size_t i2s_read(uint8_t* data, size_t size) {
    size_t bytesRead;
    i2s_read(I2S_NUM_0, data, size, &bytesRead, portMAX_DELAY);
    return bytesRead;
}

size_t i2s_write_sound(uint8_t* data, size_t size, float output_volume) 
{
    size_t byteswritten;
    i2s_write(I2S_NUM_1, data, size, &byteswritten, portMAX_DELAY);
    return byteswritten;
}

// I2S 写入函数，带音量调整
size_t i2s_write_with_volume(uint8_t *data, size_t length, float volume) 
{
    // 确保输入数据是 16 位 PCM，length 是偶数（每样本 2 字节）
    if (length % 2 != 0) 
    {
        return 0; // 数据长度错误
    }
    
    // 将 uint8_t 数据转换为 int16_t 以便处理
    int16_t *samples = (int16_t *)data;
    size_t num_samples = length / 2; // 样本数
    
    // 创建临时缓冲区存储调整后的数据
    int16_t *adjusted_samples = (int16_t *)malloc(length);
    if (adjusted_samples == NULL) 
    {
        return 0; // 内存分配失败
    }
    
    // 调整音量
    for (size_t i = 0; i < num_samples; i++) 
    {
        // 将样本值乘以音量因子
        int32_t temp = (int32_t)(samples[i] * volume);
        
        // 限制输出范围，避免溢出
        if (temp > 32767) 
        {
            adjusted_samples[i] = 32767;
        } 
        else if (temp < -32768) 
        {
            adjusted_samples[i] = -32768;
        } 
        else 
        {
            adjusted_samples[i] = (int16_t)temp;
        }
    }
    size_t bytes_written;    
    // 调用底层 I2S 写入函数
    i2s_write(I2S_NUM_1, (uint8_t *)adjusted_samples, length, &bytes_written, portMAX_DELAY);
    
    // 释放临时缓冲区
    free(adjusted_samples);
    
    return bytes_written;
}

// int read30msMic(uint8_t* buf)//30MS 16000K 16BIT 
// {
//     static unsigned long starttime = 0;
//     unsigned long endtime;
//     int ret, vadret;
//     ret = i2s_read(buf, FRAME_SIZE_8); // 读取I2S数据到缓冲区
//     vadret = WebRtcVad_Process(vadInst, SAMPLE_RATE, (int16_t *)buf, SAMPLES);
//     if (!starttime) 
//     {
//         starttime = millis();
//     }
//     endtime = millis();
//     if (vadret == 1)
//     {
//         Serial.println("detect");
//     }
//     else
//     {
//         Serial.println("no detect");
//         count++;
//     }
//     if (endtime - starttime >= 2000)
//     {
//         if (count > 50)     //如果计数超过，则认为录音结束
//         {
//             Serial.println("close mic");
//             count = 0;
//             starttime = 0;
//             return -1;       //表示录音结束
//         }
//         count = 0;
//         starttime = 0;
//     }
//     if (ret > 0)
//     {
//         return ret;
//     }
//     else
//     {
//         Serial.println("i2s read error");
//         return 0;
//     }
// }

int read60msMic(uint8_t* buf)//60MS 16000K 16BIT 
{
    static unsigned long starttime = 0;
    unsigned long endtime;
    int ret, vadret;
    ret = i2s_read(buf, FRAME_IN_SIZE_8); // 读取I2S数据到缓冲区
    vadret = WebRtcVad_Process(vadInst, SAMPLE_IN_RATE, (int16_t *)buf, SAMPLES_IN / 2);
    if (vadret == 1)
    {
        Serial.println("detect");
    }
    else
    {
        Serial.println("no detect");
        count++;
    }
    if (!starttime) 
    {
        starttime = millis();
    }
    endtime = millis();
    //如果2秒内，没有检测到声音的数值超过一定值，则认为录音结束，返回-1
    if (endtime - starttime >= 3000)
    {
        if (count > 40)     //如果计数超过，则认为录音结束
        {
            Serial.println("close mic");
            count = 0;
            starttime = 0;
            return -1;       //表示录音结束
        }
        count = 0;
        starttime = millis();
    }
    if (ret > 0)
    {
        return ret;
    }
    else
    {
        Serial.println("i2s read error");
        return 0;
    }
}

int read30msMic(uint8_t* buf)//30MS 16000K 16BIT 
{
    static unsigned long starttime = 0;
    unsigned long endtime;
    int ret, vadret;
    ret = i2s_read(buf, FRAME_IN_SIZE_8); // 读取I2S数据到缓冲区
    vadret = WebRtcVad_Process(vadInst, SAMPLE_IN_RATE, (int16_t *)buf, SAMPLES_IN);
    if (vadret == 1)
    {
        Serial.println("detect");
    }
    else
    {
        // Serial.println("no detect");
        count++;
    }
    //初始化计时器
    if (!starttime) 
    {
        starttime = millis();
    }
    endtime = millis();
    //如果2秒内，没有检测到声音的数值超过一定值，则认为录音结束，返回-1
    if (endtime - starttime >= 2000)
    {
        if (count > 50)     //如果计数超过，则认为录音结束
        {
            Serial.println("close mic");
            count = 0;
            starttime = 0;
            return -1;       //表示录音结束
        }
        count = 0;
        starttime = 0;
    }
    //正常返回读取的长度
    if (ret > 0)
    {
        return ret;
    }
    else
    {
        Serial.println("i2s read error");
        return 0;
    }
}

// DecodeAudio AudioBuffer[AUDIOBUFFER_SIZE]; // 环形缓冲区
// volatile int audiohead = 0; // 指向下一个写入的位置
// volatile int audiotail = 0; // 指向下一个读取的位置
// volatile bool aduiobufferFull = false; // 标记缓冲区是否满了

// // 添加数据到环形缓冲区
// void addToAudioBuffer(const uint8_t *data, uint16_t length)
// {
//     memcpy(AudioBuffer[audiohead].data, data, length);
//     AudioBuffer[audiohead].len = length;

//     audiohead = (audiohead + 1) % AUDIOBUFFER_SIZE;

//     if (aduiobufferFull)
//     {
//         audiotail = (audiotail + 1) % AUDIOBUFFER_SIZE; // 覆盖旧数据
//     }

//     if (audiohead == audiotail)
//     {
//         aduiobufferFull = true; // 缓冲区满了
//         Serial.println("Audio buffer is full!");
//     }
// }

// // 取出数据（如果有）
// bool getFromAudioBuffer(uint8_t **data, uint16_t *length)
// {
//     if (audiohead == audiotail && !aduiobufferFull) // 没有数据
//     {
//         return false;
//     }

//     *data = AudioBuffer[audiotail].data;
//     *length = AudioBuffer[audiotail].len;
//     audiotail = (audiotail + 1) % AUDIOBUFFER_SIZE;
//     aduiobufferFull = false;
//     return true;
// }

// int getLengthAudioBuffer(void)
// {
//     if (aduiobufferFull)
//     {
//         return AUDIOBUFFER_SIZE;
//     }
//     return (audiohead - audiotail + AUDIOBUFFER_SIZE) % AUDIOBUFFER_SIZE;
// }



// 添加数据到环形缓冲区
void addToBuffer(const uint8_t *data, uint16_t length)
{
    memcpy(ringBuffer[head].data, data, length);
    ringBuffer[head].len = length;

    head = (head + 1) % BUFFER_SIZE;

    if (bufferFull)
    {
        tail = (tail + 1) % BUFFER_SIZE; // 覆盖旧数据
    }

    if (head == tail)
    {
        bufferFull = true; // 缓冲区满了
        Serial.println("UDP buffer is full!");
    }
}

// 取出数据（如果有）
bool getFromBuffer(uint8_t **data, uint16_t *length)
{
    if (head == tail && !bufferFull) // 没有数据
    {
        return false;
    }

    *data = ringBuffer[tail].data;
    *length = ringBuffer[tail].len;

    tail = (tail + 1) % BUFFER_SIZE;
    bufferFull = false;
    return true;
}

int getLengthBuffer(void)
{
    if (bufferFull)
    {
        return BUFFER_SIZE;
    }
    return (head - tail + BUFFER_SIZE) % BUFFER_SIZE;
}

void loadSound(void)
{
    sound_welcome.len  = readFileBytes("/welcome.p3", (char *)sound_welcome.data);
    sound_init.len = readFileBytes("/init.p3", (char *)sound_init.data);
}

void addSoundToBuffer(SoundKind sound)
{
    uint8_t buf[PACKET_SIZE];
    uint8_t head[4];
    Sound *sound_data;
    size_t i = 0;   //当前已读字节
    if (sound == SOUND_WELCOME)
    {
        sound_data = &sound_welcome;
        
    }
    if (sound == SOUND_INIT)
    {
        sound_data = &sound_init;
    }
    while (i < sound_data->len)
    {
        //先读取4字节头部
        memcpy(head, sound_data->data + i, 4);
        //读取帧
        uint16_t frame_len = (head[2] << 8) | head[3]; // 大端
        i += 4;
        // Serial.printf("frame_len:%d\n", frame_len);
        memcpy(buf, sound_data->data + i, frame_len);
        addToBuffer(buf, frame_len);
        i += frame_len;
        vTaskDelay(1);
    }
}
