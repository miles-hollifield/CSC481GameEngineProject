#include "game3.h"
#include "PropertyManager.h"
#include "ThreadManager.h"
#include <iostream>
#include <cstring>
#include "EventManager.h"
#include "DeathEvent.h"
#include "SpawnEvent.h"

// Constructor
Game3::Game3(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), eventReqSocket(eventReqSocket), quit(false),
    gameOver(false), score(0), gameTimeline(nullptr, INITIAL_SPEED), font(nullptr), scoreTexture(nullptr), clientId(-1) {
    srand(static_cast<unsigned>(time(nullptr))); // Seed for random number generation

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init error: " << TTF_GetError() << std::endl;
        quit = true;
        return;
    }

    // Load font
    font = TTF_OpenFont("./fonts/PixelPowerline-9xOK.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        quit = true;
        return;
    }

    registerWithServer();
    initGameObjects();
}

// Destructor
Game3::~Game3() {
    if (font) {
        TTF_CloseFont(font);
    }
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture);
    }
    TTF_Quit();
}

// Register the client with the server
void Game3::registerWithServer() {
    zmq::message_t request(sizeof(clientId));
    memcpy(request.data(), &clientId, sizeof(clientId));
    reqSocket.send(request, zmq::send_flags::none);

    zmq::message_t reply;
    reqSocket.recv(reply);
    memcpy(&clientId, reply.data(), sizeof(clientId));

    std::cout << "Assigned client ID: " << clientId << std::endl;
}

// Initialize game objects
void Game3::initGameObjects() {
    snakeBody.clear();

    // Initialize snake
    for (int i = 0; i < INITIAL_SNAKE_LENGTH; ++i) {
        snakeBody.push_back({ SCREEN_WIDTH / 2 / GRID_SIZE - i, SCREEN_HEIGHT / 2 / GRID_SIZE });
    }
    direction = { 1, 0 }; // Start moving right

    // Place the first food
    placeFood();
}

// Place food at a random position
void Game3::placeFood() {
    food.x = rand() % (SCREEN_WIDTH / GRID_SIZE);
    food.y = rand() % (SCREEN_HEIGHT / GRID_SIZE);

    // Ensure food is not placed on the snake
    while (checkCollision(food)) {
        food.x = rand() % (SCREEN_WIDTH / GRID_SIZE);
        food.y = rand() % (SCREEN_HEIGHT / GRID_SIZE);
    }
}

// Check for collisions
bool Game3::checkCollision(const SDL_Point& position) {
    for (const auto& segment : snakeBody) {
        if (segment.x == position.x && segment.y == position.y) {
            return true;
        }
    }
    return false;
}

// Main game loop
void Game3::run() {
    while (!quit) {
        try {
            if (gameOver) {
                initGameObjects();
                score = 0;
                gameTimeline.changeTic(INITIAL_SPEED);
                gameOver = false;
            }

            handleEvents();
            receiveServerUpdates(); // Integrate server updates
            update();
            render();

            SDL_Delay(static_cast<int>(1000 / (gameTimeline.getTic() * 10))); // Adjust delay for speed
        }
        catch (const std::exception& ex) {
            std::cerr << "Exception in main loop: " << ex.what() << std::endl;
            quit = true;
        }
    }
}

// Handle input events
void Game3::handleEvents() {
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }

        // Handle arrow keys for direction
        const Uint8* keystates = SDL_GetKeyboardState(nullptr);
        if (keystates[SDL_SCANCODE_UP] && direction.y == 0) {
            direction = { 0, -1 };
        }
        else if (keystates[SDL_SCANCODE_DOWN] && direction.y == 0) {
            direction = { 0, 1 };
        }
        else if (keystates[SDL_SCANCODE_LEFT] && direction.x == 0) {
            direction = { -1, 0 };
        }
        else if (keystates[SDL_SCANCODE_RIGHT] && direction.x == 0) {
            direction = { 1, 0 };
        }
    }

    sendPlayerUpdate(); // Send player state to server
}

// Send player state to the server
void Game3::sendPlayerUpdate() {
    zmq::message_t request(sizeof(clientId) + sizeof(PlayerPosition));

    PlayerPosition pos;
    pos.x = snakeBody.front().x;
    pos.y = snakeBody.front().y;
    pos.score = score;

    memcpy(request.data(), &clientId, sizeof(clientId));
    memcpy(static_cast<char*>(request.data()) + sizeof(clientId), &pos, sizeof(pos));

    reqSocket.send(request, zmq::send_flags::none);
}

// Receive updates from the server
void Game3::receiveServerUpdates() {
    zmq::message_t update;
    if (subSocket.recv(update, zmq::recv_flags::dontwait)) {
        char* buffer = static_cast<char*>(update.data());
        while (buffer < static_cast<char*>(update.data()) + update.size()) {
            int id;
            PlayerPosition pos;
            memcpy(&id, buffer, sizeof(id));
            buffer += sizeof(id);
            memcpy(&pos, buffer, sizeof(pos));
            buffer += sizeof(pos);

            allPlayers[id] = pos; // Update all players' positions
        }
    }
}

// Update game state
void Game3::update() {
    updateGameObjects();
}

// Update game objects
void Game3::updateGameObjects() {
    // Calculate new head position
    SDL_Point newHead = { snakeBody.front().x + direction.x, snakeBody.front().y + direction.y };

    // Check for collisions with walls or itself
    if (newHead.x < 0 || newHead.y < 0 || newHead.x >= SCREEN_WIDTH / GRID_SIZE || newHead.y >= SCREEN_HEIGHT / GRID_SIZE || checkCollision(newHead)) {
        gameOver = true;
        return;
    }

    // Add new head
    snakeBody.push_front(newHead);

    // Check if the snake eats the food
    if (newHead.x == food.x && newHead.y == food.y) {
        score += FOOD_SCORE;
        placeFood();
        gameTimeline.changeTic(gameTimeline.getTic() + 0.05f); // Increase speed
    }
    else {
        // Remove the tail if no food is eaten
        snakeBody.pop_back();
    }
}

// Render game objects
void Game3::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render snake
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (const auto& segment : snakeBody) {
        SDL_Rect rect = { segment.x * GRID_SIZE, segment.y * GRID_SIZE, GRID_SIZE, GRID_SIZE };
        SDL_RenderFillRect(renderer, &rect);
    }

    // Render food
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect foodRect = { food.x * GRID_SIZE, food.y * GRID_SIZE, GRID_SIZE, GRID_SIZE };
    SDL_RenderFillRect(renderer, &foodRect);

    // Render score text
    renderScoreText();

    SDL_RenderPresent(renderer);
}

// Render score text
void Game3::renderScoreText() {
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture);
        scoreTexture = nullptr;
    }

    std::stringstream ss;
    ss << "Score: " << score;
    std::string scoreStr = ss.str();

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreStr.c_str(), white);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid error: " << TTF_GetError() << std::endl;
        return;
    }

    scoreTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    if (!scoreTexture) {
        std::cerr << "SDL_CreateTextureFromSurface error: " << SDL_GetError() << std::endl;
        return;
    }

    scoreRect.x = 10;
    scoreRect.y = 10;
    SDL_QueryTexture(scoreTexture, nullptr, nullptr, &scoreRect.w, &scoreRect.h);
    SDL_RenderCopy(renderer, scoreTexture, nullptr, &scoreRect);
}
