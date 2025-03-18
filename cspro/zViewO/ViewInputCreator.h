#pragma once

#include <zViewO/zViewO.h>
#include <zViewO/ViewInput.h>


// --------------------------------------------------------------------------
// ViewInputCreator
//
// ViewInputCreator processes the input, specified as a PFF, a PFF file path,
// or a file path, and returns the appropriate ViewInput.
//
// Exceptions are thrown on error.
// --------------------------------------------------------------------------

class ZVIEWO_API ViewInputCreator
{
private:
    static std::unique_ptr<ViewInput> CreateInput(std::unique_ptr<PFF> pff, std::string input_file_path,
                                                  bool create_only_specialized_viewers);

    static std::unique_ptr<ViewInput> CreateInputFromPff(std::unique_ptr<PFF> pff);

public:
    static std::unique_ptr<ViewInput> CreateInputFromPff(const PFF& pff);
    static std::unique_ptr<ViewInput> CreateInputFromPff(const std::string& pff_file_path);
    static std::unique_ptr<ViewInput> CreateInputFromFilePath(const std::string& file_path);

    // Returns null if there is no specialized viewer for the file type.
    static std::unique_ptr<ViewInput> CreateInputForViewFunction(const std::string& file_path);
};
