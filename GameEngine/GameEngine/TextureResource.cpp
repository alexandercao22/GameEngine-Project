#include "TextureResource.h"

#include <iostream>
#include <array>

bool TextureResource::LoadFromData(const char* data, size_t size, const std::string& fileExtension)
{
	// Format check
	static const std::array<std::string, 10> supportedFormats = {
		".jpg",
		".jpeg",
		".png",
		".bmp",	// Windows bitmap
		".tga",	// Truevision TGA
		".gif",	// Only first frame (static)
		".psd",	// Photoshop (composited view only)
		".hdr",	// High dynamic range (RGBE)
		".pic", // Softimage PIC
		".qoi"	// Quite OK Image (Should be faster than PNG)
	};

	if (std::find(supportedFormats.begin(), supportedFormats.end(), fileExtension) == supportedFormats.end()) {
		std::cerr << "TextureResource::LoadFromData(): Failed to load unsupported format" << std::endl;
		return false;
	}

	Image image = LoadImageFromMemory(fileExtension.c_str(), (const unsigned char*)data, size);
	if (!IsImageValid(image)) {
		std::cerr << "TextureResource::LoadFromData(): Failed to load image from data" << std::endl;
		return false;
	}

	_texture = LoadTextureFromImage(image);
	if (!IsTextureValid(_texture)) {
		std::cerr << "TextureResource::LoadFromData(): Failed to load texture from image" << std::endl;
		return false;
	}

	// Calculate memory usage
	size_t textureSize = image.width * image.height * 4;
	_memoryUsage = (uint64_t)textureSize;

	UnloadImage(image);

	return true;
}

uint64_t TextureResource::GetMemoryUsage()
{
	return _memoryUsage;
}

TextureResource *TextureResource::LoadFromDisk(std::string path)
{
	Resource::RefAdd();

	Texture2D texture = LoadTexture(path.c_str());
	if (!IsTextureValid(texture)) {
		std::cerr << "TextureResource::LoadFromDisk(): Texture was invalid: " << path << std::endl;
		return nullptr;
	}
	_texture = texture;
	return this;
}

void TextureResource::Unload()
{
	UnloadTexture(_texture);
}

Texture2D TextureResource::GetTexture()
{
	return _texture;
}
