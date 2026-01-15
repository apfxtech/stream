#include "crc.h"

void crc8_init(crc8_ctx_t *ctx, uint8_t poly)
{
    ctx->poly = poly;
    for (uint16_t i = 0; i < CRCLEN; ++i) {
        uint8_t c = (uint8_t)i;
        for (uint8_t j = 0; j < 8; ++j) {
            c = (uint8_t)((c << 1) ^ ((c & 0x80) ? poly : 0));
        }
        ctx->table[i] = c;
    }
}

uint8_t ICACHE_RAM_ATTR crc8_update_byte(const crc8_ctx_t *ctx, uint8_t crc, uint8_t data)
{
    return ctx->table[(uint8_t)(crc ^ data)];
}

uint8_t ICACHE_RAM_ATTR crc8_update(const crc8_ctx_t *ctx, uint8_t crc, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    while (len--) {
        crc = ctx->table[(uint8_t)(crc ^ *p++)];
    }
    return crc;
}

void crc16_init(crc16_ctx_t *ctx, uint16_t poly)
{
    ctx->poly = poly;
    for (uint16_t i = 0; i < CRCLEN; ++i) {
        uint16_t c = (uint16_t)(i << 8);
        for (uint8_t j = 0; j < 8; ++j) {
            c = (uint16_t)((c << 1) ^ ((c & 0x8000u) ? poly : 0));
        }
        ctx->table[i] = c;
    }
}

uint16_t ICACHE_RAM_ATTR crc16_update_byte(const crc16_ctx_t *ctx, uint16_t crc, uint8_t data)
{
    uint8_t idx = (uint8_t)(((crc >> 8) ^ data) & 0xFFu);
    return (uint16_t)((crc << 8) ^ ctx->table[idx]);
}

uint16_t ICACHE_RAM_ATTR crc16_update(const crc16_ctx_t *ctx, uint16_t crc, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    while (len--) {
        uint8_t idx = (uint8_t)(((crc >> 8) ^ *p++) & 0xFFu);
        crc = (uint16_t)((crc << 8) ^ ctx->table[idx]);
    }
    return crc;
}

static crc16_ctx_t g_crc16_ccitt;
static uint8_t g_crc16_ccitt_inited;

static crc8_ctx_t g_crc8_07;
static uint8_t g_crc8_07_inited;

static crc8_ctx_t g_crc8_dvb_s2;
static uint8_t g_crc8_dvb_s2_inited;

static inline void crc16_ccitt_lazy_init(void)
{
    if (!g_crc16_ccitt_inited) {
        crc16_init(&g_crc16_ccitt, 0x1021);
        g_crc16_ccitt_inited = 1;
    }
}

static inline void crc8_07_lazy_init(void)
{
    if (!g_crc8_07_inited) {
        crc8_init(&g_crc8_07, 0x07);
        g_crc8_07_inited = 1;
    }
}

static inline void crc8_dvb_s2_lazy_init(void)
{
    if (!g_crc8_dvb_s2_inited) {
        crc8_init(&g_crc8_dvb_s2, 0xD5);
        g_crc8_dvb_s2_inited = 1;
    }
}

uint16_t ICACHE_RAM_ATTR crc16_ccitt(uint16_t crc, unsigned char a)
{
    crc16_ccitt_lazy_init();
    return crc16_update_byte(&g_crc16_ccitt, crc, (uint8_t)a);
}

uint16_t ICACHE_RAM_ATTR crc16_ccitt_update(uint16_t crc, const void *data, uint32_t length)
{
    crc16_ccitt_lazy_init();
    return crc16_update(&g_crc16_ccitt, crc, data, (size_t)length);
}

void crc16_ccitt_sbu_append(sbu_t *dst, uint8_t *start)
{
    crc16_ccitt_lazy_init();
    uint16_t crc = 0;
    const uint8_t *end = sbu_ptr(dst);
    for (const uint8_t *p = start; p < end; ++p) {
        crc = crc16_update_byte(&g_crc16_ccitt, crc, *p);
    }
    sbu_write_u16le(dst, crc);
}

uint8_t ICACHE_RAM_ATTR crc8(uint8_t crc, uint8_t a)
{
    crc8_07_lazy_init();
    return crc8_update_byte(&g_crc8_07, crc, a);
}

uint8_t ICACHE_RAM_ATTR crc8_update(uint8_t crc, const void *data, uint32_t length)
{
    crc8_07_lazy_init();
    return crc8_update(&g_crc8_07, crc, data, (size_t)length);
}

uint8_t ICACHE_RAM_ATTR crc8_dvb_s2(uint8_t crc, uint8_t a)
{
    crc8_dvb_s2_lazy_init();
    return crc8_update_byte(&g_crc8_dvb_s2, crc, a);
}

uint8_t ICACHE_RAM_ATTR crc8_dvb_s2_update(uint8_t crc, const void *data, uint32_t length)
{
    crc8_dvb_s2_lazy_init();
    return crc8_update(&g_crc8_dvb_s2, crc, data, (size_t)length);
}

void crc8_dvb_s2_sbu_append(sbu_t *dst, uint8_t *start)
{
    crc8_dvb_s2_lazy_init();
    uint8_t crc = 0;
    const uint8_t *end = sbu_ptr(dst);
    for (const uint8_t *p = start; p < end; ++p) {
        crc = crc8_update_byte(&g_crc8_dvb_s2, crc, *p);
    }
    sbu_write_u8(dst, crc);
}

uint8_t ICACHE_RAM_ATTR crc8_xor_update(uint8_t crc, const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    for (uint32_t i = 0; i < length; ++i) {
        crc ^= p[i];
    }
    return crc;
}

void crc8_xor_sbu_append(sbu_t *dst, uint8_t *start)
{
    uint8_t crc = 0;
    const uint8_t *end = sbu_ptr(dst);
    for (const uint8_t *p = start; p < end; ++p) {
        crc ^= *p;
    }
    sbu_write_u8(dst, crc);
}

uint8_t ICACHE_RAM_ATTR crc8_sum_update(uint8_t crc, const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    for (uint32_t i = 0; i < length; ++i) {
        crc = (uint8_t)(crc + p[i]);
    }
    return crc;
}

