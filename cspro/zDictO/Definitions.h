#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DDClass.h>


namespace DictionaryDefaults
{
    constexpr bool RelativePositions  = false;
    constexpr bool ZeroFill           = false;
    constexpr bool DecChar            = false;

    constexpr bool ReadOptimization   = true;
                                      
    constexpr unsigned RecTypeStart   = 0;
    constexpr unsigned RecTypeLen     = 0;
                                      
    constexpr unsigned MaxRecs        = 1;
    constexpr bool Required           = true;
    constexpr unsigned RecLen         = 1;
                                      
    constexpr ItemType ItemType       = ItemType::Item;
    constexpr ContentType ContentType = ContentType::Numeric;
    constexpr unsigned ItemLen        = 1;
    constexpr unsigned Occurs         = 1;
    constexpr unsigned Decimal        = 0;
                                      
    CLASS_DECL_ZDICTO extern const PortableColor& ValueLabelTextColor;
}
