#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Date.h>


// --------------------------------------------
// timestamp-style functions
// --------------------------------------------

double CIntDriver::extimestamp(int iExpr)
{
    const auto& timestamp_node = GetNode<Nodes::Timestamp>(iExpr);

    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) || timestamp_node.type == Nodes::Timestamp::Type::Current )
    {
        return GetTimestamp();
    }

    else if( timestamp_node.type == Nodes::Timestamp::Type::RFC3339 )
    {
        std::wstring date_time = EvalAlphaExpr(timestamp_node.argument);

        if( date_time.length() < PortableFunctions::MinLengthRFC3339DateTimeString )
            return DEFAULT;

        return static_cast<double>(PortableFunctions::ParseRFC3339DateTime(std::move(date_time)));
    }

    else
    {
        ASSERT(timestamp_node.type == Nodes::Timestamp::Type::SpecifiedDate);
        const auto& arguments_list_node = GetListNode(timestamp_node.argument);
        ASSERT(arguments_list_node.number_elements >= 4 && arguments_list_node.number_elements <= 7);
        bool special_value_read = false;

        auto evaluate_int = [&](size_t index)
        {
            double value = evalexpr(arguments_list_node.elements[index]);

            if( IsSpecial(value) || value < 0 )
            {
                special_value_read = true;
                return 0;
            }
            
            return static_cast<int>(value);
        };

        tm time_struct
        {
            0,
            0,
            0,
            evaluate_int(2),        // day
            evaluate_int(1) - 1,    // month
            evaluate_int(0) - 1900, // year
            0,
            0,
            0
        };

        if( arguments_list_node.number_elements >= 5 )
        {
            time_struct.tm_hour = evaluate_int(3);

            if( arguments_list_node.number_elements >= 6 )
            {
                time_struct.tm_min = evaluate_int(4);

                if( arguments_list_node.number_elements == 7 )
                    time_struct.tm_sec = evaluate_int(5);
            }
        }

        if( special_value_read )
            return DEFAULT;

        double timestamp = static_cast<double>(_mkgmtime(&time_struct));
        int utc_offset_expression = arguments_list_node.elements[arguments_list_node.number_elements - 1];

        // local time
        if( utc_offset_expression == -1 )
        {
            // assume that the offset does not change during a single run of the program
            static double ufc_offset_seconds = static_cast<double>(GetUtcOffset()) * DateHelper::SecondsInMinute<double>();
            timestamp -= ufc_offset_seconds;
        }

        // UTC time with a potential offset
        else
        {
            double utc_offset_hours = evalexpr(utc_offset_expression);

            if( IsSpecial(utc_offset_hours) )
                return DEFAULT;

            timestamp -= utc_offset_hours * DateHelper::SecondsInHour<double>();
        }

        return timestamp;
    }
}


double CIntDriver::extimestring(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    const int& timestamp_expression = va_node.arguments[0];
    const int& format_expression = va_node.arguments[1];
    double timestamp;

    if( timestamp_expression == -1 )
    {
        timestamp = GetTimestamp();
    }

    else
    {
        timestamp = evalexpr(timestamp_expression);

        if( IsSpecial(timestamp) )
            return AssignBlankAlphaValue();
    }

    std::string formatter = ( format_expression == -1 ) ? "%c" :
                                                          EvalAlphaExpr<std::string>(format_expression);

    return AssignAlphaValue(FormatTimestamp<std::wstring>(timestamp, formatter.c_str()));
}



// --------------------------------------------
// sysdate/systime functions
// --------------------------------------------

namespace
{
    void sysitime(int* h, int* m, int* s)
    {
        struct tm tp = GetLocalTime();
        *h = tp.tm_hour;
        *m = tp.tm_min;
        *s = tp.tm_sec;
    }

    void sysidate(int* y, int* m, int* d)
    {
        struct tm tp = GetLocalTime();
        *y = tp.tm_year + 1900;
        *m = tp.tm_mon+1;
        *d = tp.tm_mday;
    }
}


double CIntDriver::exsysdate(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    int year;
    int month;
    int day;

    // use a provided timestamp...
    if( fnn_node.fn_nargs == 2 ) 
    {
        time_t tm = evalexpr<time_t>(fnn_node.fn_expr[1]);
        TmToReadableTime(localtime(&tm), &year, &month, &day, nullptr, nullptr, nullptr);
    }

    // ...or the current date
    else
    {
        sysidate(&year, &month, &day);
    }

    // the default format is YYMMDD
    if( fnn_node.fn_nargs == 0 )
    {
        return DateHelper::ToYYYYMMDD(year % 100, month, day);
    }

    else
    {
        CString formatter = EvalAlphaExpr<CString>(fnn_node.fn_expr[0]);
        formatter.Trim();

        return FormatDateToDouble(formatter, year, month, day);
    }
}


double CIntDriver::exsystime(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    int hour;
    int minute;
    int second;

    // use a provided timestamp...
    if( fnn_node.fn_nargs == 2 )
    {
        time_t tm = evalexpr<time_t>(fnn_node.fn_expr[1]);
        TmToReadableTime(localtime(&tm), nullptr, nullptr, nullptr, &hour, &minute, &second);
    }

    // ...or the current time
    else
    {
        sysitime(&hour, &minute, &second);
    }

    // the default format is HHMMSS
    if( fnn_node.fn_nargs == 0 )
    {
        return ( hour * 10000 ) + ( minute * 100 ) + second;
    }

    else 
    {
        CString formatter = EvalAlphaExpr<CString>(fnn_node.fn_expr[0]);
        formatter.Trim();

        std::optional<int> result;

        for( int i = 0; i < formatter.GetLength(); i += 2 )
        {
            CString type = formatter.Mid(i, 2).MakeUpper();
            int value;

            if( type == _T("HH") )
            {
                value = hour;
            }

            else if( type == _T("MM") )
            {
                value = minute;
            }

            else if( type == _T("SS") )
            {
                value = second;
            }

            else
            {
                return DEFAULT;
            }

            result = value + ( result.has_value() ? ( *result * 100 ) : 0 );
        }

        return result.has_value() ? *result : DEFAULT;
    }
}



// --------------------------------------------
// YYYYMMDD-style functions
// --------------------------------------------

namespace
{
    // the date passed in will be assumed to be valid;
    // this will return the number of days after January 1, 2000
    long DateToPost2000DayCount(int year, int month, int day)
    {
        long days_post_2000 = 0;

        // handle the years
        for( int y = 2000; y < year; ++y )
            days_post_2000 += DateHelper::GetDaysInYear(y);

        for( int y = 2000 - 1; y >= year; --y )
            days_post_2000 -= DateHelper::GetDaysInYear(y);

        // handle the months
        for( int m = 1; m < month; ++m )
            days_post_2000 += DateHelper::GetDaysInMonth(year, m);

        // handle the days
        days_post_2000 += day - 1;

#if defined(_DEBUG) && defined(WIN_DESKTOP)
        // check against the pre-portable calculation
        COleDateTime ole_date(year, month, day, 0, 0, 0);
        ole_date += COleDateTimeSpan(-1 * days_post_2000);
        ASSERT(ole_date.GetYear() == 2000 && ole_date.GetMonth() == 1 && ole_date.GetDay() == 1);
#endif

        return days_post_2000;
    }

    long DateToPost2000DayCount(int date)
    {
        return DateToPost2000DayCount(DateHelper::GetYYYY(date), DateHelper::GetMM(date), DateHelper::GetDD(date));
    }

    void Post2000DayCountToDate(long days_post_2000, int& year, int& month, int& day)
    {
        // years
        year = 2000;

        while( days_post_2000 >= 365 )
        {
            // quit out on December 31 of a leap year
            if( days_post_2000 == 365 && DateHelper::IsLeapYear(year) )
                break;

            days_post_2000 -= DateHelper::GetDaysInYear(year);
            ++year;
        }

        while( days_post_2000 < 0 )
        {
            --year;
            days_post_2000 += DateHelper::GetDaysInYear(year);
        }

        // months
        month = 1;

        for( int m = 1; m < 12; ++m )
        {
            int days_in_month = DateHelper::GetDaysInMonth(year, m);

            if( days_post_2000 < days_in_month )
                break;

            ++month;
            days_post_2000 -= days_in_month;
        }

        // days
        day = days_post_2000 + 1;

        ASSERT(DateHelper::IsValid(year, month, day));
    }


    void DecrementDateIfFebruary29OnInvalidLeapYear(int& date)
    {
        // adjust the date so that a calculated date does not end up on an invalid February 29
        if( ( date % 10000 ) == 229 && !DateHelper::IsLeapYear(DateHelper::GetYYYY(date)) )
            --date;
    }

    bool AdjustAndCheckDate(int& date, bool adjust_date_without_year)
    {
        if( date < DateHelper::ToYYYYMMDD(0, 1, 1) || date > DateHelper::ToYYYYMMDD(9999, 12, 31) )
            return false;

        // if the year is not set, set the date to the earliest possible date before "now"
        if( date < DateHelper::ToYYYYMMDD(1, 1, 1) )
        {
            // if not adjusting a date without a year, return whether the date is plausible in a leap year
            if( !adjust_date_without_year )
                return DateHelper::IsValid(2000, DateHelper::GetMM(date), DateHelper::GetDD(date));

            int year;
            int month;
            int day;
            sysidate(&year, &month, &day);

            int now_mmdd = month * 100 + day;

            // if the calculated date would be later than "now," drop back the year
            if( now_mmdd < date )
                --year;

            date += year * 10000;

            DecrementDateIfFebruary29OnInvalidLeapYear(date);
        }

        return DateHelper::IsValid(date);
    }
}


double CIntDriver::exdateadd(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    int date = evalexpr<int>(fnn_node.fn_expr[0]);
    double period_double = evalexpr(fnn_node.fn_expr[1]);

    if( !AdjustAndCheckDate(date, true) || IsSpecial(period_double) )
        return DEFAULT;

    int period_int = (int)period_double;
    bool period_was_integer = ( period_double == period_int );

    enum class PeriodType { Year, Month, Day };
    PeriodType period_type = PeriodType::Day;

    // evaluate user-specified period types
    if( fnn_node.fn_nargs > 2 )
    {
        CString period_type_text = EvalAlphaExpr<CString>(fnn_node.fn_expr[2]).MakeUpper();

        // year
        if( period_type_text == _T("Y") )
        {
            if( period_was_integer )
            {
                period_type = PeriodType::Year;
            }

            else
            {
                period_int = (int)( period_double * 365.25 );
            }
        }

        // month
        else if( period_type_text == _T("M") )
        {
            if( period_was_integer )
            {
                period_type = PeriodType::Month;
            }

            else
            {
                period_int = (int)( period_double * 365.25 / 12 );
            }
        }

        // week = 7 days
        else if( period_type_text == _T("W") )
        {
            period_int = (int)( period_double * 7 );
        }

        // if not day, invalid
        else if( period_type_text != _T("D") )
        {
            return DEFAULT;
        }
    }

    int year = DateHelper::GetYYYY(date);
    int month = DateHelper::GetMM(date);
    int day = DateHelper::GetDD(date);

#if defined(_DEBUG) && defined(WIN_DESKTOP)
    COleDateTime ole_date(year, month, day, 0, 0, 0);
#endif

    std::optional<int> new_date;

    // for years we only have to worry about the leap years
    if( period_type == PeriodType::Year )
    {
        ASSERT(period_was_integer);

        new_date = date + ( period_int * 10000 );
        DecrementDateIfFebruary29OnInvalidLeapYear(*new_date);
    }

    // for months we only have to worry about the case when the new date is, for example, April 31 -> April 30
    else if( period_type == PeriodType::Month ) 
    {
        ASSERT(period_was_integer);

        year += period_int / 12;
        month += period_int % 12;

        if( month > 12 )
        {
            ++year;
            month -= 12;
        }

        else if( month < 1 )
        {
            --year;
            month += 12;
        }

        day = std::min(day, DateHelper::GetDaysInMonth(year, month));
    }

    // days
    else 
    {
        long days_post_2000 = DateToPost2000DayCount(year, month, day);
        Post2000DayCountToDate(days_post_2000 + period_int, year, month, day);
    }

    // for months and days, calculate the final date
    if( !new_date.has_value() )
        new_date = DateHelper::ToYYYYMMDD(year, month, day);

    ASSERT(DateHelper::IsValid(*new_date));

#if defined(_DEBUG) && defined(WIN_DESKTOP)
    // check against the pre-portable calculation
    if( period_type == PeriodType::Day )
    {
        ole_date += COleDateTimeSpan(period_int);
        int check_date = DateHelper::ToYYYYMMDD(ole_date.GetYear(), ole_date.GetMonth(), ole_date.GetDay());
        ASSERT(check_date == new_date);
    }
#endif

    return *new_date;
}


double CIntDriver::exdatediff(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    int start_date = evalexpr<int>(fnn_node.fn_expr[0]);
    int end_date = evalexpr<int>(fnn_node.fn_expr[1]);

    if( !AdjustAndCheckDate(start_date, false) || !AdjustAndCheckDate(end_date, false) )
        return DEFAULT;

    bool start_year_is_missing = ( DateHelper::GetYYYY(start_date) == 0 );
    bool end_year_is_missing = ( DateHelper::GetYYYY(end_date) == 0 );

    // if the end year is missing...
    if( end_year_is_missing )
    {
        // ...and the start year is also missing, set the end date and then 
        // the start date will be adjusted below
        if( start_year_is_missing )
        {
            AdjustAndCheckDate(end_date, true);
        }

        // ...or adjust the end date to be after the start date
        else
        {
            end_date += DateHelper::GetYYYY(start_date) * 10000;

            if( end_date < start_date )
                end_date += 10000;

            DecrementDateIfFebruary29OnInvalidLeapYear(end_date);
        }
    }

    ASSERT(DateHelper::IsValid(end_date));

    // if the start year is missing, adjust it to be before the end date
    if( start_year_is_missing )
    {
        start_date += DateHelper::GetYYYY(end_date) * 10000;

        if( start_date > end_date )
            start_date -= 10000;

        DecrementDateIfFebruary29OnInvalidLeapYear(start_date);
    }

    ASSERT(DateHelper::IsValid(start_date));

    enum class PeriodType { Year, Month, Day, Week, MonthDay, YearMonth, YearDay };
    PeriodType period_type = PeriodType::Day;

    // evaluate user-specified period types
    if( fnn_node.fn_nargs > 2 )
    {
        CString period_type_text = EvalAlphaExpr<CString>(fnn_node.fn_expr[2]).MakeUpper();

        if( period_type_text == _T("Y") )
        {
            period_type = PeriodType::Year;
        }

        else if( period_type_text == _T("M") )
        {
            period_type = PeriodType::Month;
        }

        else if( period_type_text == _T("D") )
        {
            period_type = PeriodType::Day;
        }

        else if( period_type_text == _T("W") )
        {
            period_type = PeriodType::Week;
        }

        else if( period_type_text == _T("MD") )
        {
            period_type = PeriodType::MonthDay;
        }

        else if( period_type_text == _T("YM") )
        {
            period_type = PeriodType::YearMonth;
        }

        else if( period_type_text == _T("YD") )
        {
            period_type = PeriodType::YearDay;
        }

        else
        {
            return DEFAULT;
        }
    }

    if( start_date == end_date )
        return 0;

    // calculate the date with the start date before the end date
    bool make_negative = ( start_date > end_date );

    if( make_negative )
        std::swap(start_date, end_date);

    int date_difference;

    // years 
    if( period_type == PeriodType::Year )
    {
        int& added_years = date_difference = 0;
        int& next_date = start_date;

        while( true )
        {
            next_date += 10000;

            if( next_date > end_date )
                break;

            ++added_years;
        }
    }

    // days and weeks
    else if( bool weeks = ( period_type == PeriodType::Week ); weeks || period_type == PeriodType::Day )
    {
        date_difference = DateToPost2000DayCount(end_date) - DateToPost2000DayCount(start_date);

        if( weeks )
            date_difference /= 7;
    }

    // the rest...
    else
    {
        // if not calculating direct months, then adjust the start date to use the end date's year
        if( period_type != PeriodType::Month )
        {
            start_date = DateHelper::ToYYYYMMDD(DateHelper::GetYYYY(end_date),
                                                DateHelper::GetMM(start_date), DateHelper::GetDD(start_date));

            // make sure the start date is before the end date
            if( start_date > end_date )
                start_date -= 10000;

            DecrementDateIfFebruary29OnInvalidLeapYear(start_date);
        }

        // days (ignoring the years)
        if( period_type == PeriodType::YearDay )
        {
            date_difference = DateToPost2000DayCount(end_date) - DateToPost2000DayCount(start_date);
        }

        // the rest...
        else
        {
            // calculate the difference in months
            int added_months = 0;
            int& next_date = start_date;

            while( true )
            {
                next_date += 100;

                // adjust the year when the month becomes 13
                if( ( next_date % 10000 ) > 1300 )
                    next_date += 8800;

                if( next_date > end_date )
                    break;

                ++added_months;
            }

            // months and months (ignoring the years)
            if( period_type == PeriodType::Month || period_type == PeriodType::YearMonth )
            {
                date_difference = added_months;
            }

            // days (ignoring the years and months)
            else
            {
                ASSERT(period_type == PeriodType::MonthDay);

                // next_date is now later than the end date so drop it back one month
                next_date -= 100;

                if( ( next_date % 10000 ) < 100 )
                    next_date -= 8800;

                int next_day = DateHelper::GetDD(next_date);
                int end_day = DateHelper::GetDD(end_date);

                if( next_day <= end_day )
                {
                    date_difference = end_day - next_day;
                }

                // handle days that cross months
                else
                {
                    int days_in_previous_month = DateHelper::GetDaysInMonth(2001, DateHelper::GetMM(next_date));

                    // adjust the next day in case it is not valid for the month
                    next_day = std::min(next_day, days_in_previous_month);

                    date_difference = end_day + ( days_in_previous_month - next_day );
                }
            }
        }
    }

    return make_negative ? ( -1 * date_difference ) : date_difference;
}


double CIntDriver::exdatevalid(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    int date = evalexpr<int>(fnn_node.fn_expr[0]);

    return DateHelper::IsValid(date) ? 1 : 0;
}



// --------------------------------------------
// date functions for DHS surveys
// --------------------------------------------

namespace
{
    double cmcode(double month, double year)
    {
        if( month >= 1 && month <= 12 )
        {
            if( year >= 0 && year <= 99 )
                return 12 * year + month;

            if( year >= 1900 && year <= 2099 )
                return 12 * ( year - 1900 ) + month;
        }

        return 9999;
    }
}


double CIntDriver::excmcode(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double month = evalexpr(fnn_node.fn_expr[0]);
    double year = evalexpr(fnn_node.fn_expr[1]);

    return cmcode(month, year);
}


double CIntDriver::exsetlb_setub(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double month = evalexpr(fnn_node.fn_expr[0]);
    double year = evalexpr(fnn_node.fn_expr[1]);
    double default_value = evalexpr(fnn_node.fn_expr[2]);

    if( cmcode(1, year) == 9999 )
        return default_value;

    double cm = cmcode(month, year);

    if( cm == 9999 )
    {
        double adjusted_month = ( fnn_node.fn_code == FunctionCode::FNSETLB_CODE ) ? 1 : 12;
        return cmcode(adjusted_month, year);
    }

    return cm;
}


double CIntDriver::exadjlba(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double ldb = evalexpr(fnn_node.fn_expr[0]);
    double ref_age = evalexpr(fnn_node.fn_expr[4]);

    if( IsSpecial(ref_age) )
        return ldb;

    double udb = evalexpr(fnn_node.fn_expr[1]);
    double lrefd = evalexpr(fnn_node.fn_expr[2]);

    double cm = lrefd - 12 * ( ref_age + 1 );

    return ( cm > udb ) ? -1 :
           ( ldb > cm ) ? ldb :
                          cm;
}


double CIntDriver::exadjuba(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double udb = evalexpr(fnn_node.fn_expr[1]);
    double ref_age = evalexpr(fnn_node.fn_expr[4]);

    if( IsSpecial(ref_age) )
        return udb;

    double ldb = evalexpr(fnn_node.fn_expr[0]);
    double urefd = evalexpr(fnn_node.fn_expr[3]);

    double cm = urefd - 12 * ref_age;

    return ( cm < ldb ) ? -1 :
           ( udb < cm ) ? udb :
                          cm;
}


double CIntDriver::exadjlbi(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double l1 = evalexpr(fnn_node.fn_expr[0]);
    double l2 = evalexpr(fnn_node.fn_expr[2]);
    double u2 = evalexpr(fnn_node.fn_expr[3]);
    double interval = evalexpr(fnn_node.fn_expr[4]);

    if( IsSpecial(interval) )
        return l2;

    double cm = l1 + interval;

    return ( cm > u2 ) ? -1 :
           ( cm > l2 ) ? cm :
                         l2;
}


double CIntDriver::exadjubi(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double l1 = evalexpr(fnn_node.fn_expr[0]);
    double u1 = evalexpr(fnn_node.fn_expr[1]);
    double u2 = evalexpr(fnn_node.fn_expr[3]);
    double interval = evalexpr(fnn_node.fn_expr[4]);

    if( IsSpecial(interval) )
        return u1;

    double cm = u2 - interval;

    return ( cm < l1 ) ? -1 :
           ( cm < u1 ) ? cm :
                         u1;
}
