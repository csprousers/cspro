#include "StdAfx.h"
#include "DateTime.h"
#include "Special.h"
#include <chrono>


template<typename T/* = double*/>
T GetTimestamp()
{
    using namespace std::chrono;

    if constexpr(std::is_same_v<T, double>)
    {
        milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        return static_cast<T>(ms.count()) / 1000;
    }

    else
    {
        seconds s = duration_cast<seconds>(system_clock::now().time_since_epoch());
        return static_cast<T>(s.count());
    }
}

template CLASS_DECL_ZTOOLSO double GetTimestamp();
template CLASS_DECL_ZTOOLSO int64_t GetTimestamp();


template<typename T/* = std::string*/>
T FormatTimestamp(double timestamp, const std::string& formatter)
{
    constexpr std::string_view ValidFormatters = "aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%";
    constexpr std::string_view InvalidFormatterText = "<invalid formatter>";

    // ensure that the formatting options are all valid, with an optimization for %c, which will
    // be used the most frequently (by the Paradata Viewer)
    if( formatter != "%c" )
    {
        auto formatter_end = formatter.cend();

        for( auto formatter_itr = formatter.cbegin(); formatter_itr != formatter_end; ++formatter_itr )
        {
            // the next character must be in the valid formatters
            if( *formatter_itr == '%' ) 
            {
                if( ++formatter_itr == formatter_end ||
                    ValidFormatters.find(*formatter_itr) == std::string_view::npos )
                {
                    return UTF8Convert::GetString<T>(InvalidFormatterText);
                }
            }
        }
    }

    constexpr int BufferSize = 256;
    std::string buffer(BufferSize, '\0');

    time_t tm = static_cast<time_t>(timestamp);
    std::tm* local_time = localtime(&tm);

    if( local_time == nullptr )
        return T();

    int string_length = strftime(buffer.data(), buffer.size(), formatter.c_str(), local_time);
    buffer.resize(string_length);

    return UTF8Convert::GetString<T>(std::move(buffer));
}

template CLASS_DECL_ZTOOLSO std::string FormatTimestamp(double timestamp, const std::string& formatter);
template CLASS_DECL_ZTOOLSO std::wstring FormatTimestamp(double timestamp, const std::string& formatter);


long GetUtcOffset()
{
#ifdef WIN32
    TIME_ZONE_INFORMATION timeZoneInformation;
    GetTimeZoneInformation(&timeZoneInformation);
    return -1 * ( timeZoneInformation.Bias + timeZoneInformation.DaylightBias );
#else
    time_t tm = time(nullptr);
    struct tm lt = {0};
    localtime_r(&tm,&lt);
    return lt.tm_gmtoff / 60; // tm_gmtoff is in seconds
#endif
}


struct tm GetLocalTime()
{
    struct tm tp;
    time_t t = time(&t);
    memcpy(&tp, localtime(&t), sizeof(tp));
    return tp;
}


void ReadableTimeToTm(std::tm* ptm,int iYYYYMMDD,int iHHMMSS)
{
    memset(ptm,0,sizeof(tm));
    ptm->tm_year = DateHelper::GetYYYY(iYYYYMMDD) - 1900;
    ptm->tm_mon = DateHelper::GetMM(iYYYYMMDD) - 1;
    ptm->tm_mday = DateHelper::GetDD(iYYYYMMDD);
    ptm->tm_hour = iHHMMSS / 10000;
    ptm->tm_min = ( iHHMMSS / 100 ) % 100;
    ptm->tm_sec = iHHMMSS % 100;
}


void TmToReadableTime(const std::tm* ptm,int* pYear,int* pMonth,int* pDay,int* pHour,int* pMinute,int* pSecond)
{
#define TmToReadableTime_Assign(pValue,iAssignedValue) { if( pValue != nullptr ) *pValue = ( iAssignedValue ); }

    TmToReadableTime_Assign(pYear,ptm->tm_year + 1900);
    TmToReadableTime_Assign(pMonth,ptm->tm_mon + 1);
    TmToReadableTime_Assign(pDay,ptm->tm_mday);
    TmToReadableTime_Assign(pHour,ptm->tm_hour);
    TmToReadableTime_Assign(pMinute,ptm->tm_min);
    TmToReadableTime_Assign(pSecond,ptm->tm_sec);
}


double FormatDateToDouble(CString csFormat,int iYear,int iMonth,int iDay)
{
    double dRet = 0;
    bool bHadEntry = false;

    for( int i = 0; i < csFormat.GetLength(); )
    {
        int iValue = -1;
        int iTypeWidth = 2;

        if( ( ( i + 4 ) <= csFormat.GetLength() ) && ( csFormat.Mid(i,4).CompareNoCase(_T("YYYY")) == 0 ) )
        {
            iValue = iYear;
            iTypeWidth = 4;
        }

        else if( ( i + 2 ) <= csFormat.GetLength() )
        {
            CString csType = csFormat.Mid(i,2);

            if( csType.CompareNoCase(_T("YY")) == 0 )
            {
                iValue = ( iYear % 100 );
            }

            else if( csType.CompareNoCase(_T("MM")) == 0 )
            {
                iValue = iMonth;
            }

            else if( csType.CompareNoCase(_T("DD")) == 0 )
            {
                iValue = iDay;
            }
        }

        if( iValue < 0 )
        {
            dRet = DEFAULT;
            break;
        }

        if( i > 0 )
            dRet *= pow(10,iTypeWidth);

        dRet += iValue;
        bHadEntry = true;
        i += iTypeWidth;
    }

    if( !bHadEntry )
        dRet = DEFAULT;

    return dRet;
}


namespace DateHelper
{
    const int DaysPerMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    int GetDaysInMonth(int year, int month)
    {
        ASSERT(month >= 1 && month <= 12);

        return ( month == 2 && IsLeapYear(year) ) ? 29 : DaysPerMonth[month - 1];
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
