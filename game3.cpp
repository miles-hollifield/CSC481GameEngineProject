#include "game3.h"
#include "PropertyManager.h"
#include "EventManager.h"
#include "SpawnEvent.h"
#include "DeathEvent.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>

// Constructor to initialize the game
Game3::Game3(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), eventReqSocket(eventReqSocket), quit(false), gameOver(false), score(0), gameTimeline(nullptr, INITIAL_SPEED), font(nullptr), scoreTexture(nullptr), speedTexture(nullptr) {
    srand(static_cast<unsigned>(time(nullptr))); // Seed for random number generation

    // Initialize SDL_ttf for rendering text
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init error: " << TTF_GetError() << std::endl;
        quit = true; // Quit the game if TTF initialization fails
        return;
    }

    // Load the font for rendering score and speed
    font = TTF_OpenFont("./fonts/PixelPowerline-9xOK.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        quit = true; // Quit the game if font loading fails
        return;
    }

    initGameObjects(); // Initialize game objects (snake and food)
}

// Destructor to clean up resources
Game3::~Game3() {
    if (font) {
        TTF_CloseFont(font); // Free the font resource
    }
    if (scoreTexture) {
        SDL_DestroyTexture(scoreTexture); // Free the score texture
    }
    if (speedTexture) {
        SDL_DestroyTexture(speedTexture); // Free the speed texture
    }
    TTF_Quit(); // Quit SDL_ttf
}

// Initialize game objects like the snake and food
void Game3::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();
    auto& eventManager = EventManager::getInstance();

    // Register event handlers for SPAWN and DEATH events
    eventManager.registerHandler(SPAWN, [this](std::shared_ptr<Event> event) {
        auto spawnEvent = std::static_pointer_cast<SpawnEvent>(event);
        handleSpawn(spawnEvent->getObjectID());
        });

    eventManager.registerHandler(DEATH, [this](std::shared_ptr<Event> event) {
        auto deathEvent = std::static_pointer_cast<DeathEvent>(event);
        handleDeath(deathEvent->getObjectID());
        });

    // Clear the snake body and reset its position
    snakeBody.clear();
    for (int i = 0; i < INITIAL_SNAKE_LENGTH; ++i) {
        snakeBody.push_back({ SCREEN_WIDTH / 2 / GRID_SIZE - i, (SCREEN_HEIGHT / 2 + SCORE_ZONE_HEIGHT) / GRID_SIZE });
    }
    direction = { 1, 0 }; // Start moving to the right

    // Place the first food on the grid
    placeFood();
}

// Place food at a random position, ensuring it does not overlap with the snake
void Game3::placeFood() {
    auto& propertyManager = PropertyManager::getInstance();

    // Generate random grid coordinates for the food
    SDL_Point newFoodPosition;
    do {
        newFoodPosition.x = rand() % (SCREEN_WIDTH / GRID_SIZE);
        newFoodPosition.y = rand() % ((SCREEN_HEIGHT - SCORE_ZONE_HEIGHT) / GRID_SIZE) + (SCORE_ZONE_HEIGHT / GRID_SIZE);
    } while (checkCollision(newFoodPosition)); // Ensure food does not spawn on the snake

    // Destroy the previous food object if it exists
    if (propertyManager.hasObject(foodID)) {
        propertyManager.destroyObject(foodID);
    }

    // Create a new food object with updated properties
    foodID = propertyManager.createObject();
    propertyManager.addProperty(foodID, "Rect", std::make_shared<RectProperty>(
        newFoodPosition.x * GRID_SIZE, newFoodPosition.y * GRID_SIZE, GRID_SIZE, GRID_SIZE));
    propertyManager.addProperty(foodID, "Render", std::make_shared<RenderProperty>(255, 0, 0)); // Red food

    std::cout << "New food placed at: (" << newFoodPosition.x << ", " << newFoodPosition.y << ")" << std::endl;

    // Raise a spawn event for the new food
    EventManager::getInstance().raiseEvent(std::make_shared<SpawnEvent>(foodID, &gameTimeline));
}


// Check if a given position collides with the snake
bool Game3::checkCollision(const SDL_Point& position) {
    for (const auto& segment : snakeBody) {
        if (segment.x == position.x && segment.y == position.y) {
            return true; // Collision detected
        }
    }
    return false; // No collision
}

// Main game loop
void Game3::run() {
    while (!quit) {
        if (gameOver) {
            resetGame(); // Reset the game if it is over
        }

        handleEvents(); // Handle player input
        update();       // Update game state
        render();       // Render game objects

        SDL_Delay(100); // Delay for consistent game speed
    }
}

// Handle player input events
void Game3::handleEvents() {
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true; // Exit the game loop if quit event is detected
        }

        // Handle arrow key inputs for changing snake direction
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

// Update the game state, including snake movement and food collision
void Game3::update() {
    // Calculate the new position for the snake's head
    SDL_Point newHead = { snakeBody.front().x + direction.x, snakeBody.front().y + direction.y };

    // Check for collisions with the walls or the snake itself
    if (newHead.x < 0 || newHead.y < SCORE_ZONE_HEIGHT / GRID_SIZE || newHead.x >= SCREEN_WIDTH / GRID_SIZE || newHead.y >= SCREEN_HEIGHT / GRID_SIZE || checkCollision(newHead)) {
        gameOver = true; // End the game if a collision is detected
        return;
    }

    // Add the new head position to the snake
    snakeBody.push_front(newHead);

    auto& propertyManager = PropertyManager::getInstance();
    auto foodRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(foodID, "Rect"));
    if (newHead.x == foodRect->x / GRID_SIZE && newHead.y == foodRect->y / GRID_SIZE) {
        score += FOOD_SCORE; // Increase the score
        placeFood();         // Place a new food item
        gameTimeline.changeTic(gameTimeline.getTic() + 0.05f); // Speed up the game
    }
    else {
        snakeBody.pop_back(); // Remove the tail segment if no food is eaten
    }

    sendPlayerUpdate(); // Send the updated game state to the server
}

// Render the game objects (snake, food, score, and speed)
void Game3::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Clear the screen with black
    SDL_RenderClear(renderer);

    // Render the snake
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green snake
    for (const auto& segment : snakeBody) {
        SDL_Rect rect = { segment.x * GRID_SIZE, segment.y * GRID_SIZE, GRID_SIZE, GRID_SIZE };
        SDL_RenderFillRect(renderer, &rect);
    }

    // Render the food
    auto& propertyManager = PropertyManager::getInstance();
    auto foodRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(foodID, "Rect"));
    SDL_Rect sdlFoodRect = { foodRect->x, foodRect->y, foodRect->w, foodRect->h };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red food
    SDL_RenderFillRect(renderer, &sdlFoodRect);

    renderScoreText(); // Render score and speed
    SDL_RenderPresent(renderer); // Display the rendered frame
}

// Render the score and speed text
void Game3::renderScoreText() {
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    if (speedTexture) SDL_DestroyTexture(speedTexture);

    std::stringstream scoreStream;
    scoreStream << "Score: " << score; // Format the score text
    std::stringstream speedStream;
    speedStream << "Speed: " << std::fixed << std::setprecision(2) << gameTimeline.getTic(); // Format the speed text

    SDL_Color white = { 255, 255, 255, 255 }; // White text color
    SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, scoreStream.str().c_str(), white);
    SDL_Surface* speedSurface = TTF_RenderText_Solid(font, speedStream.str().c_str(), white);

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
        memcpy(&clientId, reply.data(), sizeof(clientId)); // Assign client ID on the first connection
        std::cout << "Connected to server with client ID: " << clientId << std::endl;
    }
}

// Handle spawn events
void Game3::handleSpawn(int objectID) {
    std::cout << "Spawn event triggered for object ID: " << objectID << std::endl;

    // Ensure the spawned object is food
    if (objectID == foodID) {
        std::cout << "Spawn event handled for food object. Food ID: " << objectID << std::endl;
    }
    else {
        std::cerr << "Unhandled spawn event for object ID: " << objectID << std::endl;
    }
}


// Handle death events
void Game3::handleDeath(int objectID) {
    std::cout << "Death event triggered for object ID: " << objectID << std::endl;

    auto& propertyManager = PropertyManager::getInstance();

    // Handle snake death
    if (objectID == snakeBody.front().x * GRID_SIZE + snakeBody.front().y) {
        std::cout << "Snake collided with itself or the wall. Game over." << std::endl;
        gameOver = true;
        return;
    }

    // Handle food consumption
    if (objectID == foodID) {
        std::cout << "Food consumed. Spawning a new food object." << std::endl;

        // Reuse `placeFood` to spawn new food and raise a spawn event
        placeFood();
        EventManager::getInstance().raiseEvent(std::make_shared<SpawnEvent>(foodID, &gameTimeline));
    } else {
        std::cerr << "Unhandled death event for object ID: " << objectID << std::endl;
    }
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
