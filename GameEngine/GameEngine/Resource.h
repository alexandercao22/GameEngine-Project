#pragma once

#include <string>

class Resource {
private:
	int _refCount = 0;

protected:
	uint64_t _memoryUsage = 0;

public:
	virtual ~Resource() = default;

	virtual bool LoadFromData(const char* data, size_t size, const std::string& fileExtension) = 0;
	virtual uint64_t GetMemoryUsage() = 0;

	virtual Resource* LoadFromDisk(std::string id) = 0;
	virtual void Unload() = 0;
	
	virtual void RefAdd();
	virtual void RefSub();
	virtual int GetRef();
};