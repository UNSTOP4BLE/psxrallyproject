#include "assetmanager.hpp"
#include "hash.hpp"

namespace ENGINE {

    ENGINE::TEMPLATES::ServiceLocator<AssetManager> g_assetManagerInstance;

    AssetManager &AssetManager::instance() {
        static AssetManager *instance;
        
        instance = new AssetManager();

        return *instance;
    }


    const Asset *TestAsset::loadFromFile(const char *path) {
        auto asset = new TestAsset();
        asset->id = ENGINE::HASH::FromString(path);
        asset->testvar = 1;
        return asset;
    }
    
    TestAsset::~TestAsset(void) {}

} //namespace ENGINE