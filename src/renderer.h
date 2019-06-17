#ifndef RENDERER_H
#define RENDERER_H

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/common.hpp>

#include "shaders.h"
#include "app.h"

void draw_current_level(const app& a, shader_programs& shaders);
void draw_test_tri(shader_programs& shaders, glm::mat4 mvp, glm::vec3 colour);

#endif
