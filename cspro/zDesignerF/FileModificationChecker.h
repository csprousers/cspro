#pragma once

#include <zDesignerF/zDesignerF.h>

class TextSourceEditable;


// --------------------------------------------------------------------------
// FileModificationChecker
//
// This class, used with a TextSourceEditable, prompts the user to reload a
// file when the file has changed on the disk.
//
// If the user decides not to the reload the file, they won't be prompted to
// reload the file again until the file has changed again on the disk (based
// on the file modified time changing).
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF FileModificationChecker
{
public:
    // Returns true if the file was modified and the user decides to reload it.
    bool ShouldReloadFile(const TextSourceEditable* text_source);

private:
    int64_t m_fileModifiedTimeAtLastRejection = 0;
};
