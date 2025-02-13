#pragma once

#include <zToolsO/Range.h>


namespace CompilationExtendedInformation
{
    struct InCrosstabInformation
    {
        std::vector<Range<double>> ranges;
        std::vector<bool> implicit_highs;
    };
}
