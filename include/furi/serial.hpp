#pragma once

#include "stream.hpp"

#ifdef FURI_OS
#include <furi.h>
#include <furi_hal_serial.h>
#include <furi_hal_serial_control.h>
#include <furi_hal_serial_types.h>
#include <furi_hal_cortex.h>

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Буфер для приема данных через DMA
#define FURI_SERIAL_RX_BUFFER_SIZE 256

class uSerial : public uStream
{
public:
    uSerial();
    ~uSerial();

    bool open(const char *port, unsigned long baudrate) override;
    bool begin(FuriHalSerialId serial_id, unsigned long baudrate);
    bool begin(FuriHalSerialHandle* handle, unsigned long baudrate);
    bool attach(FuriHalSerialHandle* handle) { return begin(handle, 0); }
    bool attach(FuriHalSerialHandle* handle, unsigned long baudrate) { return begin(handle, baudrate); }

    void close() override;
    int available() const override;
    uint8_t read() override;
    size_t read(uint8_t *buffer, size_t length) override;
    size_t write(uint8_t byte) override;
    size_t write(const uint8_t *buffer, size_t length) override;
    void flush() override;
    bool poll(int timeout_ms) override;
    bool isOpen() const override;

private:
    FuriHalSerialHandle* m_handle;
    bool m_is_acquired;
    bool m_rx_enabled;
    
    // Буфер для приема данных
    uint8_t m_rx_buffer[FURI_SERIAL_RX_BUFFER_SIZE];
    size_t m_rx_head;
    size_t m_rx_tail;
    
    static void rx_callback(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, size_t data_len, void* context);
    void on_rx_event(FuriHalSerialRxEvent event, size_t data_len);
};

#endif // FURI_OS
