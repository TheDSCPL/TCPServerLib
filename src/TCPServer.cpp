#include <iostream>
#include "../headers/TCPServer.hpp"
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cctype>

using namespace std;

void TCPServer::write(int socketfd, std::string tosend) {
    ::write(socketfd, tosend.c_str(), tosend.length());
}

void TCPServer::readLine(int socketfd, std::string &line) {
    string ret;

    char b[1];

    while(true) {
        read(socketfd, b);
        if (b[0] == '\n' || b[0] == '\0') {
            line = ret;
            return;
        } else if (b[0] == '\r') {
            continue;
        } else {
            if(!isprint(b[0]))
                continue;
            ret += b[0];
        }
    }
}

void TCPServer::read(int socketfd, char *c) {
    errno = 0;

    char b[1];

    ssize_t t=::read(socketfd,b,1);

    if(t<0) {
        switch (errno) {
            case EAGAIN:
            case EBADF:
            case EISDIR:
                throw ReadError("Invalid socketf "+socketfd);
            case EFAULT:
                throw ReadError("Buffer error");
            case EINTR:
                throw ReadError("Interrupted read");
            case EINVAL:
            case EIO:
                throw ReadError("Error reading!");
            case 104:   //Connection reset by peer (should be closed!)
                for(auto it=connections.begin() ; it!=connections.end() ; it++) {
                    if((*it)->socket==socketfd) {
                        (*it)->close();
                        *c = '\0';
                        return;
                    }
                }
                //if it's a connection that doesn't belong to this server, just throw an exception
                //this should not happen but it's here for robustness
                throw ConnectionEOF();
            default:
                throw ReadError("Another error!");
        }
    } else if (t==0) {
        *c = '\0';
        throw ConnectionEOF();
    } else
        *c=b[0];
}

void TCPServer::read(int socketfd, std::string &word) {

    word = "";

    char c;

    do {
        read(socketfd,&c);
        if(c=='\0')
            return;
    } while(isspace(c) || iscntrl(c));

    do {
        word+=c;
        read(socketfd,&c);
    } while(c!='\0' && !isspace(c) && !iscntrl(c));
}

string TCPServer::getIP(const std::string& netInterface) {
    //int fd;
    struct ifreq ifr;

    int socket_ref = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to netInterface */
    strncpy(ifr.ifr_name, netInterface.c_str(), IFNAMSIZ - 1);

    ioctl(socket_ref, SIOCGIFADDR, &ifr);

    close(socket_ref);

    static Mutex m;
    m.lock();   //because 'inet_ntoa' uses a shared char* buffer which means it is susceptible to data race problems
    string ret = string(inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr));
    m.unlock();

    //close(socket_ref);

    return ret;
}

TCPServer::TCPServer(std::string IP, int port, std::function<void(Connection &)> clientRoutine) :
        IP(IP.empty()?INADDR_ANY:inet_addr(IP.c_str())),
        port(port),
        acceptor([this](){
            while (true) {
                // Aceitar uma nova ligação. O endereço do cliente fica guardado em
                // cli_addr - endereço do cliente
                // newsockfd - id do socket que comunica com este cliente

                int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &client_addr_length);

                if (newsockfd < 0) {
                    //vcout << "Shutdown signal recieved." << endl;
                    break;
                }

                setsockopt(newsockfd, SOL_SOCKET, SO_REUSEADDR, &enable_resuse_addr, sizeof(int));

                //https://tools.ietf.org/html/rfc854#page-14
                //https://stackoverflow.com/a/279271
                // IAC DO LINEMODE IAC WILL ECHO
                //::write(newsockfd,"\377\375\042\377\373\001",6);

                new Connection(this,newsockfd);
            }
        }, [this]() {
            for(auto it=connections.begin() ; it!=connections.end() ; it++) {
                (*it)->close();
            }
            TCPServer::close(sockfd);
        }),
        clientRoutine(clientRoutine) {
    start();
}

TCPServer::TCPServer(int port, std::function<void(Connection &)> clientRoutine) : TCPServer("",port,clientRoutine) {}

bool TCPServer::isRunning() const {
    return acceptor.isRunning();
}

void TCPServer::shutdown() {
    if(!isRunning())
        return;
    acceptor.cancel();
}

void TCPServer::start() {
    if(isRunning())
        return;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        cerr << "Error creating server socket" << endl;
        ::exit(1);
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable_resuse_addr, sizeof(int));

    // Criar a estrutura que guarda o endereço do servidor
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = this->IP;
    serv_addr.sin_port = htons((uint16_t) port);
    client_addr_length = sizeof(cli_addr);

    int res = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (res < 0) {
        cerr << "Error binding server to socket" << endl;
        exit(-1);
    }

    listen(sockfd, 5);

    acceptor.start();

    cout << "Server started on " << getIP() << ":" << getPort() << endl;
}

void TCPServer::join() const {
    acceptor.join();
}

int TCPServer::getNConnections() const {
    return connections.size();
}

int TCPServer::getPort() const {
    return port;
}

std::string TCPServer::getIP() const {
    struct in_addr temp;
    temp.s_addr=IP;
    return std::string(inet_ntoa(temp));
}

void TCPServer::close(int socket) {
    ::shutdown(socket,SHUT_RDWR);
    ::close(socket);
}

function<void(Connection &)> TCPServer::getClientRoutine() const {
    return clientRoutine;
}

void TCPServer::sendToAll(const std::string &s, Connection *exceptSocketFD) const {
    for(auto it = connections.begin() ; it!= connections.end() ; it++) {
        if(*it == exceptSocketFD)
            continue;
        **it << s;
    }
}

void Connection::throwIfClosed() const {
    if(!isConnected()) {
        throw ConnectionClosed(socket);
    }
}

Connection::Connection(TCPServer *server, int socket) :
        server(server),
        socket(socket),
        clientHandler([this,&server](){
            server->getClientRoutine()(*this);
        }, [this]() {
            this->close();
        }) {
    server->connections.insert(this);

    cout << "Connection created with socket " << socket << endl;

    clientHandler.start();
}

void Connection::onDisconnect() {
    connected=false;
    server->connections.erase(this);
    cout << "Connection to socket " << this->socket << " closed!" << endl;
}

void Connection::close() {
    if(!isConnected())
        return;
    TCPServer::close(this->socket);
    onDisconnect();
}

bool Connection::isConnected() const {
    return connected;
}

TCPServer& Connection::getServer() const {
    return *server;
}

int Connection::getSocket() const {
    return socket;
}

std::string Connection::readLine() {
    string ret;
    do {
        this->getServer().readLine(socket, ret);
    } while(ret.empty());
    return ret;
}

ConnectionClosed::ConnectionClosed(int socketId) : whatMessage(
        string("Conection with socket ") + socketId + " was closed!") {}

const char *ConnectionClosed::what() const throw() {
    return whatMessage.c_str();
}

ReadError::ReadError(const string &what, int error) : whatMessage(what), error(error) {}

const char* ReadError::what() const throw() {
    return whatMessage.c_str();
}