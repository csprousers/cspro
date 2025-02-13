#pragma once

#include <zToolsO/PortableFunctions.h>


class FileAssociation
{
public:
    enum class Type { Dictionary, FormFile, Report, Logic, Message, ResourceFolder };

    FileAssociation(Type type, const CString& description, bool required,
                    const CString& original_filename = CString(), const CString& new_filename = CString())
        :   m_type(type),
            m_description(description),
            m_required(required),
            m_originalFilename(original_filename),
            m_newFilename(new_filename)
    {
    }

    Type GetType() const       { return m_type; }
    bool IsFileBased() const   { return !IsFolderBased(); }
    bool IsFolderBased() const { return ( m_type == Type::ResourceFolder ); }

    const CString& GetDescription() const { return m_description; }

    bool IsRequired() const { return m_required; }

    const CString& GetOriginalFilename() const { return m_originalFilename; }

    const CString& GetNewFilename() const                         { return m_newFilename; }
    void SetNewFilename(const CString& new_filename)              { m_newFilename = new_filename; }

    void MakeNewFilenameFullPath(const CString& relative_to_path)
    {
        m_newFilename = WS2CS(MakeFullPath(PortableFunctions::PathGetDirectory(relative_to_path), CS2WS(m_newFilename)));
    }

    void FixExtension(CString& filename) const
    {
        if( SO::IsBlank(filename) )
            return;

        CString extension = PortableFunctions::PathGetFileExtension<CString>(filename);

        auto fix = [&](std::initializer_list<const TCHAR*> valid_extensions)
        {
            bool valid = false;

            for( const TCHAR* valid_extension : valid_extensions )
            {
                if( extension.CompareNoCase(valid_extension) == 0 )
                {
                    valid = true;
                    break;
                }
            }

            if( !valid )
                filename.AppendFormat(_T(".%s"), *valid_extensions.begin());
        };

        if( m_type == FileAssociation::Type::Dictionary )
        {
            fix({ FileExtensions::Dictionary });
        }

        else if( m_type == FileAssociation::Type::FormFile )
        {
            fix({ FileExtensions::Form });
        }

        else if( m_type == FileAssociation::Type::Logic )
        {
            fix({ FileExtensions::Logic, FileExtensions::Old::Logic });
        }

        else if( m_type == FileAssociation::Type::Message )
        {
            fix({ FileExtensions::Message });
        }

        else
        {
            ASSERT(m_type == FileAssociation::Type::Report ||
                   m_type == FileAssociation::Type::ResourceFolder);
        }
    }

    std::tuple<const TCHAR*, const TCHAR*> GetFilterAndDefaultExtension() const
    {
        if( m_type == FileAssociation::Type::Dictionary )
        {
            return std::make_tuple(_T("Data Dictionary Files (*.dcf)|*.dcf|"), FileExtensions::Dictionary);
        }

        else if( m_type == FileAssociation::Type::FormFile )
        {
            return std::make_tuple(_T("Data Entry Form Files (*.fmf)|*.fmf|"), FileExtensions::Form);
        }
        
        else if( m_type == FileAssociation::Type::Report )
        {
            return std::make_tuple(_T("Report Files (*.html)|*.html|"), _T(""));
        }

        else if( m_type == FileAssociation::Type::Logic )
        {
            return std::make_tuple(_T("Logic Files (*.apc;*.app)|*.apc;*.app|"), FileExtensions::Logic);
        }

        else if( m_type == FileAssociation::Type::Message )
        {
            return std::make_tuple(_T("Message Files (*.mgf)|*.mgf|"), FileExtensions::Message);
        }

        else
        {
            ASSERT(m_type == FileAssociation::Type::ResourceFolder);
            ASSERT(false);
            return std::make_tuple(_T(""), _T(""));
        }
    }

private:
    Type m_type;
    CString m_description;
    bool m_required;
    CString m_originalFilename;
    CString m_newFilename;
};
