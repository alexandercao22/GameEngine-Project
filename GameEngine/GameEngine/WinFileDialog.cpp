#include "WinFileDialog.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShObjIdl.h>
#include <string>
#include <filesystem>
#include <iostream>

std::string OpenFileDialog()
{
    std::string result;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return "";

    IFileOpenDialog *pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pFileOpen));
    if (SUCCEEDED(hr)) {
        if (SUCCEEDED(pFileOpen->Show(NULL))) {
            IShellItem *pItem = nullptr;
            if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    // convert wide string to UTF-8 std::string
                    int bufSize = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    if (bufSize > 0) {
                        std::string path(bufSize, '\0');
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &path[0], bufSize, NULL, NULL);
                        // remove trailing null char added by WideCharToMultiByte
                        if (!path.empty() && path.back() == '\0') path.pop_back();
                        result = path;
                    }
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }

    CoUninitialize();
    return result;
}

void CopyFileToResources(const std::string &srcPath, const std::string &destFolder) {
    try {
        // Extract filename from the path
        std::filesystem::path source(srcPath);
        std::filesystem::path destination = std::filesystem::path(destFolder) / source.filename();

        // Create directory if needed
        std::filesystem::create_directories(destFolder);

        // Copy (overwrite if exists)
        std::filesystem::copy_file(
            source,
            destination,
            std::filesystem::copy_options::skip_existing
        );

        std::cout << "Copied to: " << destination << std::endl;
    }
    catch (std::exception &e) {
        std::cerr << "Copy failed: " << e.what() << std::endl;
    }
}