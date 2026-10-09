#pragma once

#include <zUtilO/zUtilO.h>


// --------------------------------------------------------------------------
// TextSource
//
// This class is used to wrap code, message, and report files.
// Implementations that do more than wrap the file path are in:
//     - TextSourceEditable
//     - TextSourceExternal
//     - TextSourceString
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO TextSource
{
public:
    TextSource(std::string file_path = std::string());

    virtual ~TextSource() { }

    const std::string& GetFilePath() const { ASSERT(!m_filePath.empty()); return m_filePath; }

    virtual const std::string& GetText() const;
    virtual SharableString GetTextAsSharableString() const;

    // Returns the timestamp of the last time that the text source was modified.
    // This is not guaranteed to always increase, as a text source's modification
    // iteration could increase as a user makes edits and then go back to the
    // file modified time if the changes are undone or reloaded from the disk.
    virtual int64_t GetModifiedIteration() const;

    virtual void SetText(SharableString text);

    virtual bool RequiresSave() const;
    virtual void Save();

    void serialize(Serializer& ar);

protected:
    std::string m_filePath;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline TextSource::TextSource(std::string file_path/* = std::string()*/)
    :   m_filePath(std::move(file_path))
{
}
