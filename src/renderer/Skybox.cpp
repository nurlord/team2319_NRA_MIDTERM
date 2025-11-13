#include "Minesweeper/Skybox.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>

namespace {
constexpr std::array<float, 108> kCubeVertices = {
    -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f};

inline float frac(float v) { return v - std::floor(v); }

glm::vec3 fractVec(const glm::vec3 &v) {
  return glm::vec3(frac(v.x), frac(v.y), frac(v.z));
}

float hashVec(glm::vec3 p) {
  p = fractVec(p * 0.3183099f + glm::vec3(0.1f, 0.3f, 0.7f));
  p *= 17.0f;
  return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

glm::vec3 proceduralSky(const glm::vec3 &direction) {
  glm::vec3 dir = glm::normalize(direction);
  float horizon = glm::smoothstep(-0.2f, 0.6f, dir.y);
  glm::vec3 skyTop(0.05f, 0.15f, 0.35f);
  glm::vec3 skyMid(0.15f, 0.25f, 0.55f);
  glm::vec3 baseSky = glm::mix(skyMid, skyTop, horizon);

  float band = std::sin((dir.x + dir.z) * 4.0f) * 0.5f + 0.5f;
  float swirl = std::sin(dir.y * 12.0f) * 0.5f + 0.5f;
  glm::vec3 clouds =
      glm::mix(glm::vec3(0.12f, 0.18f, 0.28f), glm::vec3(0.45f, 0.55f, 0.75f),
               band * swirl);
  baseSky = glm::mix(baseSky, clouds, 0.35f * horizon);

  float aurora = std::pow(std::max(dir.y, 0.0f), 3.0f) *
                 (std::sin(dir.x * 10.0f) * 0.5f + 0.5f);
  glm::vec3 auroraColor = glm::vec3(0.1f, 0.7f, 0.55f) * aurora * 0.35f;

  float starLayer = hashVec(glm::floor(dir * 40.0f));
  float stars = glm::smoothstep(0.98f, 1.0f, starLayer) * (1.0f - horizon) *
                0.7f;

  glm::vec3 groundColor(0.02f, 0.04f, 0.08f);
  float blend = glm::smoothstep(-0.4f, 0.1f, dir.y);

  glm::vec3 color = glm::mix(groundColor, baseSky + auroraColor, blend);
  color += stars;
  return glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
}

glm::vec3 directionForFace(int face, float u, float v) {
  switch (face) {
  case 0:
    return glm::vec3(1.0f, v, -u);
  case 1:
    return glm::vec3(-1.0f, v, u);
  case 2:
    return glm::vec3(u, 1.0f, -v);
  case 3:
    return glm::vec3(u, -1.0f, v);
  case 4:
    return glm::vec3(u, v, 1.0f);
  default:
    return glm::vec3(-u, v, -1.0f);
  }
}
} // namespace

Skybox::Skybox() { setup(); }

Skybox::~Skybox() {
  if (cubemapTexture)
    glDeleteTextures(1, &cubemapTexture);
  if (VBO)
    glDeleteBuffers(1, &VBO);
  if (VAO)
    glDeleteVertexArrays(1, &VAO);
}

void Skybox::setup() {
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * kCubeVertices.size(),
               kCubeVertices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  glBindVertexArray(0);

  glGenTextures(1, &cubemapTexture);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

  constexpr int size = 256;
  std::vector<float> faceData(size * size * 3);

  for (int face = 0; face < 6; ++face) {
    for (int y = 0; y < size; ++y) {
      for (int x = 0; x < size; ++x) {
        float u = 2.0f * static_cast<float>(x) / (size - 1) - 1.0f;
        float v = 2.0f * static_cast<float>(y) / (size - 1) - 1.0f;
        glm::vec3 dir = directionForFace(face, u, v);
        glm::vec3 color = proceduralSky(dir);
        size_t idx = (static_cast<size_t>(y) * size + static_cast<size_t>(x)) *
                     3;
        faceData[idx + 0] = color.r;
        faceData[idx + 1] = color.g;
        faceData[idx + 2] = color.b;
      }
    }
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB32F, size,
                 size, 0, GL_RGB, GL_FLOAT, faceData.data());
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Skybox::Draw() const {
  glBindVertexArray(VAO);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}
