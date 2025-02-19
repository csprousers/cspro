#pragma once

#include <zToolsO/WinSettings.h>


namespace LocalhostSettings
{
    // --------------------------------------------------------------------------
    // port options
    // --------------------------------------------------------------------------

    constexpr int MinPort = 49215; // ports in the IANA recommended range
    constexpr int MaxPort = 65535;

    inline std::optional<int> GetPreferredPort()
    {
        const int port = static_cast<int>(WinSettings::Read<DWORD>(WinSettings::Type::LocalhostPreferredPort, static_cast<DWORD>(-1)));

        return ( port >= MinPort && port <= MaxPort ) ? std::make_optional(port) :
                                                        std::nullopt;
    }


    // --------------------------------------------------------------------------
    // automatic drive mappings options
    // --------------------------------------------------------------------------

    constexpr char AutomaticallyMappedDrivesSeparator = '|';

    inline std::vector<std::string> GetDrivesToAutomaticallyMap(const std::string& automatically_mapped_drives_text)
    {
        return SO::SplitString(automatically_mapped_drives_text, AutomaticallyMappedDrivesSeparator, true, false);
    }

    inline std::vector<std::string> GetDrivesToAutomaticallyMap()
    {
        const std::string automatically_mapped_drives_text = WinSettings::Read<std::string>(WinSettings::Type::LocalhostAutomaticallyMappedDrives, std::string());

        return GetDrivesToAutomaticallyMap(automatically_mapped_drives_text);
    }
}
