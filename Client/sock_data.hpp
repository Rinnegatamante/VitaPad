#ifndef SOCK_DATA_HPP__
#define SOCK_DATA_HPP__
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <cstring>

#include <flatbuffers/flatbuffers.h>

#ifdef __linux__
#include <errno.h>
#elif defined(_WIN32)
#include "vigem.h"
#endif

class SocketData
{
private:
    uint8_t size_buf[4];
    std::vector<uint8_t> buffer_;
    uint8_t size_bytes_set;
    bool device_usable;
public:
#ifdef __linux__
    SocketData(int fd) : fd(fd), buffer_(128), size_bytes_set(0){};
    int fd;
#elif defined(_WIN32)
    SocketData(SOCKET sock, VIGEM_TARGET_TYPE type) : buffer_(128), size_bytes_set(0), target(NULL)
    {
        auto target_handler = [this](_client, target, res) {
            assert(VIGEM_SUCESS(res));
            this->device_usable = true;
        };

        switch (type)
        {
        case Xbox360Wired:
            new_x360_target(&target_handler);
            break;

        default:
            new_ds4_target(&target_handler);
            break;
        };
    };
    SOCKET socket;
    PVIGEM_TARGET target;
#endif
    ~SocketData();

    const uint8_t *buffer() const
    {
        return this->buffer_.data();
    };
    void push_buf_data(const uint8_t *, uint32_t);
    void clear()
    {
        this->buffer_.clear();
        std::memset(this->size_buf, 0, 4);
        this->size_bytes_set = 0;
    };

    void push_size_data(const uint8_t *, uint32_t);
    const uint32_t size() const { return flatbuffers::ReadScalar<uint32_t>(this->size_buf); }
    bool size_fetched() const { return this->size_bytes_set == 4; };
    const uint32_t remaining() const
    {
        assert(this->size_fetched());
        return this->size() - this->buffer_.size();
    };
    const size_t buffer_size() const { return this->buffer_.size(); };
};

#endif