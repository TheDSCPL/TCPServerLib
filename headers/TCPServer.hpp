#pragma once

#include <set>
#include "Thread.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include <netinet/in.h>

class Connection;

class TCPServer {
    friend class Connection;
    std::set<Connection *> connections;

    Thread acceptor;

    socklen_t client_addr_length;
    struct sockaddr_in serv_addr, cli_addr;
    const int port,enable_resuse_addr=1;
    int sockfd;
    const in_addr_t IP;
    std::function<void(Connection &)> clientRoutine;

    TCPServer(const TCPServer&) = delete;
    void operator=(TCPServer const &) = delete;

    static void write(int socketfd, std::string tosend);

    void readLine(int socketfd, std::string &line);

    void read(int socketfd, char *c);

    void read(int socketfd, std::string &word);

    static std::string getIP(const std::string& netInterface);

public:
    TCPServer(std::string IP, int port, std::function<void(Connection &)> clientRoutine);

    TCPServer(int port, std::function<void(Connection &)> clientRoutine);

    bool isRunning() const;

    void shutdown();

    void start();

    void join() const;

    int getNConnections() const;

    int getPort() const;

    std::string getIP() const;

    static void close(int socket);

    std::function<void(Connection &)> getClientRoutine() const;

    void sendToAll(const std::string &, Connection *exceptSocketFD = {}) const;
};

class Connection {
    friend class TCPServer;
    void onDisconnect();

    const int socket;
    TCPServer * const server;

    volatile bool connected=true;

    Connection(const Connection&) = delete;
    void operator=(Connection const &) = delete;

    mutable Mutex writeMutex;

    void throwIfClosed() const;

    Thread clientHandler;

    Connection(TCPServer * server, int socket);
public:

    void close();

    bool isConnected() const;

    TCPServer& getServer() const;

    int getSocket() const;

    std::string readLine();

    const Connection &operator<<(const std::string &out) const {
        throwIfClosed();
        writeMutex.lock();
        TCPServer::write(socket, out);
        writeMutex.unlock();
        return *this;
    }

    const Connection &operator<<(const char *out) {
        return *this << std::string(out);
    }

    const Connection &operator<<(const int &i) const {
        return *this << std::to_string(i);
    }

    const Connection &operator<<(const long int &i) const {
        return *this << std::to_string(i);
    }

    const Connection &operator<<(const double &d) const {
        return *this << std::to_string(d);
    }

//    const Connection &operator<<(const bool &b) const {
//        std::string s(b ? "true" : "false");
//        throw 1;
//        return *this << s;
//    }

    //Reads a word from the Connection
    Connection &operator>>(std::string &s) {
        throwIfClosed();
        this->getServer().read(socket, s);
        return *this;
    }

    Connection &operator>>(int &i) {
        std::string temp;
        *this >> temp;
        i = Utils::stoi(temp);
        return *this;
    }

    Connection &operator>>(double &d) {
        std::string temp;
        *this >> temp;
        d = Utils::stod(temp);
        return *this;
    }

    Connection & operator>>(bool &b) {
        std::string temp;
        *this >> temp;
        b = Utils::s2b(temp);
        return *this;
    }

    Connection &operator>>(char &b) {
        this->getServer().read(this->socket,&b);
        return *this;
    }
};

class ConnectionClosed : public std::exception {
    const std::string whatMessage;
public:
    explicit ConnectionClosed(int socketId);

    const char *what() const throw() final;
};

class ReadError : public std::exception {
    const std::string whatMessage;
public:
    ReadError(const std::string &what, int error=errno);

    const int error;

    const char *what() const throw();
};

class ConnectionEOF : public std::exception {

};