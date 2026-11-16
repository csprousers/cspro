#pragma once

#include <zToolsO/EnumHelpers.h>
#include <zToolsO/WinSettings.h>
#include <zEditO/resource_shared.h>


namespace DeprecationWarnings
{
    enum class Level { None, Most, All };
    constexpr Level DefaultLevel = Level::Most;

    inline Level GetLevel()
    {
        return static_cast<Level>(WinSettings::Read<DWORD>(WinSettings::Type::DeprecationWarnings, static_cast<DWORD>(DefaultLevel)));
    }

    inline void SetLevel(const Level level)
    {
        WinSettings::Write<DWORD>(WinSettings::Type::DeprecationWarnings, static_cast<DWORD>(level));
    }

    constexpr Level GetLevelFromId(const UINT nID)
    {
        return AddToEnum(Level::None, nID - ID_CODE_DEPRECATION_WARNINGS_NONE);
    }
}
