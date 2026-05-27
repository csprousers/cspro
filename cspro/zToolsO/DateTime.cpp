#include "StdAfx.h"
#include "DateTime.h"
#include <chrono>


// because time_t is 64-bits on Windows and 32-bits on Android,
// the cross-platform int64_t will be used instead of time_t
static_assert(sizeof(time_t) <= sizeof(int64_t));

namespace portable
{
#ifdef WIN32
    inline tm* gmtime(const int64_t* const time)    { return ::gmtime(time); }
    inline tm* localtime(const int64_t* const time) { return ::localtime(time); }
#elif defined(ANDROID) || defined(WASM)
    inline time_t get_time(const int64_t* const time) { ASSERT(time != nullptr); return static_cast<time_t>(*time); }
    inline tm* gmtime(const int64_t* const time)      { const time_t t = get_time(time); return ::gmtime(&t); }
    inline tm* localtime(const int64_t* const time)   { const time_t t = get_time(time); return ::localtime(&t); }
#else
    static_assert(false);
#endif
}


const tm& DateTime::UtcTm(const int64_t time)
{
    const tm* const tm = portable::gmtime(&time);
    ASSERT(tm != nullptr);
    return *tm;
}


const tm& DateTime::LocalTm(const int64_t time)
{
    const tm* const tm = portable::localtime(&time);
    ASSERT(tm != nullptr);
    return *tm;
}


tm DateTime::ToTm(const Components& components)
{
    tm tm;
    tm.tm_isdst = -1;

    tm.tm_year = YearToTm(components.year);
    tm.tm_mon = MonthToTm(components.month);
    tm.tm_mday = components.day;
    tm.tm_hour = components.hour;
    tm.tm_min = components.minute;
    tm.tm_sec = components.second;

    return tm;
}


tm DateTime::ToTm(int yyyymmdd, int hhmmss)
{
    tm tm;
    tm.tm_isdst = -1;

    tm.tm_mday = yyyymmdd % 100;
    yyyymmdd /= 100;
    tm.tm_mon = MonthToTm(yyyymmdd % 100);
    tm.tm_year = YearToTm(yyyymmdd / 100);

    tm.tm_sec = hhmmss % 100;
    hhmmss /= 100;
    tm.tm_min = hhmmss % 100;
    tm.tm_hour = hhmmss / 100;

    return tm;
}


std::string DateTime::LocalDateTimeString(const int64_t time, const cs::string_sz formatter/* = "%Y-%m-%d %H:%M:%S"*/) noexcept
{
    constexpr size_t BufferSize = 30;
    std::string text(BufferSize, '\0');

    const tm& tm = LocalTm(time);
    const size_t length = std::strftime(text.data(), BufferSize, formatter.c_str(), &tm);

    ASSERT(length > 0);
    text.resize(length);

    return text;
}


std::string DateTime::LocalDateString(const int64_t time, const bool use_abbreviated_month/* = true*/)
{
    return LocalDateTimeString(time, use_abbreviated_month ? "%b %d, %Y" :
                                                             "%B %d, %Y");
}


std::string DateTime::LocalTimeString()
{
    const tm& tm = LocalTm(Now());
    return FormatText("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
}


DateTime::Components DateTime::TimeToComponents(const tm& tm)
{
    return Components
    {
        TmToYear(tm),
        TmToMonth(tm),
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec
    };
}


DateTime::Components DateTime::TimeToComponents(const int64_t time, const bool adjust_to_local_time/* = false*/)
{
    const tm* const tm = adjust_to_local_time ? portable::localtime(&time) :
                                                portable::gmtime(&time);
    ASSERT(tm != nullptr);

    return TimeToComponents(*tm);
}


int DateTime::TimeToYYYYMMDD(const Components& components)
{
    return components.year * 10000 +
           components.month * 100 +
           components.day;
}


int DateTime::TimeToYYMMDD(const Components& components)
{
    return ( components.year % 100 ) * 10000 +
           components.month * 100 +
           components.day;
}


int DateTime::TimeToHHMMSS(const Components& components)
{
    return components.hour * 10000 +
           components.minute * 100 +
           components.second;
}


int64_t DateTime::TimeToYYYYMMDDHHMMSS(const Components& components)
{
    return static_cast<int64_t>(TimeToYYYYMMDD(components)) * 1000000 +
           TimeToHHMMSS(components);
}


std::string DateTime::TimeToRFC3339(const int64_t time)
{
    const Components components = TimeToComponents(time, false);

    return FormatText("%04d-%02d-%02dT%02d:%02d:%02dZ",
                      components.year, components.month, components.day,
                      components.hour, components.minute, components.second);
}


int64_t DateTime::CreateTime(tm tm, const bool adjust_to_local_time/* = false*/)
{
    if( adjust_to_local_time )
    {
        tm.tm_isdst = -1;
        return mktime(&tm);
    }

    else
    {
        return _mkgmtime(&tm);
    }
}


int DateTime::GetUtcOffsetNow()
{
#ifdef WIN32
    DYNAMIC_TIME_ZONE_INFORMATION dtzi;
    const DWORD result = GetDynamicTimeZoneInformation(&dtzi);

    if( result == TIME_ZONE_ID_INVALID )
        return ReturnProgrammingError(0);

    if( result == TIME_ZONE_ID_DAYLIGHT )
        dtzi.Bias += dtzi.DaylightBias;

    return -1 * dtzi.Bias;
#else
    time_t tm = time(nullptr);
    struct tm local_time;
    localtime_r(&tm, &local_time);
    return local_time.tm_gmtoff / 60; // tm_gmtoff is in seconds
#endif
}


int DateTime::GetUtcOffset(const Components& components)
{
    // get the time locally...
    const tm local_time = ToTm(components);
    const time_t local_timestamp = static_cast<time_t>(CreateTime(local_time, true));

    // ...convert it back to UTC...
    tm utc_time;

#if defined(_WIN32)
    gmtime_s(&utc_time, &local_timestamp);
#else
    gmtime_r(&local_timestamp, &utc_time);
#endif

    const int64_t utc_timestamp = CreateTime(utc_time, true);

    // ...and then return the difference in minutes
    return static_cast<int>(( local_timestamp - utc_timestamp ) / 60);
}


template<typename T/* = int64_t*/>
T GetTimestamp()
{
    using namespace std::chrono;

    if constexpr(std::is_same_v<T, int64_t>)
    {
        const seconds s = duration_cast<seconds>(system_clock::now().time_since_epoch());
        return static_cast<T>(s.count());
    }

    else
    {
        static_assert(std::is_same_v<T, double>);
        const milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        return static_cast<T>(ms.count()) / 1000;
    }
}

template CLASS_DECL_ZTOOLSO int64_t GetTimestamp();
template CLASS_DECL_ZTOOLSO double GetTimestamp();


std::string FormatTimestamp(const double timestamp, const std::string& formatter/* = "%c"*/)
{
    constexpr std::string_view ValidFormatters_sv = "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%";
    constexpr std::string_view InvalidFormatterText_sv = "<invalid formatter>";

    // ensure that the formatting options are all valid, with an optimization for %c, which will
    // be used the most frequently (by the Paradata Viewer)
    if( formatter != "%c" )
    {
        const auto formatter_end = formatter.cend();

        for( auto formatter_itr = formatter.cbegin(); formatter_itr != formatter_end; ++formatter_itr )
        {
            // the next character must be in the valid formatters
            if( *formatter_itr == '%' )
            {
                if( ++formatter_itr == formatter_end ||
                    ValidFormatters_sv.find(*formatter_itr) == std::string_view::npos )
                {
                    return std::string(InvalidFormatterText_sv);
                }
            }
        }
    }

    constexpr int BufferSize = 256;
    std::string buffer(BufferSize, '\0');

    const time_t time = static_cast<time_t>(timestamp);
    tm* const local_time = localtime(&time);

    if( local_time == nullptr )
        return std::string();

    const size_t string_length = std::strftime(buffer.data(), buffer.size(), formatter.c_str(), local_time);
    buffer.resize(string_length);

    return buffer;
}


std::string GetElapsedTimeTextHHMMSS(int elapsed_seconds)
{
    ASSERT(elapsed_seconds >= 0);

    const int seconds = elapsed_seconds % 60;
    elapsed_seconds /= 60;

    return FormatText("%02d:%02d:%02d", elapsed_seconds / 60,
                                        elapsed_seconds % 60,
                                        seconds);
}


std::string GetElapsedTimeText(const int64_t start_timestamp, const int64_t end_timestamp)
{
    const int elapsed_seconds = static_cast<int>(end_timestamp - start_timestamp);
    ASSERT(elapsed_seconds >= 0);

    return ( elapsed_seconds < 60 ) ? FormatText("%d second%s", elapsed_seconds, PluralizeWord(elapsed_seconds)) :
                                      FormatText("%d:%02d minutes", elapsed_seconds / 60, elapsed_seconds % 60);
}


std::string GetTimeAgo(const double timestamp)
{
    const double seconds_elapsed = std::max(GetTimestamp<double>() - timestamp, 0.0);
    double current_threshold = 1;
    std::string time_ago;

    auto format_time_ago = [&](const double new_multiplier, const char* name, const char* single_name = nullptr)
    {
        const double new_threshold = new_multiplier * current_threshold;

        if( seconds_elapsed < new_threshold )
        {
            const int units = static_cast<int>(seconds_elapsed / current_threshold);

            if( units == 1 )
            {
                time_ago = ( single_name != nullptr ) ? single_name :
                                                        FormatText("a %s ago", name);
            }

            else
            {
                time_ago = FormatText("%d %ss ago", units, name);
            }

            return true;
        }

        else
        {
            current_threshold = new_threshold;
            return false;
        }
    };

    format_time_ago(60, "second") ||
    format_time_ago(60, "minute") ||
    format_time_ago(24, "hour", "an hour ago") ||
    format_time_ago(365.25 / 12, "day", "yesterday") ||
    format_time_ago(12, "month") ||
    format_time_ago(100000, "year");

    return time_ago;
}


std::optional<uint64_t> FormatDate(std::string_view format_sv, const int year, const int month, const int day)
{
    std::optional<uint64_t> date;

    while( format_sv.length() >= 2 )
    {
        const int first_ch = std::toupper(format_sv.front());

        if( first_ch != std::toupper(format_sv[1]) )
            return std::nullopt;

        auto add_value = [&](const int value, const int value_width)
        {
            ASSERT(value_width == 2 || value_width == 4);

            if( date.has_value() )
            {
                *date = *date * ( ( value_width == 2 ) ? 100 : 10000 ) + value;
            }

            else
            {
                date = value;
            }

            format_sv = format_sv.substr(value_width);
        };

        switch( first_ch )
        {
            case 'Y':
                SO::StartsWithNoCase(format_sv.substr(2, 2), "YY") ? add_value(year, 4) :
                                                                     add_value(year % 100, 2);
                break;

            case 'M':
                add_value(month, 2);
                break;

            case 'D':
                add_value(day, 2);
                break;

            default:
                return std::nullopt;
        }
    }

    // there should be no remaining formats
    if( !format_sv.empty() )
        date.reset();

    return date;
}


std::optional<uint64_t> FormatDate(const std::string_view format_sv, const DateTime::Components& date_time_components)
{
    return FormatDate(format_sv, date_time_components.year, date_time_components.month, date_time_components.day);
}


namespace DateHelper
{
    constexpr int DaysPerMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    int GetDaysInMonth(int year, int month)
    {
        ASSERT(month >= 1 && month <= 12);

        return ( month == 2 && IsLeapYear(year) ) ? 29 :
                                                    DaysPerMonth[month - 1];
    }

    bool IsValid(int year_month_day)
    {
        if( year_month_day < 10000101 || year_month_day > 99991231 )
            return false;

        return IsValid(GetYYYY(year_month_day), GetMM(year_month_day), GetDD(year_month_day));
    }

    bool IsValid(int year, int month, int day)
    {
        return ( month >= 1 && month <= 12 &&
                 day >= 1 && day <= GetDaysInMonth(year, month) );
    }
}


#ifndef WIN32
// Android doesn't have _mkgmtime
time_t _mkgmtime(struct tm *tm)
{
    time_t ret;
    char *tz;

    tz = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();
    return ret;
}
#endif
