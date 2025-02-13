#pragma once

#include <zToolsO/zToolsO.h>


///<summary>Gets a timestamp to the millisecond level.</summary>
template<typename T = double>
CLASS_DECL_ZTOOLSO T GetTimestamp();

///<summary>Formats a timestamp to a string using strftime formatting.</summary>
template<typename T = std::string>
CLASS_DECL_ZTOOLSO T FormatTimestamp(double timestamp, const std::string& formatter);

///<summary>Gets the number of minutes off UTC of the system clock.</summary>
CLASS_DECL_ZTOOLSO long GetUtcOffset();

///<summary>Gets the local time.</summary>
CLASS_DECL_ZTOOLSO struct tm GetLocalTime();

///<summary>Get a time value from date/time integers in the format YYYYMMDD HHMMSS.</summary>
CLASS_DECL_ZTOOLSO void ReadableTimeToTm(std::tm* ptm,int iYYYYMMDD,int iHHMMSS);

///<summary>Converts a time value to date/time integers.</summary>
CLASS_DECL_ZTOOLSO void TmToReadableTime(const std::tm* ptm,int* pYear,int* pMonth,int* pDay,int* pHour,int* pMinute,int* pSecond);

///<summary>Converts a date to a double using a formatting string.</summary>
CLASS_DECL_ZTOOLSO double FormatDateToDouble(CString csFormat,int iYear,int iMonth,int iDay);


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
