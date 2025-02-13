#pragma once

class DictionaryDescription;


namespace CSPro
{
    namespace Dictionary
    {
        public ref class DictionaryDescription sealed
        {
        public:
            System::String^ Path;
            System::String^ Type;

            System::String^ ToString() override;

            static System::Collections::Generic::List<DictionaryDescription^>^ GetFromApplication(System::String^ application_filename);
        };
    }
}
