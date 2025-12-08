#include "StdAfx.h"
#include "Versioning.h"
#include "CSProExecutables.h"


namespace
{
    constexpr int CSProReleaseDate = 20240319;
    constexpr std::string_view BetaDesignation_sv = "beta";

    // to override the version that appears in the UI (but not serialized files), replace 'x' with the version override
    constexpr std::string_view NumberDetailedTextOverride_sv = "x.x.x";
}


int Versioning::GetReleaseDate()
{
    return CSProReleaseDate;
}


std::string Versioning::GetReleaseDateString()
{
#ifdef WIN_DESKTOP
    // if the shift key is pressed, display the build date/time
    if( ( GetKeyState(VK_SHIFT) & 0x8000 ) != 0 )
        return FormatText("%s %s", __DATE__, __TIME__);
#endif

    // in debug mode on the desktop, show the executable date/time
#if defined(_DEBUG) && defined(WIN_DESKTOP)
    return DateTime::LocalDateString(CSProExecutables::GetModuleModifiedTime(), false) + " (debug build)";

#else
    // otherwise show the release date
    const DateTime::Components date_time_components
    {
        DateHelper::GetYYYY(CSProReleaseDate), DateHelper::GetMM(CSProReleaseDate), DateHelper::GetDD(CSProReleaseDate),
        0, 0, 0
    };

    const int64_t release_date = DateTime::CreateTime(date_time_components, true);
    return DateTime::LocalDateString(release_date, false);
#endif
}


std::string Versioning::GetVersionString(std::string version, const bool include_cspro)
{
    if constexpr(NumberDetailedTextOverride_sv != "x.x.x")
        version = std::string_view(NumberDetailedTextOverride_sv).substr(0, version.length());

    if( include_cspro )
        version.insert(0, "CSPro ");

    if constexpr(IsBeta)
        version.append(" (beta)");

    return version;
}


std::string Versioning::GetVersionString(const bool include_cspro/* = false*/)
{
    return GetVersionString(NumberText, include_cspro);
}


std::string Versioning::GetVersionDetailedString(const bool include_cspro/* = false*/)
{
    std::string version_text = GetVersionString(NumberDetailedText, include_cspro);

    // add the architecture
    version_text.append(IsX64() ? " (64-bit)" : " (32-bit)");

    return version_text;
}
