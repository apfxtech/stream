#pragma once

#include "stream.hpp"

#ifndef ARDUINO
#include <string>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>

#ifdef __linux__
#include <linux/serial.h>
#if defined(__aarch64__) || defined(__arm__)
struct termios2
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[19];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

#ifndef TCGETS2
#define TCGETS2 0x542a
#endif
#ifndef TCSETS2
#define TCSETS2 0x542b
#endif
#define BOTHER 0x1000
#define HAS_TERMIOS2 1

#elif defined(__x86_64__) || defined(__i386__)
#ifndef _ASM_GENERIC_TERMIOS_H
#include <linux/termios.h>
#include <asm/termbits.h>
#endif

#ifndef TCGETS2
#define TCGETS2 _IOR('T', 0x2A, struct termios2)
#endif
#ifndef TCSETS2
#define TCSETS2 _IOW('T', 0x2B, struct termios2)
#endif
#ifndef BOTHER
#define BOTHER 0010000
#endif
#define HAS_TERMIOS2 1

#elif defined(__riscv)
struct termios2
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t c_line;
    cc_t c_cc[19];
    speed_t c_ispeed;
    speed_t c_ospeed;
};

#define TCGETS2 0x542a
#define TCSETS2 0x542b
#define BOTHER 0x1000
#define HAS_TERMIOS2 1

#else
#define HAS_TERMIOS2 0
#endif

#ifndef CBAUD
#define CBAUD 0010017
#endif
#endif // __linux__

#ifdef __APPLE__
#include <IOKit/serial/ioss.h>
#endif // __APPLE__

class uSerial : public uStream
{
public:
    uSerial();
    ~uSerial();

    bool open(const char *port, unsigned long baudrate) override;
    bool begin(const char *port, unsigned long baudrate) { return open(port, baudrate); }
    bool begin(int fd, unsigned long baudrate); 
    bool begin(int fd);  
    bool attach(int fd) { return begin(fd); }
    bool attach(int fd, unsigned long baudrate) { return begin(fd, baudrate); }

    void close() override;
    int available() const override;
    uint8_t read() override;
    size_t read(uint8_t *buffer, size_t length) override;
    size_t write(uint8_t byte) override;
    size_t write(const uint8_t *buffer, size_t length) override;
    void flush() override;
    bool poll(int timeout_ms) override;
    bool isOpen() const override;

    void setLowLatency(bool enable = true);

private:
    int m_fd;
    bool m_is_external;

    bool configureSerial(unsigned long baudrate);
    speed_t getBaudRateConstant(unsigned long baudrate);
    bool setCustomBaudrate(unsigned long baudrate);

#ifdef __linux__
    bool setCustomBaudrateLinux(unsigned long baudrate);
    unsigned long getClosestStandardBaudrate(unsigned long target);
#endif // __linux__

#ifdef __APPLE__
    bool setCustomBaudrateMacOS(unsigned long baudrate);
#endif // __APPLE__
};
#endif // ARDUINO

#ifdef ARDUINO
// #undef uSerial

class uSerial : public uStream
{
public:
    uSerial() : m_serial(nullptr) {}
    ~uSerial() {}

    template <typename T>
    bool begin(T &external, unsigned long baudrate)
    {
        m_stream = &external;
        m_obj = &external;

        m_ops.begin = [](void *p, unsigned long b)
        {
            static_cast<T *>(p)->begin(b);
        };
        m_ops.end = [](void *p)
        {
            static_cast<T *>(p)->end();
        };

        m_ops.begin(m_obj, baudrate);
        return true;
    }

    bool open(const char *port, unsigned long baudrate) override
    {
        (void)port;
        if (!m_obj || !m_ops.begin)
            return false;
        m_ops.begin(m_obj, baudrate);
        return true;
    }

    void close() override
    {
        if (m_obj && m_ops.end)
            m_ops.end(m_obj);
    }

    int available() const override
    {
        return m_stream ? m_stream->available() : 0;
    }

    uint8_t read() override
    {
        return m_stream ? m_stream->read() : 0;
    }

    size_t read(uint8_t *buffer, size_t length) override
    {
        return m_stream ? m_stream->readBytes(buffer, length) : 0;
    }

    size_t write(uint8_t byte) override
    {
        return m_stream ? m_stream->write(byte) : 0;
    }

    size_t write(const uint8_t *buffer, size_t length) override
    {
        return m_stream ? m_stream->write(buffer, length) : 0;
    }

    void flush() override
    {
        if (m_stream)
            m_stream->flush();
    }

    bool poll(int timeout_ms) override
    {
        if (!m_stream)
            return false;
        unsigned long start = millis();
        while (millis() - start < timeout_ms)
        {
            if (available() > 0)
                return true;
            yield(); // Чтобы не блокировать
        }
        return false;
    }

    bool isOpen() const override
    {
        return m_stream != nullptr;
    }

private:
    ::uStream *m_stream = nullptr; // Arduino uStream (HardwareSerial/SoftwareSerial)
    void *m_obj = nullptr;        // указатель на реальный объект (T*)

    struct Ops
    {
        void (*begin)(void *, unsigned long) = nullptr;
        void (*end)(void *) = nullptr;
    } m_ops;
};

#endif // ARDUINO