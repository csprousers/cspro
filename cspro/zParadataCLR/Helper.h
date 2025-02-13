#pragma once

namespace CSPro
{
    namespace ParadataViewer
    {
        public ref class Helper sealed
        {
        public:
            static void CreateParadataConcatPff(
                System::String^ pffFilename,
                System::String^ lstFilename,
                System::Collections::Generic::List<System::String^>^ inputLogFilenames,
                System::String^ outputLogFilename
            );

            static System::String^ FormatTimestamp(System::String^ formatter,double dTimestamp);
            static System::String^ FormatTimestamp(System::String^ formatter);
        };
    }
}
