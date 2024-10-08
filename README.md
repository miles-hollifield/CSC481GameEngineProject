# CSC481 Game Engine

This folder contains the **CSC481 Game Engine** and the associated game files. The engine is built using **SDL2**, **ZeroMQ** for client-server communication, and multithreaded support for handling platform movement and input simultaneously. All necessary dependencies are already set up within the project properties.

## How to Compile and Run the Game

### 1. **Unzip the Folder**:
   - Extract the contents of the provided zip file to a location on your machine.

### 2. **Install Dependencies**:
   - The project uses **SDL2** for rendering and **ZeroMQ** for client-server communication. These dependencies are managed through **vcpkg**, a C++ package manager.
   - Follow the steps below to integrate **vcpkg**:
   
#### 2.1 **Open the Project in Visual Studio**:
   - Open **Visual Studio 2022** (or a similar C++ development environment).
   - In Visual Studio, open the **CSC481GameEngineProject.sln** solution by navigating to the folder where you unzipped the project and open the `.sln` (solution) file.

#### 2.2 **Run vcpkg Integration**:
   - **vcpkg** is a package manager for C++ libraries that helps manage dependencies like **SDL2** for this project. By running the integration command, Visual Studio will automatically recognize and link the required libraries without manual setup.
   - In Visual Studio, go to **View** -> **Terminal** to open a terminal window.
   - In the terminal window, navigate to the **vcpkg** directory within the project folder. Visual Studio typically opens the terminal in the project directory.
   - Run the following command to integrate **vcpkg** with Visual Studio:
     ```
     ./vcpkg integrate install
     ```
   - After running the integration command, the project will compile, but Visual Studio may still show errors in the editor. Simply restart Visual Studio and reopen the project, and the errors will disappear. This will ensure Visual Studio recognizes the installed libraries.

### 3. **Build the Project**:
   - In Visual Studio, go to **Build** -> **Clean Solution** to remove any existing build artifacts, , then go to **Build** -> **Build Solution** or press **Ctrl + Shift + B**.
   - The project will compile, and all dependencies (like SDL2 and ZeroMQ) are already configured via **vcpkg**, so no additional setup is needed.

### 4. **Running the Server**:
   - The game uses a client-server architecture, so you'll need to run the server in parallel with the game.
   - In Visual Studio, locate the **ServerProject** solution in the Solution Explorer.
   - **Right-click** on the **ServerProject** and select **Debug** -> **Start New Instance**.
   - This will start the server in debug mode, allowing it to handle incoming connections from clients running the game.

### 5. **Running the Game**:
   - In Visual Studio, locate the **CSC481_GameEngine** solution in the Solution Explorer.
   - **Right-click** on the **CSC481_GameEngine** and select **Debug** -> **Start New Instance**.
   - This will launch the game in debug mode, opening the SDL window to run the game.

### 6. **Running on Multiple Clients**:
   - You can run the game on multiple clients by launching another instance of the game in Visual Studio.

## Game Controls

- **Arrow Keys**: Move the controllable object left and right. Jump with the up arrow key.
- **Collision Handling**: The object interacts with static and moving platforms.
- **P**: Pause the game.
- **U**: Unpause the game.
- **1**: Set time to normal speed (1.0x).
- **2**: Set time to half speed (0.5x).
- **3**: Set time to double speed (2.0x).

## Issues and Troubleshooting

If you encounter any issues while compiling or running the game:

- **Missing Dependencies**: Make sure you've run `vcpkg integrate install` so Visual Studio can find the necessary dependencies.
- **Other Errors**: If the game fails to build or run, feel free to reach out for assistance.
  
## Authors and Developers
- **Miles Hollifield (mfhollif)**
- **Alan Skufca (anskufca)**
- **Nomar Ledesma Gonzalez (nledesm)**
