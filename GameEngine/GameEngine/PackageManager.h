#pragma once

#include "Libraries/lz4/lz4.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <shared_mutex>

inline constexpr char SIGNATURE[8] = "GEPAKV0"; // Signature for files packaged by this package manager

// Written at the very start of a package file
struct PackageHeader {
	char signature[5];
	uint32_t AssetCount;
	uint64_t tableOfContentsOffset;
};

// Specifies the details of a file inside a package
struct PackageEntry {
	uint64_t offset;
	uint64_t size;
	uint64_t sizeCompressed;
};

// Represents a file in a package
struct TOCEntry {
	std::string guid;
	std::string key;
	PackageEntry packageEntry;
};

// Supports lookup by GUID and by path
struct MountedPackage {
	std::string path; // Path to the mounted package
	std::unordered_map<std::string, TOCEntry> tocByKey; // Key (string): name of the file inside the package (file.extension)
	std::unordered_map<std::string, TOCEntry> tocByGuid; // Key (string): GUID of the asset found in its .meta file (file.extension.meta)
};

// Used to load and store asset file data
struct AssetData {
	std::unique_ptr<char[]> data;
	uint64_t size;
	std::string fileExtension;
};

class PackageManager {
private:
	std::unordered_map<std::string, MountedPackage> _mountedPackages; // key (string): name of the package file (package.gepak)
	std::vector<std::string> _mountOrder; // The order of which an asset loading functions checks packages for assets (back = highest priority)

	std::shared_mutex _mountMutex; // Used to ensure that only one thread can mount and unmount at once

	// Loads asset from a specified mounted package
	bool LoadAsset(MountedPackage& mountedPackage, const TOCEntry& tocEntry, AssetData& asset);

public:
	PackageManager() = default;
	~PackageManager() = default;

	// Creates a package from a directory specified by source (path) inside directory specified by target (path)
	bool Pack(const std::string& source, const std::string& target);
	// Creates a directory from a package specifed by source (path) inside directory specifed by target (path)
	bool Unpack(const std::string& source, const std::string& target);

	// Mounts the package at the path source
	bool MountPackage(const std::string& source);
	// Unmounts the package mounted most recently
	bool UnmountPackage();
	// Unmount the package specifed by packageKey
	bool UnmountPackage(const std::string& packageKey);
	// Unmounts all currently mounted packages
	bool UnmountAllPackages();

	// Loads asset specified by GUID
	bool LoadAssetByGuid(const std::string& guid, AssetData& asset);
	// Loads asset specified by key (path relative to the package that holds it)
	bool LoadAssetByKey(const std::string& key, AssetData& asset);

	// Get the thread loaded package to load all resources
	std::vector<std::string> GetGUIDsInPackage(const std::string& packageKey);
	// Gets all GUID's in the package with the highest priority
	std::vector<std::string> GetGUIDsInLastMountedPackage();
};