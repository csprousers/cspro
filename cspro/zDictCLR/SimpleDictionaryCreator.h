#pragma once

namespace CSPro
{
    namespace Dictionary
    {
        public ref class SimpleDictionaryCreator
        {
        public:
            SimpleDictionaryCreator(System::String^ name_prefix, bool decimal_char_default, bool zero_fill_default);

            ~SimpleDictionaryCreator() { this->!SimpleDictionaryCreator(); }
            !SimpleDictionaryCreator();

            void AddItem(bool is_id, System::String^ name, bool numeric, int length, int decimal,
                         System::Collections::Generic::SortedSet<double>^ values);

            void Save(System::String^ filename);

        private:
            CDataDict* m_dictionary;
            int m_nextStartPos;
        };
    }
}
