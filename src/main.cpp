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
#include "Minesweeper/Skybox.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <vector>
#include <limits>

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
bool gameOver = false;
bool gameWon = false;
bool inMenu = true;
bool enterPressedLast = false;
bool menuPressedLast = false;
unsigned int textVAO = 0;
unsigned int textVBO = 0;

namespace {
struct TilePalette {
  glm::vec3 face;
  glm::vec3 border;
};

const TilePalette kHiddenPalette{glm::vec3(0.18f, 0.26f, 0.38f),
                                 glm::vec3(0.03f, 0.05f, 0.09f)};
const TilePalette kRevealedPalette{glm::vec3(0.86f, 0.9f, 0.96f),
                                   glm::vec3(0.46f, 0.56f, 0.78f)};
const TilePalette kFlaggedPalette{glm::vec3(0.9f, 0.34f, 0.26f),
                                  glm::vec3(0.55f, 0.14f, 0.1f)};
const TilePalette kMinePalette{glm::vec3(0.96f, 0.28f, 0.35f),
                               glm::vec3(0.6f, 0.12f, 0.18f)};

const std::array<glm::vec3, 8> kNumberColors = {
    glm::vec3(0.32f, 0.68f, 1.0f), glm::vec3(0.35f, 0.9f, 0.45f),
    glm::vec3(0.98f, 0.45f, 0.45f), glm::vec3(0.7f, 0.5f, 0.98f),
    glm::vec3(0.98f, 0.72f, 0.4f), glm::vec3(0.45f, 0.9f, 0.9f),
    glm::vec3(0.95f, 0.88f, 0.45f), glm::vec3(0.96f, 0.96f, 0.96f)};
} // namespace

void startNewGame(GLFWwindow *window);
void updateCursorMode(GLFWwindow *window);
bool worldToScreen(const glm::vec3 &world, const glm::mat4 &view,
                   const glm::mat4 &projection, int fbW, int fbH,
                   glm::vec2 &screen);
void drawText(Shader &shader, const std::string &text, float x, float y,
              float scale, const glm::vec3 &color, int fbW, int fbH);
void drawCenteredText(Shader &shader, const std::string &text, float centerX,
                      float centerY, float scale, const glm::vec3 &color,
                      int fbW, int fbH);

struct TextMesh {
  std::vector<float> vertices;
  float width = 0.0f;
  float height = 0.0f;
};

bool buildTextMesh(const std::string &text, float scale, TextMesh &outMesh);
void renderTextMesh(Shader &shader, const TextMesh &mesh,
                    const glm::vec3 &color, int fbW, int fbH);

static glm::mat4 makePerspectiveFromFramebuffer(GLFWwindow *w) {
  int fbW = 0, fbH = 0;
  glfwGetFramebufferSize(w, &fbW, &fbH);
  float aspect = (fbW > 0 && fbH > 0) ? (float)fbW / (float)fbH : 1.0f;
  return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
}

static inline glm::vec3 gridCenter(int x, int y, const Board &b) {
  // Center grid precisely: use (dim-1)/2.0f, not integer dim/2
  const float spacing = 1.05f;
  float gx = (x - (b.width - 1) * 0.5f) * spacing;
  float gy = (y - (b.height - 1) * 0.5f) * spacing;
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

  if (gameOver || inMenu)
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
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      bool hitMine = board.reveal(hitX, hitY);
      if (hitMine) {
        gameOver = true;
        gameWon = false;
        inMenu = false;
        board.revealAllMines();
        updateCursorMode(window);
      } else if (board.checkWin()) {
        gameOver = true;
        gameWon = true;
        inMenu = false;
        board.revealAllMines();
        updateCursorMode(window);
      }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      board.toggleFlag(hitX, hitY);
    }
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
Camera camera(glm::vec3(0.0f, 0.0f, 15.0f));
Board board(12, 12, 28); // larger board for denser gameplay
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
  updateCursorMode(window);

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
  Shader textShader("shaders/text.vert", "shaders/text.frag");
  Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

  cubeShader.use();
  cubeShader.setInt("skyboxMap", 0);
  skyboxShader.use();
  skyboxShader.setInt("skyboxMap", 0);

  // Text rendering setup
  glGenVertexArrays(1, &textVAO);
  glGenBuffers(1, &textVBO);
  glBindVertexArray(textVAO);
  glBindBuffer(GL_ARRAY_BUFFER, textVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2, nullptr, GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Cube object
  Cube cube;
  Skybox skybox;

  // Game/render loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    float currentFrame = (float)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);

    glClearColor(0.05f, 0.07f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    backgroundShader.use();
    backgroundShader.setFloat("time", currentFrame);
    glBindVertexArray(backgroundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 projection = makePerspectiveFromFramebuffer(window);
    glm::mat4 view = camera.GetViewMatrix();

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    skyboxShader.use();
    skyboxShader.setMat4("projection", projection);
    skyboxShader.setMat4("view", glm::mat4(glm::mat3(view)));
    skyboxShader.setFloat("time", currentFrame);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.texture());
    skybox.Draw();
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    cubeShader.use();
    cubeShader.setMat4("projection", projection);
    cubeShader.setMat4("view", view);
    cubeShader.setVec3("cameraPos", camera.Position);
    cubeShader.setFloat("time", currentFrame);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.texture());

    for (int x = 0; x < board.width; ++x) {
      for (int y = 0; y < board.height; ++y) {
        const Cell &cell = board.get(x, y);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, gridCenter(x, y, board));

        TilePalette palette = kHiddenPalette;
        if (cell.state == CellState::Revealed) {
          if (cell.type == CellType::Mine) {
            palette = kMinePalette;
          } else {
            palette = kRevealedPalette;
          }
        } else if (cell.state == CellState::Flagged) {
          palette = kFlaggedPalette;
        }

        glm::mat4 borderModel = glm::scale(model, glm::vec3(1.04f));
        glm::mat4 tileModel = glm::scale(model, glm::vec3(0.92f));

        cubeShader.setMat4("model", borderModel);
        cubeShader.setVec3("color", palette.border);
        cubeShader.setFloat("reflectivity", 0.6f);
        cube.Draw();

        cubeShader.setMat4("model", tileModel);
        cubeShader.setVec3("color", palette.face);
        cubeShader.setFloat("reflectivity", 0.35f);
        cube.Draw();
      }
    }

    if (drawDebugRay) {
      cubeShader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
      drawRay(debugRayOrigin, debugRayDir, cubeShader);
    }

    glDisable(GL_DEPTH_TEST);

    float resolutionScale = (float)fbH / 1080.0f;
    float cellTextScale = 4.2f * resolutionScale;
    for (int x = 0; x < board.width; ++x) {
      for (int y = 0; y < board.height; ++y) {
        const Cell &cell = board.get(x, y);
        glm::vec2 screenPos;
        if (!worldToScreen(gridCenter(x, y, board), view, projection, fbW, fbH,
                           screenPos))
          continue;

        if (cell.state == CellState::Revealed) {
          if (cell.type == CellType::Mine) {
            drawCenteredText(textShader, "B", screenPos.x, screenPos.y,
                             cellTextScale, glm::vec3(1.0f, 0.3f, 0.3f), fbW,
                             fbH);
          } else if (cell.neighborMines > 0) {
            std::string numberText = std::to_string(cell.neighborMines);
            glm::vec3 numberColor =
                kNumberColors[std::min(cell.neighborMines, 8) - 1];
            drawCenteredText(textShader, numberText, screenPos.x, screenPos.y,
                             cellTextScale, numberColor, fbW, fbH);
          }
        } else if (cell.state == CellState::Flagged) {
          drawCenteredText(textShader, "F", screenPos.x, screenPos.y,
                           cellTextScale, glm::vec3(1.0f, 0.85f, 0.2f), fbW,
                           fbH);
        }
      }
    }

    float overlayScale = 3.6f * resolutionScale;
    if (inMenu) {
      drawCenteredText(textShader, "3D Minesweeper", fbW * 0.5f,
                       fbH * 0.75f, overlayScale * 1.3f, glm::vec3(0.9f), fbW,
                       fbH);
      drawCenteredText(textShader,
                       "Left Click: Reveal   Right Click: Flag", fbW * 0.5f,
                       fbH * 0.6f, overlayScale * 0.6f,
                       glm::vec3(0.8f, 0.8f, 0.8f), fbW, fbH);
      drawCenteredText(textShader, "Press Enter to start", fbW * 0.5f,
                       fbH * 0.45f, overlayScale * 0.8f,
                       glm::vec3(0.6f, 0.8f, 1.0f), fbW, fbH);
    } else if (gameOver) {
      const char *resultText =
          gameWon ? "You cleared the field!" : "Boom! You hit a mine.";
      drawCenteredText(textShader, resultText, fbW * 0.5f, fbH * 0.6f,
                       overlayScale, glm::vec3(0.95f), fbW, fbH);
      drawCenteredText(textShader, "Press Enter to play again", fbW * 0.5f,
                       fbH * 0.45f, overlayScale * 0.7f,
                       glm::vec3(0.6f, 0.8f, 1.0f), fbW, fbH);
    } else {
      drawText(textShader, "Press M to toggle menu", 20.0f, fbH - 40.0f,
               overlayScale * 0.4f, glm::vec3(0.8f, 0.8f, 0.8f), fbW, fbH);
    }

    glEnable(GL_DEPTH_TEST);

    glfwSwapBuffers(window);
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

  if (!inMenu && !gameOver) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
      camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
      camera.ProcessKeyboard(DOWN, deltaTime);
  }

  int enterState = glfwGetKey(window, GLFW_KEY_ENTER);
  if (enterState == GLFW_PRESS && !enterPressedLast) {
    if (inMenu || gameOver)
      startNewGame(window);
  }
  enterPressedLast = (enterState == GLFW_PRESS);

  int menuState = glfwGetKey(window, GLFW_KEY_M);
  if (menuState == GLFW_PRESS && !menuPressedLast) {
    if (!gameOver) {
      inMenu = !inMenu;
      if (!inMenu)
        firstMouse = true;
      updateCursorMode(window);
    }
  }
  menuPressedLast = (menuState == GLFW_PRESS);
}

void mouse_callback(GLFWwindow * /*window*/, double xpos, double ypos) {
  if (inMenu || gameOver)
    return;

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

void startNewGame(GLFWwindow *window) {
  board.reset();
  gameOver = false;
  gameWon = false;
  inMenu = false;
  drawDebugRay = false;
  firstMouse = true;
  updateCursorMode(window);
}

void updateCursorMode(GLFWwindow *window) {
  if (!window)
    return;
  if (inMenu || gameOver)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  else
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

bool worldToScreen(const glm::vec3 &world, const glm::mat4 &view,
                   const glm::mat4 &projection, int fbW, int fbH,
                   glm::vec2 &screen) {
  glm::vec4 clip = projection * view * glm::vec4(world, 1.0f);
  if (clip.w <= 0.0f)
    return false;

  glm::vec3 ndc = glm::vec3(clip) / clip.w;
  if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f ||
      ndc.z < 0.0f || ndc.z > 1.0f)
    return false;

  screen.x = (ndc.x * 0.5f + 0.5f) * (float)fbW;
  screen.y = (ndc.y * 0.5f + 0.5f) * (float)fbH;
  return true;
}

bool buildTextMesh(const std::string &text, float scale, TextMesh &outMesh) {
  if (text.empty() || scale <= 0.0f)
    return false;

  stb_easy_font_spacing(0.0f);

  char buffer[99999];
  int numQuads =
      stb_easy_font_print(0.0f, 0.0f, const_cast<char *>(text.c_str()), nullptr,
                          buffer, sizeof(buffer));
  if (numQuads <= 0)
    return false;

  struct EasyFontVertex {
    float x, y;
    unsigned char color[4];
    unsigned char padding[4];
  };

  auto *quadVertices = reinterpret_cast<EasyFontVertex *>(buffer);

  float minX = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float minY = std::numeric_limits<float>::max();
  float maxY = std::numeric_limits<float>::lowest();

  for (int i = 0; i < numQuads * 4; ++i) {
    const EasyFontVertex &v = quadVertices[i];
    minX = std::min(minX, v.x);
    maxX = std::max(maxX, v.x);
    minY = std::min(minY, v.y);
    maxY = std::max(maxY, v.y);
  }

  float widthUnits = std::max(0.0f, maxX - minX);
  float heightUnits = std::max(0.0f, maxY - minY);
  if (widthUnits <= 0.0f)
    widthUnits = static_cast<float>(
        stb_easy_font_width(const_cast<char *>(text.c_str())));
  if (heightUnits <= 0.0f)
    heightUnits = static_cast<float>(
        stb_easy_font_height(const_cast<char *>(text.c_str())));
  outMesh.width = widthUnits * scale;
  outMesh.height = heightUnits * scale;

  outMesh.vertices.clear();
  outMesh.vertices.reserve(numQuads * 6 * 2);

  auto appendVertex = [&](const EasyFontVertex &v) {
    float localX = (v.x - minX) * scale;
    float localY = (maxY - v.y) * scale;
    outMesh.vertices.push_back(localX);
    outMesh.vertices.push_back(localY);
  };

  for (int i = 0; i < numQuads; ++i) {
    const EasyFontVertex &v0 = quadVertices[i * 4 + 0];
    const EasyFontVertex &v1 = quadVertices[i * 4 + 1];
    const EasyFontVertex &v2 = quadVertices[i * 4 + 2];
    const EasyFontVertex &v3 = quadVertices[i * 4 + 3];

    appendVertex(v0);
    appendVertex(v1);
    appendVertex(v2);
    appendVertex(v0);
    appendVertex(v2);
    appendVertex(v3);
  }

  return !outMesh.vertices.empty();
}

void renderTextMesh(Shader &shader, const TextMesh &mesh,
                    const glm::vec3 &color, int fbW, int fbH) {
  if (mesh.vertices.empty() || textVAO == 0 || textVBO == 0)
    return;

  shader.use();
  shader.setVec3("textColor", color);
  glm::mat4 ortho = glm::ortho(0.0f, (float)fbW, 0.0f, (float)fbH);
  shader.setMat4("projection", ortho);

  glBindVertexArray(textVAO);
  glBindBuffer(GL_ARRAY_BUFFER, textVBO);
  glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float),
               mesh.vertices.data(), GL_DYNAMIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(mesh.vertices.size() / 2));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void drawText(Shader &shader, const std::string &text, float x, float y,
              float scale, const glm::vec3 &color, int fbW, int fbH) {
  TextMesh mesh;
  if (!buildTextMesh(text, scale, mesh))
    return;

  float posX = x;
  float posY = y - mesh.height;
  for (size_t i = 0; i < mesh.vertices.size(); i += 2) {
    mesh.vertices[i] += posX;
    mesh.vertices[i + 1] += posY;
  }

  renderTextMesh(shader, mesh, color, fbW, fbH);
}

void drawCenteredText(Shader &shader, const std::string &text, float centerX,
                      float centerY, float scale, const glm::vec3 &color,
                      int fbW, int fbH) {
  TextMesh mesh;
  if (!buildTextMesh(text, scale, mesh))
    return;

  float posX = centerX - mesh.width * 0.5f;
  float posY = centerY - mesh.height * 0.5f;

  for (size_t i = 0; i < mesh.vertices.size(); i += 2) {
    mesh.vertices[i] += posX;
    mesh.vertices[i + 1] += posY;
  }

  renderTextMesh(shader, mesh, color, fbW, fbH);
}
