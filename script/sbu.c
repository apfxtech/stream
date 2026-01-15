/*
 * streambuf.с - stream buffer utils (sbu)
 *
 * MIT License
 * Copyright (c) 2026 ApertureFox Technology
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

#include "sbu.h"

sbu_t *sbu_init(sbu_t *sbu, uint8_t *ptr, uint8_t *end)
{
    sbu->ptr = ptr;
    sbu->end = end;
    return sbu;
}

uint8_t *sbu_ptr(sbu_t *buf)
{
    return buf->ptr;
}

const uint8_t *sbu_const_ptr(const sbu_t *buf)
{
    return buf->ptr;
}

int sbu_size(sbu_t *buf)
{
    if (!buf || !buf->ptr || !buf->end) return 0;
    if (buf->end < buf->ptr) return 0;
    return (int)(buf->end - buf->ptr);
}

int sbu_left(sbu_t *buf)
{
    return sbu_size(buf);
}

void sbu_skip(sbu_t *buf, int size)
{
    if (!buf || size <= 0) return;
    buf->ptr += size;
}

void sbu_fill(sbu_t *dst, uint8_t data, int len)
{
    if (!dst || len <= 0) return;
    memset(dst->ptr, data, (size_t)len);
    dst->ptr += len;
}

void sbu_switch_to_reader(sbu_t *buf, uint8_t *base)
{
    if (!buf) return;
    buf->end = buf->ptr;
    buf->ptr = base;
}

void sbu_write_data(sbu_t *dst, const void *data, int len)
{
    if (!dst || !data || len <= 0) return;
    memcpy(dst->ptr, data, (size_t)len);
    dst->ptr += len;
}

bool sbu_write_data_safe(sbu_t *dst, const void *data, int len)
{
    if (!dst || !data || len <= 0) return false;
    if (sbu_left(dst) < len) return false;
    memcpy(dst->ptr, data, (size_t)len);
    dst->ptr += len;
    return true;
}

void sbu_write_u8(sbu_t *dst, uint8_t val)
{
    *dst->ptr++ = val;
}

void sbu_write_i8(sbu_t *dst, int8_t val)
{
    *dst->ptr++ = (uint8_t)val;
}

void sbu_write_u16le(sbu_t *dst, uint16_t val)
{
    *dst->ptr++ = (uint8_t)(val >> 0);
    *dst->ptr++ = (uint8_t)(val >> 8);
}

void sbu_write_u16be(sbu_t *dst, uint16_t val)
{
    *dst->ptr++ = (uint8_t)(val >> 8);
    *dst->ptr++ = (uint8_t)(val >> 0);
}

void sbu_write_u32le(sbu_t *dst, uint32_t val)
{
    *dst->ptr++ = (uint8_t)(val >> 0);
    *dst->ptr++ = (uint8_t)(val >> 8);
    *dst->ptr++ = (uint8_t)(val >> 16);
    *dst->ptr++ = (uint8_t)(val >> 24);
}

void sbu_write_u32be(sbu_t *dst, uint32_t val)
{
    *dst->ptr++ = (uint8_t)(val >> 24);
    *dst->ptr++ = (uint8_t)(val >> 16);
    *dst->ptr++ = (uint8_t)(val >> 8);
    *dst->ptr++ = (uint8_t)(val >> 0);
}

void sbu_write_string(sbu_t *dst, const char *string)
{
    sbu_write_data(dst, string, (int)strlen(string));
}

void sbu_write_string_pscl(sbu_t *dst, const char *string)
{
    int length = (int)strlen(string);
    length = MIN(length, 255);
    sbu_write_u8(dst, (uint8_t)length);
    sbu_write_data(dst, string, length);
}

void sbu_write_string_zero(sbu_t *dst, const char *string)
{
    sbu_write_data(dst, string, (int)strlen(string) + 1);
}

void sbu_read_data(sbu_t *src, void *data, int len)
{
    if (!src || len <= 0) return;
    if (data) memcpy(data, src->ptr, (size_t)len);
    src->ptr += len;
}

uint8_t sbu_read_u8(sbu_t *src)
{
    return *src->ptr++;
}

int8_t sbu_read_i8(sbu_t *src)
{
    return (int8_t)(*src->ptr++);
}

uint16_t sbu_read_u16le(sbu_t *src)
{
    uint16_t b0 = (uint16_t)sbu_read_u8(src);
    uint16_t b1 = (uint16_t)sbu_read_u8(src);
    return (uint16_t)(b0 | (uint16_t)(b1 << 8));
}

uint16_t sbu_read_u16be(sbu_t *src)
{
    uint16_t b0 = (uint16_t)sbu_read_u8(src);
    uint16_t b1 = (uint16_t)sbu_read_u8(src);
    return (uint16_t)((uint16_t)(b0 << 8) | b1);
}

int16_t sbu_read_i16le(sbu_t *src)
{
    return (int16_t)sbu_read_u16le(src);
}

int16_t sbu_read_i16be(sbu_t *src)
{
    return (int16_t)sbu_read_u16be(src);
}

uint32_t sbu_read_u32le(sbu_t *src)
{
    uint32_t b0 = (uint32_t)sbu_read_u8(src);
    uint32_t b1 = (uint32_t)sbu_read_u8(src);
    uint32_t b2 = (uint32_t)sbu_read_u8(src);
    uint32_t b3 = (uint32_t)sbu_read_u8(src);
    return (uint32_t)(b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
}

uint32_t sbu_read_u32be(sbu_t *src)
{
    uint32_t b0 = (uint32_t)sbu_read_u8(src);
    uint32_t b1 = (uint32_t)sbu_read_u8(src);
    uint32_t b2 = (uint32_t)sbu_read_u8(src);
    uint32_t b3 = (uint32_t)sbu_read_u8(src);
    return (uint32_t)((b0 << 24) | (b1 << 16) | (b2 << 8) | b3);
}

int32_t sbu_read_i32le(sbu_t *src)
{
    return (int32_t)sbu_read_u32le(src);
}

int32_t sbu_read_i32be(sbu_t *src)
{
    return (int32_t)sbu_read_u32be(src);
}

bool sbu_read_data_safe(sbu_t *src, void *data, int len)
{
    if (!src || len <= 0) return false;
    if (sbu_left(src) < len) return false;
    sbu_read_data(src, data, len);
    return true;
}

bool sbu_read_u8_safe(uint8_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 1) return false;
    {
        uint8_t v = sbu_read_u8(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_i8_safe(int8_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 1) return false;
    {
        int8_t v = sbu_read_i8(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_u16le_safe(uint16_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 2) return false;
    {
        uint16_t v = sbu_read_u16le(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_i16le_safe(int16_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 2) return false;
    {
        int16_t v = sbu_read_i16le(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_u16be_safe(uint16_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 2) return false;
    {
        uint16_t v = sbu_read_u16be(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_i16be_safe(int16_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 2) return false;
    {
        int16_t v = sbu_read_i16be(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_u32le_safe(uint32_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 4) return false;
    {
        uint32_t v = sbu_read_u32le(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_i32le_safe(int32_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 4) return false;
    {
        int32_t v = sbu_read_i32le(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_u32be_safe(uint32_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 4) return false;
    {
        uint32_t v = sbu_read_u32be(src);
        if (val) *val = v;
    }
    return true;
}

bool sbu_read_i32be_safe(int32_t *val, sbu_t *src)
{
    if (!src) return false;
    if (sbu_left(src) < 4) return false;
    {
        int32_t v = sbu_read_i32be(src);
        if (val) *val = v;
    }
    return true;
}
