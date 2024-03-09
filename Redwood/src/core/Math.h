#pragma once
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
//#include "glm/gtx/string_cast.hpp"

namespace rwd {

	using Vec2 = glm::vec2;
	using Vec3 = glm::vec3;
	using Vec4 = glm::vec4;

	using Mat3 = glm::mat3x3;
	using Mat4 = glm::mat4;

	using Quat = glm::quat;

	//template<typename T, glm::qualifier Q>
	//using Translate = glm::translate(glm::mat<4, 4, T, Q> const& m, glm::vec<3, T, Q> const& v);
}
