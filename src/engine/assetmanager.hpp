#pragma once

#include "constants.hpp"
#include "templates.hpp"

#include <stdint.h>

namespace ENGINE {

    class Asset {
    public:
        const uint32_t getID(void) const {return id;}
        
        Asset(void) {id = 0;}
        virtual ~Asset() = default;

        static const Asset* loadFromFile(const char *path) { (void)path; return nullptr; }
    protected:
        uint32_t id;
    };

    class TestAsset : public Asset {
    public:
        uint8_t testvar;
        TestAsset() = default;
        ~TestAsset();

        static const Asset* loadFromFile(const char *path);
    };

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
        static AssetManager &instance();

        // Allow loading new assets: assetManager.get<ImageAsset>(path)
        template<typename T>
        const T* get(const char *path) {
            return reinterpret_cast<const T*>(_get(path, &(T::loadFromFile)));
        }

        // Only retrieve already loaded assets: assetManager.get(asset->id)
        const Asset* get(uint32_t id) {
            return _get(id);
        }

        void release(uint32_t id);
    private:
        AssetEntry loadedassets[ENGINE::CONST::ASSET_MAX];

        const Asset* _get(const char *path, const Asset* (*loader)(const char *));
        const Asset* _get(uint32_t id) {
            for (int i = 0; i < ENGINE::CONST::ASSET_MAX; i++)
                if (id == loadedassets[i].ptr->getID())
                    return loadedassets[i].ptr;
        };
    };

    extern ENGINE::TEMPLATES::ServiceLocator<AssetManager> g_assetManagerInstance;

} //namespace ENGINE