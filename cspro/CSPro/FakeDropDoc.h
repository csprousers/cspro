#pragma once


// for dropping files that are not represented by a CDocument object

class FakeDropDoc : public CDocument
{
    DECLARE_DYNCREATE(FakeDropDoc);

private:
    FakeDropDoc() { }

public:
    FakeDropDoc(NullTerminatedString filename, AppFileType app_file_type, size_t index)
        :   m_appFileType(app_file_type),
            m_index(index)
    {
        ASSERT(app_file_type == AppFileType::Code ||
               app_file_type == AppFileType::Message ||
               app_file_type == AppFileType::Report ||
               app_file_type == AppFileType::ResourceFolder);

        SetPathName(filename.c_str(), FALSE);
    }

    FakeDropDoc(const CodeFile& code_file, size_t index)
        :   FakeDropDoc(code_file.GetFilename(), AppFileType::Code, index)
    {
        m_label = ToString(code_file.GetCodeType());
    }

    FakeDropDoc(const std::shared_ptr<TextSource>& text_source, AppFileType app_file_type, size_t index)
        :   FakeDropDoc(text_source->GetFilename(), app_file_type, index)
    {
    }

    FakeDropDoc(const std::shared_ptr<NamedTextSource>& text_source, AppFileType app_file_type, size_t index)
        :   FakeDropDoc(text_source->text_source, app_file_type, index)
    {
        m_label = text_source->name;
    }

    AppFileType GetAppFileType() const { return m_appFileType; }
    size_t GetArrayIndex() const       { return m_index; }

    const TCHAR* GetLabel() const
    {
        return !m_label.empty() ? m_label.c_str() :
                                  ToString(m_appFileType);
    }

private:
    AppFileType m_appFileType;
    size_t m_index;
    std::wstring m_label;
};
