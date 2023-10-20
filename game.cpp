#include <iostream>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <sock/Poll.hpp>
#include <tcp/TcpClient.hpp>
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

Message fetchMessage(sock::Socket socket){
    Message message;
    socket.recv(&message.header, 5, MSG_WAITALL);
    if (message.header.length > 0) {
        message.data.resize(message.header.length);
        socket.recv(message.data.data(), message.header.length, MSG_WAITALL);
    }
    return message;
}

void writeMessage(sock::Socket socket, MessageType type, unsigned int length, const void *data){
    socket.send(&type, 1);
    socket.send(&length, 4);
    socket.send(data, length);
}

struct Position {
    double x, y;
};

int main(int argc, char **argv) {
    tcp::TcpClient client;
    int t = 1;
    setsockopt(client.fd(), IPPROTO_TCP, TCP_NODELAY, &t, 4);
    if (argc < 2) {
        std::cout << "Connecting to localhost :3\n";
        client.connect(sock::IPAddress::parse("127.0.0.1"), 25565);
        std::cout << "Connected to localhost! ^w^\n";
    }
    else {
        std::cout << "Connecting to " << argv[1] << " :3\n";
        client.connect(sock::IPAddress::parse(argv[1]), 25565);
        std::cout << "Connected to " << argv[1] << "! ^w^\n";
    }
    sf::RenderWindow window{{800, 600}, "Pong!", sf::Style::Titlebar | sf::Style::Close};
    sf::Event event{};

    float player1PadPosition = 0;
    float player2PadPosition = 0;

    int player1Score;
    int player2Score;

    sf::Vector2f ballPosition{400, 300};

    sf::Clock clock;

    sf::RectangleShape ballShape({10, 10}), player1PadShape({10, 80}), player2PadShape({10, 80});
    ballShape.setOrigin(5, 5);
    player1PadShape.setOrigin(5, 40);
    player2PadShape.setOrigin(5, 40);

    PollList pollList;
    pollList.add(client, POLLIN);

    sf::Font openSans;
    openSans.loadFromFile("./assets/OpenSans.ttf");

    sf::Text player1ScoreText{"0", openSans, 30}, player2ScoreText{"0", openSans, 30};
    player1ScoreText.setPosition(800 / 5. - 10, 35);
    player2ScoreText.setPosition(800 / 5. * 4 - 10, 35);

    sf::Text gameStartingText{"Game is starting, please wait...", openSans, 30};
    gameStartingText.setPosition(200, 40);

    int movePad = 0;
    bool wPressed = false, sPressed = false;
    bool gameStarted = false;
    while (window.isOpen()){
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed){
                if (event.key.code == sf::Keyboard::W) wPressed = true;
                if (event.key.code == sf::Keyboard::S) sPressed = true;
            }
            if (event.type == sf::Event::KeyReleased){
                if (event.key.code == sf::Keyboard::W) wPressed = false;
                if (event.key.code == sf::Keyboard::S) sPressed = false;
            }
        }

        movePad = -wPressed + sPressed;

        if (pollList.poll(0) != 0){
            if (pollList[client].canRead()){
                Message message = fetchMessage(client);
                if (message.header.type == BallUpdate){
                    Position position = *(Position *)message.data.data();
                    ballPosition.x = (float)position.x;
                    ballPosition.y = (float)position.y;
                }
                if (message.header.type == PadUpdate){
                    auto *pads = (double *)message.data.data();
                    player1PadPosition = (float)pads[0];
                    player2PadPosition = (float)pads[1];
                }
                if (message.header.type == ScoreUpdate){
                    auto *scores = (int *)message.data.data();
                    player1Score = scores[0];
                    player2Score = scores[1];
                    player1ScoreText.setString(std::to_string(player1Score));
                    player2ScoreText.setString(std::to_string(player2Score));
                }
                if (message.header.type == Tick){
                    if (movePad != 0) writeMessage(client, MovePad, sizeof(int), &movePad);
                }
                if (message.header.type == GameStart){
                    gameStarted = true;
                }
                if (message.header.type == GameEnd){
                    gameStarted = false;
                }
            }
        }

        ballShape.setPosition(ballPosition);

        player1PadShape.setPosition(20, player1PadPosition);
        player2PadShape.setPosition(780, player2PadPosition);

        window.clear();
        if (gameStarted) {
            window.draw(player1PadShape);
            window.draw(player2PadShape);
            window.draw(player1ScoreText);
            window.draw(player2ScoreText);
            window.draw(ballShape);
        }else{
            window.draw(gameStartingText);
        }
        window.display();
    }

    client.close();
    return 0;
}
