#pragma once

#include "constants.hpp"
#include "templates.hpp"

#include <stdint.h>
#include <assert.h>


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

        AssetEntry() : refcount(0), ptr(nullptr) {}
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
        //uint8_t: allows for 256 unique assets max, limited in ENGINE::CONST::ASSET_MAX
        const uint8_t findAsset(uint32_t id);
        const uint8_t findEmptySlot(void);

        const Asset* _get(const char *path, const Asset* (*loader)(const char *));
        const Asset* _get(uint32_t id) {
            return loadedassets[findAsset(id)].ptr;
        }
    };

    extern ENGINE::TEMPLATES::ServiceLocator<AssetManager> g_assetManagerInstance;

} //namespace ENGINE