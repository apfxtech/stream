/*
 * crc.h 
 * Cyclic Redundancy Check
 *
 * The MIT License (MIT)
 * 
 * Copyright (c) 2024 ApertureFox Technology
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

// https://en.wikipedia.org/wiki/Cyclic_redundancy_check

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "sbu.h"

#ifndef CRCLEN
#define CRCLEN 256
#endif

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t table[CRCLEN];
    uint8_t poly;
} crc8_ctx_t;

typedef struct {
    uint16_t table[CRCLEN];
    uint16_t poly;
} crc16_ctx_t;

void crc8_init(crc8_ctx_t *ctx, uint8_t poly);
uint8_t ICACHE_RAM_ATTR crc8_update_byte(const crc8_ctx_t *ctx, uint8_t crc, uint8_t data);
uint8_t ICACHE_RAM_ATTR crc8_update(const crc8_ctx_t *ctx, uint8_t crc, const void *data, size_t len);

void crc16_init(crc16_ctx_t *ctx, uint16_t poly);
uint16_t ICACHE_RAM_ATTR crc16_update_byte(const crc16_ctx_t *ctx, uint16_t crc, uint8_t data);
uint16_t ICACHE_RAM_ATTR crc16_update(const crc16_ctx_t *ctx, uint16_t crc, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

// #ifdef __cplusplus

// class CRC8 {
//     uint8_t table_[CRCLEN];
//     uint8_t poly_;
// public:
//     explicit CRC8(uint8_t poly) : poly_(poly) {
//         for (uint16_t i = 0; i < CRCLEN; ++i) {
//             uint8_t c = (uint8_t)i;
//             for (uint8_t j = 0; j < 8; ++j) {
//                 c = (uint8_t)((c << 1) ^ ((c & 0x80) ? poly_ : 0));
//             }
//             table_[i] = c;
//         }
//     }

//     uint8_t ICACHE_RAM_ATTR update_byte(uint8_t crc, uint8_t data) const {
//         return table_[(uint8_t)(crc ^ data)];
//     }

//     uint8_t ICACHE_RAM_ATTR update(uint8_t crc, const void *data, size_t len) const {
//         const uint8_t *p = (const uint8_t *)data;
//         while (len--) crc = table_[(uint8_t)(crc ^ *p++)];
//         return crc;
//     }

//     uint8_t poly() const { return poly_; }
// };

// class CRC16 {
//     uint16_t table_[CRCLEN];
//     uint16_t poly_;
// public:
//     explicit CRC16(uint16_t poly) : poly_(poly) {
//         for (uint16_t i = 0; i < CRCLEN; ++i) {
//             uint16_t c = (uint16_t)(i << 8);
//             for (uint8_t j = 0; j < 8; ++j) {
//                 c = (uint16_t)((c << 1) ^ ((c & 0x8000u) ? poly_ : 0));
//             }
//             table_[i] = c;
//         }
//     }

//     uint16_t ICACHE_RAM_ATTR update_byte(uint16_t crc, uint8_t data) const {
//         uint8_t idx = (uint8_t)(((crc >> 8) ^ data) & 0xFFu);
//         return (uint16_t)((crc << 8) ^ table_[idx]);
//     }

//     uint16_t ICACHE_RAM_ATTR update(uint16_t crc, const void *data, size_t len) const {
//         const uint8_t *p = (const uint8_t *)data;
//         while (len--) {
//             uint8_t idx = (uint8_t)(((crc >> 8) ^ *p++) & 0xFFu);
//             crc = (uint16_t)((crc << 8) ^ table_[idx]);
//         }
//         return crc;
//     }

//     uint16_t poly() const { return poly_; }
// };

// #endif // __cplusplus