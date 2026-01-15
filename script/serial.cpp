#ifndef ARDUINO
#include "serial.hpp"

uSerial::uSerial() : m_fd(-1), m_is_external(false) {}

uSerial::~uSerial()
{
    if (!m_is_external && m_fd >= 0)
    {
        ::close(m_fd);
    }
}

bool uSerial::open(const char *port, unsigned long baudrate)
{
    if (!m_is_external && m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }

    std::string port_str = port ? port : "<null>";

    struct stat st;
    if (stat(port, &st) != 0)
    {
        LOG_ERROR_F("Port '%s' does not exist", port_str.c_str());
        return false;
    }

    m_is_external = false;
    m_fd = ::open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd == -1)
    {
        LOG_ERROR_F("Failed to open port '%s' at %ld baud: %s",
                    port_str.c_str(), baudrate, strerror(errno));
        return false;
    }

    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags != -1)
    {
        fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK);
    }

    LOG_INFO_F("uSerial port '%s' opened successfully at %ld baud",
               port_str.c_str(), baudrate);

    return configureSerial(baudrate);
}

bool uSerial::begin(int fd)
{
    if (!m_is_external && m_fd >= 0)
    {
        ::close(m_fd);
    }
    m_fd = fd;
    m_is_external = true;
    return (m_fd >= 0);
}

bool uSerial::begin(int fd, unsigned long baudrate)
{
    if (!begin(fd))
        return false;
    if (baudrate == 0)
        return true;
    return configureSerial(baudrate);
}

void uSerial::close()
{
    if (!m_is_external && m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }
}

int uSerial::available() const
{
    if (m_fd < 0)
        return 0;

    int bytes_available = 0;
    ioctl(m_fd, FIONREAD, &bytes_available);
    return bytes_available;
}

uint8_t uSerial::read()
{
    if (m_fd < 0)
        return -1;

    uint8_t byte;
    if (::read(m_fd, &byte, 1) == 1)
    {
        return byte;
    }
    return -1;
}

size_t uSerial::read(uint8_t *buffer, size_t length)
{
    if (m_fd < 0 || !buffer || length == 0)
        return 0;
    return ::read(m_fd, buffer, length);
}

size_t uSerial::write(uint8_t byte)
{
    if (m_fd < 0)
        return 0;
    return ::write(m_fd, &byte, 1);
}

size_t uSerial::write(const uint8_t *buffer, size_t length)
{
    if (m_fd < 0 || !buffer || length == 0)
        return 0;
    return ::write(m_fd, buffer, length);
}

void uSerial::flush()
{
    if (m_fd >= 0)
    {
        tcdrain(m_fd);
        tcflush(m_fd, TCIOFLUSH);
    }
}

bool uSerial::poll(int timeout_ms)
{
    if (m_fd < 0)
        return false;

    struct pollfd pfd;
    pfd.fd = m_fd;
    pfd.events = POLLIN;

    int ret = ::poll(&pfd, 1, timeout_ms);
    return (ret > 0 && (pfd.revents & POLLIN));
}

bool uSerial::isOpen() const
{
    return m_fd >= 0;
}

void uSerial::setLowLatency(bool enable)
{
#ifdef __linux__
    if (m_fd >= 0)
    {
        struct serial_struct serial_info;
        if (ioctl(m_fd, TIOCGSERIAL, &serial_info) == 0)
        {
            if (enable)
            {
                serial_info.flags |= ASYNC_LOW_LATENCY;
            }
            else
            {
                serial_info.flags &= ~ASYNC_LOW_LATENCY;
            }
            ioctl(m_fd, TIOCSSERIAL, &serial_info);
        }
    }
#else
    (void)enable;
#endif
}

bool uSerial::configureSerial(unsigned long baudrate)
{
    struct termios options;

    tcgetattr(m_fd, &options);

    cfmakeraw(&options);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    speed_t speed = getBaudRateConstant(baudrate);
    if (speed != B0)
    {
        cfsetispeed(&options, speed);
        cfsetospeed(&options, speed);

        if (tcsetattr(m_fd, TCSANOW, &options) == 0)
        {
            setLowLatency(true);
            return true;
        }
    }

    return setCustomBaudrate(baudrate);
}

speed_t uSerial::getBaudRateConstant(unsigned long baudrate)
{
    switch (baudrate)
    {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
#ifdef B460800
    case 460800:
        return B460800;
#endif
#ifdef B500000
    case 500000:
        return B500000;
#endif
#ifdef B576000
    case 576000:
        return B576000;
#endif
#ifdef B921600
    case 921600:
        return B921600;
#endif
#ifdef B1000000
    case 1000000:
        return B1000000;
#endif
#ifdef B1152000
    case 1152000:
        return B1152000;
#endif
#ifdef B1500000
    case 1500000:
        return B1500000;
#endif
#ifdef B2000000
    case 2000000:
        return B2000000;
#endif
#ifdef B2500000
    case 2500000:
        return B2500000;
#endif
#ifdef B3000000
    case 3000000:
        return B3000000;
#endif
#ifdef B3500000
    case 3500000:
        return B3500000;
#endif
#ifdef B4000000
    case 4000000:
        return B4000000;
#endif
    default:
        return B0;
    }
}

bool uSerial::setCustomBaudrate(unsigned long baudrate)
{
#ifdef __linux__
    return setCustomBaudrateLinux(baudrate);
#elif defined(__APPLE__)
    return setCustomBaudrateMacOS(baudrate);
#else
    return false;
#endif
}

#ifdef __linux__
bool uSerial::setCustomBaudrateLinux(unsigned long baudrate)
{
#if HAS_TERMIOS2
    struct termios2 tio;

    memset(&tio, 0, sizeof(tio));

    if (ioctl(m_fd, TCGETS2, &tio) == 0)
    {
        tio.c_cflag &= ~CBAUD;
        tio.c_cflag |= BOTHER;
        tio.c_ispeed = baudrate;
        tio.c_ospeed = baudrate;

        if (ioctl(m_fd, TCSETS2, &tio) == 0)
        {
            LOG_INFO_F("Custom baudrate %lu set successfully using termios2", baudrate);
            return true;
        }
        else
        {
            LOG_WARN_F("Failed to set custom baudrate %lu with termios2: %s",
                       baudrate, strerror(errno));
        }
    }
    else
    {
        LOG_WARN_F("Failed to get termios2 settings: %s", strerror(errno));
    }
#endif

    unsigned long closest_baud = getClosestStandardBaudrate(baudrate);
    if (closest_baud != baudrate)
    {
        LOG_INFO_F("Custom baudrate %lu not supported, using closest: %lu",
                   baudrate, closest_baud);
    }

    speed_t speed = getBaudRateConstant(closest_baud);
    if (speed != B0)
    {
        struct termios options;
        tcgetattr(m_fd, &options);
        cfmakeraw(&options);
        options.c_cflag |= (CLOCAL | CREAD);
        cfsetispeed(&options, speed);
        cfsetospeed(&options, speed);

        if (tcsetattr(m_fd, TCSANOW, &options) == 0)
        {
            return true;
        }
    }

    LOG_ERROR_F("Failed to set baudrate %lu", baudrate);
    return false;
}

unsigned long uSerial::getClosestStandardBaudrate(unsigned long target)
{
    static const unsigned long standard_rates[] = {
        9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000,
        576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000,
        3000000, 3500000, 4000000};

    unsigned long closest = standard_rates[0];
    unsigned long min_diff = abs((long)(target - closest));

    for (size_t i = 1; i < sizeof(standard_rates) / sizeof(standard_rates[0]); i++)
    {
        unsigned long diff = abs((long)(target - standard_rates[i]));
        if (diff < min_diff)
        {
            min_diff = diff;
            closest = standard_rates[i];
        }
    }

    return closest;
}
#endif // __linux__

#ifdef __APPLE__
bool uSerial::setCustomBaudrateMacOS(unsigned long baudrate)
{
    return ioctl(m_fd, IOSSIOSPEED, &baudrate) == 0;
}
#endif // __APPLE__
#endif // ARDUINO