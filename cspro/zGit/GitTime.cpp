#include "StdAfx.h"
#include "GitTime.h"
#include <chrono>


GitTime::GitTime(const git_time& time) noexcept
{
    static_assert(GitTimeSize == sizeof(git_time));
    memcpy(m_time, &time, GitTimeSize);
}


int64_t GitTime::GetTimestamp() const noexcept
{
    return static_cast<const git_time*>(*this)->time;
}


std::string GitTime::GetRfc2822String() const noexcept
{
    const git_time& time = *static_cast<const git_time*>(*this);
    const int64_t timestamp = GetTimestamp();
    tm tm;

    if( localtime_s(&tm, &timestamp) != 0 )
    {
        ASSERT(false);
        return std::string();
    }

    char buffer[40];
    const size_t length = std::strftime(buffer, _countof(buffer), "%a, %d %b %Y %H:%M:%S", &tm);
    ASSERT(length != 0);

    std::string time_string(buffer, length);

    // append the time zone offset
    const int offset_abs = std::abs(time.offset);
    const char offset_sign = ( time.offset >= 0 ) ? '+' : '-';
    time_string.append(FormatText(" %c%02d%02d", offset_sign, offset_abs / 60, offset_abs % 60));

    return time_string;
}


std::string GitTime::GetLocalDateTimeString(const cs::string_sz formatter/* = "%Y-%m-%d %H:%M:%S"*/) const noexcept
{
    return DateTime::LocalDateTimeString(GetTimestamp(), formatter);
}


bool operator==(const git_time& gt1, const git_time& gt2)
{
    return ( gt1.time == gt2.time );
}


bool operator<(const git_time& gt1, const git_time& gt2)
{
    return ( gt1.time < gt2.time );
}


bool operator==(const GitTime& gt1, const GitTime& gt2)
{
    return operator==(*static_cast<const git_time*>(gt1), *static_cast<const git_time*>(gt2));
}


bool operator<(const GitTime& gt1, const GitTime& gt2)
{
    return operator<(*static_cast<const git_time*>(gt1), *static_cast<const git_time*>(gt2));
}
