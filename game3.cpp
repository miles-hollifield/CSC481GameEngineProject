#include "game3.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>

// Define a safe zone for the score display
constexpr int SCORE_ZONE_HEIGHT = 50;

// Constructor
Game3::Game3(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), eventReqSocket(eventReqSocket), quit(false), gameOver(false), score(0), gameTimeline(nullptr, INITIAL_SPEED), font(nullptr), scoreTexture(nullptr) {
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
    if (font) {
        TTF_CloseFont(font);
    }
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture);
    }
    TTF_Quit();
}

// Initialize game objects
void Game3::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

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

// Place food at a random position
void Game3::placeFood() {
    food.x = rand() % (SCREEN_WIDTH / GRID_SIZE);
    food.y = rand() % ((SCREEN_HEIGHT - SCORE_ZONE_HEIGHT) / GRID_SIZE) + (SCORE_ZONE_HEIGHT / GRID_SIZE); // Exclude score zone

    // Ensure food is not placed on the snake
    while (checkCollision(food)) {
        food.x = rand() % (SCREEN_WIDTH / GRID_SIZE);
        food.y = rand() % ((SCREEN_HEIGHT - SCORE_ZONE_HEIGHT) / GRID_SIZE) + (SCORE_ZONE_HEIGHT / GRID_SIZE); // Exclude score zone
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
}

// Update game state
void Game3::update() {
    updateGameObjects();
}

// Update snake and food
void Game3::updateGameObjects() {
    // Calculate new head position
    SDL_Point newHead = { snakeBody.front().x + direction.x, snakeBody.front().y + direction.y };

    // Check for collisions with walls or itself
    if (newHead.x < 0 || newHead.y < SCORE_ZONE_HEIGHT / GRID_SIZE || newHead.x >= SCREEN_WIDTH / GRID_SIZE || newHead.y >= SCREEN_HEIGHT / GRID_SIZE || checkCollision(newHead)) {
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
