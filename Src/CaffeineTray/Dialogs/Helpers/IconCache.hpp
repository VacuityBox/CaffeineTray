#pragma once

#include <filesystem>
#include <map>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <shellapi.h>

namespace {
    namespace fs = std::filesystem;
}

namespace Caffeine {

constexpr static auto INVALID_ICON_ID = int{-1};

class IconCache final
{
    HIMAGELIST mImgList;
    int        mNextIndex;

    std::map<std::wstring, int> mIconMap;

public:
    IconCache ()
        : mNextIndex (0)
    {
        mImgList = ImageList_Create(
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            ILC_COLOR32,
            16,
            16
        );
    }

    ~IconCache ()
    {
        ImageList_Destroy(mImgList);
    }

    auto Insert (std::wstring name, HICON icon) -> int
    {
        if (icon == NULL)
        {
            return INVALID_ICON_ID;
        }

        const auto& cachedIcon = mIconMap.find(name);
        if (cachedIcon != mIconMap.end())
        {
            return cachedIcon->second;
        }

        ImageList_AddIcon(mImgList, icon);
        mIconMap.insert(std::make_pair(std::move(name), mNextIndex));
                
        return mNextIndex++;
    }

    auto Insert (fs::path path) -> int
    {
        auto pathStr = path.wstring();

        auto id = GetId(pathStr);
        if (id == INVALID_ICON_ID)
        {
            auto hInstance = GetModuleHandle(NULL);
            auto icon = ExtractIcon(hInstance, pathStr.c_str(), 0);
            if (icon != NULL && icon != reinterpret_cast<HICON>(1))
            {
                id = Insert(pathStr, icon);
                DestroyIcon(icon);
            }
        }

        return id;
    }

    auto GetId (const std::wstring name) -> int
    {
        const auto& cachedIcon = mIconMap.find(name);
        if (cachedIcon != mIconMap.end())
        {
            return cachedIcon->second;
        }

        return INVALID_ICON_ID;
    }

    auto GetImageList () -> HIMAGELIST&
    {
        return mImgList;
    }
};

} // namespace Caffeine
