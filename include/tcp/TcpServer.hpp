#ifndef TESTS_TCPSERVER_HPP
#define TESTS_TCPSERVER_HPP

#include "../sock/Socket.hpp"

namespace tcp {
    class TcpServer : public sock::Socket {
    public:
        TcpServer() : TcpServer(AF_INET){

        }

        explicit TcpServer(const sock::Socket &socket) : sock::Socket(socket){

        }

        explicit TcpServer(sock::Socket &&socket) : sock::Socket(socket){

        }

        explicit TcpServer(int domain) : sock::Socket(domain, SOCK_STREAM, IPPROTO_TCP){

        }
    private:
        using Socket::connect, Socket::send, Socket::recv;
    };
}

#endif //TESTS_TCPSERVER_HPP
