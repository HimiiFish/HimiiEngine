#include "Hepch.h"

#include "OpenGLTextureCube.h"
#include "EngineCore/Core/FileSystem.h"
#include "glad/glad.h"
#include "stb_image.h"

namespace Himii
{
    OpenGLTextureCube::OpenGLTextureCube(const std::vector<std::string> &paths)
    {
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

        int width, height, channels;
        stbi_set_flip_vertically_on_load(false); // 天空盒通常不需要翻转Y轴，具体看素材

        if (paths.size() > 0)
             m_Path = paths[0];

        for (unsigned int index = 0; index < paths.size(); index++)
        {
            unsigned char *data = nullptr;
            const auto fileBytes = FileSystem::ReadBytes(paths[index]);
            if (fileBytes)
                data = stbi_load_from_memory(fileBytes->data(), static_cast<int>(fileBytes->size()), &width, &height,
                                             &channels, 3);
            else
                data = stbi_load(paths[index].c_str(), &width, &height, &channels, 3);

            if (data)
            {
                m_Width = width;
                m_Height = height;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, GL_RGB, width, height, 0, GL_RGB,
                             GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                HIMII_CORE_ERROR("Failed to load cubemap texture: {0}", paths[index]);
            }
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        m_Specification.Width = m_Width;
        m_Specification.Height = m_Height;
        m_Specification.Format = ImageFormat::RGB8; 
        m_IsLoaded = true;

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // 关键：防止接缝处出现黑线
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    OpenGLTextureCube::~OpenGLTextureCube()
    {
        glDeleteTextures(1, &m_RendererID);
    }

    void OpenGLTextureCube::Bind(uint32_t slot) const
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
    }
}