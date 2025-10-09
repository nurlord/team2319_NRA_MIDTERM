# Assignment 1

### by Nurzat Samatov SE-2319

## Project Structure

ns2319_a1/
├─ include/ # GLFW, KHR, GLAD, glm headers
├─ src/ # Source files
│ ├─ red_triangle.cpp
│ ├─ blue_square.cpp
│ ├─ glad.c
│ └─ task2_picture.cpp
├─ shaders/ # Vertex and fragment shaders
├─ Makefile # Builds all programs
└─ README.md

### Tested on Ubuntu 24.04 LTS

## Install dependencies

```bash
sudo apt update && sudo apt install -y \
    build-essential \
    g++ \
    cmake \
    libglfw3-dev \
    libgl1-mesa-dev \
    libx11-dev \
    libxrandr-dev \
    libxi-dev \
    mesa-utils \
    git
```

## Build project

```bash
make
```

## Run apps

```bash
./red_triangle
```

```bash
./blue_square

```

```bash
./task2
```
