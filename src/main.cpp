#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font/stb_easy_font.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Minesweeper/Shader.h"
#include "Minesweeper/Camera.h"
#include "Minesweeper/Cube.h"
#include "Minesweeper/Board.h"

#include <iostream>

// Window settings (initial only)
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods);

extern Camera camera;
extern Board board;

glm::vec3 debugRayOrigin;
glm::vec3 debugRayDir;
bool drawDebugRay = false;

static glm::mat4 makePerspectiveFromFramebuffer(GLFWwindow *w) {
  int fbW = 0, fbH = 0;
  glfwGetFramebufferSize(w, &fbW, &fbH);
  float aspect = (fbW > 0 && fbH > 0) ? (float)fbW / (float)fbH : 1.0f;
  return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

static inline glm::vec3 gridCenter(int x, int y, const Board &b) {
  // Center grid precisely: use (dim-1)/2.0f, not integer dim/2
  float gx = (x - (b.width - 1) * 0.5f) * 1.2f;
  float gy = (y - (b.height - 1) * 0.5f) * 1.2f;
  return glm::vec3(gx, gy, 0.0f);
}

void drawRay(const glm::vec3 &origin, const glm::vec3 &direction,
             Shader &shader) {
  shader.setMat4("model", glm::mat4(1.0f)); // draw in world space

  float length = 100.0f;
  glm::vec3 end = origin + direction * length;

  float vertices[] = {origin.x, origin.y, origin.z, end.x, end.y, end.z};

  unsigned int VAO = 0, VBO = 0;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glDrawArrays(GL_LINES, 0, 2);

  glDeleteBuffers(1, &VBO);
  glDeleteVertexArrays(1, &VAO);
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  (void)mods;
  if (action != GLFW_PRESS)
    return;

  // If the cursor is DISABLED (typical FPS mode), we want a center screen ray.
  bool useCenterRay =
      (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED);

  glm::vec3 rayOrigin = camera.Position;
  glm::vec3 rayWorld;

  if (useCenterRay) {
    // "Straight from my face"
    rayWorld = glm::normalize(camera.Front);
  } else {
    // Cursor picking: map cursor -> NDC using WINDOW size
    double xposD = 0.0, yposD = 0.0;
    glfwGetCursorPos(window, &xposD, &yposD);

    int winW = 0, winH = 0;
    glfwGetWindowSize(window, &winW, &winH);

    float x = 2.0f * float(xposD) / float(winW) - 1.0f;
    float y = 1.0f - 2.0f * float(yposD) / float(winH);

    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    glm::mat4 projection = makePerspectiveFromFramebuffer(window);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::mat4 view = camera.GetViewMatrix();
    rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
  }

  // --- Ray-AABB picking ---
  int hitX = -1, hitY = -1;
  float minDist = 1e9f;
  const float maxClickDistance = 100.0f;

  for (int x = 0; x < board.width; ++x) {
    for (int y = 0; y < board.height; ++y) {
      glm::vec3 c = gridCenter(x, y, board);
      glm::vec3 minB = c - glm::vec3(0.5f);
      glm::vec3 maxB = c + glm::vec3(0.5f);

      float invDirX = (rayWorld.x != 0.0f) ? 1.0f / rayWorld.x : 1e6f;
      float invDirY = (rayWorld.y != 0.0f) ? 1.0f / rayWorld.y : 1e6f;
      float invDirZ = (rayWorld.z != 0.0f) ? 1.0f / rayWorld.z : 1e6f;

      float t1 = (minB.x - rayOrigin.x) * invDirX;
      float t2 = (maxB.x - rayOrigin.x) * invDirX;
      float t3 = (minB.y - rayOrigin.y) * invDirY;
      float t4 = (maxB.y - rayOrigin.y) * invDirY;
      float t5 = (minB.z - rayOrigin.z) * invDirZ;
      float t6 = (maxB.z - rayOrigin.z) * invDirZ;

      float tmin =
          std::max({std::min(t1, t2), std::min(t3, t4), std::min(t5, t6)});
      float tmax =
          std::min({std::max(t1, t2), std::max(t3, t4), std::max(t5, t6)});

      if (tmax >= std::max(0.0f, tmin) && tmin < minDist &&
          tmin < maxClickDistance) {
        minDist = tmin;
        hitX = x;
        hitY = y;
      }
    }
  }

  if (hitX >= 0 && hitY >= 0) {
    if (button == GLFW_MOUSE_BUTTON_LEFT)
      board.reveal(hitX, hitY);
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
      board.toggleFlag(hitX, hitY);
  }

  // Debug ray
  debugRayOrigin = rayOrigin;
  debugRayDir = rayWorld;
  drawDebugRay = true;

  // Simple log
  std::cout << "Ray origin: " << rayOrigin.x << ", " << rayOrigin.y << ", "
            << rayOrigin.z << "\n";
  std::cout << "Ray dir:    " << rayWorld.x << ", " << rayWorld.y << ", "
            << rayWorld.z << "\n";
}

// Camera setup
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
Board board(5, 5, 5); // 5x5 board, 5 mines
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Callbacks
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void processInput(GLFWwindow *window);

int main() {
  // GLFW init
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Window
  GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Minesweeper",
                                        nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // FPS-style

  // GLAD load
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
    return -1;
  }

  // Initialize viewport
  {
    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
  }

  glEnable(GL_DEPTH_TEST);

  // Shaders
  Shader cubeShader("shaders/cube.vert", "shaders/cube.frag");
  // NOTE: Temporarily drop the text shader/overlay to avoid GL_QUADS + shader
  // issues.

  // Cube object
  Cube cube;

  // Game/render loop
  while (!glfwWindowShouldClose(window)) {
    float currentFrame = (float)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = makePerspectiveFromFramebuffer(window);
    glm::mat4 view = camera.GetViewMatrix();

    cubeShader.use();
    cubeShader.setMat4("projection", projection);
    cubeShader.setMat4("view", view);

    for (int x = 0; x < board.width; ++x) {
      for (int y = 0; y < board.height; ++y) {
        const Cell &cell = board.get(x, y);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, gridCenter(x, y, board));

        glm::vec3 color;
        if (cell.state == CellState::Revealed) {
          if (cell.type == CellType::Mine)
            color = glm::vec3(1.0f, 0.2f, 0.2f);
          else if (cell.neighborMines > 0)
            color = glm::vec3(0.2f, 0.5f + 0.1f * cell.neighborMines, 0.2f);
          else
            color = glm::vec3(0.2f, 1.0f, 0.2f);
        } else if (cell.state == CellState::Flagged)
          color = glm::vec3(1.0f, 1.0f, 0.2f);
        else
          color = glm::vec3(0.2f, 0.4f, 1.0f);

        cubeShader.setMat4("model", model);
        cubeShader.setVec3("color", color);
        cube.Draw(cubeShader, model);
      }
    }

    if (drawDebugRay) {
      cubeShader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
      drawRay(debugRayOrigin, debugRayDir, cubeShader);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

// --- Callback + input functions ---

void framebuffer_size_callback(GLFWwindow * /*window*/, int width, int height) {
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);
}

void mouse_callback(GLFWwindow * /*window*/, double xpos, double ypos) {
  if (firstMouse) {
    lastX = (float)xpos;
    lastY = (float)ypos;
    firstMouse = false;
  }

  float xoffset = (float)xpos - lastX;
  float yoffset = lastY - (float)ypos;
  lastX = (float)xpos;
  lastY = (float)ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}
