#pragma once

class Case;


namespace CSPro
{
    namespace Data
    {
        public ref class Case sealed
        {
        public:
            Case(std::shared_ptr<::Case> data_case);

            ~Case() { this->!Case(); }
            !Case();

            ::Case& GetNativeReference();

            property System::String^ Key { System::String^ get(); }

            property System::String^ KeyForSingleLineDisplay { System::String^ get(); }

        private:
            std::shared_ptr<::Case>* m_case;
        };
    }
}
