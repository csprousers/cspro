#pragma once

namespace CSPro
{
    namespace ParadataViewer
    {
        public ref class Helper sealed
        {
        public:
            static void CreateParadataConcatPff(System::String^ pff_file_path,
                                                System::String^ listing_file_path,
                                                System::Collections::Generic::List<System::String^>^ input_log_file_paths,
                                                System::String^ output_log_File_path
            );

            static System::String^ FormatTimestamp(System::String^ formatter, double timestamp);
            static System::String^ FormatTimestamp(System::String^ formatter);
        };
    }
}
