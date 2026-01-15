#include "socket.hpp"

#ifndef ARDUINO
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <poll.h>

static std::string base64_encode(const std::string& input) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string output;
    int val = 0, valb = -6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (output.size() % 4) {
        output.push_back('=');
    }
    
    return output;
}

WebSocket::WebSocket() 
    : m_fd(-1), m_is_external(false), m_connected(false), m_reader_stop(false) {
    m_recv_queue.reserve(1024);
}

WebSocket::~WebSocket() {
    close();
}

bool WebSocket::open(const char* url, unsigned long baudrate) {
    close();

    if (!url) {
        LOG_ERROR("WebSocket URL is null");
        return false;
    }

    std::string host, path;
    int port = 80;
    
    if (!parseWebSocketURI(url, host, port, path)) {
        LOG_ERROR_F("Failed to parse WebSocket URL: %s", url);
        return false;
    }

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0) {
        LOG_ERROR_F("Failed to create socket: %s", strerror(errno));
        return false;
    }

    // Настройка таймаута
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(m_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(host.c_str());
        if (!he) { 
            LOG_ERROR_F("Failed to resolve host: %s", host.c_str());
            ::close(m_fd);
            m_fd = -1;
            return false; 
        }
        memcpy(&serv_addr.sin_addr, he->h_addr, he->h_length);
    }
    
    if (connect(m_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        LOG_ERROR_F("Failed to connect to %s:%d: %s", host.c_str(), port, strerror(errno));
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    if (!performWebSocketHandshake(host, path)) {
        LOG_ERROR("WebSocket handshake failed");
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    m_is_external = false;
    m_connected = true;
    m_reader_stop = false;
    
    m_reader_thread = std::thread(&WebSocket::readerThread, this);
    
    LOG_INFO_F("WebSocket connected to %s", url);
    return true;
}

bool WebSocket::beginExternal(void* external) {
    int fd = *(int*)external;
    if (!m_is_external && m_fd >= 0) {
        ::close(m_fd);
    }
    
    m_fd = fd;
    m_is_external = true;
    m_connected = (m_fd >= 0);
    return m_connected;
}

void WebSocket::close() {
    m_connected = false;
    m_reader_stop = true;
    
    if (m_reader_thread.joinable()) {
        m_reader_thread.join();
    }
    
    if (!m_is_external && m_fd >= 0) {
        sendWebSocketCloseFrame();
        ::close(m_fd);
        m_fd = -1;
    }
    
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_recv_queue.clear();
    }
}

int WebSocket::available() const {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    return static_cast<int>(m_recv_queue.size());
}

uint8_t WebSocket::read() {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (m_recv_queue.empty()) {
        return static_cast<uint8_t>(-1);
    }
    uint8_t byte = m_recv_queue.front();
    m_recv_queue.erase(m_recv_queue.begin());
    return byte;
}

size_t WebSocket::read(uint8_t* buffer, size_t length) {
    if (!buffer || length == 0) return 0;
    
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    size_t bytes_to_read = std::min(length, m_recv_queue.size());
    
    for (size_t i = 0; i < bytes_to_read; ++i) {
        buffer[i] = m_recv_queue[i];
    }
    
    m_recv_queue.erase(m_recv_queue.begin(), m_recv_queue.begin() + bytes_to_read);
    return bytes_to_read;
}

size_t WebSocket::write(uint8_t byte) {
    return write(&byte, 1);
}

size_t WebSocket::write(const uint8_t* buffer, size_t length) {
    if (!m_connected || m_fd < 0 || !buffer || length == 0) {
        return 0;
    }

    std::vector<uint8_t> frame;
    buildWebSocketFrame(buffer, length, frame, true);
    
    ssize_t bytes_sent = ::send(m_fd, frame.data(), frame.size(), MSG_NOSIGNAL);
    if (bytes_sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            m_connected = false;
        }
        return 0;
    }
    
    return length;
}

void WebSocket::flush() {
    // Для WebSocket flush обычно не требуется
    if (m_fd >= 0) {
        // fsync не работает с сокетами, используем tcdrain для последовательных портов
        // Для WebSocket просто делаем пустую операцию
    }
}

bool WebSocket::poll(int timeout_ms) {
    if (m_fd < 0) return false;
    
    struct pollfd pfd;
    pfd.fd = m_fd;
    pfd.events = POLLIN;
    
    int ret = ::poll(&pfd, 1, timeout_ms);
    return (ret > 0 && (pfd.revents & POLLIN));
}

bool WebSocket::isOpen() const {
    return m_connected && m_fd >= 0;
}

bool WebSocket::parseWebSocketURI(const std::string& uri, std::string& host, int& port, std::string& path) {
    if (uri.compare(0, 5, "ws://") == 0) {
        port = 80;
    } else {
        return false; // Поддерживаем только ws:// для простоты
    }
    
    size_t host_start = 5;
    size_t path_start = uri.find('/', host_start);
    size_t colon = uri.find(':', host_start);
    
    if (colon != std::string::npos && (path_start == std::string::npos || colon < path_start)) {
        host = uri.substr(host_start, colon - host_start);
        size_t port_end = (path_start != std::string::npos) ? path_start : uri.length();
        try {
            port = std::stoi(uri.substr(colon + 1, port_end - colon - 1));
        } catch (const std::exception&) {
            return false;
        }
    } else {
        size_t host_end = (path_start != std::string::npos) ? path_start : uri.length();
        host = uri.substr(host_start, host_end - host_start);
    }
    
    path = (path_start != std::string::npos) ? uri.substr(path_start) : "/";
    
    return !host.empty();
}

bool WebSocket::performWebSocketHandshake(const std::string& host, const std::string& path) {
    std::string key = generateWebSocketKey();
    std::string request = 
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + key + "\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "User-Agent: WebSocketStream/1.0\r\n\r\n";

    if (::send(m_fd, request.data(), request.size(), 0) != static_cast<ssize_t>(request.size())) {
        return false;
    }

    char buffer[4096];
    int bytes_received = ::recv(m_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        return false;
    }
    
    buffer[bytes_received] = '\0';
    std::string response(buffer);
    
    // Простая проверка ответа
    return response.find("HTTP/1.1 101") != std::string::npos &&
           response.find("Upgrade: websocket") != std::string::npos;
}

std::string WebSocket::generateWebSocketKey() {
    // Фиксированный ключ как в uStream
    return "dGhlIHNhbXBsZSBub25jZQ==";
}

void WebSocket::buildWebSocketFrame(const uint8_t* data, size_t len, std::vector<uint8_t>& frame, bool binary) {
    frame.clear();
    frame.reserve(len + 14);
    
    // FIN + opcode
    frame.push_back(binary ? 0x82 : 0x81);
    
    // Payload length
    if (len < 126) {
        frame.push_back(0x80 | static_cast<uint8_t>(len));
    } else if (len < 65536) {
        frame.push_back(0x80 | 126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(0x80 | 127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((len >> (8*i)) & 0xFF);
        }
    }
    
    // Mask (фиксированная маска как в uStream)
    uint8_t mask[4] = {0x12, 0x34, 0x56, 0x78};
    frame.insert(frame.end(), mask, mask + 4);
    
    // Masked payload
    for (size_t i = 0; i < len; ++i) {
        frame.push_back(data[i] ^ mask[i % 4]);
    }
}

void WebSocket::sendWebSocketCloseFrame() {
    if (m_fd >= 0) {
        uint8_t close_frame[] = {0x88, 0x80, 0x12, 0x34, 0x56, 0x78};
        ::send(m_fd, close_frame, sizeof(close_frame), MSG_NOSIGNAL);
    }
}

void WebSocket::readerThread() {
    uint8_t buffer[4096];
    std::vector<uint8_t> incomplete_frame;
    
    while (!m_reader_stop && m_connected) {
        if (m_fd < 0) break;
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(m_fd, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; // 10ms
        
        int select_ret = select(m_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (select_ret > 0 && FD_ISSET(m_fd, &read_fds)) {
            ssize_t bytes_received = ::recv(m_fd, buffer, sizeof(buffer), MSG_DONTWAIT);
            if (bytes_received > 0) {
                processWebSocketData(buffer, static_cast<size_t>(bytes_received));
            } else if (bytes_received == 0) {
                // Соединение закрыто
                m_connected = false;
                break;
            } else {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    m_connected = false;
                    break;
                }
            }
        }
    }
}

void WebSocket::processWebSocketData(const uint8_t* data, size_t len) {
    static std::vector<uint8_t> incomplete_frame;
    
    std::vector<uint8_t> combined_data;
    if (!incomplete_frame.empty()) {
        combined_data = incomplete_frame;
        combined_data.insert(combined_data.end(), data, data + len);
        incomplete_frame.clear();
    } else {
        combined_data.assign(data, data + len);
    }
    
    size_t offset = 0;
    while (offset < combined_data.size()) {
        auto result = parseWebSocketFrame(combined_data.data() + offset, combined_data.size() - offset);
        if (result.complete) {
            if (result.opcode == 1 || result.opcode == 2) { // Text or binary
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                m_recv_queue.insert(m_recv_queue.end(), result.payload.begin(), result.payload.end());
                
                // Ограничение размера буфера как в uStream
                if (m_recv_queue.size() > 8192) {
                    m_recv_queue.erase(m_recv_queue.begin(), m_recv_queue.begin() + 1024);
                }
            } else if (result.opcode == 8) { // Close frame
                m_connected = false;
                break;
            }
            offset += result.frame_length;
        } else {
            // Неполный фрейм - сохраняем для следующего раза
            incomplete_frame.assign(combined_data.begin() + offset, combined_data.end());
            break;
        }
    }
}

WebSocket::WebSocketFrameResult WebSocket::parseWebSocketFrame(const uint8_t* data, size_t len) {
    WebSocketFrameResult result;
    
    if (len < 2) return result;
    
    uint8_t fin_opcode = data[0];
    uint8_t len_byte = data[1];
    
    result.opcode = fin_opcode & 0x0F;
    
    size_t payload_len = len_byte & 0x7F;
    bool masked = len_byte & 0x80;
    
    size_t header_len = 2;
    
    if (payload_len == 126) {
        if (len < header_len + 2) return result;
        payload_len = (data[header_len] << 8) | data[header_len + 1];
        header_len += 2;
    } else if (payload_len == 127) {
        if (len < header_len + 8) return result;
        payload_len = 0;
        for (int i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | data[header_len + i];
        }
        header_len += 8;
    }
    
    uint8_t mask[4] = {0, 0, 0, 0};
    if (masked) {
        if (len < header_len + 4) return result;
        memcpy(mask, data + header_len, 4);
        header_len += 4;
    }
    
    if (len < header_len + payload_len) return result;
    
    result.payload.resize(payload_len);
    for (size_t i = 0; i < payload_len; ++i) {
        result.payload[i] = masked ? (data[header_len + i] ^ mask[i % 4]) : data[header_len + i];
    }
    
    result.complete = true;
    result.frame_length = header_len + payload_len;
    
    return result;
}

#endif // ARDUINO