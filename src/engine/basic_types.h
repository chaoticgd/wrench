/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2025 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ENGINE_BASIC_TYPES_H
#define ENGINE_BASIC_TYPES_H

#include <glm/glm.hpp>

#include <core/util.h>

packed_struct(Vec3f,
	f32 x;
	f32 y;
	f32 z;
	
	glm::vec3 unpack() const
	{
		return glm::vec3(x, y, z);
	}
	
	static Vec3f pack(glm::vec3 vec)
	{
		Vec3f result;
		result.x = vec.x;
		result.y = vec.y;
		result.z = vec.z;
		return result;
	}
)

packed_struct(Vec4f,
	f32 x;
	f32 y;
	f32 z;
	f32 w;
	
	glm::vec4 unpack() const
	{
		return glm::vec4(x, y, z, w);
	}
	
	static Vec4f pack(glm::vec4 vec)
	{
		Vec4f result;
		result.x = vec.x;
		result.y = vec.y;
		result.z = vec.z;
		result.w = vec.w;
		return result;
	}
	
	void swap(glm::vec4& vec)
	{
		SWAP_PACKED(x, vec.x);
		SWAP_PACKED(y, vec.y);
		SWAP_PACKED(z, vec.z);
		SWAP_PACKED(w, vec.w);
	}
)

packed_struct(Mat3,
	Vec4f m_0;
	Vec4f m_1;
	Vec4f m_2;
	
	glm::mat3x4 unpack() const
	{
		glm::mat3x4 result;
		result[0] = m_0.unpack();
		result[1] = m_1.unpack();
		result[2] = m_2.unpack();
		return result;
	}
	
	static Mat3 pack(glm::mat3x4 mat)
	{
		Mat3 result;
		result.m_0 = Vec4f::pack(mat[0]);
		result.m_1 = Vec4f::pack(mat[1]);
		result.m_2 = Vec4f::pack(mat[2]);
		return result;
	}
)

packed_struct(Mat4,
	Vec4f m_0;
	Vec4f m_1;
	Vec4f m_2;
	Vec4f m_3;
	
	glm::mat4 unpack() const
	{
		glm::mat4 result;
		result[0] = m_0.unpack();
		result[1] = m_1.unpack();
		result[2] = m_2.unpack();
		result[3] = m_3.unpack();
		return result;
	}
	
	static Mat4 pack(glm::mat4 mat)
	{
		Mat4 result;
		result.m_0 = Vec4f::pack(mat[0]);
		result.m_1 = Vec4f::pack(mat[1]);
		result.m_2 = Vec4f::pack(mat[2]);
		result.m_3 = Vec4f::pack(mat[3]);
		return result;
	}
)

#endif
