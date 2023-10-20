#include <sock/Poll.hpp>
#include <tcp/TcpServer.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <netinet/tcp.h>

enum MessageType : char {
    MovePad, Tick, BallUpdate, PadUpdate, ScoreUpdate, PlayerAssignment, GameStart, GameEnd
};

struct __attribute__((packed)) MessageHeader {
    MessageType type;
    unsigned int length;
};

struct Message {
    MessageHeader header{MovePad, 0};
    std::string data;
};

std::chrono::nanoseconds fetchTime(){
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
}

Message fetchMessage(sock::Socket socket){
    Message message;
    socket.recv(&message.header, 5, MSG_WAITALL);
    message.data.resize(message.header.length);
    socket.recv(message.data.data(), message.header.length, MSG_WAITALL);
    return message;
}

void writeMessage(sock::Socket socket, MessageType type, unsigned int length, const void *data){
    socket.send(&type, 1, MSG_NOSIGNAL);
    socket.send(&length, 4, MSG_NOSIGNAL);
    socket.send(data, length, MSG_NOSIGNAL);
}

void broadcastMessage(std::initializer_list<sock::Socket> sockets, MessageType type, unsigned int length, const void *data){
    for (sock::Socket socket : sockets)
        writeMessage(socket, type, length, data);
}

bool rectIntersect(double x1, double y1, double w1, double h1, double x2, double y2, double w2, double h2){
    return x1 < x2 + w2 &&
           x1 + w1 > x2 &&
           y1 < y2 + h2 &&
           y1 + h1 > y2;
}

struct Position {
    double x, y;
};

#define WIN_SIZEX 800
#define WIN_SIZEY 600
#define PAD_SIZEX 10
#define PAD_SIZEY 80
#define BALL_SIZE 10
#define PAD_OFFST 20
#define BALL_DSPD 400
#define BALL_MSPD 600
#define TPS 144

void lobbyPollMessages(sock::Socket &server, sock::Socket &player1, sock::Socket &player2){
    PollList pollList;
    pollList.add(server, POLLIN);
    if (player1 != 0) pollList.add(player1, POLLIN);
    if (player2 != 0) pollList.add(player2, POLLIN);
    while (pollList.poll(0) != 0) {
        if (pollList[server].canRead()){
            if (player1 == 0) {
                player1 = server.accept();
                int player = 1;
                writeMessage(player1, PlayerAssignment, sizeof(int), &player);
                std::cout << "[SERVER] P1 connected\n";
            }
            else if (player2 == 0) {
                player2 = server.accept();
                int player = 2;
                writeMessage(player2, PlayerAssignment, sizeof(int), &player);
                std::cout << "[SERVER] P2 connected\n";
            }
        }
        if (pollList[player1].canRead()){
            try {
                Message message = fetchMessage(player1);
            }catch(sock::DisconnectionException &){
                player1.close();
                std::cout << "[SERVER] P1 disconnected in lobby\n";
                player1 = sock::Socket{0};
                break;
            }
        }
        if (pollList[player2].canRead()){
            try {
                Message message = fetchMessage(player2);
            }catch(sock::DisconnectionException &){
                player2.close();
                std::cout << "[SERVER] P2 disconnected in lobby\n";
                player2 = sock::Socket{0};
                break;
            }
        }
    }
}

void gamePollMessages(sock::Socket &player1, sock::Socket &player2, const double &padSpeed, double &pad1, double &pad2){
    PollList pollList;
    pollList.add(player1, POLLIN);
    pollList.add(player2, POLLIN);
    while (pollList.poll(0) != 0) {
        for (const auto &result: pollList.results()) {
            if (result.hanged() || result.closed()){
                std::cout << (result.socket() == player1 ? "P1 disconnected :c\n" : "P2 disconnected :c\n");
                pollList.remove(result.socket());
                break;
            }
            if (result.canRead()) {
                Message message = fetchMessage(result.socket());
                if (message.header.type == MovePad) {
                    double &pad = (result.socket() == player1 ? pad1 : pad2);
                    pad += *(int *)(message.data.data()) * padSpeed / TPS;
                    if (pad - PAD_SIZEY / 2. < 0) pad = PAD_SIZEY / 2.;
                    if (pad + PAD_SIZEY / 2. >= WIN_SIZEY) pad = WIN_SIZEY - PAD_SIZEY / 2.;
                    double pads[2]{pad1, pad2};
                    broadcastMessage({player1, player2}, PadUpdate, sizeof(double) * 2, pads);
                }
            }
        }
    }
}

void gameUpdateBall(sock::Socket &player1, sock::Socket &player2, const double &pad1, const double &pad2, double &ballDirection, double &ballSpeed, Position &ball, int &p1Score, int &p2Score){
    ball.x += std::cos(ballDirection) * ballSpeed / TPS;
    ball.y += -std::sin(ballDirection) * ballSpeed / TPS;
    if (ball.x + BALL_SIZE / 2. < 0){
        p2Score++;
        ball.x = WIN_SIZEX / 2.;
        ball.y = WIN_SIZEY / 2.;
        ballDirection = 0;
        int scores[2]{p1Score, p2Score};
        broadcastMessage({player1, player2}, ScoreUpdate, sizeof(int) * 2, scores);
        ballSpeed = BALL_DSPD;
    }
    if (ball.x - BALL_SIZE / 2. >= WIN_SIZEX){
        p1Score++;
        ball.x = WIN_SIZEX / 2.;
        ball.y = WIN_SIZEY / 2.;
        ballDirection = M_PI;
        int scores[2]{p1Score, p2Score};
        broadcastMessage({player1, player2}, ScoreUpdate, sizeof(int) * 2, scores);
        ballSpeed = BALL_DSPD;
    }
    if (ball.y < 5){
        ball.y = 5;
        ballDirection = M_PI * 2 - ballDirection;
        ballSpeed *= 1.1;
        if (ballSpeed > BALL_MSPD) ballSpeed = BALL_MSPD;
    }
    if (ball.y > WIN_SIZEY - 5){
        ball.y = WIN_SIZEY - 5;
        ballDirection = M_PI * 2 - ballDirection;
        ballSpeed *= 1.1;
        if (ballSpeed > BALL_MSPD) ballSpeed = BALL_MSPD;
    }
    if (rectIntersect(ball.x - BALL_SIZE / 2., ball.y - BALL_SIZE / 2., BALL_SIZE, BALL_SIZE,
                      PAD_OFFST - PAD_SIZEX / 2., pad1 - PAD_SIZEY / 2., PAD_SIZEX, PAD_SIZEY)){
        ball.x = PAD_OFFST + PAD_SIZEX / 2. + BALL_SIZE / 2.;
        double distance = (pad1 - ball.y) / PAD_SIZEY * 2.;
        double angle = distance * M_PI_4;
        ballDirection = angle;
        ballSpeed *= 1.1;
        if (ballSpeed > BALL_MSPD) ballSpeed = BALL_MSPD;
    }
    if (rectIntersect(ball.x - BALL_SIZE / 2., ball.y - BALL_SIZE / 2., BALL_SIZE, BALL_SIZE,
                      WIN_SIZEX - PAD_OFFST - PAD_SIZEX / 2., pad2 - PAD_SIZEY / 2., PAD_SIZEX, PAD_SIZEY)){
        ball.x = WIN_SIZEX - PAD_OFFST - PAD_SIZEX / 2. - BALL_SIZE / 2.;
        double distance = (ball.y - pad2) / PAD_SIZEY * 2.;
        double angle = distance * M_PI_4;
        ballDirection = angle + M_PI;
        ballSpeed *= 1.1;
        if (ballSpeed > BALL_MSPD) ballSpeed = BALL_MSPD;
    }
}

int main(int argc, char **argv){
    tcp::TcpServer server;
    int t = 1;
    setsockopt(server.fd(), SOL_SOCKET, SO_REUSEADDR, &t, 4);
    setsockopt(server.fd(), IPPROTO_TCP, TCP_NODELAY, &t, 4);
    if (argc < 2) {
        std::cout << "Binding to localhost\n";
        server.bind(sock::IPAddress::parse("127.0.0.1"), 25565);
    }
    else {
        std::cout << "Binding to " << argv[1] << "\n";
        server.bind(sock::IPAddress::parse(argv[1]), 25565);
    }
    std::cout << "Listening for connections\n";
    server.listen();
    std::cout << "Server on! ^w^\n";
    sock::Socket player1, player2;
    Position ball{WIN_SIZEX / 2., WIN_SIZEY / 2.};
    double ballDirection = M_PI;
    double ballSpeed = BALL_DSPD;
    double pad1 = WIN_SIZEY / 2., pad2 = WIN_SIZEY / 2.;
    int p1Score = 0, p2Score = 0;
    double padSpeed = 350;
    bool gameRunning = false;
    std::chrono::nanoseconds lastTime = fetchTime();
    while (true){
        std::chrono::nanoseconds tickStartTime = fetchTime();
        // Here goes logic
        if (gameRunning){
            try {
                gamePollMessages(player1, player2, padSpeed, pad1, pad2);
                gameUpdateBall(player1, player2, pad1, pad2, ballDirection, ballSpeed, ball, p1Score, p2Score);
                broadcastMessage({player1, player2}, Tick, 0, nullptr);
                broadcastMessage({player1, player2}, BallUpdate, sizeof(Position), &ball);
            } catch (sock::SocketException &e){
                if (e.socket == player1) {
                    std::cout << "[SERVER] P1 disconnected\n";
                    player1.close();
                    player1 = {};
                    writeMessage(player2, GameEnd, 0, nullptr);
                }
                if (e.socket == player2) {
                    std::cout << "[SERVER] P2 disconnected\n";
                    player2.close();
                    player2 = {};
                    writeMessage(player1, GameEnd, 0, nullptr);
                }
                gameRunning = false;
            }
        }else{
            lobbyPollMessages(server, player1, player2);
            if (player1 != 0 && player2 != 0){
                std::cout << "[SERVER] Starting game\n";
                gameRunning = true;
                broadcastMessage({player1, player2}, GameStart, 0, nullptr);
                ball = {WIN_SIZEX / 2., WIN_SIZEY / 2.};
                ballDirection = M_PI;
                ballSpeed = BALL_DSPD;
                pad1 = WIN_SIZEY / 2.;
                pad2 = WIN_SIZEY / 2.;
                p1Score = 0;
                p2Score = 0;
                int scores[2]{p1Score, p2Score};
                broadcastMessage({player1, player2}, ScoreUpdate, sizeof(int) * 2, scores);
                double pads[2]{pad1, pad2};
                broadcastMessage({player1, player2}, PadUpdate, sizeof(double) * 2, pads);
            }
        }
        // Here ends logic
        size_t tps = (size_t)std::round(1 / ((double)(fetchTime() - lastTime).count() / 1000000000.));
        if (tps < TPS / 2) std::cout << "[SERVER] Server is running at less than half the set TPS (Running at " << tps << " tps)\n";
        lastTime = fetchTime();
        std::chrono::nanoseconds tookTime = fetchTime() - tickStartTime;
        std::this_thread::sleep_for(std::chrono::nanoseconds ((int)(1. / TPS * 1000000000)) - tookTime);
    }
}