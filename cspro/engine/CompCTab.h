#pragma once

#include <zToolsO/Range.h>


class ExplicitIncludeRanges
{
public:
    const std::vector<Range<double>>& getRanges() const { return m_ranges; }

    void calculateRanges(int options);

private:
    void reset()
    {
        m_ranges.clear();
    }

    void addRange(double value)
    {
        // potentially modify an existing range
        if( !m_ranges.empty() && ( value - 1 ) == m_ranges.back().high )
        {
            m_ranges.back().high = value;
        }

        else
        {
            m_ranges.emplace_back(value, value);
        }
    }

private:
    std::vector<Range<double>> m_ranges;
};
