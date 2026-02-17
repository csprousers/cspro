#pragma once

#include <zGit/zGit.h>

struct git_time;


// --------------------------------------------------------------------------
// GitTime
//
// Wraps git_time: "time in a signature."
// --------------------------------------------------------------------------

class ZGIT_API GitTime
{
public:
    GitTime(const git_time& time) noexcept;

    // Returns the non-null git_time object that GitTime wraps.
    operator const git_time*() const noexcept { return reinterpret_cast<const git_time*>(m_time); }

    // Returns the time as a UNIX timestamp.
    int64_t GetTimestamp() const noexcept;

    // Returns the time formatted as RFC 2822; e.g.,: Thu, 03 Jul 2025 15:00:00 -0400
    std::string GetRfc2822String() const noexcept;

    // Returns the time in the local timezone in the specified strftime format.
    std::string GetLocalDateTimeString(cs::string_sz formatter = "%Y-%m-%d %H:%M:%S") const noexcept;

private:
    static constexpr size_t GitTimeSize = 16;
    std::byte m_time[GitTimeSize];
};


// --------------------------------------------------------------------------
// GitTime / git_time comparisons
//
// The comparisons compare the UNIX timestamps, ignoring the offset and sign
// values.
// --------------------------------------------------------------------------

ZGIT_API bool operator==(const git_time& gt1, const git_time& gt2);
ZGIT_API bool operator<(const git_time& gt1, const git_time& gt2);

ZGIT_API bool operator==(const GitTime& gt1, const GitTime& gt2);
ZGIT_API bool operator<(const GitTime& gt1, const GitTime& gt2);
