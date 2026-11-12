#pragma once
#ifndef _WIN32
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

/***
 * A TCP proxy between an API client and TWS that can break connections on command,
 * so tests can drop a working connection without restarting TWS.
 *
 * It listens on 127.0.0.1 on a free port (Port()) and forwards each connection to TWS.
 * POSIX only.
 */
class TwsProxy
{
    public:
    TwsProxy(const std::string& twsHost, int twsPort);
    ~TwsProxy();
    TwsProxy(const TwsProxy&) = delete;
    TwsProxy& operator=(const TwsProxy&) = delete;

    /*** the port clients should connect to */
    int Port() const { return port; }
    /*** abort every proxied connection: the client sees a reset (TWS API reports 509)
     * @returns the number of connections reset */
    int Reset();
    /*** close every proxied connection gracefully: the client sees end of stream
     * (TWS API calls connectionClosed)
     * @returns the number of connections closed */
    int Close();
    /*** stop listening: new connects are refused (TWS API reports 502) */
    void Refuse();
    /*** listen again, on the same port */
    void Accept();
    /*** for the next connection only, forward this many messages from TWS, then end it.
     * 1 lets the handshake reply through and cuts the connection before nextValidId.
     * @param reset true to abort with RST (TWS API reports 509), false to close gracefully
     *      (TWS API calls connectionClosed) */
    void CutNextAfter(int messages, bool reset = false);
    /*** number of open proxied connections */
    int OpenConnections();

    private:
    struct Connection
    {
        int client = -1;
        int tws = -1;
        std::atomic<bool> stop = false;
        std::atomic<int> pumpsFinished = 0;
        bool resetWhenDone = false; // how to end it when the pumps stop on their own
        std::thread toTws;
        std::thread toClient;
    };
    void Listen();
    void AcceptLoop();
    static void Pump(Connection* conn, int from, int to, std::optional<int> messageLimit);
    /*** stop and join the pumps, then close the sockets (reset = abort with RST) */
    static void Finish(Connection& conn, bool reset);
    /*** finish connections whose pumps have ended on their own. Call with mutex held */
    void Reap();

    std::string twsHost;
    int twsPort;
    int port = 0;
    int listener = -1; // only the accept thread opens and closes it (after the constructor)
    std::mutex mutex; // guards connections, cutNext, listening, listenError
    std::condition_variable listenChanged;
    bool wantListening = true;
    bool listening = false;
    std::string listenError;
    std::vector<std::unique_ptr<Connection>> connections;
    std::optional<int> cutNext;
    bool cutNextReset = false;
    std::atomic<bool> shuttingDown = false;
    std::thread acceptThread;
};
#endif
