#pragma once

#include <zDictCLR/ValuePair.h>

class DictValue;


namespace CSPro
{
    namespace Dictionary
    {
        ///<summary>
        ///Value (response/range) of an item in a CSPro data dictionary. One entry in a ValueSet.
        ///</summary>
        public ref class DictionaryValue sealed
        {
        public:
            DictionaryValue(const DictValue& dict_value);

            property System::String^ Label { System::String^ get(); }

            property array<ValuePair^>^ ValuePairs { array<ValuePair^>^ get(); }

        private:
            const DictValue& m_dictValue;
        };
    }
}
