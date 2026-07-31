#include "Hepch.h"
#include "EngineCore/Utils/PlatformUtils.h"
#include "EngineCore/Core/Application.h"

#include <unistd.h>
#include <cstdlib>

#include <nfd.h>

#include <vector>
#include <sstream>

namespace Himii {

	// 解析 Windows 风格的过滤器字符串
	struct NFDFilterSpec {
        std::string name;
        std::string spec;
    };

    static NFDFilterSpec ParseWindowsFilter(const char* filter) {
        NFDFilterSpec result;
        if (!filter) return result;

        std::string filterStr = filter;
        
        //提取 Name
        size_t parenPos = filterStr.find('(');
        if (parenPos != std::string::npos) {
            result.name = filterStr.substr(0, parenPos);
            // 去掉末尾空格
            while (!result.name.empty() && result.name.back() == ' ')
                result.name.pop_back();
        } else {
            result.name = "File";
        }

        // 提取 Spec
        size_t dotPos = filterStr.find('.');
        if (dotPos != std::string::npos) {
            size_t endPos = filterStr.find(')', dotPos);
            if (endPos == std::string::npos) endPos = filterStr.length();
            
            std::string rawSpec = filterStr.substr(dotPos + 1, endPos - dotPos - 1);
            
            std::stringstream ss(rawSpec);
            std::string segment;
            std::string finalSpec;
            
            while(std::getline(ss, segment, ';')) {
                // segment 可能是 *.png 或 *png 或 png
                size_t lastStar = segment.find_last_of('*');
                if (lastStar != std::string::npos) {
                    segment = segment.substr(lastStar + 1);
                }
                // 去掉点
                if (!segment.empty() && segment[0] == '.') {
                    segment = segment.substr(1);
                }
                
                if (!finalSpec.empty()) finalSpec += ",";
                finalSpec += segment;
            }
            result.spec = finalSpec;
        }

        // 如果解析失败，回退为空，NFD 会显示所有文件
        return result;
    }
    // --------------------------------------------------------

	std::string FileDialog::OpenFile(const char* filter)
	{
        NFD_Init();

		nfdchar_t* outPath = nullptr;
        nfdfilteritem_t filterItem[1];
        NFDFilterSpec parsed = ParseWindowsFilter(filter);
        
        // 设置过滤
        if (!parsed.spec.empty()) {
            filterItem[0] = { parsed.name.c_str(), parsed.spec.c_str() };
        }

		nfdresult_t result = NFD_OpenDialog(&outPath, !parsed.spec.empty() ? filterItem : nullptr, 1, nullptr);

		std::string filePath;
		if (result == NFD_OKAY)
		{
			filePath = outPath;
			NFD_FreePath(outPath);
		}
		else if (result == NFD_CANCEL)
		{
			// 用户取消，返回空字符串
		}
		else
		{
			HIMII_CORE_ERROR("NFD Error: {0}", NFD_GetError());
		}

		return filePath;
	}

	std::string FileDialog::SaveFile(const char* filter)
	{
        NFD_Init();

		nfdchar_t* outPath = nullptr;
        nfdfilteritem_t filterItem[1];
        NFDFilterSpec parsed = ParseWindowsFilter(filter);

        if (!parsed.spec.empty()) {
            filterItem[0] = { parsed.name.c_str(), parsed.spec.c_str() };
        }

		nfdresult_t result = NFD_SaveDialog(&outPath, !parsed.spec.empty() ? filterItem : nullptr, 1, nullptr, nullptr);

		std::string filePath;
		if (result == NFD_OKAY)
		{
			filePath = outPath;
			NFD_FreePath(outPath);
		}
		else if (result == NFD_ERROR)
        {
            HIMII_CORE_ERROR("NFD Error: {0}", NFD_GetError());
        }

		return filePath;
	}

    // 新增的 OpenFolder 实现
	std::string FileDialog::OpenFolder(const char* initialPath)
	{
		NFD_Init();

		nfdchar_t* outPath = nullptr;
		nfdresult_t result = NFD_PickFolder(&outPath, initialPath);

		std::string folderPath;
		if (result == NFD_OKAY)
		{
			folderPath = outPath;
			NFD_FreePath(outPath);
		}
		else if (result == NFD_CANCEL)
		{
			// 用户取消
		}
		else
		{
			HIMII_CORE_ERROR("NFD Error: {0}", NFD_GetError());
		}

		return folderPath;
	}

    std::filesystem::path PlatformProcess::GetExecutableDirectory()
    {
        char buffer[1024];
        ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
        if (count != -1)
            return std::filesystem::path(std::string(buffer, static_cast<size_t>(count))).parent_path();
        return std::filesystem::current_path();
    }

    void PlatformProcess::SetDynamicLibrarySearchDirectory(const std::filesystem::path &directory)
    {
        (void)directory;
        // Linux 主要通过 rpath / LD_LIBRARY_PATH；此处保留空实现以统一 API。
    }

    bool PlatformProcess::SetEnvironmentVariableValue(const std::string &name, const std::string &value)
    {
        return setenv(name.c_str(), value.c_str(), 1) == 0;
    }
}