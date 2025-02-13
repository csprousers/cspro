#pragma once

#include <zUtilO/UWMRanges.h>

class PFF;


namespace UWM::Dictionary
{
    const unsigned UpdateLanguageList    = UWM::Ranges::DictionaryStart + 0;
    const unsigned NameChange            = UWM::Ranges::DictionaryStart + 1;
    const unsigned ValueLabelChange      = UWM::Ranges::DictionaryStart + 2;
    const unsigned GetApplicationPff     = UWM::Ranges::DictionaryStart + 3;
    const unsigned MarkUsedItems         = UWM::Ranges::DictionaryStart + 4;
    const unsigned GetTableCursor        = UWM::Ranges::DictionaryStart + 5;
    const unsigned DraggedOffTable       = UWM::Ranges::DictionaryStart + 6;
    
    // unlike the above messages, the following messages are only used within the project
    const unsigned Find                  = UWM::Ranges::DictionaryStart + 7;

    CHECK_MESSAGE_NUMBERING(Find, UWM::Ranges::DictionaryLast)


    struct GetApplicationPffParameters
    {
        const CDataDict& dictionary; // in
        bool is_input_dictionary;    // out
        std::unique_ptr<PFF> pff;    // out
    };
}
