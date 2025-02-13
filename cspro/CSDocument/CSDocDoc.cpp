#include "StdAfx.h"
#include "CSDocDoc.h"
#include "CSDocument.h"
#include "DocSetSpec.h"


namespace
{
    // previous associations will be be persisted for four weeks
    constexpr const TCHAR* PreviousAssociationsTableName    = _T("csdoc_associations");
    constexpr int64_t PreviousAssociationsExpirationSeconds = DateHelper::SecondsInWeek(4);
}


IMPLEMENT_DYNCREATE(CSDocDoc, TextEditDoc)


CSDocDoc::CSDocDoc()
    :   m_settingsDb(CSProExecutables::Program::CSDocument, PreviousAssociationsTableName, PreviousAssociationsExpirationSeconds, SettingsDb::KeyObfuscator::Hash)
{
}


BOOL CSDocDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(lpszPathName), FileExtensions::CSDocument) )
    {
        ErrorMessage::Display(FormatTextCS2WS(_T("Only documents with the extensions %s and %s can be opened."),
                                              FileExtensions::WithDot::CSDocument, FileExtensions::WithDot::CSDocumentSet));

        return FALSE;
    }

    if( !__super::OnOpenDocument(lpszPathName) )
        return FALSE;

    AutomaticallyAssociateWithDocSet(lpszPathName);

    return TRUE;
}


void CSDocDoc::AutomaticallyAssociateWithDocSet(const std::wstring& csdoc_filename)
{
    // 1)  check if the CSPro Document was opened via the Document Set tree
    CSDocumentApp& csdoc_app = *assert_cast<CSDocumentApp*>(AfxGetApp());

    if( csdoc_app.HasDocSetParametersForNextOpen(csdoc_filename) )
    {
        std::tie(m_docSetSpec, std::ignore) = csdoc_app.ReleaseDocSetParametersForNextOpen();
        ASSERT(m_docSetSpec != nullptr && assert_cast<CMainFrame*>(AfxGetMainWnd())->IsCSDocPartOfDocSet(*m_docSetSpec, csdoc_filename));
        return;
    }

    CMainFrame* main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    auto test_doc_set = [&](const std::wstring& doc_set_filename)
    {
        try
        {
            std::shared_ptr<DocSetSpec> doc_set_spec = main_frame->FindSharedDocSetSpec(doc_set_filename, true);

            if( main_frame->IsCSDocPartOfDocSet(*doc_set_spec, csdoc_filename) )
            {
                m_docSetSpec = std::move(doc_set_spec);
                return true;
            }
        }
        catch(...) { }

        return false;
    };

    // 2) check if a previous association exists 
    const std::wstring* previously_associated_doc_set_filename = m_settingsDb.Read<std::wstring*>(csdoc_filename);

    if( previously_associated_doc_set_filename != nullptr &&
        PortableFunctions::FileIsRegular(*previously_associated_doc_set_filename) &&
        test_doc_set(*previously_associated_doc_set_filename) )
    {
        return;
    }

    // 3) check the current directory and all previous directories (unless the user has disabled this automatic check)
    if( !main_frame->GetGlobalSettings().automatically_associate_documents_with_doc_sets )
        return;

    DirectoryLister directory_lister;
    directory_lister.SetNameFilter(FileExtensions::Wildcard::CSDocumentSet);

    std::wstring test_directory = PortableFunctions::PathGetDirectory(csdoc_filename);

    while( true )
    {
        ASSERT(test_directory.back() == PATH_CHAR);

        for( const std::wstring& doc_set_filename : directory_lister.GetPaths(test_directory) )
        {
            if( test_doc_set(doc_set_filename) )
                return;
        }

        test_directory = PortableFunctions::PathGetDirectory(PortableFunctions::PathRemoveTrailingSlash(test_directory));
        const std::wstring test_directory_with_trailing_slash_removed = PortableFunctions::PathRemoveTrailingSlash(test_directory);

        if( test_directory == test_directory_with_trailing_slash_removed )
            return;
    }
}


void CSDocDoc::OnCloseDocument()
{
    // save the current Document Set association
    m_settingsDb.Write(GetPathName(), ( m_docSetSpec != nullptr ) ? m_docSetSpec->GetFilename() :
                                                                    std::wstring());

    __super::OnCloseDocument();
}
