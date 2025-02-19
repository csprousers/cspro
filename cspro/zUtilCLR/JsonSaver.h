#pragma once


namespace CSPro
{
    namespace Util
    {
        public ref class JsonSaver sealed
        {
        public:
            static void SaveInSpecFileFormat(System::String^ file_path, System::String^ json_text);
        };
    }
}
