#pragma once

#include <zToolsO/CSProException.h>


CREATE_CSPRO_EXCEPTION(ApplicationLoadException)


struct ApplicationFileNotFoundException : public ApplicationLoadException
{
    ApplicationFileNotFoundException(NullTerminatedString filename, const TCHAR* file_type)
        :   ApplicationLoadException(_T("The %s file was not found: %s"), file_type, filename.c_str())
    {
    }

    ApplicationFileNotFoundException(NullTerminatedString filename)
        :   ApplicationLoadException(_T("The file was not found: %s"), filename.c_str())
    {
    }
};


struct ApplicationFileLoadException : public ApplicationLoadException
{
    ApplicationFileLoadException(NullTerminatedString filename, const TCHAR* file_type)
        :   ApplicationLoadException(_T("There was an error loading the %s file: %s"), file_type, filename.c_str())
    {
    }

    ApplicationFileLoadException(NullTerminatedString filename)
        :   ApplicationLoadException(_T("There was an error loading the file: %s"), filename.c_str())
    {
    }
};
