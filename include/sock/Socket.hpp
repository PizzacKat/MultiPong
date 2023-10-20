#ifndef TESTS_SOCKET_HPP
#define TESTS_SOCKET_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <unistd.h>
#include <stdexcept>

namespace sock {
    typedef int socket_t;
    class SocketException : public std::exception{
    public:
        explicit SocketException(socket_t socket) : socket(socket){

        }

        socket_t socket;
    };

    class ReadException : public SocketException {
    public:
        explicit ReadException(const char *what, socket_t socket) : m_what(what), SocketException(socket){

        }

        [[nodiscard]] const char *what() const noexcept override{
            return m_what;
        }
    private:
        const char *m_what;
    };

    class WriteException : public SocketException {
    public:
        explicit WriteException(const char *what, socket_t socket) : m_what(what), SocketException(socket){

        }

        [[nodiscard]] const char *what() const noexcept override{
            return m_what;
        }
    private:
        const char *m_what;
    };

    class ConnectException : public SocketException {
    public:
        explicit ConnectException(const char *what, socket_t socket) : m_what(what), SocketException(socket){

        }

        [[nodiscard]] const char *what() const noexcept override{
            return m_what;
        }
    private:
        const char *m_what;
    };

    class BindException : public SocketException {
    public:
        explicit BindException(const char *what, socket_t socket) : m_what(what), SocketException(socket){

        }

        [[nodiscard]] const char *what() const noexcept override{
            return m_what;
        }
    private:
        const char *m_what;
    };

    class ListenException : public SocketException {
    public:
        explicit ListenException(const char *what, socket_t socket) : m_what(what), SocketException(socket){

        }

        [[nodiscard]] const char *what() const noexcept override{
            return m_what;
        }
    private:
        const char *m_what;
    };

    class AcceptException : public SocketException {
    public:
        explicit AcceptException(const char *what, socket_t socket) : m_what(what), SocketException(socket){

        }

        [[nodiscard]] const char *what() const noexcept override{
            return m_what;
        }
    private:
        const char *m_what;
    };

    class DisconnectionException : public SocketException{
    public:
        explicit DisconnectionException(socket_t socket) : SocketException(socket){

        }

        [[nodiscard]] const char *what() const noexcept override{
            return "socket disconnected";
        }
    };

    class IPAddress {
    public:
        IPAddress(){
            m_family = 0;
            m_address = {};
        }

        explicit IPAddress(const sockaddr *addr){
            if (addr->sa_family == AF_INET){
                const auto *addr4 = (const sockaddr_in *)addr;
                std::vector<unsigned char> address;
                address.push_back(addr4->sin_addr.s_addr & 0xFF);
                address.push_back((addr4->sin_addr.s_addr >> 8) & 0xFF);
                address.push_back((addr4->sin_addr.s_addr >> 16) & 0xFF);
                address.push_back((addr4->sin_addr.s_addr >> 24) & 0xFF);
                *this = IPAddress(address);
            }
            else if (addr->sa_family == AF_INET6){
                const auto *addr6 = (const sockaddr_in6 *)addr;
                std::vector<unsigned char> address;
                for (size_t i = 0; i < 16; i++) address.push_back(addr6->sin6_addr.s6_addr[15 - i]);
                *this = IPAddress(address);
            }
            else
                throw std::invalid_argument("IPAddress");
        }

        explicit IPAddress(const std::vector<unsigned char> &address){
            if (address.size() == 4)
                m_family = AF_INET;
            else if (address.size() == 16)
                m_family = AF_INET6;
            else
                throw std::invalid_argument("IPAddress");
            m_address = address;
        }

        static IPAddress parse4(const char *ptr){
            std::vector<unsigned char> addr(4);
            if (inet_pton(AF_INET, ptr, addr.data()) == -1) throw std::invalid_argument("IPAddress::parse4");
            std::reverse(addr.begin(), addr.end());
            return IPAddress(addr);
        }

        static IPAddress parse6(const char *ptr){
            std::vector<unsigned char> addr(16);
            if (inet_pton(AF_INET6, ptr, addr.data()) == -1) throw std::invalid_argument("IPAddress::parse6");
            std::reverse(addr.begin(), addr.end());
            return IPAddress(addr);
        }

        static IPAddress parse(const char *ptr){
            bool is6 = false;
            for (const char *p = ptr; *p != 0; p++)
                if (*p == ':'){
                    is6 = true; // If string contains a ':', then we'll assume it's an ipv6 address
                    break;
                }
            return is6 ? parse6(ptr) : parse4(ptr);
        }

        [[nodiscard]] unsigned short family() const{
            return m_family;
        }

        [[nodiscard]] const std::vector<unsigned char> &address() const{
            return m_address;
        }

        [[nodiscard]] sockaddr_in addr4() const{
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = (m_address[0] << 24) | (m_address[1] << 16) | (m_address[2] << 8) | m_address[3];
            return addr;
        }

        [[nodiscard]] sockaddr_in6 addr6() const{
            sockaddr_in6 addr{};
            addr.sin6_family = AF_INET6;
            for (size_t i = 0; i < 16; i++) addr.sin6_addr.s6_addr[15 - i] = m_address[i];
            return addr;
        }
    private:
        unsigned short m_family{};
        std::vector<unsigned char> m_address;
    };

    class Socket {
    public:
        typedef size_t len_t;
        Socket() : m_fd(0){

        }

        explicit Socket(socket_t fd) : m_fd(fd){

        }

        Socket(int domain, int type, int protocol) : m_fd(::socket(domain, type, protocol)){

        }

        void connect(const sockaddr *addr, socklen_t len){
            if (::connect(m_fd, addr, len) == -1) throw ConnectException("connect", fd());
            m_connected = true;
        }

        void connect(const IPAddress& address, unsigned short port){
            if (address.family() == AF_INET){
                sockaddr_in addr = address.addr4();
                addr.sin_port = htons(port);
                connect((const sockaddr *)&addr, sizeof(sockaddr_in));
                return;
            }
            sockaddr_in6 addr = address.addr6();
            addr.sin6_port = htons(port);
            connect((const sockaddr *)&addr, sizeof(sockaddr_in6));
        }

        void bind(const sockaddr *addr, socklen_t len){
            if (::bind(m_fd, addr, len) == -1) throw BindException("bind", fd());
        }

        void bind(const IPAddress &address, int port = 0){
            if (address.family() == AF_INET){
                sockaddr_in addr = address.addr4();
                addr.sin_port = htons(port);
                bind((const sockaddr *)&addr, sizeof(sockaddr_in));
                return;
            }
            sockaddr_in6 addr = address.addr6();
            addr.sin6_port = htons(port);
            bind((const sockaddr *)&addr, sizeof(sockaddr_in6));
        }

        void disconnectThrows(bool throws){
            m_throwOnDisconnection = throws;
        }

        void listen(int backlog = 1){
            if (::listen(m_fd, backlog) == -1) throw ListenException("listen", fd());
        }

        Socket accept(struct sockaddr *addr, socklen_t *len){
            Socket socket{::accept(m_fd, addr, len)};
            if (socket.fd() == -1) throw AcceptException("accept", fd());
            socket.m_connected = true;
            return socket;
        }

        Socket accept(IPAddress &address){
            sockaddr_storage addr{};
            socklen_t len;
            Socket socket = accept((sockaddr *)&addr, &len);
            address = IPAddress((sockaddr *)&addr);
            return socket;
        }

        Socket accept(){
            return accept(nullptr, nullptr);
        }

        len_t recv(void *data, len_t len, int flags = 0){
            len_t res = ::recv(m_fd, data, len, flags);
            if (res == -1) throw ReadException("recv", fd());
            if (res == 0 && len != 0) {
                m_connected = false;
                if (m_throwOnDisconnection) throw DisconnectionException(fd());
            }
            return res;
        }

        std::vector<char> recv(len_t len, int flags = 0){
            std::vector<char> array(len);
            len_t read = recv(array.data(), len, flags);
            array.resize(read);
            return array;
        }

        len_t send(const void *data, len_t len, int flags = 0){
            len_t res = ::send(m_fd, data, len, flags);
            if (res == -1) throw WriteException("send", fd());
            return res;
        }

        len_t send(const std::vector<char> &data, int flags = 0){
            len_t sent = send(data.data(), data.size(), flags);
            return sent;
        }

        [[nodiscard]] socket_t fd() const noexcept{
            return m_fd;
        }

        void close(){
            ::close(m_fd);
            m_connected = false;
        }

        [[nodiscard]] bool connected() const noexcept{
            return m_connected;
        }

        [[nodiscard]] bool operator==(const Socket &socket) const noexcept{
            return m_fd == socket.m_fd;
        }

        [[nodiscard]] bool operator==(const socket_t &fd) const noexcept{
            return m_fd == fd;
        }

        friend bool operator==(const socket_t &fd, const Socket &socket) noexcept{
            return socket.fd() == fd;
        };
    private:
        socket_t m_fd;
        bool m_throwOnDisconnection = true;
        bool m_connected = false;
    };
}

#endif //TESTS_SOCKET_HPP
