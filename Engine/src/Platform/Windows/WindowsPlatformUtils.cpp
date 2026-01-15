#include "Hepch.h"
#include "Himii/Core/Application.h"
#include "Himii/Utils/PlatformUtils.h"

// 使用windows sdk里提供的头文件
#include <GLFW/glfw3.h>
#include <commdlg.h>
#include <shlobj.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// TODO: if def window platform
namespace Himii
{
    // 打开搜索文件的窗口
    std::string FileDialog::OpenFile(const char *filter)
    {
        // 打开搜索文件的窗口, 需要传入一个OPENFILENAMEA对象
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        CHAR currentDir[256] = {0};
        ZeroMemory(&ofn, sizeof(OPENFILENAME));

        // 填充ofn的信息
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow *)Application::Get().GetWindow().GetNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        if (GetCurrentDirectoryA(256,currentDir))
            ofn.lpstrInitialDir = currentDir;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        //
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        // 调用win32 api
        if (GetOpenFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return std::string();
    }

    std::string FileDialog::SaveFile(const char *filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        CHAR currentDir[256] = {0};
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window((GLFWwindow *)Application::Get().GetWindow().GetNativeWindow());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        if (GetCurrentDirectoryA(256, currentDir))
            ofn.lpstrInitialDir = currentDir;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = strchr(filter, '\0') + 1;
        if (GetSaveFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return std::string();
    }

    std::string FileDialog::OpenFolder(const char* initialPath)
    {
        std::string result = "";
        IFileOpenDialog* pFileOpen;

        // Need to initialize COM library for this thread if not already done
        // However, as we are in a message loop context often controlled by GLFW, 
        // we assume CoInitialize has been called or we call it safely.
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            DWORD dwOptions;
            if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions)))
            {
                pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
            }
            
            if (SUCCEEDED(pFileOpen->Show(NULL)))
            {
                IShellItem* pItem;
                if (SUCCEEDED(pFileOpen->GetResult(&pItem)))
                {
                    PWSTR pszFilePath;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                    {
                        // Convert WideChar to std::string (UTF-8)
                        int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                        if (len > 0) 
                        {
                            std::vector<char> str(len);
                            WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &str[0], len, NULL, NULL);
                            result = str.data();
                        }
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        return result;
    }
} // namespace Hazel
