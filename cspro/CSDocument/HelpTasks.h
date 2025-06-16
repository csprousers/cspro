#pragma once


// --------------------------------------------------------------------------
// HelpTasks
//
// Methods in this class are used to help create the CSPro documentation.
// They are not accessible via any interface and must be called explicitly
// (in code).
// --------------------------------------------------------------------------

class HelpTasks
{
public:
    // Creates a table with the EXIF tag names and descriptions.
    static void CreateExifTable();

private:
    static std::string CreateFilePathForOutput(const char* filename_suffix);
};
