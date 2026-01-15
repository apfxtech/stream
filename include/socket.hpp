#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include "stream.hpp"

class WebSocket : public uStream {
public:
    WebSocket();
    ~WebSocket();
    
    bool open(const char* url, unsigned long baudrate = 0) override;
    void close() override;
    int available() const override;
    uint8_t read() override;
    size_t read(uint8_t* buffer, size_t length) override;
    size_t write(uint8_t byte) override;
    size_t write(const uint8_t* buffer, size_t length) override;
    void flush() override;
    bool poll(int timeout_ms) override;
    bool isOpen() const override;

private:
    int m_fd;
    bool m_is_external;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_reader_stop;
    std::thread m_reader_thread;
    
    mutable std::mutex m_queue_mutex;
    std::vector<uint8_t> m_recv_queue;
    
    bool parseWebSocketURI(const std::string& uri, std::string& host, int& port, std::string& path);
    bool performWebSocketHandshake(const std::string& host, const std::string& path);
    std::string generateWebSocketKey();
    void buildWebSocketFrame(const uint8_t* data, size_t len, std::vector<uint8_t>& frame, bool binary = true);
    void sendWebSocketCloseFrame();
    void processWebSocketData(const uint8_t* data, size_t len);
    void readerThread();
    
    struct WebSocketFrameResult {
        bool complete = false;
        uint8_t opcode = 0;
        std::vector<uint8_t> payload;
        size_t frame_length = 0;
    };
    
    WebSocketFrameResult parseWebSocketFrame(const uint8_t* data, size_t len);
};