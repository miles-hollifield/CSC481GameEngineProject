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

### 5. **Running the Game**:
   - In Visual Studio, locate the **CSC481_GameEngineProject** solution in the Solution Explorer.
   - **Right-click** on **CSC481_GameEngineProject** and select **Debug** -> **Start New Instance** to launch the game.

### 6. **Running on Multiple Clients**:
   - You can run multiple instances of the game by repeating step 5, launching additional game clients in **Debug** mode.
   - The server will handle all connected clients, and player movements will be synchronized across all clients.

## Game Controls

- **Arrow Keys**: Move the controllable object left and right. Jump with the up arrow key.
- **Collision Handling**: The object interacts with static and moving platforms.

## Issues and Troubleshooting

If you encounter any issues while compiling or running the game:

- **Missing Dependencies**: Make sure you've run both `vcpkg integrate install` and `vcpkg install` to properly link the dependencies.
- **Other Errors**: If the game fails to build or run, try restarting Visual Studio or ensuring that all steps were followed correctly.

## Authors and Developers

- **Miles Hollifield (mfhollif)**
- **Alan Skufca (anskufca)**
- **Nomar Ledesma Gonzalez (nledesm)**
