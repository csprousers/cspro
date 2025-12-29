#include "StdAfx.h"
#include "Helpers.h"


bool Helpers::CompareFilePathsByDirectory(const std::string& file_path1, const std::string& file_path2)
{
    const auto count_slashes = [](const char ch) { return Path::IsSlashChar(ch); };
    const size_t num_dirs1 = std::count_if(file_path1.cbegin(), file_path1.cend(), count_slashes);
    const size_t num_dirs2 = std::count_if(file_path2.cbegin(), file_path2.cend(), count_slashes);

    return ( num_dirs1 != num_dirs2 ) ? ( num_dirs1 < num_dirs2 ) :
                                        ( SO::CompareNoCase(file_path1, file_path2) < 0 );
}
