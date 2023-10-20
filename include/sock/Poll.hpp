#ifndef TESTS_POLL_HPP
#define TESTS_POLL_HPP

#include <vector>
#include <algorithm>
#include "Socket.hpp"
#include <poll.h>

class PollException : public std::exception {
public:
    PollException()= default;

    [[nodiscard]] const char *what() const noexcept override{
        return "poll";
    }
};

class PollList {
    struct PollResult {
    public:
        PollResult() : PollResult(0, sock::Socket(0)){

        }

        explicit PollResult(short flags, const sock::Socket &socket) : m_flags(flags), m_socket(socket){

        }

        [[nodiscard]] bool canRead() const{
            return m_flags & POLLIN;
        }

        [[nodiscard]] bool canWrite() const{
            return m_flags & POLLOUT;
        }

        [[nodiscard]] bool closed() const{
            return m_flags & POLLRDHUP;
        }

        [[nodiscard]] bool hanged() const{
            return m_flags & POLLHUP;
        }

        [[nodiscard]] bool invalid() const{
            return m_flags & POLLNVAL;
        }

        [[nodiscard]] bool has(short flag) const{
            return m_flags & flag;
        }

        [[nodiscard]] sock::Socket socket() const{
            return m_socket;
        }
    private:
        short m_flags;
        sock::Socket m_socket;
    };

    struct PollIterator {
    public:
        explicit PollIterator(const pollfd *ptr) : m_ptr(ptr){

        }

        sock::Socket operator*(){
            return sock::Socket(m_ptr->fd);
        }

        PollList::PollIterator &operator++(){
            m_ptr++;
            return *this;
        }

        [[nodiscard]] bool operator==(const PollIterator &iterator){
            return m_ptr == iterator.m_ptr;
        }
    private:
        const pollfd *m_ptr;
    };
public:
    void add(const sock::Socket &socket, short events){
        m_sockets.push_back(pollfd{socket.fd(), events, 0});
    }

    void remove(const sock::Socket &socket){
        std::erase_if(m_sockets, [&socket](const pollfd &fd){ return fd.fd == socket.fd(); });
    }

    int poll(int timeout = -1){
        for (pollfd &fd : m_sockets) fd.revents = 0;
        int res = ::poll(m_sockets.data(), m_sockets.size(), timeout);
        if (res == -1) throw PollException();
        return res;
    }

    PollResult operator[](const sock::Socket &socket){
        auto it = std::find_if(m_sockets.begin(), m_sockets.end(), [&socket](const pollfd &fd){ return fd.fd == socket.fd(); });
        if (it == m_sockets.end())
            return PollResult(0, socket);
        return PollResult(it->revents, socket);
    }

    pollfd operator[](size_t i){
        return m_sockets[i];
    }

    [[nodiscard]] std::vector<PollResult> results() const{
        std::vector<PollResult> results(m_sockets.size());
        std::transform(m_sockets.begin(), m_sockets.end(), results.begin(), [](const pollfd &fd){ return PollResult(fd.revents, sock::Socket(fd.fd)); });
        return results;
    }

    [[nodiscard]] std::vector<sock::Socket> sockets() const{
        std::vector<sock::Socket> sockets(m_sockets.size());
        std::transform(m_sockets.begin(), m_sockets.end(), sockets.begin(), [](const pollfd &fd){ return sock::Socket(fd.fd); });
        return sockets;
    }
private:
    std::vector<pollfd> m_sockets;
};

#endif //TESTS_POLL_HPP
