#include "StdAfx.h"
#include "NewlineSubstitutor.h"


SharableString& NewlineSubstitutor::MakeNewlineToSpace(SharableString& sharable_string)
{
    ASSERT(sharable_string->find('\r') == std::string::npos);

    const size_t newline_pos = sharable_string->find('\n');

    if( newline_pos != std::string::npos )
    {
        std::string& text = sharable_string.MakeModifiable();
        text.replace(text.begin() + newline_pos, text.end(), '\n', ' ');
    }

    return sharable_string;
}
