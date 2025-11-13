#pragma once
#include <glad/glad.h>
class Cube {
public:
  Cube();
  ~Cube();
  void Draw() const;

private:
  unsigned int VAO, VBO;
  void setupCube();
};
