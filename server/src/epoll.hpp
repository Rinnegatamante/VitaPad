#ifndef EPOLL_HPP
#define EPOLL_HPP

#include <psp2/net/net.h>
#include <psp2/rtc.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "heartbeat.hpp"
#define FLATBUFFERS_TRACK_VERIFIER_BUFFER_SIZE

#include <netprotocol_generated.h>

constexpr unsigned int MIN_POLLING_INTERVAL_MICROS = (1 * 1000 / 250) * 1000;

class TimeHelper {
public:
  TimeHelper() : last_time() { sceRtcGetCurrentClockLocalTime(&last_time); }

  void update() { sceRtcGetCurrentClockLocalTime(&last_time); }

  uint64_t elapsed_time_secs() const {
    SceDateTime current_time;
    uint64_t current_time_secs;
    uint64_t last_time_secs;

    sceRtcGetCurrentClockLocalTime(&current_time);
    sceRtcConvertDateTimeToTime64_t(&current_time, &current_time_secs);
    sceRtcConvertDateTimeToTime64_t(&last_time, &last_time_secs);

    return current_time_secs - last_time_secs;
  }

  uint64_t elapsed_time_micros() const {
    constexpr uint64_t MICROSECONDS_IN_SECOND = 1E6;

    SceDateTime current_time;
    uint64_t current_time_secs;
    uint64_t last_time_secs;

    sceRtcGetCurrentClockLocalTime(&current_time);
    sceRtcConvertDateTimeToTime64_t(&current_time, &current_time_secs);
    sceRtcConvertDateTimeToTime64_t(&last_time, &last_time_secs);

    uint64_t current_micros = current_time_secs * MICROSECONDS_IN_SECOND +
                              sceRtcGetMicrosecond(&current_time);
    uint64_t last_micros = last_time_secs * MICROSECONDS_IN_SECOND +
                           sceRtcGetMicrosecond(&last_time);

    return current_micros - last_micros;
  }

private:
  SceDateTime last_time;
};

class EpollSocket {
public:
  EpollSocket(int sock_fd, SceUID epoll) : fd_(sock_fd), epoll_(epoll) {}
  ~EpollSocket() {
    sceNetEpollControl(epoll_, SCE_NET_EPOLL_CTL_DEL, fd_, nullptr);
    sceNetSocketClose(fd_);
  }
  int fd() const { return fd_; }

private:
  int fd_;
  SceUID epoll_;
};

class EpollMember;

class ClientDataException : public std::exception {
public:
  ClientDataException(std::string const &msg) : msg_(msg) {}
  char const *what() const noexcept override { return msg_.c_str(); }

private:
  std::string msg_;
};

class ClientData {
public:
  enum class State { WaitingForHandshake, WaitingForServerConfirm, Connected };

  static constexpr size_t MAX_BUFFER_ACCEPTABLE_SIZE = 1 * 1024 * 1024;

  ClientData(int fd, SceUID epoll) : sock_(fd, epoll) {}

  int ctrl_fd() const { return sock_.fd(); }

  State state() const { return state_; }
  void set_state(State state) { state_ = state; }

  /**
   * @brief Returns time in seconds since last heartbeat
   */
  uint64_t time_since_last_heartbeat() const {
    return heartbeat_time_helper_.elapsed_time_secs();
  }
  void update_heartbeat_time() { heartbeat_time_helper_.update(); }

  /**
   * @brief Returns time in microseconds since last sent data
   */
  uint64_t time_since_last_sent_data() const {
    return sent_data_time_helper_.elapsed_time_micros();
  }
  void update_sent_data_time() { sent_data_time_helper_.update(); }
  bool is_polling_time_elapsed() const {
    return time_since_last_sent_data() > polling_time_;
  }

  void add_to_buffer(uint8_t const *data, size_t size) {
    buffer_.insert(buffer_.end(), data, data + size);

    if (buffer_.size() > MAX_BUFFER_ACCEPTABLE_SIZE) {
      buffer_.clear();
      throw ClientDataException("Buffer size exceeded");
    }
  }

  bool handle_data() {
    typedef void (ClientData::*buffer_handler)(const void *);

    flatbuffers::Verifier verifier(buffer_.data(), buffer_.size());

    if (!NetProtocol::VerifySizePrefixedPacketBuffer(verifier))
      return false;

    auto data = NetProtocol::GetSizePrefixedPacket(buffer_.data());

    std::unordered_map<NetProtocol::PacketContent, buffer_handler> handlers = {
        {NetProtocol::PacketContent::Handshake, &ClientData::handle_handshake},
        {NetProtocol::PacketContent::Config, &ClientData::handle_config},
    };

    auto handler_entry = handlers.find(data->content_type());
    if (handler_entry == handlers.end())
      return false;
    auto [_, handler] = *handler_entry;

    std::invoke(handler, this, data->content());

    auto size = verifier.GetComputedSize();
    buffer_.erase(buffer_.begin(), buffer_.begin() + size);
    return true;
  }

  void handle_handshake(const void *buffer) {
    auto handshake = static_cast<NetProtocol::Handshake const *>(buffer);

    SceNetSockaddrIn clientaddr;
    unsigned int addrlen = sizeof(clientaddr);
    sceNetGetpeername(
        ctrl_fd(), reinterpret_cast<SceNetSockaddr *>(&clientaddr), &addrlen);
    clientaddr.sin_port = sceNetHtons(handshake->port());

    auto addr = reinterpret_cast<SceNetSockaddr *>(&clientaddr);
    set_data_conn_info(*addr);
    set_state(ClientData::State::WaitingForServerConfirm);
  }

  void handle_config(const void *buffer) {
    auto config = static_cast<NetProtocol::Config const *>(buffer);

    if (config->polling_interval() > MIN_POLLING_INTERVAL_MICROS)
      polling_time_ = config->polling_interval();
  }

  bool handle_heartbeat() {
    if (buffer_.size() < heartbeat_magic.size() ||
        !std::equal(heartbeat_magic.begin(), heartbeat_magic.end(),
                    buffer_.begin()))
      return false;

    update_heartbeat_time();

    constexpr auto size = heartbeat_magic.size();
    buffer_.erase(buffer_.begin(), buffer_.begin() + size);
    return true;
  }

  void shrink_buffer() { buffer_.shrink_to_fit(); }

  bool to_be_removed() const { return to_be_removed_; }
  void mark_for_removal() { to_be_removed_ = true; }

  SceNetSockaddr data_conn_info() const { return data_conn_info_; }
  void set_data_conn_info(SceNetSockaddr info) { data_conn_info_ = info; }

  std::unique_ptr<EpollMember> &member_ptr() { return member_ptr_; }
  void set_member_ptr(std::unique_ptr<EpollMember> ptr) {
    member_ptr_ = std::move(ptr);
  }

private:
  EpollSocket sock_;
  TimeHelper heartbeat_time_helper_;
  TimeHelper sent_data_time_helper_;
  /**
   * @brief Time in microseconds between polling for data
   */
  uint64_t polling_time_ = MIN_POLLING_INTERVAL_MICROS;

  bool to_be_removed_ = false;
  State state_ = State::WaitingForHandshake;
  std::vector<uint8_t> buffer_;
  SceNetSockaddr data_conn_info_;
  std::unique_ptr<EpollMember> member_ptr_;
};

class ClientsManager {
public:
  ClientsManager() : clients_() {}

  void add_client(std::shared_ptr<ClientData> member) {
    clients_.push_back(member);
  }
  std::vector<std::shared_ptr<ClientData>> &clients() { return clients_; }
  void remove_marked_clients() {
    clients_.erase(std::remove_if(clients_.begin(), clients_.end(),
                                  [](const auto &client) {
                                    return client->to_be_removed();
                                  }),
                   clients_.end());
  }

private:
  std::vector<std::shared_ptr<ClientData>> clients_;
};

enum class SocketType {
  SERVER,
  CLIENT,
};

class EpollMember {
public:
  SocketType type;

  EpollMember() : type(SocketType::SERVER) {}

  EpollMember(const std::shared_ptr<ClientData> &client_ctrl)
      : type(SocketType::CLIENT), data_(client_ctrl) {}

  int fd() const {
    assert(type != SocketType::SERVER);
    return data_.lock()->ctrl_fd();
  }

  std::shared_ptr<ClientData> client() {
    assert(type != SocketType::SERVER);
    return data_.lock();
  }

private:
  std::weak_ptr<ClientData> data_;
};

#endif // EPOLL_HPP
