# CSC481 Game Engine

This folder contains the **CSC481 Game Engine** and the associated game files. The engine is built using **SDL2**, **ZeroMQ** for client-server communication, and multithreaded support for handling platform movement and input simultaneously. All necessary dependencies are already set up within the project properties.

## How to Compile and Run the Game

### 1. **Open the Solution**:
   - Open **Visual Studio 2022** (or a similar C++ development environment).
   - Navigate to the folder where you unzipped the project and open the `.sln` (solution) file.

### 2. **Install Dependencies**:
   - The project uses **SDL2** for rendering and **ZeroMQ** for client-server communication. These dependencies are managed through **vcpkg**, a C++ package manager.
   - Follow the steps below to integrate **vcpkg**:

#### 2.1 **Open Terminal**:
   - Open **Visual Studio** and, within the IDE, go to **View** -> **Terminal** to open a terminal window.
   - In the terminal window, **cd** into the `vcpkg_installed` directory by navigating to the **vcpkg** directory within the project folder.

#### 2.2 **Run vcpkg Integration and Installation**:
   - Run the following commands to integrate and install the necessary dependencies:
     ```
     vcpkg integrate install
     vcpkg install
     ```

### 3. **Build the Project**:
   - After installing dependencies, return to Visual Studio and do the following:
     - Go to **Build** -> **Clean Solution** to remove any existing build artifacts.
     - Then, go to **Build** -> **Build Solution** or press **Ctrl + Shift + B** to compile the project.
     - If errors don't go away after building, close Visual Studio and reopen the solution. This should resolve any remaining issues.

### 4. **Running the Server**:
   - The game uses a client-server architecture, so you'll need to run the server in parallel with the game.
   - In Visual Studio, locate the **ServerProject** solution in the Solution Explorer.
   - **Right-click** on **ServerProject** and select **Debug** -> **Start New Instance** to launch the server.

### 5. **Choosing a Game**:
   - The engine supports three games: Platformer, Space Invaders, and Snake. Follow the steps below to select and launch a specific game:

#### 5.1 **Platformer Game (Default Setup)**:
   - The project is pre-configured to run the platformer game.
   - Simply build and run the game as described in Step 6.

#### 5.2 **Boss Dodge Game**:
   - Modify the main.h file:
    - Change #include "game.h" to #include "game2.h".
   - Modify the main.cpp file:
    - Change the Game instance to Game2 (e.g., Game2 game(renderer, reqSocket, subSocket, eventReqSocket);).
   - Rebuild the project and launch the game.

#### 5.3 **Maze Break Game**:
   - Modify the main.h file:
    - Change #include "game.h" to #include "game3.h".
   - Modify the main.cpp file:
    - Change the Game instance to Game3 (e.g., Game3 game(renderer, reqSocket, subSocket, eventReqSocket);).
   - Rebuild the project and launch the game.

### 6. **Running the Game**:
   - In Visual Studio, locate the CSC481_GameEngineProject solution in the Solution Explorer.
   - **Right-click** on **CSC481GameEngineProject** and select **Debug** -> **Start New Instance** to launch the game.

### 7. **Running on Multiple Clients**:
   - Only the Platformer Game supports multi-client gameplay.
   - You can run multiple instances of the platformer game by repeating Step 6, launching additional game clients in Debug mode.
   - The server will handle all connected clients, and player movements will be synchronized across all clients.

## Game Controls

### Platformer Game Controls:
   - Left/Right Arrow Keys: Move left or right.
   - Up Arrow Key: Jump.
   - Collision Handling: Interacts with static and moving platforms.

### Boss Dodge Game Controls:
   - Left/Right Arrow Keys: Move the player left or right.
   - Boss Behavior: Continuously fires projectiles.

### Maze Break Game Controls:
   - Arrow Keys: Move the player in the desired direction.
   - Collision Handling: Avoid hitting the walls.
   - Wall breaking: On collision with a wall, the wall is broken opening a new path.
   - Finish: Reach the green finish square without lossing all your lives.

## Issues and Troubleshooting

If you encounter any issues while compiling or running the game:

- **Missing Dependencies**: Make sure you've run both `vcpkg integrate install` and `vcpkg install` to properly link the dependencies.
- **Other Errors**: If the game fails to build or run, try restarting Visual Studio or ensuring that all steps were followed correctly.

## Authors and Developers

- **Miles Hollifield (mfhollif)**
- **Alan Skufca (anskufca)**
- **Nomar Ledesma Gonzalez (nledesm)**
