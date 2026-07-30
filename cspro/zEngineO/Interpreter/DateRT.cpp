#include "stdafx.h"
#include "IncludesRT.h"
#include "Nodes/Date.h"


// --------------------------------------------------------------------------
// timestamp-style functions
// --------------------------------------------------------------------------

double LogicInterpreter::ex_timestamp(const int program_index)
{
    const auto& timestamp_node = GetNode<Nodes::Timestamp>(program_index);

    if( m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) || timestamp_node.type == Nodes::Timestamp::Type::Current )
    {
        return GetTimestamp<double>();
    }

    else if( timestamp_node.type == Nodes::Timestamp::Type::RFC3339 )
    {
        SharableString date_time = Evaluate<SharableString>(timestamp_node.argument);

        if( date_time->length() < PortableFunctions::MinLengthRFC3339DateTimeString )
            return DEFAULT;

        return static_cast<double>(PortableFunctions::ParseRFC3339DateTime(date_time.Release()));
    }

    else
    {
        ASSERT(timestamp_node.type == Nodes::Timestamp::Type::SpecifiedDate);
        const Nodes::List& arguments_list_node = GetListNode(timestamp_node.argument);
        ASSERT(arguments_list_node.number_elements >= 4 && arguments_list_node.number_elements <= 7);
        bool special_value_read = false;

        auto evaluate_int = [&](const size_t index)
        {
            const double value = Evaluate<double>(arguments_list_node.elements[index]);

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

        const int utc_offset_expression = arguments_list_node.elements[arguments_list_node.number_elements - 1];
        double utc_offset_in_seconds;

        // local time
        if( utc_offset_expression == -1 )
        {
            utc_offset_in_seconds = DateTime::GetUtcOffset(DateTime::TimeToComponents(time_struct)) * DateHelper::SecondsInMinute<double>();
        }

        // UTC time with a potential offset
        else
        {
            const double utc_offset_hours = Evaluate<double>(utc_offset_expression);

            if( IsSpecial(utc_offset_hours) )
                return DEFAULT;

            utc_offset_in_seconds = utc_offset_hours * DateHelper::SecondsInHour<double>();
        }

        return static_cast<double>(_mkgmtime(&time_struct)) - utc_offset_in_seconds;
    }
}


double LogicInterpreter::ex_timestring(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const int& timestamp_expression = va_node.arguments[0];
    const int& format_expression = va_node.arguments[1];
    double timestamp;

    if( timestamp_expression == -1 )
    {
        timestamp = GetTimestamp<double>();
    }

    else
    {
        timestamp = Evaluate<double>(timestamp_expression);

        if( IsSpecial(timestamp) )
            return AssignStringNull();
    }

    const SharableString formatter = ( format_expression == -1 ) ? SharableString("%c") :
                                                                   Evaluate<SharableString>(format_expression);

    return AssignString(FormatTimestamp(timestamp, *formatter));
}



// --------------------------------------------------------------------------
// sysdate/systime functions
// --------------------------------------------------------------------------

double LogicInterpreter::ex_sysdate(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);

    // use a provided timestamp or the current date
    const int64_t time = ( fnn_node.fn_nargs == 2 ) ? Evaluate<int64_t>(fnn_node.fn_expr[1]) :
                                                      DateTime::Now();

    // return information about the date using the local timezone
    const DateTime::Components date_time_components = DateTime::TimeToComponents(time, true);

    // the default format is YYMMDD
    if( fnn_node.fn_nargs == 0 )
    {
        return DateTime::TimeToYYMMDD(date_time_components);
    }

    else
    {
        const SharableString formatter = Evaluate<SharableString>(fnn_node.fn_expr[0]);

        const std::optional<uint64_t> date = FormatDate(SO::Trim(*formatter), date_time_components);

        return date.has_value() ? *date :
                                  DEFAULT;
    }
}


double LogicInterpreter::ex_systime(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);

    // use a provided timestamp or the current date
    const int64_t time = ( fnn_node.fn_nargs == 2 ) ? Evaluate<int64_t>(fnn_node.fn_expr[1]) :
                                                      DateTime::Now();

    // return information about the time using the local timezone
    const DateTime::Components date_time_components = DateTime::TimeToComponents(time, true);

    // the default format is HHMMSS
    if( fnn_node.fn_nargs == 0 )
    {
        return DateTime::TimeToHHMMSS(date_time_components);
    }

    else
    {
        const SharableString formatter = Evaluate<SharableString>(fnn_node.fn_expr[0]);
        std::string_view formatter_sv = SO::Trim(*formatter);

        std::optional<uint64_t> result;

        while( !formatter_sv.empty() )
        {
            int value;

            if( SO::StartsWithNoCase(formatter_sv, "HH") )
            {
                value = date_time_components.hour;
            }

            else if( SO::StartsWithNoCase(formatter_sv, "MM") )
            {
                value = date_time_components.minute;
            }

            else if( SO::StartsWithNoCase(formatter_sv, "SS") )
            {
                value = date_time_components.second;
            }

            else
            {
                return DEFAULT;
            }

            result = value + ( result.has_value() ? ( *result * 100 ) : 0 );

            formatter_sv = formatter_sv.substr(2);
        }

        return result.has_value() ? *result :
                                    DEFAULT;
    }
}



// --------------------------------------------------------------------------
// YYYYMMDD-style functions
// --------------------------------------------------------------------------

namespace
{
    // the date passed in will be assumed to be valid;
    // this will return the number of days after January 1, 2000
    int64_t DateToPost2000DayCount(const int year, const int month, const int day)
    {
        int64_t days_post_2000 = 0;

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
        ole_date += COleDateTimeSpan(-1.0 * days_post_2000);
        ASSERT(ole_date.GetYear() == 2000 && ole_date.GetMonth() == 1 && ole_date.GetDay() == 1);
#endif

        return days_post_2000;
    }


    int64_t DateToPost2000DayCount(const int date)
    {
        return DateToPost2000DayCount(DateHelper::GetYYYY(date), DateHelper::GetMM(date), DateHelper::GetDD(date));
    }


    void Post2000DayCountToDate(int64_t days_post_2000, int& year, int& month, int& day)
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
            const int days_in_month = DateHelper::GetDaysInMonth(year, m);

            if( days_post_2000 < days_in_month )
                break;

            ++month;
            days_post_2000 -= days_in_month;
        }

        // days
        day = static_cast<int>(days_post_2000 + 1);

        ASSERT(DateHelper::IsValid(year, month, day));
    }


    void DecrementDateIfFebruary29OnInvalidLeapYear(int& date)
    {
        // adjust the date so that a calculated date does not end up on an invalid February 29
        if( ( date % 10000 ) == 229 && !DateHelper::IsLeapYear(DateHelper::GetYYYY(date)) )
            --date;
    }


    bool AdjustAndCheckDate(int& date, const bool adjust_date_without_year)
    {
        if( date < DateHelper::ToYYYYMMDD(0, 1, 1) || date > DateHelper::ToYYYYMMDD(9999, 12, 31) )
            return false;

        // if the year is not set, set the date to the earliest possible date before "now"
        if( date < DateHelper::ToYYYYMMDD(1, 1, 1) )
        {
            // if not adjusting a date without a year, return whether the date is plausible in a leap year
            if( !adjust_date_without_year )
                return DateHelper::IsValid(2000, DateHelper::GetMM(date), DateHelper::GetDD(date));

            DateTime::Components date_time_components = DateTime::TimeToComponents(DateTime::Now(), true);
            const int now_mmdd = date_time_components.month * 100 + date_time_components.day;

            // if the calculated date would be later than "now," drop back the year
            if( now_mmdd < date )
                --date_time_components.year;

            date += date_time_components.year * 10000;

            DecrementDateIfFebruary29OnInvalidLeapYear(date);
        }

        return DateHelper::IsValid(date);
    }
}


double LogicInterpreter::ex_dateadd(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    int date = Evaluate<int>(fnn_node.fn_expr[0]);
    const double period_double = Evaluate<double>(fnn_node.fn_expr[1]);

    if( !AdjustAndCheckDate(date, true) || IsSpecial(period_double) )
        return DEFAULT;

    int period_int = static_cast<int>(period_double);
    const bool period_was_integer = ( period_double == period_int );

    enum class PeriodType { Year, Month, Day };
    PeriodType period_type = PeriodType::Day;

    // evaluate user-specified period types
    if( fnn_node.fn_nargs > 2 )
    {
        SharableString period_type_text = Evaluate<SharableString>(fnn_node.fn_expr[2]);
        period_type_text.MakeTrim();

        if( period_type_text->length() != 1 )
            return DEFAULT;

        switch( std::toupper(period_type_text->front()) )
        {
            // year
            case 'Y':
            {
                if( period_was_integer )
                {
                    period_type = PeriodType::Year;
                }

                else
                {
                    period_int = static_cast<int>(period_double * 365.25 );
                }

                break;
            }

            // month
            case 'M':
            {
                if( period_was_integer )
                {
                    period_type = PeriodType::Month;
                }

                else
                {
                    period_int = static_cast<int>(period_double * 365.25 / 12);
                }

                break;
            }

            // week = 7 days
            case 'W':
            {
                period_int = static_cast<int>(period_double * 7);
                break;
            }

            // day (the default)
            case 'D':
            {
                break;
            }

            // invalid period type
            default:
            {
                return DEFAULT;
            }
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
        const int64_t days_post_2000 = DateToPost2000DayCount(year, month, day);
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
        const int check_date = DateHelper::ToYYYYMMDD(ole_date.GetYear(), ole_date.GetMonth(), ole_date.GetDay());
        ASSERT(check_date == new_date);
    }
#endif

    return *new_date;
}


double LogicInterpreter::ex_datediff(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    int start_date = Evaluate<int>(fnn_node.fn_expr[0]);
    int end_date = Evaluate<int>(fnn_node.fn_expr[1]);

    if( !AdjustAndCheckDate(start_date, false) || !AdjustAndCheckDate(end_date, false) )
        return DEFAULT;

    const bool start_year_is_missing = ( DateHelper::GetYYYY(start_date) == 0 );
    const bool end_year_is_missing = ( DateHelper::GetYYYY(end_date) == 0 );

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
    PeriodType period_type;

    // evaluate user-specified period types
    if( fnn_node.fn_nargs > 2 )
    {
        const SharableString period_type_text = Evaluate<SharableString>(fnn_node.fn_expr[2]);

        if( period_type_text->length() == 1 )
        {
            switch( std::toupper(period_type_text->front()) )
            {
                case 'Y': period_type = PeriodType::Year;  break;
                case 'M': period_type = PeriodType::Month; break;
                case 'D': period_type = PeriodType::Day;   break;
                case 'W': period_type = PeriodType::Week;  break;
                default:                                   return DEFAULT;
            }
        }

        else if( SO::StartsWithNoCase(*period_type_text, "MD") )
        {
            period_type = PeriodType::MonthDay;
        }

        else if( SO::StartsWithNoCase(*period_type_text, "YM") )
        {
            period_type = PeriodType::YearMonth;
        }

        else if( SO::StartsWithNoCase(*period_type_text, "YD") )
        {
            period_type = PeriodType::YearDay;
        }

        else
        {
            return DEFAULT;
        }
    }

    // otherwise default to days
    else
    {
        period_type = PeriodType::Day;
    }

    if( start_date == end_date )
        return 0;

    // calculate the date with the start date before the end date
    const bool make_negative = ( start_date > end_date );

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
    else if( const bool weeks = ( period_type == PeriodType::Week ); weeks || period_type == PeriodType::Day )
    {
        date_difference = static_cast<int>(DateToPost2000DayCount(end_date) - DateToPost2000DayCount(start_date));

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
            date_difference = static_cast<int>(DateToPost2000DayCount(end_date) - DateToPost2000DayCount(start_date));
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
                const int end_day = DateHelper::GetDD(end_date);

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

    return make_negative ? ( -1 * date_difference ) :
                           date_difference;
}


double LogicInterpreter::ex_datevalid(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const int date = Evaluate<int>(fnn_node.fn_expr[0]);

    return DateHelper::IsValid(date) ? 1 : 0;
}



// --------------------------------------------------------------------------
// date functions for DHS surveys
// --------------------------------------------------------------------------

namespace
{
    double cmcode(const double month, const double year)
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


double LogicInterpreter::ex_cmcode(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double month = Evaluate<double>(fnn_node.fn_expr[0]);
    const double year = Evaluate<double>(fnn_node.fn_expr[1]);

    return cmcode(month, year);
}


double LogicInterpreter::ex_setlb_setub(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double month = Evaluate<double>(fnn_node.fn_expr[0]);
    const double year = Evaluate<double>(fnn_node.fn_expr[1]);
    const double default_value = Evaluate<double>(fnn_node.fn_expr[2]);

    if( cmcode(1, year) == 9999 )
        return default_value;

    const double cm = cmcode(month, year);

    if( cm == 9999 )
    {
        const double adjusted_month = ( fnn_node.fn_code == FunctionCode::FNSETLB_CODE ) ? 1 : 12;
        return cmcode(adjusted_month, year);
    }

    return cm;
}


double LogicInterpreter::ex_adjlba(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double ldb = Evaluate<double>(fnn_node.fn_expr[0]);
    const double ref_age = Evaluate<double>(fnn_node.fn_expr[4]);

    if( IsSpecial(ref_age) )
        return ldb;

    const double udb = Evaluate<double>(fnn_node.fn_expr[1]);
    const double lrefd = Evaluate<double>(fnn_node.fn_expr[2]);

    const double cm = lrefd - 12 * ( ref_age + 1 );

    return ( cm > udb ) ? -1 :
           ( ldb > cm ) ? ldb :
                          cm;
}


double LogicInterpreter::ex_adjuba(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double udb = Evaluate<double>(fnn_node.fn_expr[1]);
    const double ref_age = Evaluate<double>(fnn_node.fn_expr[4]);

    if( IsSpecial(ref_age) )
        return udb;

    const double ldb = Evaluate<double>(fnn_node.fn_expr[0]);
    const double urefd = Evaluate<double>(fnn_node.fn_expr[3]);

    double cm = urefd - 12 * ref_age;

    return ( cm < ldb ) ? -1 :
           ( udb < cm ) ? udb :
                          cm;
}


double LogicInterpreter::ex_adjlbi(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double l1 = Evaluate<double>(fnn_node.fn_expr[0]);
    const double l2 = Evaluate<double>(fnn_node.fn_expr[2]);
    const double u2 = Evaluate<double>(fnn_node.fn_expr[3]);
    const double interval = Evaluate<double>(fnn_node.fn_expr[4]);

    if( IsSpecial(interval) )
        return l2;

    const double cm = l1 + interval;

    return ( cm > u2 ) ? -1 :
           ( cm > l2 ) ? cm :
                         l2;
}


double LogicInterpreter::ex_adjubi(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double l1 = Evaluate<double>(fnn_node.fn_expr[0]);
    const double u1 = Evaluate<double>(fnn_node.fn_expr[1]);
    const double u2 = Evaluate<double>(fnn_node.fn_expr[3]);
    const double interval = Evaluate<double>(fnn_node.fn_expr[4]);

    if( IsSpecial(interval) )
        return u1;

    const double cm = u2 - interval;

    return ( cm < l1 ) ? -1 :
           ( cm < u1 ) ? cm :
                         u1;
}
