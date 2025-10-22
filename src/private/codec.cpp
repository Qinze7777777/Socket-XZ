#include "private/codec.h"

OpusEncoder *opus_encoder = NULL;
OpusDecoder *opus_decoder = NULL;
esp_aes_context aes_ctx;
int local_sequence = 0;         //递增的序列号



void resetOpusSequence(void)
{
    local_sequence = 0;
}

/**
 * @description: 初始化opus编码器
 * @return 0成功 -1失败
 */
int initOpusEncoder(void)
{
    int error;
    //创建编码器
    if (opus_encoder == NULL)
    {
        opus_encoder = opus_encoder_create(SAMPLE_IN_RATE, 1, OPUS_APPLICATION_AUDIO, &error);
        if (error != OPUS_OK) 
        {
            Serial.printf("Failed to create OPUS encoder, error%d", error);
            return -1;
        }   
        opus_encoder_ctl(opus_encoder, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_60_MS));
        // opus_encoder_ctl(opus_encoder, OPUS_SET_VBR(1));
        opus_encoder_ctl(opus_encoder, OPUS_SET_BITRATE(16000));   // 8kbps
        opus_encoder_ctl(opus_encoder, OPUS_SET_COMPLEXITY(5));    // 复杂度 5
        // opus_encoder_ctl(opus_encoder, OPUS_SET_VBR(0));  // 关闭 VBR，启用 CBR
        // opus_encoder_ctl(opus_encoder, OPUS_SET_FORCE_CHANNELS(1));// 单声道
        opus_encoder_ctl(opus_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
        return 0;
    }
    return -1;
}
/**
 * @description: 初始化opus解码器
 * @return 0成功 -1失败
 */
int initOpusDecoder(void)
{
    int error;
    //创建解码器
    if (opus_decoder == NULL)
    {
        opus_decoder = opus_decoder_create(SAMPLE_OUT_RATE, 1, &error);
        if (error != OPUS_OK) 
        {
            Serial.printf("Failed to create OPUS decoder, error%d", error);
            return -1;
        }
        return 0;
    }
    return -1;
}
/**
 * @description: 销毁opus编码器
 * @return 0成功 -1失败
 */
int destoryOpusEncoder(void)
{
    if (opus_encoder != NULL)
    {
        opus_encoder_destroy(opus_encoder);
        opus_encoder = NULL;
        return 0;
    }
    return -1;
}
/**
 * @description: 销毁opus解码器
 * @return 0成功 -1失败
 */
int destoryOpusDecoder(void)
{
    if (opus_decoder != NULL)
    {
        opus_decoder_destroy(opus_decoder);
        opus_decoder = NULL;
        return 0;
    }
    return -1;
}

/**
 * @description: 进行opus编码
 * @param {uint8_t} *input PCM源数据
 * @param {uint16_t} len PCM源数据长度
 * @param {uint8_t} *output 编码数据输出
 * @return 返回编码后的长度 -1表示失败
 */
int encodeOpus(uint8_t *input, uint16_t len, uint8_t *output)
{
    int opus_length;
    opus_length = opus_encode(opus_encoder, (int16_t *)input, len / 2, output, 512);
    if (opus_length < 0)
    {
        Serial.printf("Failed to encode opus, error%d", opus_length);
        return -1;
    }
    return opus_length;
} 

/**
 * @description: 进行opus解码
 * @param {uint8_t} *input 解密完的数据
 * @param {uint16_t} len 解密完的数据长度
 * @param {uint8_t} *output 解码后的PCM数据
 * @return 返回编码后的长度 -1表示失败
 */
int decodeOpus(uint8_t *input, uint16_t len, uint8_t *output)
{
    int opus_length;
    opus_length = opus_decode(opus_decoder, input, len, (int16_t *)output, SAMPLES_OUT , 0);
    if (opus_length < 0)
    {
        Serial.printf("Failed to decode opus, error%d", opus_length);
        return -1;
    }
    return opus_length;
}

/**
 * @description: 初始化AES
 * @return 无
 */
void initAES(String key)
{
    unsigned char key_bytes[16];
    mbedtls_aes_init(&aes_ctx);
    hex_string_to_byte_array(key.c_str(), key_bytes, 16); 
    mbedtls_aes_setkey_enc(&aes_ctx, key_bytes, 128);    //加解密都是用这个api来加载key
    // local_sequence = 0;//重置序列号
}

/**
 * @description: 销毁AES
 * @return 无
 */
void destoryAES(void)
{
    mbedtls_aes_free(&aes_ctx);
    // local_sequence = 0;//重置序列号
}

/**
 * @description: 按照服务器的格式处理nonce
 * @param {String} nonce 服务器返回的nonce
 * @param {uint16_t} len opus编码后的数据长度
 * @param {unsigned char} *output 处理数据输出

 */
void processNonce(String nonce, uint16_t len, unsigned char *output)
{
    //处理nonce
    char len_hex[5];
    char seq_hex[9];
    sprintf(len_hex, "%04x", len); // 编码长度，4位hex
    sprintf(seq_hex, "%08x", local_sequence); // 序列号，8位hex
    String nonce_part1 = nonce.substring(0, 4); 
    String nonce_part2 = nonce.substring(8, 24); 
    String nonce_str = nonce_part1 + len_hex + nonce_part2 + seq_hex;
    local_sequence++; // 序列号递增
    hex_string_to_byte_array(nonce_str.c_str(), output, 16);
}


/**
 * @description: aes-ctr 128加密
 * @param {unsigned char} *nonce 处理好的nonce
 * @param {uint8_t} *input opus编码后的数据
 * @param {uint16_t} len opus编码后的数据长度
 * @param {uint8_t} *output 加密数据输出
 * @return 0表示成功 -1表示失败
 */
int encryptAESCTR128(unsigned char *nonce_bytes, uint8_t *input, uint16_t len, uint8_t *output)
{
    size_t nc_off = 0;
    unsigned char stream_block[16];
    unsigned char nonce_copy[16];
    uint16_t length;
    memcpy(nonce_copy, nonce_bytes, 16);
    memset(stream_block, 0, 16);        
    if (mbedtls_aes_crypt_ctr(&aes_ctx, len, &nc_off, nonce_copy, stream_block, input, output) == 0)      //这里会改变nonce内容，我们需要没有更改过的nonce
    {
        return 0;
    }
    return -1;
}
/**
 * @description: aes-ctr 128解密
 * @param {unsigned char} *nonce_bytes 服务器返回的nonce
 * @param {uint8_t} *input 服务器返回的数据
 * @param {uint16_t} len 服务器返回的数据长度
 * @param {uint8_t} *output 解密数据输出
 * @return 0表示成功 -1表示失败
 */
int decryptAESCTR128(unsigned char *nonce_bytes, uint8_t *input, uint16_t len, uint8_t *output)
{
    size_t nc_off = 0;
    unsigned char stream_block[16];
    if (mbedtls_aes_crypt_ctr(&aes_ctx, len, &nc_off, nonce_bytes, stream_block, input, output) == 0) 
    {
        return 0;
    }
    return -1;
}


/**
 * @description: 将16进制字符串转换成字节数组
 * @param {char} *hex_str 16进制字符串输入
 * @param {unsigned char} *byte_array 字节数组输出
 * @param {size_t} len 字节数组长度
 * @return 无
 */
void hex_string_to_byte_array(const char *hex_str, unsigned char *byte_array, size_t len) 
{
    for (size_t i = 0; i < len; i++) 
    {
        sscanf(hex_str + 2 * i, "%2hhx", &byte_array[i]);
    }
}
/**
 * @description: 将字节数组转换成16进制字符串
 * @param {unsigned char} *byte_array 字节数组输入
 * @param {size_t} len 字节数组长度
 * @param {char} *hex_str 16进制字符串输出
 * @return 无
 */
void byte_array_to_hex_string(const unsigned char *byte_array, size_t len, char *hex_str) 
{
    for (size_t i = 0; i < len; i++) 
    {
        sprintf(hex_str + 2 * i, "%02X", byte_array[i]);
    }
}