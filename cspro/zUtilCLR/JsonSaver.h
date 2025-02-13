#pragma once


namespace CSPro
{
    namespace Util
    {
        public ref class JsonSaver sealed
        {
        public:
            static void SaveInSpecFileFormat(System::String^ filename, System::String^ json_text);
        };
    }
}
