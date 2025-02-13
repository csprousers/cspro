#pragma once


namespace CSPro
{
    namespace Logic
    {
        public ref class Colorizer sealed
        {
        public:
            // a formatter used by the PFF Editor
            static System::String^ LogicToHtml(System::String^ text);
        };
    }
}
