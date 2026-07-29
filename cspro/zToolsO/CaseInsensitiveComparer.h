#pragma once

#include <zToolsO/StringOperations.h>

namespace cs { class case_insensitive_less; }


class cs::case_insensitive_less
{
public:
    using is_transparent = std::true_type;

    template<typename ST1, typename ST2>
    bool operator()(ST1&& left, ST2&& right) const
    {
        return ( SO::CompareNoCase(std::forward<ST1>(left), std::forward<ST2>(right)) < 0 );
    }
};
