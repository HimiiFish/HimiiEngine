#pragma once
#include "glm/glm.hpp"
#include "Himii/Renderer/Texture.h"

namespace Himii {

	class SubTexture2D
	{
	public:
        SubTexture2D(const Ref<Texture2D> &texture, const glm::vec2 &min, const glm::vec2 &max);

	private:
		Ref<Texture2D> m_Texture;
		glm::vec2 m_TexCoords[4];
		glm::vec2 m_Size;
	};

} // namespace Himii
