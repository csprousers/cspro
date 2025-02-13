#pragma once

namespace CSPro
{
    namespace Logic
    {
        public ref class Names
        {
        public:
            static bool IsValid(System::String^ name);
            static System::String^ MakeName(System::String^ label);
        };
    }
}
