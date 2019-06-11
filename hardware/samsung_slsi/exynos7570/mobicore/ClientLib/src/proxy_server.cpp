/*
 * Copyright (c) 2013-2015 TRUSTONIC LIMITED
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the TRUSTONIC LIMITED nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <memory>
#include <mutex>
#include <thread>
#include <string>

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "driver_client.h"
#undef LOG_TAG
#define LOG_TAG "TeeProxyServer"
#include "log.h"
#include "proxy_common.h"
#include "proxy_server.h"

using namespace com::trustonic::tee_proxy;

template <class T>
class FreeMe {
    T* t_;
public:
    FreeMe(T* t): t_(t) {}
    ~FreeMe() {
        ::free(t_);
    }
    operator T* () {
        return t_;
    }
    operator uintptr_t () {
        return reinterpret_cast<uintptr_t>(t_);
    }
};

class ServerConnection {
    int listen_socket_;
    int socket_ = -1;
    pid_t pid_ = 0;
    DriverClient driver;
    std::mutex connection_mutex;     // Protects is_running_ and session_
    bool is_running_ = false;
    Session* session_ = nullptr;
    std::thread thread_;
    int receiveCredentials() {
        // Make sure we can receive credentials
        int enable = 1;
        if (::setsockopt(socket_, SOL_SOCKET, SO_PASSCRED, &enable,
                         sizeof(enable))) {
            LOG_ERRNO("setsockopt");
            return -1;
        }

        struct ucred credentials;
        char ctrl[CMSG_SPACE(sizeof(credentials))];
        struct iovec iov[1] = { { 0, 0 } };
        struct msghdr msg = { 0, 0, iov, 1, ctrl, sizeof(ctrl), 0 };

        msg.msg_iov->iov_base = nullptr;
        msg.msg_iov->iov_len  = 0;
        do {
            if (::recvmsg(socket_, &msg, 0) == 0) {
                break;
            }
            if ((errno != EINTR) && (errno != EAGAIN)) {
                LOG_ERRNO("recvmsg");
                return -1;
            }
        } while (true);

        if (msg.msg_controllen < sizeof(struct cmsghdr)) {
            errno = EINVAL;
            LOG_ERRNO("check");
            return -1;
        }
        struct cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
        size_t size = sizeof(credentials);
        if ((cmptr->cmsg_len != CMSG_LEN(size)) ||
                (cmptr->cmsg_level != SOL_SOCKET) ||
                (cmptr->cmsg_type != SCM_CREDENTIALS)) {
            errno = EINVAL;
            LOG_ERRNO("check");
            return -1;
        }

        credentials = *(struct ucred*)CMSG_DATA(cmptr);
        pid_ = credentials.pid;
        return 0;
    }
    void connectionThread() {
        LOG_D("thread entered %p", this);
        std::string buffer;
        do {
            // Request
            RpcMethod method;
            {
                // Receive request header
                RequestHeader header;
                if (recv_all(socket_, "header", &header, sizeof(header),
                             true)) {
                    break;
                }
                if (::memcmp(header.magic, PROTOCOL_MAGIC,
                             sizeof(header.magic))) {
                    LOG_E("Wrong magic");
                    break;
                }
                if (header.version != PROTOCOL_VERSION) {
                    LOG_E("Wrong version");
                    break;
                }
                method = static_cast<RpcMethod>(header.method);
                buffer.resize(header.length);
                LOG_D("receive %u bytes of data for method %s",
                      header.length, methodToString(method));

                // Receive request data
                if (recv_all(socket_, "data", &buffer[0], header.length)) {
                    break;
                }
            }

            // Dispatch:
            // * method_rc is return code from lower layer
            // * call returns -errno in case of error, length of data otherwise
            int method_rc = 0;
            int proto_rc = call(method, buffer, method_rc);

            // Response
            {
                // Send response header
                ResponseHeader header;
                ::memcpy(header.magic, PROTOCOL_MAGIC, sizeof(header.magic));
                header.version = PROTOCOL_VERSION;
                header.method = static_cast<uint16_t>(method);
                header.proto_rc = proto_rc;
                header.method_rc = method_rc;
                if (send_all(socket_, "header", &header, sizeof(header))) {
                    break;
                }

                // Send response data
                if (header.proto_rc > 0) {
                    LOG_D("send %d bytes of data", header.proto_rc);
                    if (send_all(socket_, "data", &buffer[0],
                                 header.proto_rc)) {
                        break;
                    }
                }
            }
        } while (driver.isOpen());
        LOG_D("thread exiting %p", this);

        // Close associated client
        driver.close();
        ::close(socket_);
        is_running_ = false;
        LOG_D("thread exited %p", this);
    }
public:
    ServerConnection(int listen_socket): listen_socket_(listen_socket) {}
    ~ServerConnection() {
        delete session_;
    }
    int open() {
        socket_ = ::accept(listen_socket_, nullptr, nullptr);
        if (socket_ < 0) {
            return -1;
        }
        if (receiveCredentials() < 0) {
            ::close(socket_);
            return -1;
        }
        // Create specific client for this connection
        if (driver.open()) {
            LOG_ERRNO("open driver");
            ::close(socket_);
            return -1;
        }
        // Start thread
        is_running_ = true;
        thread_ = std::thread(&ServerConnection::connectionThread, this);
        LOG_I("proxy server open, client PID: %d", pid_);
        return 0;
    }
    int close() {
        thread_.join();
        // driver and socket are closed at end of thread
        LOG_I("proxy server closed, client PID: %d", pid_);
        return 0;
    }
    bool isRunning() const {
        return is_running_;
    }
    int call(RpcMethod method, std::string& buffer, int& method_rc) {
        switch (method) {
            case OPEN_SESSION:
                return openSession(buffer, method_rc);
            case OPEN_TRUSTLET:
                return openTrustlet(buffer, method_rc);
            case CLOSE_SESSION:
                return closeSession(buffer, method_rc);
            case NOTIFY:
                return notify(buffer, method_rc);
            case WAIT:
                return waitNotification(buffer, method_rc);
            case MAP:
                return map(buffer, method_rc);
            case UNMAP:
                return unmap(buffer, method_rc);
            case GET_ERROR:
                return getError(buffer, method_rc);
            case GET_VERSION:
                return getVersion(buffer, method_rc);
            case GP_REQUESTCANCELLATION:
                return gpRequestCancellation(buffer, method_rc);
        }
        LOG_E("unkown method %d", method);
        return -1;
    }
    // Actions
    int openSession(std::string& buffer, int& method_rc);
    int openTrustlet(std::string& buffer, int& method_rc);
    int closeSession(std::string& buffer, int& method_rc);
    int notify(std::string& buffer, int& method_rc);
    int waitNotification(std::string& buffer, int& method_rc);
    int map(std::string& buffer, int& method_rc);
    int unmap(std::string& buffer, int& method_rc);
    int getError(std::string& buffer, int& method_rc);
    int getVersion(std::string& buffer, int& method_rc);
    int gpRequestCancellation(std::string& buffer, int& method_rc);
};

struct ProxyServer::Impl {
    int listen_socket = -1;
    int signal_pipe[2];
    std::thread thread;
};

ProxyServer::ProxyServer(): pimpl_(new Impl) {
}

ProxyServer::~ProxyServer() {
    delete pimpl_;
}

int ProxyServer::open() {
    // Make sure we have access to the driver
    DriverClient driver;
    if (driver.open()) {
        LOG_ERRNO("open driver");
        return -1;
    }
    driver.close();

    // Make sure we're alone
    int sock = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERRNO("socket");
        return -1;
    }
    struct sockaddr_un sock_un;
    sock_un.sun_family = AF_UNIX;
    ::strncpy(sock_un.sun_path, SOCKET_PATH, sizeof(sock_un.sun_path) - 1);
    socklen_t len = static_cast<socklen_t>(strlen(sock_un.sun_path) +
                                           sizeof(sock_un.sun_family));
    sock_un.sun_path[0] = '\0';         // Abstract
    if (::bind(sock, reinterpret_cast<struct sockaddr*>(&sock_un), len) == 0) {
        if (::listen(sock, 5) == 0) {
            if (pipe(pimpl_->signal_pipe) == 0) {
                pimpl_->listen_socket = sock;
                pimpl_->thread = std::thread(&ProxyServer::run, this);
                LOG_D("proxy open");
                return 0;
            } else {
                LOG_ERRNO("pipe");
            }
        } else {
            LOG_ERRNO("listen");
        }
    } else {
        LOG_ERRNO("bind");
    }
    ::close(sock);
    return -1;
}

int ProxyServer::close() {
    if (pimpl_->listen_socket < 0) {
        errno = EBADF;
        return -1;
    }
    ::close(pimpl_->listen_socket);
    pimpl_->listen_socket = -1;
    // Make thread stop
    if (pimpl_->thread.joinable()) {
        ::write(pimpl_->signal_pipe[1], "c", 1);
        ::close(pimpl_->signal_pipe[1]);
        pimpl_->thread.join();
    }
    LOG_D("proxy closed");
    return 0;
}

void ProxyServer::run() {
    std::vector<ServerConnection*> connections;

    LOG_D("proxy ready");
    while (pimpl_->listen_socket >= 0) {
        // Wait for new connections
        fd_set fds;
        FD_ZERO(&fds);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        FD_SET(pimpl_->signal_pipe[0], &fds);
        FD_SET(pimpl_->listen_socket, &fds);
#pragma GCC diagnostic pop
        if ((::select(FD_SETSIZE, &fds, nullptr, nullptr, nullptr) < 0) &&
                (errno != EINTR)) {
            LOG_ERRNO("listen on server socket");
            break;
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if (FD_ISSET(pimpl_->signal_pipe[0], &fds)) {
#pragma GCC diagnostic pop
            LOG_I("thread received stop message");
            // Wait for client threads to stop
            for (auto& connection: connections) {
                connection->close();
                LOG_D("delete connection %p", connection);
                delete connection;
            }
            break;
        }
        auto connection = new ServerConnection(pimpl_->listen_socket);
        if (connection->open() < 0) {
            LOG_ERRNO("new connection");
            delete connection;
            // Relax
            usleep(100000);
        } else {
            LOG_D("new connection %p", connection);
            connections.push_back(connection);
        }
        // Cleanup dead connections
        auto it = connections.begin();
        while (it != connections.end()) {
            auto connection = *it;
            if (!connection->isRunning()) {
                it = connections.erase(it);
                connection->close();
                LOG_D("delete connection %p", connection);
                delete connection;
            } else {
                it++;
            }
        }
    }
    LOG_D("proxy exited");
}

int ServerConnection::openSession(std::string& buffer, int& method_rc) {
    LOG_D("enter %s", __FUNCTION__);
    std::lock_guard<std::mutex> connection_lock(connection_mutex);
    if (session_) {
        LOG_E("Session exists");
        return -EBUSY;
    }
    // Request
    OpenSessionRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    if (!request.has_uuid() || !request.has_is_gp_uuid() ||
            !request.has_login_type() || !request.has_login_data()) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }
    // Call driver
    struct mc_ioctl_open_session session;
    if (request.uuid().length() != sizeof(session.uuid)) {
        LOG_E("Incorrect parameter uuid");
        return -EINVAL;
    }
    ::memcpy(&session.uuid, request.uuid().c_str(), sizeof(session.uuid));
    session.is_gp_uuid = request.is_gp_uuid();
    Session::Buffer* tci;
    if (request.has_tci()) {
        tci = new Session::Buffer(request.tci());
        session.tcilen = tci->info().len;
        session.tci = tci->info().va;
    } else {
        tci = nullptr;
        session.tcilen = 0;
        session.tci = 0;
    }
    std::unique_ptr<Session::Buffer> tci_ptr(tci);
    session.identity.login_type =
        static_cast<mc_login_type>(request.login_type());
    session.identity.pid = pid_;
    if (request.login_data().length() != sizeof(session.identity.login_data)) {
        LOG_E("Incorrect parameter login_data");
        return -EINVAL;
    }
    ::memcpy(&session.identity.login_data, request.login_data().c_str(),
             sizeof(session.identity.login_data));

    // Call
    if (driver.openSession(session)) {
        method_rc = errno;
        LOG_D("Error reported %s", strerror(errno));
        return 0;
    }

    // Response
    OpenSessionResponse response;
    response.set_id(session.sid);
    if (!response.SerializeToString(&buffer)) {
        LOG_E("Failed to serialize");
        return -EPIPE;
    }
    LOG_D("session %x open", session.sid);
    session_ = new Session(session.sid, tci_ptr.release());
    return static_cast<int>(buffer.length());
}

int ServerConnection::openTrustlet(std::string& buffer, int& method_rc) {
    LOG_D("enter %s", __FUNCTION__);
    std::lock_guard<std::mutex> connection_lock(connection_mutex);
    if (session_) {
        LOG_E("Session exists");
        return -EBUSY;
    }
    // Request
    OpenTrustletRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    if (!request.has_spid() || !request.has_trustapp()) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }
    // Call driver
    struct mc_ioctl_open_trustlet trustlet;
    trustlet.spid = request.spid();
    trustlet.tlen = static_cast<int>(request.trustapp().length());
    // Create TrustApp buffer
    FreeMe<void> trustapp = ::malloc(trustlet.tlen);
    if (!trustapp) {
        LOG_E("Failed to allocate TrustApp");
        return -ENOMEM;
    }
    ::memcpy(trustapp, request.trustapp().c_str(), trustlet.tlen);
    trustlet.buffer = trustapp;
    Session::Buffer* tci;
    if (request.has_tci()) {
        tci = new Session::Buffer(request.tci());
        trustlet.tcilen = tci->info().len;
        trustlet.tci = tci->info().va;
    } else {
        tci = nullptr;
        trustlet.tcilen = 0;
        trustlet.tci = 0;
    }
    std::unique_ptr<Session::Buffer> tci_ptr(tci);

    // Call
    if (driver.openTrustlet(trustlet)) {
        method_rc = errno;
        LOG_D("Error reported %s", strerror(errno));
        return 0;
    }

    // Response
    OpenTrustletResponse response;
    response.set_id(trustlet.sid);
    if (!response.SerializeToString(&buffer)) {
        LOG_E("Failed to serialize");
        return -EPIPE;
    }
    LOG_D("session %x open", trustlet.sid);
    session_ = new Session(trustlet.sid, tci_ptr.release());
    return static_cast<int>(buffer.length());
}

int ServerConnection::closeSession(std::string& buffer, int& method_rc) {
    // Request
    CloseSessionRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    if (!request.has_id()) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }
    // Check session
    std::lock_guard<std::mutex> connection_lock(connection_mutex);
    if (!session_) {
        LOG_E("No session");
        return -ENOENT;
    }

    // Call
    if (driver.closeSession(request.id())) {
        method_rc = errno;
        LOG_D("Error reported %s", strerror(errno));
        return 0;
    }

    // No response
    LOG_D("session %x closed", request.id());
    delete session_;
    session_ = nullptr;
    return 0;
}

int ServerConnection::notify(std::string& buffer, int& method_rc) {
    // Request
    NotifyRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    // Check session
    std::lock_guard<std::mutex> connection_lock(connection_mutex);
    if (!session_) {
        LOG_E("No session");
        return -ENOENT;
    }
    if (!request.has_sid() || (session_->hasTci() && !request.has_tci())) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }
    // Update TCI
    if (session_->hasTci() && session_->updateTci(request.tci())) {
        LOG_E("Failed to update TCI");
        return -EINVAL;
    }
    // Update mapped buffers
    if (request.buffers_size() > static_cast<int>(session_->buffers().size())) {
        LOG_E("Too many buffers: %d > %zu", request.buffers_size(),
              session_->buffers().size());
        return -EINVAL;
    }
    for (int i = 0; i < request.buffers_size(); i++) {
        const NotifyRequest_Buffers& session_buffer = request.buffers(i);
        if (!session_buffer.has_sva()) {
            LOG_E("Given buffer has no SVA");
            return -EINVAL;
        }
        if (!session_buffer.has_data()) {
            LOG_E("Given buffer has no data");
            return -EINVAL;
        }
        auto buf = session_->findBuffer(session_buffer.sva());
        if (!buf) {
            LOG_E("No buffer record for SVA %jx in session %x",
                  session_buffer.sva(), request.sid());
            return -EPIPE;
        }
        buf->update(session_buffer.data());
    }

    // Call
    if (driver.notify(request.sid())) {
        method_rc = errno;
        return 0;
    }

    // No response
    LOG_D("session %x notification sent", request.sid());
    return 0;
}

int ServerConnection::waitNotification(std::string& buffer, int& method_rc) {
    // Request
    WaitNotificationRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    if (!request.has_sid() || !request.has_timeout() ||
            !request.has_partial()) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }
    // Refuse big timeouts (client shall retry so we know it's still around)
    if ((request.timeout() < 0) || (request.timeout() > timeout_max)) {
        LOG_E("Timeout exceeds 10s");
        return -ENOENT;
    }

    // Find session
    LOG_D("session %x wait for notification", request.sid());
    // Check session
    std::lock_guard<std::mutex> connection_lock(connection_mutex);
    if (!session_) {
        LOG_E("No session");
        return -ENOENT;
    }
    // Call driver
    struct mc_ioctl_wait wait;
    wait.sid = request.sid();
    wait.timeout = request.timeout();

    // Call
    if (driver.waitNotification(wait)) {
        method_rc = errno;
        if ((errno == ETIME) && request.partial()) {
            // No point sending buffers on partial timeout
            return 0;
        }
        // Send potentially modified buffers back on failure too
        LOG_ERRNO("waitNotification");
    }

    // Response
    WaitNotificationResponse response;
    // Send updated TCI
    if (session_->hasTci()) {
        response.set_tci(session_->tci(), session_->tciLen());
    }
    // Send updated mapped buffers
    auto& buffers = session_->buffers();
    for (auto& buf: buffers) {
        if (buf->info().flags & MC_IO_MAP_OUTPUT) {
            auto session_buffer = response.add_buffers();
            session_buffer->set_sva(buf->info().sva);
            session_buffer->set_data(buf->address(), buf->info().len);
        }
    }
    if (!response.SerializeToString(&buffer)) {
        LOG_E("Failed to serialize");
        return -EPIPE;
    }
    LOG_D("session %x notification received", request.sid());
    return static_cast<int>(buffer.length());
}

int ServerConnection::map(std::string& buffer, int& method_rc) {
    // Request
    MapRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    if (!request.has_sid()) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }
    // Check session
    std::lock_guard<std::mutex> connection_lock(connection_mutex);
    if (!session_) {
        LOG_E("No session");
        return -ENOENT;
    }
    if (request.buffers_size() > MC_MAP_MAX) {
        LOG_E("Incorrect number of buffers: %d > %d", request.buffers_size(),
              MC_MAP_MAX);
        return -EINVAL;
    }
    // Prepare local buffers
    struct mc_ioctl_map map;
    map.sid = request.sid();
    LOG_D("map %d buffers to session %x", request.buffers_size(), map.sid);
    std::vector<std::unique_ptr<Session::Buffer>> buffers;
    for (int i = 0; i < request.buffers_size(); i++) {
        const MapRequest_Buffers& session_buffer = request.buffers(i);
        if (!session_buffer.has_len() || !session_buffer.has_flags()) {
            LOG_E("Required parameter missing");
            return -EINVAL;
        }
        if (!(session_buffer.flags() & MC_IO_MAP_INPUT_OUTPUT) ||
                (session_buffer.flags() & ~MC_IO_MAP_INPUT_OUTPUT)) {
            LOG_E("Incorrect flags for buffer: %x", session_buffer.flags());
            return -EINVAL;
        }
        buffers.emplace_back(new Session::Buffer(session_buffer.len(),
                             session_buffer.flags()));
        map.bufs[i] = buffers.back()->info();
    }
    for (int i = request.buffers_size(); i < MC_MAP_MAX; i++) {
        map.bufs[i].va = 0;
    }

    // Call
    if (driver.map(map)) {
        method_rc = errno;
        LOG_D("Error reported %s", strerror(errno));
        return 0;
    }

    // Response
    MapResponse response;
    // Confirm/return buffers
    for (int i = 0; i < request.buffers_size(); i++) {
        Session::Buffer* buf = buffers[i].release();
        buf->setSva(map.bufs[i].sva);
        session_->addBuffer(buf);
        auto session_buffer = response.add_buffers();
        session_buffer->set_sva(map.bufs[i].sva);
    }
    if (!response.SerializeToString(&buffer)) {
        LOG_E("Failed to serialize");
        return -EPIPE;
    }
    LOG_D("session %x %d buffer(s) mapped", map.sid, request.buffers_size());
    return static_cast<int>(buffer.length());
}

int ServerConnection::unmap(std::string& buffer, int& method_rc) {
    // Request
    UnmapRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    if (!request.has_sid()) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }
    if (request.buffers_size() > MC_MAP_MAX) {
        LOG_E("Incorrect number of buffers: %d", request.buffers_size());
        return -EINVAL;
    }
    // Check session
    std::lock_guard<std::mutex> connection_lock(connection_mutex);
    if (!session_) {
        LOG_E("No session");
        return -ENOENT;
    }
    // Find local buffers
    struct mc_ioctl_map map;
    map.sid = request.sid();
    LOG_D("unmap %d buffers from session %x", request.buffers_size(), map.sid);
    if (request.buffers_size() > static_cast<int>(session_->buffers().size())) {
        LOG_E("More buffers to unmap than we have: %d > %zu",
              request.buffers_size(), session_->buffers().size());
        return -EINVAL;
    }
    std::vector<Session::Buffer*> buffers;
    for (int i = 0; i < request.buffers_size(); i++) {
        // Need to find buffer to get VA/length
        const UnmapRequest_Buffers& session_buffer = request.buffers(i);
        if (!session_buffer.has_sva()) {
            LOG_E("Given buffer has no SVA");
            return -EINVAL;
        }
        auto buf = session_->findBuffer(session_buffer.sva());
        if (!buf) {
            LOG_E("No buffer record for SVA %jx in session %x",
                  session_buffer.sva(), request.sid());
            method_rc = EINVAL;
            return 0;
        }
        map.bufs[i] = buf->info();
        buffers.push_back(buf);
    }
    for (int i = request.buffers_size(); i < MC_MAP_MAX; i++) {
        map.bufs[i].va = 0;
    }

    // Call
    if (driver.unmap(map)) {
        method_rc = errno;
        LOG_D("Error reported %s", strerror(errno));
        return 0;
    }

    // No response
    // Delete buffers
    for (size_t i = 0; i < buffers.size(); i++) {
        if (session_->removeBuffer(buffers[i]->info().sva)) {
            LOG_E("Did not find our buffer anymore!?!");
            return -EPIPE;
        }
    }
    LOG_D("session %x %d buffer(s) unmapped", map.sid, request.buffers_size());
    return 0;
}

int ServerConnection::getError(std::string& buffer, int& method_rc) {
    // Request
    GetErrorRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    if (!request.has_sid()) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }
    struct mc_ioctl_geterr err;
    err.sid = request.sid();

    // Call
    if (driver.getError(err)) {
        method_rc = errno;
        LOG_D("Error reported %s", strerror(errno));
        return 0;
    }

    // Response
    GetErrorResponse response;
    response.set_exit_code(err.value);
    if (!response.SerializeToString(&buffer)) {
        LOG_E("Failed to serialize");
        return -EPIPE;
    }
    LOG_D("session %x exit code %d", err.sid, err.value);
    return static_cast<int>(buffer.length());
}

int ServerConnection::getVersion(std::string& buffer, int& method_rc) {
    // Request
    GetVersionRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    // No inputs to check
    struct mc_version_info version_info;

    // Call
    if (driver.getVersion(version_info)) {
        method_rc = errno;
        LOG_D("Error reported %s", strerror(errno));
        return 0;
    }

    // Response
    GetVersionResponse response;
    response.set_product_id(version_info.product_id);
    response.set_mci(version_info.version_mci);
    response.set_so(version_info.version_so);
    response.set_mclf(version_info.version_mclf);
    response.set_container(version_info.version_container);
    response.set_mc_config(version_info.version_mc_config);
    response.set_tl_api(version_info.version_tl_api);
    response.set_dr_api(version_info.version_dr_api);
    response.set_nwd(version_info.version_nwd);
    if (!response.SerializeToString(&buffer)) {
        LOG_E("Failed to serialize");
        return -EPIPE;
    }
    return static_cast<int>(buffer.length());
}

int ServerConnection::gpRequestCancellation(std::string& buffer,
        int& method_rc) {
    // Request
    GetErrorRequest request;
    if (!request.ParseFromString(buffer)) {
        LOG_E("Failed to parse");
        return -EPIPE;
    }
    if (!request.has_sid()) {
        LOG_E("Required parameter missing");
        return -EINVAL;
    }

    // Call
    if (driver.gpRequestCancellation(request.sid())) {
        method_rc = errno;
        LOG_D("Error reported %s", strerror(errno));
        return 0;
    }

    // No response
    return 0;
}
