#include "StdAfx.h"
#include "Versioning.h"
#include "CSProExecutables.h"


namespace
{
    constexpr int CSProReleaseDate = 20260730;

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

    if constexpr(IsPrerelease)
    {
        version.append(" (")
               .append(GetReleaseIdentifier(false))
               .push_back(')');
    }

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


const char* Versioning::GetReleaseIdentifier(const ::ReleaseType release_type, const bool with_hyphen_prefix)
{
    if( release_type == ReleaseType::Release )
        return "";

    constexpr const char* IdentifiersWithHyphens[] = { "-alpha", "-beta", "-rc", };

    ASSERT(static_cast<size_t>(release_type) >= 0 &&
           static_cast<size_t>(release_type) < _countof(IdentifiersWithHyphens));

    const char* const identifier = IdentifiersWithHyphens[static_cast<size_t>(release_type)];

    return with_hyphen_prefix ? identifier : ( identifier + 1 );
}


const char* Versioning::GetReleaseIdentifier(const bool with_hyphen_prefix)
{
    return GetReleaseIdentifier(ReleaseType, with_hyphen_prefix);
}
