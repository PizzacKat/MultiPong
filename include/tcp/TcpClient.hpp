#ifndef TESTS_TCPCLIENT_HPP
#define TESTS_TCPCLIENT_HPP

#include "../sock/Socket.hpp"

namespace tcp {
    class TcpClient : public sock::Socket {
    public:
        TcpClient() : TcpClient(AF_INET){

        }

        explicit TcpClient(const sock::Socket &socket) : sock::Socket(socket){

        }

        explicit TcpClient(sock::Socket &&socket) : sock::Socket(socket){

        }

        explicit TcpClient(int domain) : sock::Socket(domain, SOCK_STREAM, IPPROTO_TCP){

        }
    private:
        using Socket::listen, Socket::bind, Socket::accept;
    };
}

#endif //TESTS_TCPCLIENT_HPP
