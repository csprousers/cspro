#pragma once

#if defined(WIN_DESKTOP) || defined(_CONSOLE)

#include <zToolsO/WinSettings.h>


// some helper functions that are used by several programs
namespace SharedSettings
{
    inline bool ViewNamesInTree()
    {
        return ( WinSettings::Read<DWORD>(WinSettings::Type::ViewNamesInTree, 0) == 1 );
    }

    inline void ToggleViewNamesInTree(std::optional<bool> value = std::nullopt)
    {
        bool new_value = value.has_value() ? *value : !ViewNamesInTree();
        WinSettings::Write<DWORD>(WinSettings::Type::ViewNamesInTree, new_value);
    }


    inline bool AppendLabelsToNamesInTree()
    {
        return ( WinSettings::Read<DWORD>(WinSettings::Type::AppendLabelsToNamesInTree, 0) == 1 );
    }

    inline void ToggleAppendLabelsToNamesInTree()
    {
        WinSettings::Write<DWORD>(WinSettings::Type::AppendLabelsToNamesInTree, !AppendLabelsToNamesInTree());
    }
}

#endif
