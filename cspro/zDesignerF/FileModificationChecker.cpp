#include "StdAfx.h"
#include "FileModificationChecker.h"
#include <zUtilO/TextSourceEditable.h>


bool FileModificationChecker::ShouldReloadFile(const TextSourceEditable* const text_source)
{
    if( text_source == nullptr )
        return false;

    const int64_t file_modified_time = PortableFunctions::FileModifiedTime(text_source->GetFilePath());

    if( file_modified_time != m_fileModifiedTimeAtLastRejection &&
        file_modified_time != text_source->GetLoadedFileModifiedTime() )
    {
        const std::string message = FormatText(
            "The file has been modified by another program.\nDo you want to reload '%s'?",
            Path::GetFilename(text_source->GetFilePath()).c_str()
        );

        const int response = AfxMessageBox(message, MB_YESNO);

        if( response == IDYES )
        {
            m_fileModifiedTimeAtLastRejection = 0;
            return true;
        }

        else
        {
            m_fileModifiedTimeAtLastRejection = file_modified_time;
        }
    }

    return false;
}
