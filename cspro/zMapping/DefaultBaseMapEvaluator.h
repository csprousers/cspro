#pragma once

#include <zAppO/PFF.h>
#include <zAppO/Properties/ApplicationProperties.h>


inline BaseMapSelection GetDefaultBaseMapSelection(const PFF& pff)
{
    // the order of precedence is...

    // ...the base map specified in the PFF
    if( pff.GetBaseMapSelection().has_value() )
    {
        return *pff.GetBaseMapSelection();
    }

    // ...the default base map from the mapping properties
    else
    {
        return pff.GetApplication()->GetApplicationProperties().GetMappingProperties().GetDefaultBaseMap();
    }
}
