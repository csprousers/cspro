#pragma once


namespace CSPro::InstallerGenerator
{
    public ref class ZipCreator sealed
    {
    public:
        static void Create(System::String^ zip_file_path,
                           System::Collections::Generic::List<System::String^>^ file_paths_on_disk,
                           System::Collections::Generic::List<System::String^>^ file_paths_in_zip);
    };
}
