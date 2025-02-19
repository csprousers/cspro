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
