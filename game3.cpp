#include "game3.h"
#include "PropertyManager.h"
#include "EventManager.h"
#include "SpawnEvent.h"
#include "DeathEvent.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>

// Constructor
Game3::Game3(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), eventReqSocket(eventReqSocket), quit(false), gameOver(false), score(0), gameTimeline(nullptr, INITIAL_SPEED), font(nullptr), scoreTexture(nullptr), speedTexture(nullptr) {
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

    initGameObjects();
}

// Destructor
Game3::~Game3() {
    // Clean up font and textures
    if (font) {
        TTF_CloseFont(font);
    }
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture);
    }
    if (speedTexture) {
        SDL_DestroyTexture(speedTexture);
    }
    TTF_Quit();
}

// Initialize game objects
void Game3::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();
    auto& eventManager = EventManager::getInstance();

    // Register event handlers
    eventManager.registerHandler(SPAWN, [this](std::shared_ptr<Event> event) {
        auto spawnEvent = std::static_pointer_cast<SpawnEvent>(event);
        handleSpawn(spawnEvent->getObjectID());
        });

    eventManager.registerHandler(DEATH, [this](std::shared_ptr<Event> event) {
        auto deathEvent = std::static_pointer_cast<DeathEvent>(event);
        handleDeath(deathEvent->getObjectID());
        });

    // Clear snake and food
    snakeBody.clear();

    // Initialize snake
    for (int i = 0; i < INITIAL_SNAKE_LENGTH; ++i) {
        snakeBody.push_back({ SCREEN_WIDTH / 2 / GRID_SIZE - i, (SCREEN_HEIGHT / 2 + SCORE_ZONE_HEIGHT) / GRID_SIZE });
    }
    direction = { 1, 0 }; // Start moving right

    // Place the first food
    placeFood();
}

// Place food at a random position using the game object model
void Game3::placeFood() {
    auto& propertyManager = PropertyManager::getInstance();

    food.x = rand() % (SCREEN_WIDTH / GRID_SIZE);
    food.y = rand() % ((SCREEN_HEIGHT - SCORE_ZONE_HEIGHT) / GRID_SIZE);

    while (checkCollision(food)) {
        food.x = rand() % (SCREEN_WIDTH / GRID_SIZE);
        food.y = rand() % ((SCREEN_HEIGHT - SCORE_ZONE_HEIGHT) / GRID_SIZE);
    }

    if (propertyManager.hasObject(foodID)) {
        propertyManager.destroyObject(foodID);
    }

    foodID = propertyManager.createObject();
    propertyManager.addProperty(foodID, "Rect", std::make_shared<RectProperty>(
        food.x * GRID_SIZE, food.y * GRID_SIZE, GRID_SIZE, GRID_SIZE));
    propertyManager.addProperty(foodID, "Render", std::make_shared<RenderProperty>(255, 0, 0)); // Red food
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
        if (gameOver) {
            resetGame();
        }

        handleEvents();
        update();
        render();

        SDL_Delay(100);
    }
}

// Handle input events
void Game3::handleEvents() {
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }

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
}

// Update game state
void Game3::update() {
    SDL_Point newHead = { snakeBody.front().x + direction.x, snakeBody.front().y + direction.y };

    if (newHead.x < 0 || newHead.y < SCORE_ZONE_HEIGHT / GRID_SIZE || newHead.x >= SCREEN_WIDTH / GRID_SIZE || newHead.y >= SCREEN_HEIGHT / GRID_SIZE || checkCollision(newHead)) {
        gameOver = true;
        return;
    }

    snakeBody.push_front(newHead);

    auto& propertyManager = PropertyManager::getInstance();
    auto foodRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(foodID, "Rect"));
    if (newHead.x == foodRect->x / GRID_SIZE && newHead.y == foodRect->y / GRID_SIZE) {
        score += FOOD_SCORE;
        placeFood();
        gameTimeline.changeTic(gameTimeline.getTic() + 0.05f); // Increase speed
    }
    else {
        snakeBody.pop_back();
    }

    sendPlayerUpdate();
}

// Render game objects
void Game3::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (const auto& segment : snakeBody) {
        SDL_Rect rect = { segment.x * GRID_SIZE, segment.y * GRID_SIZE, GRID_SIZE, GRID_SIZE };
        SDL_RenderFillRect(renderer, &rect);
    }

    auto& propertyManager = PropertyManager::getInstance();
    auto foodRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(foodID, "Rect"));
    SDL_Rect sdlFoodRect = { foodRect->x, foodRect->y, foodRect->w, foodRect->h };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red food
    SDL_RenderFillRect(renderer, &sdlFoodRect);

    renderScoreText();
    SDL_RenderPresent(renderer);
}

// Render score text
void Game3::renderScoreText() {
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    if (speedTexture) SDL_DestroyTexture(speedTexture);

    std::stringstream scoreStream;
    scoreStream << "Score: " << score;
    std::string scoreStr = scoreStream.str();

    std::stringstream speedStream;
    speedStream << "Speed: " << std::fixed << std::setprecision(2) << gameTimeline.getTic();
    std::string speedStr = speedStream.str();

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreStr.c_str(), white);
    SDL_Surface* speedSurface = TTF_RenderText_Solid(font, speedStr.c_str(), white);

    if (scoreSurface) {
        scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        SDL_FreeSurface(scoreSurface);
    }

    if (speedSurface) {
        speedTexture = SDL_CreateTextureFromSurface(renderer, speedSurface);
        SDL_FreeSurface(speedSurface);
    }

    SDL_Rect scoreRect = { 10, 10, 0, 0 };
    SDL_QueryTexture(scoreTexture, nullptr, nullptr, &scoreRect.w, &scoreRect.h);
    SDL_RenderCopy(renderer, scoreTexture, nullptr, &scoreRect);

    SDL_Rect speedRect = { scoreRect.x + scoreRect.w + 20, scoreRect.y, 0, 0 };
    SDL_QueryTexture(speedTexture, nullptr, nullptr, &speedRect.w, &speedRect.h);
    SDL_RenderCopy(renderer, speedTexture, nullptr, &speedRect);
}

// Send player updates to the server
void Game3::sendPlayerUpdate() {
    static int clientId = -1;

    struct PlayerState {
        int x, y;        // Snake head position
        int score;       // Current score
        GameType gameType; // Game type
    };

    PlayerState state = { snakeBody.front().x, snakeBody.front().y, score, SNAKE };

    zmq::message_t request(sizeof(clientId) + sizeof(PlayerState));
    memcpy(request.data(), &clientId, sizeof(clientId));
    memcpy(static_cast<char*>(request.data()) + sizeof(clientId), &state, sizeof(PlayerState));

    reqSocket.send(request, zmq::send_flags::none);

    zmq::message_t reply;
    reqSocket.recv(reply, zmq::recv_flags::none);

    if (clientId == -1) {
        memcpy(&clientId, reply.data(), sizeof(clientId));
        std::cout << "Connected to server with client ID: " << clientId << std::endl;
    }
}

// Handle spawn events
void Game3::handleSpawn(int objectID) {
    std::cout << "Spawn event triggered for object ID: " << objectID << std::endl;
}

// Handle death events
void Game3::handleDeath(int objectID) {
    std::cout << "Death event triggered for object ID: " << objectID << std::endl;
}

// Reset the game
void Game3::resetGame() {
    auto& propertyManager = PropertyManager::getInstance();

    // Clear the snake body
    snakeBody.clear();

    // Remove the food object if it exists
    if (propertyManager.hasObject(foodID)) {
        propertyManager.destroyObject(foodID);
    }

    // Reset game state variables
    score = 0;
    gameOver = false;
    gameTimeline.changeTic(INITIAL_SPEED); // Reset the speed

    // Reinitialize the snake
    for (int i = 0; i < INITIAL_SNAKE_LENGTH; ++i) {
        snakeBody.push_back({ SCREEN_WIDTH / 2 / GRID_SIZE - i, (SCREEN_HEIGHT / 2 + SCORE_ZONE_HEIGHT) / GRID_SIZE });
    }
    direction = { 1, 0 }; // Reset direction to move right

    // Reinitialize the food
    placeFood();

    std::cout << "Game reset: Snake reinitialized, food placed, and score reset." << std::endl;
}
