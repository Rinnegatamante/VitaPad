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
#include <handshake_generated.h>

class TimeHelper {
public:
  TimeHelper() : last_time_() {
    SceDateTime current_time;
    sceRtcGetCurrentClockLocalTime(&current_time);
    sceRtcConvertDateTimeToTime64_t(&current_time, &last_time_);
  }

  uint64_t last_time() const { return last_time_; }

  void update() {
    SceDateTime current_time;
    sceRtcGetCurrentClockLocalTime(&current_time);
    sceRtcConvertDateTimeToTime64_t(&current_time, &last_time_);
  }

  uint64_t elapsed_time_secs() const {
    SceDateTime current_time;
    uint64_t current_time64;
    sceRtcGetCurrentClockLocalTime(&current_time);
    sceRtcConvertDateTimeToTime64_t(&current_time, &current_time64);

    return current_time64 - last_time_;
  }

private:
  SceUInt64 last_time_;
};

class EpollSocket {
public:
  EpollSocket(int sock_fd, SceUID epoll) : fd_(sock_fd), epoll_(epoll) {}
  ~EpollSocket() {
    sceNetEpollControl(epoll_, SCE_NET_EPOLL_CTL_DEL, fd_, NULL);
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

  uint64_t time_since_last_heartbeat() const {
    return time_helper_.elapsed_time_secs();
  }
  void update_heartbeat_time() { time_helper_.update(); }

  void add_to_buffer(uint8_t const *data, size_t size) {
    if (size > MAX_BUFFER_ACCEPTABLE_SIZE)
      throw ClientDataException("Buffer size exceeded");

    buffer_.insert(buffer_.end(), data, data + size);

    if (buffer_.size() > MAX_BUFFER_ACCEPTABLE_SIZE) {
      buffer_.clear();
      throw ClientDataException("Buffer size exceeded");
    }
  }

  bool with_handshake_callback(
      std::function<void(NetProtocol::Handshake::Handshake const &)> callback) {
    flatbuffers::Verifier verifier(buffer_.data(), buffer_.size());

    if (!NetProtocol::Handshake::VerifySizePrefixedHandshakeBuffer(verifier))
      return false;

    auto handshake =
        NetProtocol::Handshake::GetSizePrefixedHandshake(buffer_.data());

    callback(*handshake);

    auto size = verifier.GetComputedSize();
    buffer_.erase(buffer_.begin(), buffer_.begin() + size);
    return true;
  }

  /**
   * R
   */
  bool retrieve_heartbeat_buffer() {
    if (buffer_.size() < heartbeat_magic.size() ||
        !std::equal(heartbeat_magic.begin(), heartbeat_magic.end(),
                    buffer_.begin()))
      return false;

    auto size = heartbeat_magic.size();
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
  TimeHelper time_helper_;
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
