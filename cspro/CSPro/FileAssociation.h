#pragma once

#include <zToolsO/PortableFunctions.h>


class FileAssociation
{
public:
    enum class Type { Dictionary, FormFile };

    FileAssociation(Type type, const CString& description, bool required,
                    const CString& original_filename = CString(), const CString& new_filename = CString());

    Type GetType() const { return m_type; }

    const CString& GetDescription() const { return m_description; }

    bool IsRequired() const { return m_required; }

    const CString& GetOriginalFilename() const { return m_originalFilename; }

    const CString& GetNewFilename() const            { return m_newFilename; }
    void SetNewFilename(const CString& new_filename) { m_newFilename = new_filename; }

    void MakeNewFilenameFullPath(const CString& relative_to_path);

    void FixExtension(CString& filename) const;

    std::tuple<const char*, const char*> GetFilterAndDefaultExtension() const;

private:
    Type m_type;
    CString m_description;
    bool m_required;
    CString m_originalFilename;
    CString m_newFilename;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline FileAssociation::FileAssociation(const Type type, const CString& description, const bool required,
                                        const CString& original_filename/* = CString()*/, const CString& new_filename/* = CString()*/)
    :   m_type(type),
        m_description(description),
        m_required(required),
        m_originalFilename(original_filename),
        m_newFilename(new_filename)
{
}


inline void FileAssociation::MakeNewFilenameFullPath(const CString& relative_to_path)
{
    m_newFilename = WS2CS(MakeFullPath(PortableFunctions::PathGetDirectory(relative_to_path), CS2WS(m_newFilename)));
}


inline void FileAssociation::FixExtension(CString& filename) const
{
    if( SO::IsBlank(filename) )
        return;

    const std::string extension = PortableFunctions::PathGetFileExtension(UTF8_TODO::GetUtf8(filename));

    auto fix = [&](const std::initializer_list<const char*> valid_extensions)
    {
        bool valid = false;

        for( const char* const valid_extension : valid_extensions )
        {
            if( SO::EqualsNoCase(extension, valid_extension) )
            {
                valid = true;
                break;
            }
        }

        if( !valid )
            filename.Append(UTF8_TODO::GetCString(FileExtensions::WithDot(*valid_extensions.begin())));
    };

    if( m_type == FileAssociation::Type::Dictionary )
    {
        fix({ FileExtensions::Dictionary });
    }

    else if( m_type == FileAssociation::Type::FormFile )
    {
        fix({ FileExtensions::Form });
    }

    else
    {
        ASSERT(false);
    }
}


inline std::tuple<const char*, const char*> FileAssociation::GetFilterAndDefaultExtension() const
{
    if( m_type == FileAssociation::Type::Dictionary )
    {
        return { "Data Dictionary Files (*.dcf)|*.dcf|", FileExtensions::Dictionary };
    }

    else if( m_type == FileAssociation::Type::FormFile )
    {
        return { "Data Entry Form Files (*.fmf)|*.fmf|", FileExtensions::Form };
    }

    else
    {
        ASSERT(false);
        return std::make_tuple("", "");
    }
}
