/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2025-03-07 14:15:12
 * @LastEditors: Qinze7 2598799271@qq.com
 * @LastEditTime: 2025-03-19 17:19:20
 * @FilePath: \xiaozhi_test\src\private\codec.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __CODEC_H__
#define __CODEC_H__

#include "opus.h"
#include <stdint.h>
#include <arduino.h>

#include "private/audio.h"
#include <mbedtls/aes.h>

int initOpusEncoder(void);
int initOpusDecoder(void);
int destoryOpusEncoder(void);
int destoryOpusDecoder(void);
int encodeOpus(uint8_t *input, uint16_t len, uint8_t *output);
int decodeOpus(uint8_t *input, uint16_t len, uint8_t *output);

void initAES(String key);
void destoryAES(void);
int decryptAESCTR128(unsigned char *nonce_bytes, uint8_t *input, uint16_t len, uint8_t *output);
int encryptAESCTR128(unsigned char *nonce_bytes, uint8_t *input, uint16_t len, uint8_t *output);

void resetOpusSequence(void);
void processNonce(String nonce, uint16_t len, unsigned char *output);
void hex_string_to_byte_array(const char *hex_str, unsigned char *byte_array, size_t len);
void byte_array_to_hex_string(const unsigned char *byte_array, size_t len, char *hex_str);


#endif