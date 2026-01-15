#pragma once

#include <string>

namespace Himii
{
    class FileDialog {
    public:
        // 打开文件对话框，返回选中的文件路径，取消则返回空字符串
        static std::string OpenFile(const char *filter);

        // 保存文件对话框，返回选中的文件路径，取消则返回空字符串
        static std::string SaveFile(const char *filter);

        // 打开文件夹对话框，返回选中的文件夹路径，取消则返回空字符串
        static std::string OpenFolder(const char* initialPath = "");
    };
}
