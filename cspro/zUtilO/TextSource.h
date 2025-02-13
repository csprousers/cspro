#pragma once

#include <zUtilO/zUtilO.h>

class Serializer;


// this class is used to wrap code, message, and report files;
// proper, non-filename-only implementations are in TextSourceEditable, TextSourceExternal

class CLASS_DECL_ZUTILO TextSource
{
public:
    TextSource(std::wstring filename = std::wstring())
        :   m_filename(std::move(filename))
    {
    }

    virtual ~TextSource()
    {
    }

    const std::wstring& GetFilename() const
    {
        ASSERT(!m_filename.empty());
        return m_filename;
    }

    virtual const std::wstring& GetText() const
    {
        ASSERT(false);
        return SO::EmptyString;
    }

    virtual int64_t GetModifiedIteration() const
    {
        ASSERT(false);
        return 0;
    }

    virtual void SetText(std::wstring /*text*/)
    {
        ASSERT(false);
    }

    virtual bool RequiresSave() const
    {
        return false;
    }

    virtual void Save()
    {
        ASSERT(false);
    }

    void serialize(Serializer& ar);

protected:
    std::wstring m_filename;
};


struct CLASS_DECL_ZUTILO NamedTextSource
{
    std::wstring name;
    std::shared_ptr<TextSource> text_source;

    void serialize(Serializer& ar);
};
