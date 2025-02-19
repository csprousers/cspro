#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO DateTime
{
public:
    // Returns the current time.
    static int64_t Now() { return time(nullptr); }

    // Returns the current year in the local timezone.
    static int LocalYear() { return TmToYear(LocalTm(Now())); }

    // Returns the specified date in the local timezone in the specified strftime format.
    static std::string LocalDateTimeString(int64_t time, cs::string_sz formatter = "%Y-%m-%d %H:%M:%S");

    // Returns the specified, or current date, in the local timezone in the strftime formats:
    // - "%b %d, %Y" (e.g., Mar 13, 2024), or
    // - "%B %d, %Y" (e.g., March 13, 2024), or
    static std::string LocalDateString(int64_t time, bool use_abbreviated_month = true);
    static std::string LocalDateString(bool use_abbreviated_month = true) { return LocalDateString(Now(), use_abbreviated_month); }

    // Returns the current time in the local timezone in the format HH:MM:SS.
    static std::string LocalTimeString();

    // Returns the time value broken up into its components.
    // If adjusting for the local timezone, the time will be converted to the local timezone (as opposed to UTC).
    struct Components { int year; int month; int day; int hour; int minute; int second; };

    static Components TimeToComponents(const tm& tm);
    static Components TimeToComponents(int64_t time, bool adjust_to_local_time = false);

    // Returns the time value in the format YYYYMMDD / YYMMDD / HHMMSS / YYYYMMDDHHMMSS.
    static int TimeToYYYYMMDD(const Components& components);
    static int TimeToYYMMDD(const Components& components);
    static int TimeToHHMMSS(const Components& components);
    static int64_t TimeToYYYYMMDDHHMMSS(const Components& components);

    static int TimeToYYYYMMDD(int64_t time, bool adjust_to_local_time = false)           { return TimeToYYYYMMDD(TimeToComponents(time, adjust_to_local_time)); }
    static int TimeToYYMMDD(int64_t time, bool adjust_to_local_time = false)             { return TimeToYYMMDD(TimeToComponents(time, adjust_to_local_time)); }
    static int TimeToHHMMSS(int64_t time, bool adjust_to_local_time = false)             { return TimeToHHMMSS(TimeToComponents(time, adjust_to_local_time)); }
    static int64_t TimeToYYYYMMDDHHMMSS(int64_t time, bool adjust_to_local_time = false) { return TimeToYYYYMMDDHHMMSS(TimeToComponents(time, adjust_to_local_time)); }

    // Returns the time value in RFC 3339 format (YYYY-MM-DDTHH:MM:SSZ).
    static std::string TimeToRFC3339(int64_t time);

    // Creates a time object from a tm struct, Components object, or integers.
    static int64_t CreateTime(tm tm, bool adjust_to_local_time = false);
    static int64_t CreateTime(const Components& components, bool adjust_to_local_time = false) { return CreateTime(ToTm(components), adjust_to_local_time); }
    static int64_t CreateTime(int yyyymmdd, int hhmmss, bool adjust_to_local_time = false)     { return CreateTime(ToTm(yyyymmdd, hhmmss), adjust_to_local_time); }

private:
    static constexpr int TmToYear(const tm& tm)  { return tm.tm_year + 1900; }
    static constexpr int YearToTm(int year)      { return year - 1900; }
    static constexpr int TmToMonth(const tm& tm) { return tm.tm_mon + 1; }
    static constexpr int MonthToTm(int month)    { return month - 1; }

    static const tm& UtcTm(int64_t time);
    static const tm& LocalTm(int64_t time);

    static tm ToTm(const Components& components);
    static tm ToTm(int yyyymmdd, int hhmmss);
};


// Gets a timestamp to the millisecond level. Defined to return either double or int64_t.
template<typename T = double>
CLASS_DECL_ZTOOLSO T GetTimestamp();

// Formats a timestamp to a string using std::strftime formatting.
CLASS_DECL_ZTOOLSO std::string FormatTimestamp(double timestamp, const std::string& formatter = "%c");

// Returns a string describing the elapsed time in the format HH:MM:SS or similar to "5 seconds".
CLASS_DECL_ZTOOLSO std::string GetElapsedTimeTextHHMMSS(int elapsed_seconds);
inline std::string GetElapsedTimeTextHHMMSS(int64_t elapsed_seconds)                        { return GetElapsedTimeTextHHMMSS(static_cast<int>(elapsed_seconds)); }
inline std::string GetElapsedTimeTextHHMMSS(int64_t start_timestamp, int64_t end_timestamp) { return GetElapsedTimeTextHHMMSS(end_timestamp - start_timestamp); }
CLASS_DECL_ZTOOLSO std::string GetElapsedTimeText(int64_t start_timestamp, int64_t end_timestamp);

// Returns a string indicating how long ago the timestamp is from the current time.
CLASS_DECL_ZTOOLSO std::string GetTimeAgo(double timestamp);

// Gets the number of minutes off UTC of the system clock.
CLASS_DECL_ZTOOLSO long GetUtcOffset();

// Converts a date using a formatting string, returning std::nullopt on error.
// If many formatters are provided, the value may exceed the capacity of uint64_t.
CLASS_DECL_ZTOOLSO std::optional<uint64_t> FormatDate(std::string_view format_sv, int year, int month, int day);
CLASS_DECL_ZTOOLSO std::optional<uint64_t> FormatDate(std::string_view format_sv, const DateTime::Components& date_time_components);


namespace DateHelper
{
    constexpr bool IsLeapYear(int year)
    {
        return ( ( year % 4 == 0 ) && ( ( year % 100 ) != 0 || ( year % 400 ) == 0 ) );
    }

    constexpr int GetDaysInYear(int year)
    {
        return IsLeapYear(year) ? 366 : 365;
    }

    CLASS_DECL_ZTOOLSO int GetDaysInMonth(int year, int month);

    CLASS_DECL_ZTOOLSO bool IsValid(int year_month_day);
    CLASS_DECL_ZTOOLSO bool IsValid(int year, int month, int day);

    constexpr int ToYYYYMMDD(int year, int month, int day)
    {
        return ( year * 10000 ) + ( month * 100 ) + day;
    }

    constexpr int GetYYYY(int year_month_day)
    {
        return ( year_month_day / 10000 );
    }

    constexpr int GetMM(int year_month_day)
    {
        return ( ( year_month_day / 100 ) % 100 );
    }

    constexpr int GetDD(int year_month_day)
    {
        return ( year_month_day % 100 );
    }

    template<typename T = int> constexpr T SecondsInMinute(T unit = 1) { return unit * 60; }
    template<typename T = int> constexpr T SecondsInHour(T unit = 1)   { return unit * SecondsInMinute(60); }
    template<typename T = int> constexpr T SecondsInDay(T unit = 1)    { return unit * SecondsInHour(24); }
    template<typename T = int> constexpr T SecondsInWeek(T unit = 1)   { return unit * SecondsInDay(7); }
}


#ifndef WIN32
time_t _mkgmtime(struct tm *tm);
#endif
