#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <algorithm>
#include <memory>
#include "common.h"
#include "sbu.h"

class uStream
{
private:
    bool m_is_writable = true;

public:
    uStream() = default;
    virtual ~uStream() = default;

    // Запрет копирования и присваивания
    uStream(const uStream &) = delete;
    uStream &operator=(const uStream &) = delete;

    uStream(uStream &&) = default;
    uStream &operator=(uStream &&) = default;

    virtual bool open(const char *port, unsigned long baudrate) = 0;
    virtual void close() = 0;
    virtual int available() const = 0;
    virtual uint8_t read() = 0;
    virtual size_t read(uint8_t *buffer, size_t length) = 0;
    virtual size_t write(uint8_t byte) = 0;
    virtual size_t write(const uint8_t *buffer, size_t length) = 0;

    inline bool readBytes(uint8_t *buffer, size_t length, uint32_t timeout_ms = 1000)
    {
        if (!buffer)
            return false;

        size_t index = 0;
        uint32_t start_time = millis();
        while (index < length)
        {
            if (available() > 0)
            {
                buffer[index++] = read();
            }
            else
            {
                if (millis() - start_time > timeout_ms)
                    break;
                usleep(100);
            }
        }
        return index == length;
    }

    inline bool readBuf(sbu_t *dst)
    {
        if (!dst || !dst->ptr || dst->ptr >= dst->end)
            return false;

        int bytes_available = available();
        if (bytes_available <= 0)
            return false;

        size_t remaining = sbu_left(dst);
        if (remaining == 0)
            return false;

        size_t to_read = std::min(static_cast<size_t>(bytes_available), remaining);
        size_t bytes_read = read(dst->ptr, to_read);

        if (bytes_read > 0)
        {
            sbu_skip(dst, bytes_read);
            return true;
        }
        return false;
    }

    inline bool writeBuf(sbu_t *src)
    {
        if (!src || !src->ptr || src->ptr >= src->end)
            return false;

        size_t bytes_to_write = sbu_left(src);
        if (bytes_to_write == 0)
            return false;

        size_t bytes_written = write(src->ptr, bytes_to_write);
        if (bytes_written > 0)
        {
            sbu_skip(src, bytes_written);
            return bytes_written == bytes_to_write;
        }
        return false;
    }

    inline size_t writeString(const char *str)
    {
        if (!str)
            return 0;
        return write(reinterpret_cast<const uint8_t *>(str), strlen(str));
    }

    inline size_t println(const char *str = "")
    {
        size_t written = writeString(str);
        written += write('\r');
        written += write('\n');
        return written;
    }

    virtual void flush() = 0;
    virtual bool poll(int timeout_ms = 0) = 0;
    virtual bool isOpen() const = 0;

    virtual bool isReadable() const
    {
        return isOpen() && (available() > 0);
    }

    virtual bool isWritable() const
    {
        return isOpen() && m_is_writable;
    }
};