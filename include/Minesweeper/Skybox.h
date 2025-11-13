#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Skybox {
public:
  Skybox();
  ~Skybox();

  Skybox(const Skybox &) = delete;
  Skybox &operator=(const Skybox &) = delete;

  void Draw() const;

private:
  GLuint VAO = 0;
  GLuint VBO = 0;

  void setup();
};
