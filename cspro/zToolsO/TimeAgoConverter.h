#pragma once

#include <zToolsO/Tools.h>

namespace TimeAgoConverter
{
    inline CString GetTimeAgo(double timestamp)
    {
        double seconds_elapsed = std::max(GetTimestamp() - timestamp, 0.0);
        double current_threshold = 1;
        CString time_ago;

        auto format_time_ago = [&](double new_multiplier, const TCHAR* name, const TCHAR* single_name = nullptr) -> bool
        {
            double new_threshold = new_multiplier * current_threshold;

            if( seconds_elapsed < new_threshold )
            {
                int units = (int)( seconds_elapsed / current_threshold );

                if( units == 1 )
                {
                    if( single_name != nullptr )
                        time_ago = single_name;

                    else
                        time_ago.Format(_T("a %s ago"), name);
                }

                else
                    time_ago.Format(_T("%d %ss ago"), units, name);

                return true;
            }

            else
            {
                current_threshold = new_threshold;
                return false;
            }
        };

        format_time_ago(60, _T("second")) ||
        format_time_ago(60, _T("minute")) ||
        format_time_ago(24, _T("hour"), _T("an hour ago")) ||
        format_time_ago(365.25 / 12, _T("day"), _T("yesterday")) ||
        format_time_ago(12, _T("month")) ||
        format_time_ago(100000, _T("year"));

        return time_ago;
    }
}
