#include "ResourceManager.h"
#include "GuidUtils.h"

ResourceManager::ResourceManager() {
	workerThread.emplace_back(&ResourceManager::WorkerThread, this);

	workerThread.back().detach();
}

ResourceManager::~ResourceManager() {
	
	for (auto pair : _cachedResources) {
		delete pair.second;
	}
}

bool ResourceManager::LoadResource(std::string guid, Resource *&resource) {
	if (guid.length() != GUID_STR_LENGTH) {
		std::cerr << "ResourceManager::LoadResource(): GUID is invalid" << std::endl;
		return false;
	}

	auto pair = _cachedResources.find(guid);
	if (pair != _cachedResources.end()) {
		// Resource already exists in cache
		resource = pair->second;
		pair->second->RefAdd();
		return true;
	}
	else {
		// Resource does not exist in cache -> load it from mounted package
		AssetData data;
		if (!_packageManager.LoadAssetByGuid(guid, data)) {
			std::cerr << "ResourceManager::LoadResource(): Could not load resource data from package" << std::endl;
			return false;
		}

		if (!resource->LoadFromData(data.data.get(), data.size, data.fileExtension)) {
			std::cerr << "ResourceManager::LoadResource(): Could not load resource from raw data" << std::endl;
			return false;
		}

		resource->RefAdd();
		_cachedResources.emplace(guid, resource);
		_memoryUsed += resource->GetMemoryUsage();

		// Memory limit warning
		if (_limitMemory && (_memoryUsed >= _memoryLimit)) {
			std::cerr << "Warning: ResourceManager memory limit reached: " << _memoryUsed << " of " << _memoryLimit << " bytes used" << std::endl;
		}

		return true;
	}
}

bool ResourceManager::UnloadResource(std::string guid) {
	Resource* res = _cachedResources[guid];
	if (res == nullptr) {
		std::cerr << "ResourceManager::UnloadResource(): Resource with GUID is nullptr" << std::endl;
		return false;
	}
	int ref = res->GetRef();
	if (ref <= 1) {
		// References goes to 0 -> remove resource from cache
		_memoryUsed -= _cachedResources[guid]->GetMemoryUsage();
		_cachedResources[guid]->Unload();
		_cachedResources.erase(guid);
		return true;
	}
	else if (ref > 1) {
		// References does not go to zero -> subtract from reference count
		_cachedResources[guid]->RefSub();
		return true;
	}
	else {
		std::cerr << "ResourceManager::UnloadResource(): Failed unloading resource with GUID: " << guid << std::endl;
		return false;
	}
}

PackageManager *ResourceManager::GetPackageManager()
{
	return &_packageManager;
}

void ResourceManager::EnableMemoryLimit(uint64_t limit)
{
	_limitMemory = true;
	_memoryLimit = limit;
}

void ResourceManager::DisableMemoryLimit()
{
	_limitMemory = false;
}

uint64_t ResourceManager::GetMemoryLimit()
{
	return _memoryLimit;
}

void ResourceManager::SetMemoryLimit(uint64_t limit)
{
	_memoryLimit = limit;
}

uint64_t ResourceManager::GetMemoryUsed()
{
	return _memoryUsed;
}

bool ResourceManager::AddPackage(std::string path) {
	std::lock_guard<std::mutex> lock(_packageMutex);
	_newPackage.push_back(path);

	return true;
}

void ResourceManager::WorkerThread() {
	// This thread will run in parallel
	while (true) {
		std::string package;
		{
			std::unique_lock<std::mutex> lock(_packageMutex);
			if (_newPackage.empty())
				continue;
			package = std::move(_newPackage.front());
			_newPackage.erase(_newPackage.begin());
		}
#ifdef TEST
		auto t0 = std::chrono::high_resolution_clock::now();
#endif		
		if (!_packageManager.MountPackage(package)) {
				std::cerr << "ResourceManager::MountPackage(): Could not mount package" << std::endl;
			}
			
			std::vector<std::string> guids = _packageManager.GetGUIDsInLastMountedPackage();

			for (const std::string& guid : guids) {

				AssetData data;
				if (!_packageManager.LoadAssetByGuid(guid, data)) {
					std::cerr << "ResourceManager::LoadResource(): Could not load resource" << std::endl;

				}
				std::lock_guard<std::mutex> lock(_threadDataMutex);
				_threadData.emplace(guid, std::move(data));
				
			}
#ifdef TEST
		auto t1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = t1 - t0;
		std::cout << "Mounting Package and reading data: " << duration.count() << std::endl;
#endif
	}
}

int ResourceManager::GetThreadDataSize() {
	return _threadData.size();
}

bool ResourceManager::LoadObject(Resource* &resource) {

#ifdef TEST
	auto t0 = std::chrono::high_resolution_clock::now();
#endif
	AssetData data;
	std::string guid;
	{

		std::lock_guard<std::mutex> lock(_threadDataMutex);
		if (_threadData.empty()) {
			std::cerr << "ResourceManager::LoadObject(): AssetData vector is empty" << std::endl;
			return false;
		}
		
		auto it = _threadData.begin();
		data = std::move(it->second);
		guid = std::move(it->first);
		_threadData.erase(it);
	
	}
	resource->LoadFromData(data.data.get(), data.size, data.fileExtension);
	_cachedResources.emplace(guid, resource);
#ifdef TEST
	auto t1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = t1 - t0;
	std::cout << "Loading resouce from data: " << duration.count() << std::endl;
#endif
	return true;
	
}

std::vector<std::string> ResourceManager::GetCachedResources() {
	std::vector<std::string> GUID;
	for (auto res : _cachedResources) {
		GUID.push_back(res.first);
	}
	return GUID;
}