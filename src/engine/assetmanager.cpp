#include "assetmanager.hpp"
#include "hash.hpp"

namespace ENGINE {

    ENGINE::TEMPLATES::ServiceLocator<AssetManager> g_assetManagerInstance;

    AssetManager &AssetManager::instance() {
        static AssetManager *instance;
        
        instance = new AssetManager();

        return *instance;
    }

    const uint8_t AssetManager::findAsset(uint32_t id) {
        bool found = false;
        uint8_t index = 0;
        for (int i = 0; i < ENGINE::CONST::ASSET_MAX; i++) {
            if (id == loadedassets[i].ptr->getID()) {
                index = i;
                found = true;
                break;
            }
        }
        assert(found);
        return index;
    }

    const uint8_t AssetManager::findEmptySlot(void) {
        bool found = false;
        uint8_t index = 0;
        for (int i = 0; i < ENGINE::CONST::ASSET_MAX; i++) {
            if (loadedassets[i].ptr == nullptr) {
                index = i;
                found = true;
                break;
            }
        }
        assert(found);
        return index;
    }

    const Asset *AssetManager::_get(const char *path, const Asset* (*loader)(const char *)) {
        
        //if (asset exists) todo
        //    uint8_t index = findAsset(ENGINE::HASH::FromString(path));
        //    loadedassets[index].refcount ++; //it exists, dont load again
        //    return loadedassets[index].ptr;
        //else { //asset doesnt exist, load it
            uint8_t emptyslot = findEmptySlot();
            assert(loader);

            auto ptr = loader(path);
            assert(ptr);

            loadedassets[emptyslot].ptr = ptr; // Create new entry
            loadedassets[emptyslot].refcount ++;
            return ptr;

        //}
        /*
        auto pathHash = Hash::FromString(path.c_str());
        auto result = loadedAssets.find(pathHash);

        if (result == loadedAssets.end()) {
            // New asset, load data
            if (!loader)
                return nullptr;

            auto ptr = loader(path);

            if (!ptr)
                return nullptr; // Loading failed

            loadedAssets[pathHash].ptr = ptr; // Create new entry
            return ptr;
        } else {
            // Already loaded asset
            auto &asset = result->second;
            asset.refCount++;
            return asset.ptr;
        }*/
    }

    void AssetManager::release(uint32_t id) {
        AssetEntry *entry = &loadedassets[findAsset(id)];
        //todo check if exists and then delete, remove check from findasset if needed
        entry->refcount --;
        //if unused delete it
        if (entry->ptr && entry->refcount <= 0) {
            delete entry->ptr;
            entry->refcount = 0;
            entry->ptr = nullptr;
        }

    }

    const Asset *TestAsset::loadFromFile(const char *path) {
        auto asset = new TestAsset();
        asset->id = ENGINE::HASH::FromString(path);
        asset->testvar = 1;
        return asset;
    }
    
    TestAsset::~TestAsset(void) {}

} //namespace ENGINE