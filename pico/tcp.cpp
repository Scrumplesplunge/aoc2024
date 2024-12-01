#include "tcp.hpp"

#include "schedule.hpp"

#include <print>

namespace aoc2024::tcp {

Socket::~Socket() {
  if (!handle_) return;
  UnsetCallbacks();
  std::println("closing connection");
  // If we successfully close the socket, the PCB will be freed in the
  // background and we should no longer touch it. Otherwise, we will let it be
  // automatically aborted by the handle.
  if (err_t error = tcp_close(handle_.get()); error == ERR_OK) {
    std::println("closed cleanly");
    handle_.release();
  }
}

Socket::Socket(Socket&& other) noexcept
    : handle_(std::move(other.handle_)),
      received_(std::move(other.received_)),
      pending_read_(std::exchange(other.pending_read_, nullptr)),
      pending_write_(std::exchange(other.pending_write_, nullptr)),
      send_eof_(std::exchange(other.send_eof_, true)),
      receive_eof_(std::exchange(other.receive_eof_, true)) {
  SetCallbacks();
}

Socket& Socket::operator=(Socket&& other) noexcept {
  handle_ = std::move(other.handle_);
  received_ = std::move(other.received_);
  pending_read_ = std::exchange(other.pending_read_, nullptr);
  pending_write_ = std::exchange(other.pending_write_, nullptr);
  send_eof_ = std::exchange(other.send_eof_, true);
  receive_eof_ = std::exchange(other.receive_eof_, true);
  SetCallbacks();
  return *this;
}

Socket::ReadAwaitable Socket::Read(std::span<char> buffer) {
  assert(handle_);
  return ReadAwaitable(*this, buffer);
}

Socket::WriteAwaitable Socket::Write(std::span<const char> bytes) {
  assert(handle_);
  return WriteAwaitable(*this, bytes);
}

Socket::Socket(Handle handle) : handle_(std::move(handle)) {
  SetCallbacks();
}

void Socket::OnSent(u16_t bytes) {
  assert(!send_eof_);
  std::println("OnSent(bytes={})", bytes);
  if (bytes == 0) {
    // (I am assuming that) 0 bytes sent only happens if the socket has been
    // closed for sending.
    send_eof_ = true;
    if (pending_write_) pending_write_->Fail(ERR_CLSD);
    return;
  }
  assert(pending_write_);
  pending_write_->Sent(bytes);
}

void Socket::OnReceived(Buffer data) {
  assert(!receive_eof_);
  if (!data) {
    std::println("OnReceived(data=null)");
    receive_eof_ = true;
    return;
  }
  std::println("OnReceived(data=<pbuf with tot_len={}>)", data->tot_len);
  if (received_) {
    pbuf_cat(received_.get(), data.release());
  } else {
    received_ = std::move(data);
  }
  if (pending_read_) ReadData();
}

void Socket::ReadData() {
  assert(pending_read_);
  assert(received_ || receive_eof_);
  if (!received_) return pending_read_->Done();
  std::span<char> destination = pending_read_->unused();
  std::println("ReadData() received_->tot_len = {}, unused.size() = {}",
               received_->tot_len, destination.size());
  // pbufs operate with 16-bit sizes, so the maximum amount we can read in
  // a single go is 65535 bytes.
  if (destination.size() > received_->tot_len) {
    destination = destination.subspan(0, received_->tot_len);
  }
  pbuf_copy_partial(received_.get(), destination.data(), destination.size(), 0);
  received_ = Buffer(pbuf_free_header(received_.release(), destination.size()));
  tcp_recved(handle_.get(), destination.size());
  pending_read_->Received(destination.size());
  if (!received_ && pending_read_ && receive_eof_) pending_read_->Done();
}

void Socket::SetCallbacks() {
  if (!handle_) return;
  std::println("configuring callbacks for socket {}", (void*)this);
  tcp_arg(handle_.get(), this);
  tcp_sent(handle_.get(), [](void* self, tcp_pcb* pcb, u16_t sent) -> err_t {
    Socket& socket = *reinterpret_cast<Socket*>(self);
    assert(socket.handle_.get() == pcb);
    socket.OnSent(sent);
    return ERR_OK;
  });
  tcp_recv(handle_.get(), [](void* self, tcp_pcb* pcb, pbuf* buffer,
                             err_t error) -> err_t {
    Socket& socket = *reinterpret_cast<Socket*>(self);
    assert(socket.handle_.get() == pcb);
    assert(error == ERR_OK);  // Let's naively assume it's all good.
    socket.OnReceived(Buffer(buffer));
    return ERR_OK;
  });
  tcp_err(handle_.get(), [](void* self, err_t error) {
    Socket& socket = *reinterpret_cast<Socket*>(self);
    // When the error callback is invoked, the PCB has already been freed.
    (void)socket.handle_.release();
    ReadAwaitable* const read = socket.pending_read_;
    WriteAwaitable* const write = socket.pending_write_;
    if (read) read->Fail(error);
    if (write) write->Fail(error);
  });
}

void Socket::UnsetCallbacks() {
  tcp_arg(handle_.get(), nullptr);
  tcp_sent(handle_.get(), nullptr);
  tcp_recv(handle_.get(), nullptr);
  tcp_err(handle_.get(), nullptr);
}

Acceptor::Acceptor(int port) {
  handle_ = Handle(tcp_new_ip_type(IPADDR_TYPE_V4));
  if (!handle_) throw AcceptorError("Failed to create acceptor socket.");
  if (err_t error = tcp_bind(handle_.get(), nullptr, port); error != ERR_OK) {
    throw AcceptorError("Failed to bind to serving port.");
  }
  handle_ = Handle(tcp_listen_with_backlog(handle_.release(), 1));
  if (!handle_) throw AcceptorError("Failed to listen for connections.");

  SetCallbacks();
}

Acceptor::~Acceptor() {
  if (!handle_) return;
  UnsetCallbacks();
}

Acceptor::Acceptor(Acceptor&& other) noexcept
    : handle_(std::move(other.handle_)) {
  SetCallbacks();
}

Acceptor& Acceptor::operator=(Acceptor&& other) noexcept {
  handle_ = std::move(other.handle_);
  SetCallbacks();
  return *this;
}

Acceptor::AcceptAwaitable Acceptor::Accept() {
  assert(handle_);
  return AcceptAwaitable(*this);
}

void Acceptor::SetCallbacks() {
  if (!handle_) return;
  tcp_arg(handle_.get(), this);
  tcp_accept(handle_.get(),
             [](void* self, tcp_pcb* client, err_t error) -> err_t {
               Acceptor& acceptor = *reinterpret_cast<Acceptor*>(self);
               assert(client || error != ERR_OK);
               acceptor.OnAccept(Socket::Handle(client), error);
               return ERR_OK;
             });
}

void Acceptor::UnsetCallbacks() {
  if (!handle_) return;
  tcp_arg(handle_.get(), nullptr);
  tcp_accept(handle_.get(), nullptr);
}

void Acceptor::OnAccept(Socket::Handle client, err_t error) {
  std::println("OnAccept(client={}, error={})", (void*)client.get(), error);
  if (!pending_accept_) {
    if (error) {
      std::println("Ignoring accept error {} with no pending accept call.",
                   error);
    } else {
      std::println("Ignoring connection with no pending accept call.");
    }
    return;
  }
  if (client) {
    pending_accept_->Resolve(std::move(client));
  } else {
    pending_accept_->Fail(error);
  }
}

bool Socket::ReadAwaitable::await_ready() const { return false; }

void Socket::ReadAwaitable::await_suspend(std::coroutine_handle<> awaiter) {
  std::println("Suspending reader");
  awaiter_ = awaiter;
  assert(!socket_.pending_read_);
  socket_.pending_read_ = this;
  if (socket_.received_ || socket_.receive_eof_) socket_.ReadData();
}

std::span<char> Socket::ReadAwaitable::await_resume() {
  std::println("Resuming reader (num_bytes_={})", num_bytes_);
  if (error_ != ERR_OK) throw SocketError("Read error");
  return buffer_.subspan(0, num_bytes_);
}

std::span<char> Socket::ReadAwaitable::unused() const {
  return buffer_.subspan(num_bytes_);
}

void Socket::ReadAwaitable::Done() {
  assert(socket_.pending_read_ == this);
  socket_.pending_read_ = nullptr;
  Schedule(awaiter_);
}

void Socket::ReadAwaitable::Received(int n) {
  assert(num_bytes_ + n <= int(buffer_.size()));
  num_bytes_ += n;
  if (num_bytes_ == int(buffer_.size())) Done();
}

void Socket::ReadAwaitable::Fail(err_t error) {
  error_ = error;
  Done();
}

bool Socket::WriteAwaitable::await_ready() const { return bytes_.empty(); }

void Socket::WriteAwaitable::await_suspend(std::coroutine_handle<> awaiter) {
  std::println("Suspending writer");
  awaiter_ = awaiter;
  assert(!socket_.pending_write_);
  socket_.pending_write_ = this;
  WriteSome();
}

void Socket::WriteAwaitable::await_resume() {
  std::println("Resuming writer");
  if (error_ != ERR_OK) throw SocketError("Write error");
}

void Socket::WriteAwaitable::WriteSome() {
  const std::span<const char> remaining = bytes_.subspan(sent_);
  std::span<const char> to_send = remaining;
  if (const u16_t limit = tcp_sndbuf(socket_.handle_.get());
      to_send.size() > limit) {
    to_send = to_send.subspan(0, limit);
  }
  const err_t error =
      tcp_write(socket_.handle_.get(), to_send.data(), to_send.size(),
                to_send.size() < remaining.size() ? TCP_WRITE_FLAG_MORE : 0);
  if (error == ERR_OK) {
    error_ = error;
    return Done();
  }
  sent_ += to_send.size();
}

void Socket::WriteAwaitable::Done() {
  assert(socket_.pending_write_ == this);
  socket_.pending_write_ = nullptr;
  Schedule(awaiter_);
}

void Socket::WriteAwaitable::Sent(int n) {
  assert(acked_ + n <= int(bytes_.size()));
  acked_ += n;
  if (acked_ == int(bytes_.size())) return Done();
  WriteSome();
}

void Socket::WriteAwaitable::Fail(err_t error) {
  error_ = error;
  Done();
}

bool Acceptor::AcceptAwaitable::await_ready() const { return false; }

void Acceptor::AcceptAwaitable::await_suspend(std::coroutine_handle<> awaiter) {
  std::println("Suspending accept awaiter");
  awaiter_ = awaiter;
  assert(!acceptor_.pending_accept_);
  acceptor_.pending_accept_ = this;
}

Socket Acceptor::AcceptAwaitable::await_resume() {
  std::println("Resuming accept awaiter");
  if (result_) {
    return std::move(*result_);
  } else {
    throw AcceptorError("Failed to accept connection");
  }
}

void Acceptor::AcceptAwaitable::Done() {
  assert(acceptor_.pending_accept_ == this);
  acceptor_.pending_accept_ = nullptr;
  Schedule(awaiter_);
}

void Acceptor::AcceptAwaitable::Resolve(Socket::Handle handle) {
  result_ = Socket(std::move(handle));
  Done();
}

void Acceptor::AcceptAwaitable::Fail(err_t error) {
  result_ = std::unexpected(error);
  Done();
}

}  // namespace aoc2024::tcp
