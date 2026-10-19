# Multi-Agent C++ Simulation

A multi-agent system simulation developed in C++ showcasing autonomous unit behavior, navigation, and pathfinding algorithms within a dynamic environment.

## Overview
This project simulates specialized autonomous agents (e.g., Soldiers, Medics, Supply Soldiers) operating cooperatively. The agents navigate complex terrain using advanced pathfinding techniques while considering environmental constraints and threat levels.

## Key Features & Algorithms
*   **A* Pathfinding with Safety Maps:** Agents calculate optimal routes while actively avoiding high-risk areas using a dynamically updated safety map.
*   **BFS Exploration:** Utilizes Breadth-First Search for "fog-of-war" exploration and environment discovery.
*   **Role-Based Agents:** Demonstrates polymorphic design with distinct agent classes (`Soldier`, `Medic`, `SupplySoldier`, `Commander`) inheriting from a base `Warrior` class.
*   **Line of Sight & Visibility:** Implements algorithms to calculate visibility and line-of-sight between agents and targets.
*   **Graphics:** Uses OpenGL/FreeGLUT for visualizing the simulation.

## Tech Stack
*   **Language:** C++
*   **Graphics Library:** FreeGLUT / GLEW

## Architecture
*   Object-Oriented Design (OOP) principles heavily applied.
*   Separation of concerns between logic (`PathFinding`, `Visibility`) and entities (`Soldier`, `Medic`, etc.).

## How to Build and Run

### Prerequisites
* **Visual Studio** (2019/2022 recommended) with the **"Desktop development with C++"** workload installed.
* The project already includes the necessary `freeglut` and `glew` libraries, so no additional installations are required.

### Steps
1. **Clone the repository:**
   ```bash
   git clone [https://github.com/netabm/Multi-Agent_Cpp.git](https://github.com/netabm/Multi-Agent_Cpp.git)
