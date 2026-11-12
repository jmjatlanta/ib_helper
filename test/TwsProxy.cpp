#ifndef _WIN32
#include "TwsProxy.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace {

constexpr int pollMs = 50; // how often blocked threads check for a stop request

int SendFlags()
{
#ifdef MSG_NOSIGNAL
    return MSG_NOSIGNAL;
#else
    return 0;
#endif
}

void NoSigPipe(int fd)
{
#ifdef SO_NOSIGPIPE
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#else
    (void)fd;
#endif
}

bool SendAll(int fd, const char* data, size_t len)
{
    while (len > 0)
    {
        ssize_t n = send(fd, data, len, SendFlags());
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return false;
        data += n;
        len -= n;
    }
    return true;
}

int ConnectTo(const std::string& host, int port)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0)
        return -1;
    int fd = -1;
    for(addrinfo* a = res; a != nullptr; a = a->ai_next)
    {
        fd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (fd < 0)
            continue;
        if (::connect(fd, a->ai_addr, a->ai_addrlen) == 0)
            break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd >= 0)
        NoSigPipe(fd);
    return fd;
}

} // namespace

TwsProxy::TwsProxy(const std::string& twsHost, int twsPort) : twsHost(twsHost), twsPort(twsPort)
{
    Listen(); // throws if it cannot listen
    listening = true;
    acceptThread = std::thread(&TwsProxy::AcceptLoop, this);
}

TwsProxy::~TwsProxy()
{
    shuttingDown = true;
    if (acceptThread.joinable())
        acceptThread.join();
    std::scoped_lock lock(mutex);
    for(auto& c : connections)
        Finish(*c, false);
    connections.clear();
    if (listener >= 0)
        close(listener);
}

void TwsProxy::Listen()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error("TwsProxy: socket failed");
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port)); // 0 the first time: pick a free port
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || listen(fd, 8) != 0)
    {
        close(fd);
        throw std::runtime_error("TwsProxy: cannot listen on port " + std::to_string(port)
                + ": " + std::strerror(errno));
    }
    socklen_t len = sizeof(addr);
    getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len);
    port = ntohs(addr.sin_port);
    listener = fd;
}

void TwsProxy::AcceptLoop()
{
    while (!shuttingDown)
    {
        {
            // apply Refuse()/Accept(); only this thread touches the listener
            std::scoped_lock lock(mutex);
            Reap();
            if (wantListening != listening)
            {
                if (wantListening)
                {
                    try
                    {
                        Listen();
                        listening = true;
                    }
                    catch(const std::exception& ex)
                    {
                        listenError = ex.what();
                        wantListening = false;
                    }
                }
                else
                {
                    close(listener);
                    listener = -1;
                    listening = false;
                }
                listenChanged.notify_all();
            }
        }
        if (listener < 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
            continue;
        }
        pollfd p{listener, POLLIN, 0};
        if (poll(&p, 1, pollMs) <= 0 || !(p.revents & POLLIN))
            continue;
        int client = accept(listener, nullptr, nullptr);
        if (client < 0)
            continue;
        NoSigPipe(client);
        int tws = ConnectTo(twsHost, twsPort);
        if (tws < 0)
        {
            close(client); // TWS is down: the client sees the connection close
            continue;
        }
        auto conn = std::make_unique<Connection>();
        conn->client = client;
        conn->tws = tws;
        Connection* c = conn.get();
        std::scoped_lock lock(mutex);
        std::optional<int> limit = cutNext;
        conn->resetWhenDone = limit && cutNextReset;
        cutNext.reset();
        conn->toTws = std::thread(&TwsProxy::Pump, c, client, tws, std::nullopt);
        conn->toClient = std::thread(&TwsProxy::Pump, c, tws, client, limit);
        connections.push_back(std::move(conn));
    }
}

void TwsProxy::Pump(Connection* conn, int from, int to, std::optional<int> messageLimit)
{
    // with a limit, forward whole length-prefixed TWS messages and count them
    std::string pending;
    int forwarded = 0;
    char buf[65536];
    while (!conn->stop)
    {
        if (messageLimit && forwarded >= *messageLimit)
            break;
        pollfd p{from, POLLIN, 0};
        int r = poll(&p, 1, pollMs);
        if (r == 0)
            continue;
        if (r < 0 && errno == EINTR)
            continue;
        if (r < 0)
            break;
        ssize_t n = recv(from, buf, sizeof(buf), 0);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            break;
        if (!messageLimit)
        {
            if (!SendAll(to, buf, n))
                break;
            continue;
        }
        pending.append(buf, n);
        bool ok = true;
        while (ok && forwarded < *messageLimit && pending.size() >= 4)
        {
            uint32_t len = (uint8_t(pending[0]) << 24) | (uint8_t(pending[1]) << 16)
                    | (uint8_t(pending[2]) << 8) | uint8_t(pending[3]);
            if (pending.size() < 4 + size_t(len))
                break;
            ok = SendAll(to, pending.data(), 4 + len);
            pending.erase(0, 4 + len);
            ++forwarded;
        }
        if (!ok)
            break;
    }
    // either side ending ends the connection
    conn->stop = true;
    ++conn->pumpsFinished;
}

void TwsProxy::Finish(Connection& conn, bool reset)
{
    conn.stop = true;
    if (conn.toTws.joinable())
        conn.toTws.join();
    if (conn.toClient.joinable())
        conn.toClient.join();
    for(int fd : {conn.client, conn.tws})
    {
        if (fd < 0)
            continue;
        if (reset)
        {
            linger l{1, 0}; // close() then sends RST instead of FIN
            setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
        }
        else
            shutdown(fd, SHUT_RDWR);
        close(fd);
    }
    conn.client = conn.tws = -1;
}

void TwsProxy::Reap()
{
    std::erase_if(connections, [](std::unique_ptr<Connection>& c)
    {
        if (c->pumpsFinished < 2)
            return false;
        Finish(*c, c->resetWhenDone);
        return true;
    });
}

int TwsProxy::Reset()
{
    std::scoped_lock lock(mutex);
    int count = 0;
    for(auto& c : connections)
    {
        Finish(*c, true);
        ++count;
    }
    connections.clear();
    return count;
}

int TwsProxy::Close()
{
    std::scoped_lock lock(mutex);
    int count = 0;
    for(auto& c : connections)
    {
        Finish(*c, false);
        ++count;
    }
    connections.clear();
    return count;
}

void TwsProxy::Refuse()
{
    std::unique_lock lock(mutex);
    wantListening = false;
    listenChanged.wait(lock, [this]{ return !listening; });
}

void TwsProxy::Accept()
{
    std::unique_lock lock(mutex);
    listenError.clear();
    wantListening = true;
    listenChanged.wait(lock, [this]{ return listening || !listenError.empty(); });
    if (!listening)
        throw std::runtime_error(listenError);
}

void TwsProxy::CutNextAfter(int messages, bool reset)
{
    std::scoped_lock lock(mutex);
    cutNext = messages;
    cutNextReset = reset;
}

int TwsProxy::OpenConnections()
{
    std::scoped_lock lock(mutex);
    Reap();
    return static_cast<int>(connections.size());
}
#endif
