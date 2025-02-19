#pragma once

#include <zToolsO/CSProException.h>


CREATE_CSPRO_EXCEPTION(ApplicationLoadException);


struct ApplicationFileNotFoundException : public ApplicationLoadException
{
    ApplicationFileNotFoundException(cs::string_sz file_path, const char* file_type)
        :   ApplicationLoadException("The %s file was not found: %s", file_type, file_path.c_str())
    {
    }

    ApplicationFileNotFoundException(cs::string_sz file_path)
        :   ApplicationLoadException("The file was not found: %s", file_path.c_str())
    {
    }
};


struct ApplicationFileLoadException : public ApplicationLoadException
{
    ApplicationFileLoadException(cs::string_sz file_path, const char* file_type)
        :   ApplicationLoadException("There was an error loading the %s file: %s", file_type, file_path.c_str())
    {
    }

    ApplicationFileLoadException(cs::string_sz file_path)
        :   ApplicationLoadException("There was an error loading the file: %s", file_path.c_str())
    {
    }
};
