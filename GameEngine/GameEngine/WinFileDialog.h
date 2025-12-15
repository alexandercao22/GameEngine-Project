#pragma once

#include <string>

// Opens the file explorer and retunrs the path of the selected file
std::string OpenFileDialog();

// Copies the source file to the destination folder
void CopyFileToResources(const std::string &srcPath, const std::string &destFolder);