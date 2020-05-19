#include "sock_data.hpp"

void SocketData::push_buf_data(const uint8_t *ptr, uint32_t ptr_size)
{
    assert(this->size_fetched());
    auto size = (this->buffer_.size() + ptr_size <= this->size()) ? ptr_size : this->remaining();
    this->buffer_.insert(this->buffer_.end(), ptr, ptr + size);
}

void SocketData::push_size_data(const uint8_t *ptr, uint32_t ptr_size)
{
    auto size = (this->size_bytes_set + ptr_size <= 4) ? ptr_size : 4 - this->size_bytes_set;
    std::memcpy(this->size_buf, ptr, ptr_size);
    this->size_bytes_set += size;
}

SocketData::~SocketData()
{
    delete[] size_buf;

#ifdef _WIN32
    if (target != NULL && vigem_target_is_attached(target))
        vigem_target_remove(target);
#endif
}