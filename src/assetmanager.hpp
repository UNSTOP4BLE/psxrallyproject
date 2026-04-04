#pragma once

class Asset {
public:
    uint32_t id;

    Asset(void) {id = ENGINE::CONST::ASSET_NULL_ID;}
    virtual ~Asset() = default;

    static const Asset* loadFromFile(const char *path) { (void)path; return nullptr; }
};

//class ImageAsset : public Asset {
//public:
//    GFX::Texture image;

//    ImageAsset() = default;
//    ~ImageAsset();

//    static const Asset* loadFromFile(std::string path);
//};

class AssetEntry {
public:
    int refcount;
    const Asset* ptr;

    AssetEntry() : refcount(1), ptr(nullptr) {}
    ~AssetEntry() {
        if (ptr) 
            delete ptr;
        ptr = nullptr;
    }
};

class AssetManager {
public:
    // Allow loading new assets: assetManager.get<ImageAsset>(path)
    template<typename T>
    const T* get(std::string path) {
        return reinterpret_cast<const T*>(_get(path, &(T::loadFromFile)));
    }

    // Only retrieve already loaded assets: assetManager.get(asset->id)
    const Asset* get(uint32_t id) {
        return _get(id);
    }

    void release(uint32_t id);
private:
    AssetTableEntry loadedassets[ENGINE::CONST::ASSET_MAX];

    const Asset* _get(const char *path, const Asset* (*loader)(const char *));
    const Asset* _get(uint32_t id) {
        return loadedassets[id].ptr;
    };
};
