#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "Shader.h"

class Cube {
public:
  Cube();
  ~Cube();
  void Draw(const Shader &shader, const glm::mat4 &model) const;

private:
  unsigned int VAO, VBO;
  void setupCube();
};
