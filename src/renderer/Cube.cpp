#include "Minesweeper/Cube.h"
#include <glad/glad.h>

Cube::Cube() { setupCube(); }

Cube::~Cube() {
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
}

void Cube::setupCube() {
  const float vertices[] = {
      // positions          // normals
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
       0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
       0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
       0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
      -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,

      -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
       0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
       0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
       0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
      -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
      -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,

      -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f,
      -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
      -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
      -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,
      -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f,
      -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f,

       0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
       0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
       0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
       0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
       0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,
       0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
       0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,
       0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f,
       0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f,
      -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,

      -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
       0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
       0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
       0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
      -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
      -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f};

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void Cube::Draw(const Shader &shader, const glm::mat4 &model) const {
  shader.setMat4("model", model);
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}
