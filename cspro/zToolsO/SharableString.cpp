#include "StdAfx.h"
#include "SharableString.h"


template<bool ToUpper>
SharableString& SharableString::MakeCaseWorker()
{
    if( HasModifiedOrModifiableString() )
    {
        std::string& text = GetModifiedOrModifiableString();
        std::string* modified_case_string = &text;
        SO::MakeWideCaseWorker<ToUpper>(text, modified_case_string);
    }

    else
    {
        // only modify the string if the case actually changes
        std::string* modified_case_string = nullptr;

        SO::MakeWideCaseWorker<ToUpper>(GetString(), modified_case_string);

        if( modified_case_string != nullptr )
            m_string = std::shared_ptr<std::string>(modified_case_string);
    }

    return *this;
}

template CLASS_DECL_ZTOOLSO SharableString& SharableString::MakeCaseWorker<true>();
template CLASS_DECL_ZTOOLSO SharableString& SharableString::MakeCaseWorker<false>();
