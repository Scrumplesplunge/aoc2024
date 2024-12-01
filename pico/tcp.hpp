#ifndef AOC2024_TCP_HPP_
#define AOC2024_TCP_HPP_

#include "../common/coro.hpp"
#include "../common/delete_with.hpp"

#include <expected>
#include <lwip/tcp.h>
#include <memory>
#include <pico/async_context.h>
#include <stdexcept>

namespace aoc2024::tcp {

// A single TCP connection. Neither threadsafe nor reentrant.
class Socket {
 public:
  Socket() = default;

  // Attempts to cleanly close the socket and falls back to aborting if needed.
  ~Socket();

  // Not copyable.
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  // Movable.
  Socket(Socket&&) noexcept;
  Socket& operator=(Socket&&) noexcept;

  // Reads bytes into the provided buffer until either the buffer is full or the
  // socket has been closed for sending by the peer. On success, the awaitable
  // yields a span containing the bytes that were read. On error, an exception
  // is thrown.
  class ReadAwaitable;
  ReadAwaitable Read(std::span<char> buffer);

  // Writes the given bytes to the socket. On error, an exception is thrown.
  class WriteAwaitable;
  WriteAwaitable Write(std::span<const char> bytes);

 private:
  friend class Acceptor;

  using Handle =
      std::unique_ptr<tcp_pcb, DeleteWith<[](tcp_pcb* x) { tcp_abort(x); }>>;

  using Buffer =
      std::unique_ptr<pbuf, DeleteWith<[](pbuf* x) { pbuf_free(x); }>>;

  explicit Socket(Handle handle);

  void OnSent(u16_t bytes);
  void OnReceived(Buffer data);
  void ReadData();

  void SetCallbacks();
  void UnsetCallbacks();

  Handle handle_;

  // Data which has been received but has not been read.
  Buffer received_;
  // A pending read which has not yet received as much data as it requested.
  ReadAwaitable* pending_read_ = nullptr;
  // A pending write which has not yet sent as much data as it needs to.
  WriteAwaitable* pending_write_ = nullptr;

  bool send_eof_ = false;
  bool receive_eof_ = false;
};

// Listens on a port and accepts incoming connections.
// Neither threadsafe nor reentrant.
class Acceptor {
 public:
  explicit Acceptor(int port);
  ~Acceptor();

  // Not copyable.
  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;

  // Movable.
  Acceptor(Acceptor&&) noexcept;
  Acceptor& operator=(Acceptor&&) noexcept;

  // Accept a new connection. On success, the awaitable yields a Socket
  // connected to the peer. On failure, an exception is thrown.
  class AcceptAwaitable;
  AcceptAwaitable Accept();

 private:
  using Handle =
      std::unique_ptr<tcp_pcb, DeleteWith<[](tcp_pcb* x) { tcp_close(x); }>>;

  void SetCallbacks();
  void UnsetCallbacks();

  void OnAccept(Socket::Handle client, err_t error);

  Handle handle_;
  AcceptAwaitable* pending_accept_ = nullptr;
};

class Error : public std::exception {
 public:
  explicit Error(const char* message) : message_(message) {}
  virtual const char* type() const noexcept { return "Error"; }
  const char* what() const noexcept override { return message_; }

 private:
  const char* message_;
};

class SocketError : public Error {
 public:
  using Error::Error;
  const char* type() const noexcept override { return "AcceptorError"; }
};

class AcceptorError : public Error {
 public:
  using Error::Error;
  const char* type() const noexcept override { return "AcceptorError"; }
};

class [[nodiscard]] Socket::ReadAwaitable {
 public:
  bool await_ready() const;
  void await_suspend(std::coroutine_handle<> awaiter);
  std::span<char> await_resume();

 private:
  friend class Socket;

  explicit ReadAwaitable(Socket& socket, std::span<char> buffer)
      : socket_(socket), buffer_(buffer) {}

  std::span<char> unused() const;

  void Done();
  void Received(int n);
  void Fail(err_t error);

  Socket& socket_;
  std::span<char> buffer_;
  int num_bytes_ = 0;
  err_t error_ = ERR_OK;
  std::coroutine_handle<> awaiter_;
};

class [[nodiscard]] Socket::WriteAwaitable {
 public:
  bool await_ready() const;
  void await_suspend(std::coroutine_handle<> awaiter);
  void await_resume();

 private:
  friend class Socket;

  explicit WriteAwaitable(Socket& socket, std::span<const char> bytes)
      : socket_(socket), bytes_(bytes) {}

  std::span<const char> unsent() const;

  void WriteSome();
  void Done();
  void Sent(int n);
  void Fail(err_t error);

  Socket& socket_;
  // Only stores unsent bytes (updated while sending).
  std::span<const char> bytes_;
  int sent_ = 0, acked_ = 0;
  err_t error_ = ERR_OK;
  std::coroutine_handle<> awaiter_;
  async_when_pending_worker_t worker_;
};

class [[nodiscard]] Acceptor::AcceptAwaitable {
 public:
  bool await_ready() const;
  void await_suspend(std::coroutine_handle<> handle);
  Socket await_resume();

 private:
  friend class Acceptor;

  explicit AcceptAwaitable(Acceptor& acceptor) : acceptor_(acceptor) {}

  void Done();
  void Resolve(Socket::Handle handle);
  void Fail(err_t error);

  Acceptor& acceptor_;
  std::expected<Socket, err_t> result_;
  std::coroutine_handle<> awaiter_;
  async_when_pending_worker_t worker_;
};

}  // namespace aoc2024::tcp

#endif  // AOC2024_TCP_HPP_
