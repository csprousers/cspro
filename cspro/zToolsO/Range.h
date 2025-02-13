#pragma once


template<typename T>
struct Range
{
    Range(T low_, T high_)
        :   low(low_),
            high(high_)
    {
    }

    T low;
    T high;
};


template<typename T>
bool HasOverlappingRanges(const std::vector<Range<T>>& ranges)
{
    auto ranges_end = ranges.cend();

    for( auto range_itr1 = ranges.cbegin(); range_itr1 != ranges_end; ++range_itr1 )
    {
        const Range<T>& r1 = *range_itr1;

        for( auto range_itr2 = range_itr1 + 1; range_itr2 != ranges_end; ++range_itr2 )
        {
            const Range<T>& r2 = *range_itr2;

            if( ( r2.low <= r1.low  && r1.low  <= r2.high ) ||
                ( r2.low <= r1.high && r1.high <= r2.high ) ||
                ( r1.low < r2.low && r1.high > r2.high ) )
            {
                return true;
            }
        }
    }

    return false;
}
