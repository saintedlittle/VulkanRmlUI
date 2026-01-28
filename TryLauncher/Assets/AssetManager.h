#pragma once

#include <stb_image.h>
#include <RmlUi/Core.h>

#include "../Engine/Engine.h"
#include <unordered_map>
#include <memory>
#include <string>

class VulkanRenderer;

class Asset {
public:
    virtual ~Asset() = default;
    virtual const std::string& GetPath() const = 0;
    virtual size_t GetMemoryUsage() const = 0;
};

class UIDocument;
class Texture;

class AssetManager : public IEngineModule {
public:
    AssetManager(VulkanRenderer* renderer, ResourceManager* resourceManager);
    ~AssetManager();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "AssetManager"; }
    int GetInitializationOrder() const override { return 400; }

    // Asset loading methods (to be implemented in later tasks)
    std::shared_ptr<UIDocument> LoadRMLDocument(const std::string& path);
    bool LoadStylesheet(const std::string& path);
    std::shared_ptr<Texture> LoadTexture(const std::string& path);
    bool LoadFont(const std::string& path, const std::string& name);
    
    // Asset management
    void UnloadAsset(const std::string& path);
    void UnloadAllAssets();
    size_t GetMemoryUsage() const;
    
    // Configuration
    void SetAssetBasePath(const std::string& path) { m_assetBasePath = path; }
    const std::string& GetAssetBasePath() const { return m_assetBasePath; }

private:
    VulkanRenderer* m_renderer;
    ResourceManager* m_resourceManager;
    std::unordered_map<std::string, std::weak_ptr<Asset>> m_assetCache;
    std::string m_assetBasePath;
    bool m_initialized = false;
};