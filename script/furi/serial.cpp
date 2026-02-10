#ifdef FURI_OS
#include "furi/serial.hpp"
#include "common.h"

uSerial::uSerial()
    : m_handle(nullptr)
    , m_is_acquired(false)
    , m_rx_enabled(false)
    , m_rx_head(0)
    , m_rx_tail(0)
{
    memset(m_rx_buffer, 0, sizeof(m_rx_buffer));
}

uSerial::~uSerial()
{
    close();
}

bool uSerial::open(const char *port, unsigned long baudrate)
{
    if (!port || !*port)
    {
        return begin(1, baudrate);
    }

    char* end_ptr = nullptr;
    long parsed_port = strtol(port, &end_ptr, 10);
    if (end_ptr && *end_ptr == '\0')
    {
        return begin((int)parsed_port, baudrate);
    }

    if (strcasecmp(port, "usart") == 0)
    {
        return begin(FuriHalSerialIdUsart, baudrate);
    }

    if (strcasecmp(port, "lpuart") == 0)
    {
        return begin(FuriHalSerialIdLpuart, baudrate);
    }

    LOG_ERROR_F("Unknown serial port '%s', use 1/2 or usart/lpuart", port);
    return false;
}

bool uSerial::begin(int port, unsigned long baudrate)
{
    FuriHalSerialId serial_id;

    if (port == 1)
    {
        serial_id = FuriHalSerialIdUsart;
    }
    else if (port == 2)
    {
        serial_id = FuriHalSerialIdLpuart;
    }
    else
    {
        LOG_ERROR_F("Invalid serial port %d, expected 1 (USART) or 2 (LPUART)", port);
        return false;
    }

    return begin(serial_id, baudrate);
}

bool uSerial::begin(FuriHalSerialId serial_id, unsigned long baudrate)
{
    if (m_handle || m_rx_enabled)
    {
        close();
    }

    uint32_t effective_baudrate = (baudrate > 0) ? baudrate : FURI_SERIAL_DEFAULT_BAUDRATE;
    
    m_handle = furi_hal_serial_control_acquire(serial_id);
    if (!m_handle)
    {
        LOG_ERROR_F("Failed to acquire serial interface %d", serial_id);
        return false;
    }
    
    m_is_acquired = true;
    
    if (!furi_hal_serial_is_baud_rate_supported(m_handle, effective_baudrate))
    {
        LOG_ERROR_F("Unsupported baudrate %lu", effective_baudrate);
        furi_hal_serial_control_release(m_handle);
        m_handle = nullptr;
        m_is_acquired = false;
        return false;
    }

    // Инициализируем serial с указанной скоростью
    furi_hal_serial_init(m_handle, effective_baudrate);
    
    // Включаем направления TX и RX
    furi_hal_serial_enable_direction(m_handle, FuriHalSerialDirectionTx);
    furi_hal_serial_enable_direction(m_handle, FuriHalSerialDirectionRx);
    
    // Запускаем асинхронный прием через DMA
    furi_hal_serial_dma_rx_start(
        m_handle,
        rx_callback,
        this,
        false // не сообщать об ошибках
    );
    
    m_rx_enabled = true;
    m_rx_head = 0;
    m_rx_tail = 0;
    
    LOG_INFO_F("uSerial opened successfully at %lu baud", effective_baudrate);
    return true;
}

bool uSerial::begin(FuriHalSerialHandle* handle, unsigned long baudrate)
{
    if (m_handle || m_rx_enabled)
    {
        close();
    }
    
    if (!handle)
    {
        LOG_ERROR("Invalid serial handle");
        return false;
    }
    
    m_handle = handle;
    m_is_acquired = false; // handle не был приобретен нами
    
    if (baudrate > 0)
    {
        // Инициализируем serial с указанной скоростью
        furi_hal_serial_init(m_handle, baudrate);
    }
    
    // Включаем направления TX и RX
    furi_hal_serial_enable_direction(m_handle, FuriHalSerialDirectionTx);
    furi_hal_serial_enable_direction(m_handle, FuriHalSerialDirectionRx);
    
    // Запускаем асинхронный прием через DMA
    furi_hal_serial_dma_rx_start(
        m_handle,
        rx_callback,
        this,
        false // не сообщать об ошибках
    );
    
    m_rx_enabled = true;
    m_rx_head = 0;
    m_rx_tail = 0;
    
    LOG_INFO_F("uSerial attached successfully at %lu baud", baudrate);
    return true;
}

void uSerial::close()
{
    if (m_rx_enabled && m_handle)
    {
        furi_hal_serial_dma_rx_stop(m_handle);
        m_rx_enabled = false;
    }
    
    if (m_handle)
    {
        if (m_is_acquired)
        {
            furi_hal_serial_deinit(m_handle);
            furi_hal_serial_control_release(m_handle);
            m_is_acquired = false;
        }
        m_handle = nullptr;
    }
    
    m_rx_head = 0;
    m_rx_tail = 0;
}

int uSerial::available() const
{
    if (!m_handle || !m_rx_enabled)
        return 0;
    
    // Защищаем чтение ring-буфера от гонок с IRQ callback
    FURI_CRITICAL_ENTER();
    
    // Вычисляем количество доступных байт в буфере
    size_t head = m_rx_head;
    size_t tail = m_rx_tail;
    
    FURI_CRITICAL_EXIT();
    
    if (head >= tail)
    {
        return head - tail;
    }
    else
    {
        return FURI_SERIAL_RX_BUFFER_SIZE - tail + head;
    }
}

uint8_t uSerial::read()
{
    if (!m_handle || !m_rx_enabled)
        return (uint8_t)-1;
    
    // Защищаем чтение ring-буфера от гонок с IRQ callback
    FURI_CRITICAL_ENTER();
    
    if (m_rx_head == m_rx_tail)
    {
        FURI_CRITICAL_EXIT();
        return (uint8_t)-1;
    }
    
    uint8_t byte = m_rx_buffer[m_rx_tail];
    m_rx_tail = (m_rx_tail + 1) % FURI_SERIAL_RX_BUFFER_SIZE;
    
    FURI_CRITICAL_EXIT();
    
    return byte;
}

size_t uSerial::read(uint8_t *buffer, size_t length)
{
    if (!buffer || length == 0 || !m_handle || !m_rx_enabled)
        return 0;

    size_t bytes_read = 0;
    FURI_CRITICAL_ENTER();
    while (bytes_read < length && m_rx_head != m_rx_tail)
    {
        buffer[bytes_read++] = m_rx_buffer[m_rx_tail];
        m_rx_tail = (m_rx_tail + 1) % FURI_SERIAL_RX_BUFFER_SIZE;
    }
    FURI_CRITICAL_EXIT();

    return bytes_read;
}

size_t uSerial::write(uint8_t byte)
{
    if (!m_handle)
        return 0;
    
    furi_hal_serial_tx(m_handle, &byte, 1);
    furi_hal_serial_tx_wait_complete(m_handle);
    
    return 1;
}

size_t uSerial::write(const uint8_t *buffer, size_t length)
{
    if (!buffer || length == 0 || !m_handle)
        return 0;
    
    furi_hal_serial_tx(m_handle, buffer, length);
    furi_hal_serial_tx_wait_complete(m_handle);
    
    return length;
}

void uSerial::flush()
{
    if (m_handle)
    {
        furi_hal_serial_tx_wait_complete(m_handle);
    }
    
    // Очищаем буфер приема синхронизированно с IRQ
    FURI_CRITICAL_ENTER();
    m_rx_head = 0;
    m_rx_tail = 0;
    FURI_CRITICAL_EXIT();
}

bool uSerial::poll(int timeout_ms)
{
    if (!m_handle || !m_rx_enabled)
        return false;

    if (timeout_ms == 0)
        return (available() > 0);

    if (timeout_ms < 0)
    {
        while (true)
        {
            if (available() > 0)
                return true;
            furi_delay_ms(1);
        }
    }

    uint32_t start_time = furi_get_tick();
    uint32_t timeout_ticks = timeout_ms;
    
    while (true)
    {
        if (available() > 0)
            return true;
        
        uint32_t current_time = furi_get_tick();
        uint32_t elapsed;
        
        // Обработка переполнения uint32_t
        if (current_time >= start_time)
        {
            elapsed = current_time - start_time;
        }
        else
        {
            elapsed = (UINT32_MAX - start_time) + current_time + 1;
        }
        
        if (elapsed >= timeout_ticks)
            break;
        
        furi_delay_ms(1);
    }
    
    return false;
}

bool uSerial::isOpen() const
{
    return m_handle != nullptr;
}

void uSerial::rx_callback(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, size_t data_len, void* context)
{
    uSerial* self = static_cast<uSerial*>(context);
    if (self && self->m_handle == handle)
    {
        self->on_rx_event(event, data_len);
    }
}

void uSerial::on_rx_event(FuriHalSerialRxEvent event, size_t data_len)
{
    if (data_len > 0)
    {
        // Полностью вычитываем DMA-буфер в рамках этого IRQ события.
        // В HAL callback может прийти по IDLE с data_len > 0 даже без флага Data.
        uint8_t temp_buffer[64];
        while (true)
        {
            size_t bytes_read = furi_hal_serial_dma_rx(m_handle, temp_buffer, sizeof(temp_buffer));
            if (bytes_read == 0)
            {
                break;
            }

            for (size_t i = 0; i < bytes_read; i++)
            {
                size_t next_head = (m_rx_head + 1) % FURI_SERIAL_RX_BUFFER_SIZE;

                // Если буфер полон, вытесняем самые старые байты.
                if (next_head == m_rx_tail)
                {
                    m_rx_tail = (m_rx_tail + 1) % FURI_SERIAL_RX_BUFFER_SIZE;
                }

                m_rx_buffer[m_rx_head] = temp_buffer[i];
                m_rx_head = next_head;
            }
        }
    }

    if (event & (FuriHalSerialRxEventOverrunError |
                 FuriHalSerialRxEventNoiseError |
                 FuriHalSerialRxEventFrameError |
                 FuriHalSerialRxEventParityError))
    {
        LOG_WARN_F("uSerial RX error event=0x%02X", event);
    }
}

#endif // FURI_OS
