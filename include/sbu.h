/*
 * sbu.h 
 * Stream Buffer Utils
 *
 * The MIT License (MIT)
 * 
 * Copyright (c) 2025 ApertureFox Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

 #pragma once

// sbu - stream buffer utils
// LE (Little-Endian)
// BE (Big-Endian)

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct sbu_s {
    uint8_t *ptr;
    uint8_t *end;
} sbu_t;

#ifdef __cplusplus 
extern "C" { 
#endif

// init
sbu_t *sbu_init(sbu_t *sbu, uint8_t *ptr, uint8_t *end);

// low-level access
uint8_t *sbu_ptr(sbu_t *buf);
const uint8_t *sbu_const_ptr(const sbu_t *buf);

// utils
int  sbu_size(sbu_t *buf);                        // полный
int  sbu_left(sbu_t *buf);                        // осталось 
void sbu_skip(sbu_t *buf, int size);              // сдвинуть
void sbu_fill(sbu_t *dst, uint8_t data, int len); // заполнить 
void sbu_switch_to_reader(sbu_t *buf, uint8_t *base);

// sbu_write basic
void sbu_write_data      (sbu_t *dst, const void *data, int len);
bool sbu_write_data_safe (sbu_t *dst, const void *data, int len);
void sbu_write_u8        (sbu_t *dst, uint8_t val);
void sbu_write_i8        (sbu_t *dst, int8_t val);

// sbu_write advanced
void sbu_write_u16le(sbu_t *dst, uint16_t val);
void sbu_write_u16be(sbu_t *dst, uint16_t val);
// void sbu_write_u24le(sbu_t *dst, uint32_t val); feature
// void sbu_write_u24be(sbu_t *dst, uint32_t val); feature
void sbu_write_u32le(sbu_t *dst, uint32_t val);
void sbu_write_u32be(sbu_t *dst, uint32_t val);

// sbu_write_string
void sbu_write_string     (sbu_t *dst, const char *string);
void sbu_write_string_pscl(sbu_t *dst, const char *string);
void sbu_write_string_zero(sbu_t *dst, const char *string);

// sbu_read basic
void sbu_read_data (sbu_t *src, void *data, int len);
uint8_t sbu_read_u8(sbu_t *src);
int8_t  sbu_read_i8(sbu_t *src);

// sbu_read advanced
uint16_t sbu_read_u16le(sbu_t *src);
uint16_t sbu_read_u16be(sbu_t *src);
int16_t sbu_read_i16le(sbu_t *src);
int16_t sbu_read_i16be(sbu_t *src);
// uint24_t sbu_read_u24le(sbu_t *src); feature
// uint24_t sbu_read_u24be(sbu_t *src); feature
// int24_t sbu_read_i24le(sbu_t *src); feature
// int24_t sbu_read_i24be(sbu_t *src); feature
uint32_t sbu_read_u32le(sbu_t *src);
uint32_t sbu_read_u32be(sbu_t *src);
int32_t sbu_read_i32le(sbu_t *src);
int32_t sbu_read_i32be(sbu_t *src);

// sbu_read_*_save basic
bool sbu_read_data_safe (sbu_t *src, void *data, int len);
bool sbu_read_u8_safe   (uint8_t *val, sbu_t *src);
bool sbu_read_i8_safe   (int8_t *val, sbu_t *src);

// sbu_read_*_save advanced
bool sbu_read_u16le_safe(uint16_t *val, sbu_t *src);
bool sbu_read_i16le_safe(int16_t *val, sbu_t *src);
bool sbu_read_u16be_safe(uint16_t *val, sbu_t *src);
bool sbu_read_i16be_safe(int16_t *val, sbu_t *src);
// bool sbu_read_u24le_safe(uint16_t *val, sbu_t *src); feature
// bool sbu_read_i24le_safe(int32_t *val, sbu_t *src); feature
// bool sbu_read_u24be_safe(int32_t *val, sbu_t *src); feature
// bool sbu_read_i24be_safe(int32_t *val, sbu_t *src); feature
bool sbu_read_u32le_safe(uint32_t *val, sbu_t *src);
bool sbu_read_i32le_safe(int32_t *val, sbu_t *src);
bool sbu_read_u32be_safe(uint32_t *val, sbu_t *src);
bool sbu_read_i32be_safe(int32_t *val, sbu_t *src);

#ifdef __cplusplus
}
#endif
