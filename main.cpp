#include <iostream>
#include <SFML/Graphics.hpp>
#include <cmath>

int main() {
    sf::RenderWindow window{{800, 600}, "Pong!", sf::Style::Titlebar | sf::Style::Close};
    sf::Event event{};

    float player1PadPosition = 300;
    float player2PadPosition = 340;

    sf::Vector2f ballPosition{400, 300};
    float ballAngle = 0;
    float ballSpeed = 100;

    float deltaTime;
    sf::Clock clock;

    sf::RectangleShape ballShape({10, 10}), player1PadShape({10, 80}), player2PadShape({10, 80});
    ballShape.setOrigin(5, 5);
    player1PadShape.setOrigin(5, 40);
    player2PadShape.setOrigin(5, 40);
    while (window.isOpen()){
        while (window.pollEvent(event)){
            if (event.type == sf::Event::Closed) window.close();
        }
        deltaTime = clock.restart().asSeconds();

        ballPosition += ballSpeed * deltaTime * sf::Vector2f{std::cos(ballAngle), -std::sin(ballAngle)};
        ballShape.setPosition(ballPosition);

        player1PadShape.setPosition(20, player1PadPosition);
        player2PadShape.setPosition(780, player2PadPosition);

        sf::FloatRect ballBounds = ballShape.getGlobalBounds();

        if (ballPosition.y < 5 || ballPosition.y > 795){
            if (ballPosition.y < 5) ballPosition.y = 5;
            if (ballPosition.y > 795) ballPosition.y = 795;
            ballAngle = M_PIf * 2.f - ballAngle;
        }

        if (ballBounds.intersects(player1PadShape.getGlobalBounds())){
            ballPosition.x = 30;
            float distance = (player1PadPosition - ballPosition.y) / 40.f;
            float angle = distance * M_PI_4f;
            ballAngle = angle;
        }

        if (ballBounds.intersects(player2PadShape.getGlobalBounds())){
            ballPosition.x = 770;
            float distance = (player2PadPosition - ballPosition.y) / 40.f;
            float angle = distance * M_PI_4f;
            ballAngle = -angle + M_PIf;
        }

        window.clear();
        window.draw(player1PadShape);
        window.draw(player2PadShape);
        window.draw(ballShape);
        window.display();
    }
    return 0;
}
