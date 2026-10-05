#include "StdAfx.h"
#include "ZipCreator.h"
#include <zZip/ZipFile.h>


void CSPro::InstallerGenerator::ZipCreator::Create(System::String^ zip_file_path,
                                                   System::Collections::Generic::List<System::String^>^ file_paths_on_disk,
                                                   System::Collections::Generic::List<System::String^>^ file_paths_in_zip)
{
    ASSERT(file_paths_on_disk->Count == file_paths_in_zip->Count);

    try
    {
        std::vector<std::string> file_paths_vector;
        std::vector<std::string> file_paths_in_zip_vector;

        for( int i = 0; i < file_paths_on_disk->Count; ++i )
        {
            file_paths_vector.emplace_back(clr_helpers::to_string(file_paths_on_disk[i]));
            file_paths_in_zip_vector.emplace_back(clr_helpers::to_string(file_paths_in_zip[i]));
        }

        ::ZipCreator zip_creator(clr_helpers::to_string(zip_file_path));
        zip_creator.AddFiles(file_paths_vector, file_paths_in_zip_vector);
    }

    catch( const std::exception& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}
