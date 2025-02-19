#pragma once

class DictionaryDescription;


namespace CSPro
{
    namespace Dictionary
    {
        public ref class DictionaryDescription sealed
        {
        public:
            DictionaryDescription(System::String^ path, bool is_input_dictionary);

            property System::String^ Path { System::String^ get(); }

            property bool IsInputDictionary { bool get(); }

            System::String^ ToString() override;

            // returns non-working dictionaries from an application
            static System::Collections::Generic::List<DictionaryDescription^>^ GetFromApplication(System::String^ application_filename);

        private:
            System::String^ m_path;
            bool m_isInputDictionary;
            System::String^ m_displayText;
        };
    }
}
