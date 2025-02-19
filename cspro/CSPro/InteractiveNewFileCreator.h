#pragma once

#include <zDesignerF/NewFileCreator.h>


// NewFileCreator and InteractiveNewFileCreator's routines will throw CSProException exceptions on error.

class InteractiveNewFileCreator : public NewFileCreator
{
public:
    // Shows a dialog with the possible file types and creates one as appropriate,
    // returning the file path and type of the file if one was successfully created.
    static std::optional<std::tuple<std::string, AppFileType>> InteractiveMode();
};
