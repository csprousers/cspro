#include "Stdafx.h"
#include "CaseSummary.h"


namespace
{
    System::String^ GetSingleLineDisplayText(System::String^ text)
    {
        // turn \n -> ␤
        return text->Replace('\n', NewlineSubstitutor::UnicodeNL);
    }
}


System::String^ CSPro::Data::CaseSummary::KeyForSingleLineDisplay::get()
{
    return GetSingleLineDisplayText(Key);
}


System::String^ CSPro::Data::CaseSummary::CaseLabelForSingleLineDisplay::get()
{
    return GetSingleLineDisplayText(CaseLabel);
}
