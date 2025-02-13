#pragma once

#include <CSDocument/DocSetComponent.h>


class CSDocumentApp : public CWinAppEx
{
public:
    CSDocumentApp();

    static bool DocumentCanBeOpenedDirectly(const std::wstring& filename);

    bool HasDocSetParametersForNextOpen(const std::wstring& filename) const;
    void SetDocSetParametersForNextOpen(DocSetComponent doc_set_component, std::shared_ptr<DocSetSpec> doc_set_spec);
    std::tuple<std::shared_ptr<DocSetSpec>, DocSetComponent::Type> ReleaseDocSetParametersForNextOpen();

    CDocument* OpenDocumentFile(LPCTSTR lpszFileName) override;
	CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU) override;

protected:
    DECLARE_MESSAGE_MAP()

    BOOL InitInstance() override;
    int ExitInstance() override;

    void OnFileNew();
    void OnFileOpen();
    void OnAppAbout();

private:
    int m_exitCode;
    CMultiDocTemplate* m_csdocTemplate;
    std::optional<std::tuple<DocSetComponent, std::shared_ptr<DocSetSpec>>> m_docSetParametersForNextOpen;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline bool CSDocumentApp::DocumentCanBeOpenedDirectly(const std::wstring& filename)
{
    const std::wstring extension = PortableFunctions::PathGetFileExtension(filename);
    return SO::EqualsOneOfNoCase(extension, FileExtensions::CSDocument, FileExtensions::CSDocumentSet);
}


inline bool CSDocumentApp::HasDocSetParametersForNextOpen(const std::wstring& filename) const
{
    return ( m_docSetParametersForNextOpen.has_value() &&
             SO::EqualsNoCase(filename, std::get<0>(*m_docSetParametersForNextOpen).filename) );
}
