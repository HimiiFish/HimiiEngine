#include "Hepch.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include "EngineCore/Core/FileSystem.h"
#include "fstream"
#include "filesystem"
#include "glad/glad.h"

#include "glm/gtc/type_ptr.hpp"

#include "shaderc/shaderc.hpp"
#include "spirv_cross/spirv_cross.hpp"
#include "spirv_cross/spirv_glsl.hpp"

#include "EngineCore/Core/Timer.h"

#include <sstream>
#include <iomanip>

namespace Himii
{
    namespace Utils
    {
        static GLenum ShaderTypeFromString(const std::string &type)
        {
            if (type == "vertex")
                return GL_VERTEX_SHADER;
            if (type == "fragment" || type == "pixel")
                return GL_FRAGMENT_SHADER;

            HIMII_CORE_ASSERT(false, "Unknown shader type!");
            return 0;
        }

        static shaderc_shader_kind GLShaderStageToShaderC(GLenum stage)
        {
            switch (stage)
            {
                case GL_VERTEX_SHADER:
                    return shaderc_glsl_vertex_shader;
                case GL_FRAGMENT_SHADER:
                    return shaderc_glsl_fragment_shader;
            }
            HIMII_CORE_ASSERT(false, "Unknown GL shader type!");
            return (shaderc_shader_kind)0;
        }

        static const char *GLShaderStageToString(GLenum stage)
        {
            switch (stage)
            {
                case GL_VERTEX_SHADER:
                    return "GL_VERTEX_SHADER";
                case GL_FRAGMENT_SHADER:
                    return "GL_FRAGMENT_SHADER";
            }
            HIMII_CORE_ASSERT(false);
            return nullptr;
        }

        static std::filesystem::path GetCacheDirectory()
        {
            return FileSystem::GetWritableCacheRoot() / "shader" / "opengl";
        }

        static void CreateCacheDirectoryIfNeeded()
        {
            const std::filesystem::path cacheDirectory = GetCacheDirectory();
            if (!std::filesystem::exists(cacheDirectory))
                std::filesystem::create_directories(cacheDirectory);
        }

        static const char *GLShaderStageCachedOpenGLFileExtension(uint32_t stage)
        {
            switch (stage)
            {
                case GL_VERTEX_SHADER:
                    return ".cached_opengl.vert";
                case GL_FRAGMENT_SHADER:
                    return ".cached_opengl.frag";
            }
            HIMII_CORE_ASSERT(false);
            return "";
        }

        static const char *GLShaderStageCachedVulkanFileExtension(uint32_t stage)
        {
            switch (stage)
            {
                case GL_VERTEX_SHADER:
                    return ".cached_vulkan.vert";
                case GL_FRAGMENT_SHADER:
                    return ".cached_vulkan.frag";
            }
            HIMII_CORE_ASSERT(false);
            return "";
        }

        // FNV-1a 64-bit：用于源码与编译参数哈希，避免过期缓存。
        static uint64_t HashBytes(const void *data, size_t size, uint64_t hash = 14695981039346656037ull)
        {
            const uint8_t *bytes = static_cast<const uint8_t *>(data);
            for (size_t index = 0; index < size; ++index)
            {
                hash ^= bytes[index];
                hash *= 1099511628211ull;
            }
            return hash;
        }

        static uint64_t HashString(const std::string &text, uint64_t hash = 14695981039346656037ull)
        {
            return HashBytes(text.data(), text.size(), hash);
        }

        static std::string ToHex64(uint64_t value)
        {
            std::ostringstream stream;
            stream << std::hex << std::setw(16) << std::setfill('0') << value;
            return stream.str();
        }
    }

    OpenGLShader::OpenGLShader(const std::string &filepath) : m_FilePath(filepath), m_RendererID(0)
    {
        HIMII_PROFILE_FUNCTION();

        Utils::CreateCacheDirectoryIfNeeded();

        std::string source = ReadFile(filepath);
        auto shaderSources = PreProcess(source);
        m_SourceHash = ComputeSourceHash(shaderSources);

        {
            Timer timer;
            CompileOrGetVulkanBinaries(shaderSources);
            CompileOrGetOpenGLBinaries();
            CreateProgram();
            HIMII_CORE_WARNING("OpenGL shader creation took {0} ms", timer.ElapsedMillis());
        }

        auto lastSlash = filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filepath.rfind('.');
        auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filepath.substr(lastSlash, count);
    }

    OpenGLShader::OpenGLShader(const std::string &name, const std::string &vertexSource,
                               const std::string &fragmentSource) : m_Name(name), m_RendererID(0)
    {
        HIMII_PROFILE_FUNCTION();

        Utils::CreateCacheDirectoryIfNeeded();

        std::unordered_map<GLenum, std::string> sources;
        sources[GL_VERTEX_SHADER] = vertexSource;
        sources[GL_FRAGMENT_SHADER] = fragmentSource;
        m_FilePath = name;
        m_SourceHash = ComputeSourceHash(sources);

        CompileOrGetVulkanBinaries(sources);
        CompileOrGetOpenGLBinaries();
        CreateProgram();
    }

    OpenGLShader::~OpenGLShader()
    {
        HIMII_PROFILE_FUNCTION();

        if (m_RendererID != 0)
            glDeleteProgram(m_RendererID);
    }

    bool OpenGLShader::IsValid() const
    {
        return m_RendererID != 0;
    }

    uint64_t OpenGLShader::ComputeSourceHash(const std::unordered_map<GLenum, std::string> &shaderSources) const
    {
        uint64_t hash = Utils::HashString("himii-shader-cache-v2");
        hash = Utils::HashString("vulkan-1.2", hash);
        hash = Utils::HashString("opengl-4.5", hash);
        hash = Utils::HashString(m_FilePath, hash);

        // 固定顺序，避免 unordered_map 遍历导致哈希不稳定。
        const GLenum stages[] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
        for (GLenum stage : stages)
        {
            auto found = shaderSources.find(stage);
            if (found == shaderSources.end())
                continue;
            hash = Utils::HashBytes(&stage, sizeof(stage), hash);
            hash = Utils::HashString(found->second, hash);
        }
        return hash;
    }

    std::filesystem::path OpenGLShader::BuildCachePath(GLenum stage, const char *extension) const
    {
        const std::string fileName =
                std::filesystem::path(m_FilePath).filename().string() + "."
                + Utils::ToHex64(m_SourceHash)
                + extension;
        return Utils::GetCacheDirectory() / fileName;
    }

    std::string OpenGLShader::ReadFile(const std::string &filepath)
    {
        HIMII_PROFILE_FUNCTION();
        return FileSystem::ReadText(filepath);
    }

    std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string &source)
    {
        HIMII_PROFILE_FUNCTION();

        std::unordered_map<GLenum, std::string> shaderSources;

        const char *typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0);
        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos);
            HIMII_CORE_ASSERT(eol != std::string::npos, "Syntax error");
            size_t begin = pos + typeTokenLength + 1;
            std::string type = source.substr(begin, eol - begin);
            HIMII_CORE_ASSERT(Utils::ShaderTypeFromString(type), "Invalid shader type specified");

            size_t nextLinePos = source.find_first_not_of("\r\n", eol);
            HIMII_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
            pos = source.find(typeToken, nextLinePos);

            shaderSources[Utils::ShaderTypeFromString(type)] =
                    source.substr(nextLinePos,
                                  pos - (pos == std::string::npos ? source.size() - 1 : nextLinePos));
        }

        return shaderSources;
    }

    void OpenGLShader::CompileOrGetVulkanBinaries(const std::unordered_map<GLenum, std::string> &shaderSources)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        auto &shaderData = m_VulkanSPIRV;
        shaderData.clear();
        for (auto &&[stage, source] : shaderSources)
        {
            const std::filesystem::path cachedPath =
                    BuildCachePath(stage, Utils::GLShaderStageCachedVulkanFileExtension(stage));

            std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
            if (in.is_open())
            {
                in.seekg(0, std::ios::end);
                auto size = in.tellg();
                in.seekg(0, std::ios::beg);

                auto &data = shaderData[stage];
                data.resize(size / sizeof(uint32_t));
                in.read((char *)data.data(), size);
            }
            else
            {
                shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                        source, Utils::GLShaderStageToShaderC(stage), m_FilePath.c_str(), options);
                if (module.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    HIMII_CORE_ERROR("Vulkan SPIR-V compile failed ({0}):\n{1}", m_FilePath,
                                     module.GetErrorMessage());
                    HIMII_CORE_ASSERT(false, "Shader Vulkan compile failed!");
                }

                shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

                std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
                if (out.is_open())
                {
                    auto &data = shaderData[stage];
                    out.write((char *)data.data(), data.size() * sizeof(uint32_t));
                    out.flush();
                    out.close();
                }
            }
        }

        for (auto &&[stage, data] : shaderData)
            Reflect(stage, data);
    }

    void OpenGLShader::CompileOrGetOpenGLBinaries()
    {
        auto &shaderData = m_OpenGLSPIRV;

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);

        shaderData.clear();
        m_OpenGLSourceCode.clear();
        for (auto &&[stage, spirv] : m_VulkanSPIRV)
        {
            const std::filesystem::path cachedPath =
                    BuildCachePath(stage, Utils::GLShaderStageCachedOpenGLFileExtension(stage));

            std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
            if (in.is_open())
            {
                in.seekg(0, std::ios::end);
                auto size = in.tellg();
                in.seekg(0, std::ios::beg);

                auto &data = shaderData[stage];
                data.resize(size / sizeof(uint32_t));
                in.read((char *)data.data(), size);
            }
            else
            {
                spirv_cross::CompilerGLSL glslCompiler(spirv);
                spirv_cross::CompilerGLSL::Options glslOptions;
                glslOptions.version = 450;
                glslOptions.es = false;
                glslOptions.vulkan_semantics = false;
                glslOptions.enable_420pack_extension = true;
                glslOptions.emit_push_constant_as_uniform_buffer = true;
                glslCompiler.set_common_options(glslOptions);
                m_OpenGLSourceCode[stage] = glslCompiler.compile();
                auto &source = m_OpenGLSourceCode[stage];

                // 调试用：保留 SPIRV-Cross 生成的 OpenGL GLSL，便于排查阶段接口失配。
                {
                    std::filesystem::path dumpPath = cachedPath;
                    dumpPath += ".cross.glsl";
                    std::ofstream dump(dumpPath, std::ios::out | std::ios::trunc);
                    if (dump.is_open())
                        dump << source;
                }

                shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                        source, Utils::GLShaderStageToShaderC(stage), m_FilePath.c_str(), options);
                if (module.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    HIMII_CORE_ERROR("OpenGL SPIR-V compile failed ({0}):\n{1}", m_FilePath,
                                     module.GetErrorMessage());
                    HIMII_CORE_ASSERT(false, "Shader OpenGL compile failed!");
                }

                shaderData[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

                std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
                if (out.is_open())
                {
                    auto &data = shaderData[stage];
                    out.write((char *)data.data(), data.size() * sizeof(uint32_t));
                    out.flush();
                    out.close();
                }
            }
        }
    }

    void OpenGLShader::CreateProgram()
    {
        GLuint program = glCreateProgram();

        std::vector<GLuint> shaderIDs;
        for (auto &&[stage, spirv] : m_OpenGLSPIRV)
        {
            GLuint shaderID = shaderIDs.emplace_back(glCreateShader(stage));
            glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(),
                           static_cast<GLsizei>(spirv.size() * sizeof(uint32_t)));
            glSpecializeShader(shaderID, "main", 0, nullptr, nullptr);
            glAttachShader(program, shaderID);
        }

        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength > 0 ? maxLength : 1);
            glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());
            HIMII_CORE_ERROR("Shader linking failed ({0}):\n{1}", m_FilePath, infoLog.data());

            glDeleteProgram(program);
            for (auto id : shaderIDs)
                glDeleteShader(id);

            m_RendererID = 0;
            HIMII_CORE_ASSERT(false, "Shader linking failed!");
            return;
        }

        for (auto id : shaderIDs)
        {
            glDetachShader(program, id);
            glDeleteShader(id);
        }

        m_RendererID = program;
    }

    void OpenGLShader::Reflect(GLenum stage, const std::vector<uint32_t> &shaderData)
    {
        spirv_cross::Compiler compiler(shaderData);
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        HIMII_CORE_TRACE("OpenGLShader::Reflect - {0} {1}", Utils::GLShaderStageToString(stage), m_FilePath);
        HIMII_CORE_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
        HIMII_CORE_TRACE("    {0} resources", resources.sampled_images.size());

        HIMII_CORE_TRACE("Uniform buffers:");
        for (const auto &resource : resources.uniform_buffers)
        {
            const auto &bufferType = compiler.get_type(resource.base_type_id);
            uint32_t bufferSize = compiler.get_declared_struct_size(bufferType);
            uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            int memberCount = static_cast<int>(bufferType.member_types.size());

            HIMII_CORE_TRACE("  {0}", resource.name);
            HIMII_CORE_TRACE("    Size = {0}", bufferSize);
            HIMII_CORE_TRACE("    Binding = {0}", binding);
            HIMII_CORE_TRACE("    Members = {0}", memberCount);
        }
    }

    void OpenGLShader::Bind() const
    {
        HIMII_PROFILE_FUNCTION();
        HIMII_CORE_ASSERT(m_RendererID != 0, "Attempted to bind an invalid shader!");
        glUseProgram(m_RendererID);
    }

    void OpenGLShader::Unbind() const
    {
        HIMII_PROFILE_FUNCTION();
        glUseProgram(0);
    }

    void OpenGLShader::SetInt(const std::string &name, int value)
    {
        HIMII_PROFILE_FUNCTION();
        UploadUniformInt(name, value);
    }

    void OpenGLShader::SetIntArray(const std::string &name, int *values, uint32_t count)
    {
        HIMII_PROFILE_FUNCTION();
        UploadUniformIntArray(name, values, count);
    }

    void OpenGLShader::SetFloat(const std::string &name, float value)
    {
        HIMII_PROFILE_FUNCTION();
        UploadUniformFloat(name, value);
    }

    void OpenGLShader::SetFloat2(const std::string &name, const glm::vec2 &value)
    {
        HIMII_PROFILE_FUNCTION();
        UploadUniformFloat2(name, value);
    }

    void OpenGLShader::SetFloat3(const std::string &name, const glm::vec3 &value)
    {
        HIMII_PROFILE_FUNCTION();
        UploadUniformFloat3(name, value);
    }

    void OpenGLShader::SetFloat4(const std::string &name, const glm::vec4 &value)
    {
        HIMII_PROFILE_FUNCTION();
        UploadUniformFloat4(name, value);
    }

    void OpenGLShader::SetMat4(const std::string &name, const glm::mat4 &value)
    {
        HIMII_PROFILE_FUNCTION();
        UploadUniformMat4(name, value);
    }

    void OpenGLShader::UploadUniformInt(const std::string &name, int value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location >= 0)
            glUniform1i(location, value);
    }

    void OpenGLShader::UploadUniformIntArray(const std::string &name, int *values, uint32_t count)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location >= 0)
            glUniform1iv(location, count, values);
    }

    void OpenGLShader::UploadUniformFloat(const std::string &name, float value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location >= 0)
            glUniform1f(location, value);
    }

    void OpenGLShader::UploadUniformFloat2(const std::string &name, const glm::vec2 &value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location >= 0)
            glUniform2f(location, value.x, value.y);
    }

    void OpenGLShader::UploadUniformFloat3(const std::string &name, const glm::vec3 &value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location >= 0)
            glUniform3f(location, value.x, value.y, value.z);
    }

    void OpenGLShader::UploadUniformFloat4(const std::string &name, const glm::vec4 &value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location >= 0)
            glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::UploadUniformMat3(const std::string &name, const glm::mat3 &matrix)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location >= 0)
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void OpenGLShader::UploadUniformMat4(const std::string &name, const glm::mat4 &matrix)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        if (location >= 0)
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }
} // namespace Himii
