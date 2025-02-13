#pragma once

#include <zDictO/DictValueSet.h>
#pragma make_public(DictValueSet)

#include <zDictCLR/DictionaryValue.h>


namespace CSPro
{
    namespace Dictionary
    {
        ///<summary>
        ///Value set (responses, ranges) associated with an item in a CSPro data dictionary
        ///</summary>
        public ref class ValueSet sealed
        {
        public:
            ValueSet(const DictValueSet& dict_value_set);

            property System::String^ Name { System::String^ get(); }

            property System::String^ Label { System::String^ get(); }

            property array<DictionaryValue^>^ Values { array<DictionaryValue^>^ get(); }

        private:
            const DictValueSet& m_dictValueSet;
        };
    }
}
