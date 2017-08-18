/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __OPENSPACE_CORE___ASSETLOADER___H__
#define __OPENSPACE_CORE___ASSETLOADER___H__

#include <openspace/scene/scenegraphnode.h>

#include <openspace/scripting/lualibrary.h>
#include <openspace/properties/property.h>
#include <openspace/properties/propertyowner.h>
#include <openspace/properties/scalarproperty.h>

#include <ghoul/misc/dictionary.h>
#include <ghoul/lua/luastate.h>

#include <ghoul/lua/ghoul_lua.h>
#include <ghoul/filesystem/directory.h>

#include <memory>
#include <string>

namespace openspace {

namespace assetloader {
int importAsset(lua_State* state);
int importAssetToggle(lua_State* state);
int resolveLocalResource(lua_State* state);
int resolveSyncedResource(lua_State* state);
} // namespace assetloader

class AssetLoader {
public:
    class Asset;

    using InitializationRequirement = ghoul::Boolean;
    using Dependency = std::pair<Asset*, InitializationRequirement>;

    class DependencyToggle : public properties::PropertyOwner {
    public:
        DependencyToggle(Asset* dependency, Asset* dependant, bool enabled = false);
    private:
        properties::BoolProperty _enabled;
        Asset* _dependant;
        Asset* _dependency;
    };

    class Asset : public properties::PropertyOwner {
    public:
        Asset(AssetLoader* loader, ghoul::filesystem::Directory directory);
        Asset(AssetLoader* loader, ghoul::filesystem::Directory baseDirectory, std::string assetPath);
        std::string id();
        std::string assetFilePath();
        std::string assetName();
        std::string assetDirectory();
        AssetLoader* loader();
        ghoul::Dictionary syncDictionary();
        std::string syncDirectory();
        bool isInitialized();
        bool hasLuaTable();
        void initialize();
        void deinitialize();

        bool hasDependency(Asset* asset, InitializationRequirement initReq);
        void addDependency(Asset* asset, bool togglableInitReq, InitializationRequirement initReq);
        void setInitializationRequirement(Asset* dependency, InitializationRequirement initReq);
        void removeDependency(Asset* asset);
        void removeDependency(const std::string& assetId);
        void dependantDidInitialize(Asset* dependant);
        void dependantWillDeinitialize(Asset* dependant);

        bool hasInitializedDependants(InitializationRequirement initReq);
        bool hasDependant(InitializationRequirement initReq);
    private:
        std::string resolveLocalResource(std::string resourceName);
        std::string resolveSyncedResource(std::string resourceName);

        // lua methods
        friend int assetloader::resolveLocalResource(lua_State* state);
        int resolveLocalResourceLua();

        friend int assetloader::resolveSyncedResource(lua_State* state);
        int resolveSyncedResourceLua();

        bool _hasLuaTable;
        bool _initialized;
        AssetLoader* _loader;

        // Base name of .asset file
        std::string _assetName;

        // Asbolute path to directory with the .asset file
        std::string _assetDirectory;

        // Other assets that this asset depend on
        std::vector<Dependency> _dependencies;

        // Other assets that depend on this asset
        std::vector<Asset*> _dependants;

        std::vector<std::unique_ptr<DependencyToggle>> _dependencyToggles;
        // Other assets that this asset may toggle
        //std::vector<Asset*> _toggles;

        // Other assets that may toggle this asset
        //std::vector<Asset*> _togglers;
    };

    AssetLoader(ghoul::lua::LuaState* _luaState, std::string assetRoot, std::string syncRoot);
    ~AssetLoader() = default;


    void loadAsset(const std::string& identifier);
    void unloadAsset(const std::string& identifier);

    scripting::LuaLibrary luaLibrary();
    
    ghoul::lua::LuaState* luaState();
    ghoul::filesystem::Directory currentDirectory();
    Asset* rootAsset();
    const std::string& syncRoot();

private:
    Asset* importAsset(
        const std::string& identifier,
        bool togglableInitializationRequirement = false,
        bool toggleOn = true);

    void pushAsset(Asset* asset);
    void popAsset();
    void updateLuaGlobals();
   
    std::unique_ptr<Asset> _rootAsset;
    std::map<std::string, std::unique_ptr<Asset>> _importedAssets;
    std::vector<Asset*> _assetStack;

    std::string _syncRoot;

    friend int assetloader::importAsset(lua_State* state);
    friend int assetloader::importAssetToggle(lua_State* state);
    int importAssetLua(
        std::string assetName,
        bool togglableInitializationRequirement = false,
        bool toggleOn = true);

    ghoul::lua::LuaState* _luaState;
};




} // namespace openspace

#endif // __OPENSPACE_CORE___ASSETLOADER___H__