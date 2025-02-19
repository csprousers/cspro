#include "StdAfx.h"
#include "CSDocDoc.h"
#include "CSDocument.h"
#include "DocSetSpec.h"


namespace
{
    // previous associations will be be persisted for four weeks
    constexpr const char* PreviousAssociationsTableName     = "csdoc_associations";
    constexpr int64_t PreviousAssociationsExpirationSeconds = DateHelper::SecondsInWeek(4);
}


IMPLEMENT_DYNCREATE(CSDocDoc, TextEditDoc)


CSDocDoc::CSDocDoc()
    :   m_settingsDb(CSProExecutables::Program::CSDocument, PreviousAssociationsTableName, PreviousAssociationsExpirationSeconds, SettingsDb::KeyObfuscator::Hash)
{
}


BOOL CSDocDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    const std::string csdoc_file_path = TC::ToUtf8(lpszPathName);

    if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(csdoc_file_path), FileExtensions::CSDocument) )
    {
        ErrorMessage::Display(FormatText("Only documents with the extensions '.%s' and '.%s' can be opened.",
                                         FileExtensions::CSDocument, FileExtensions::CSDocumentSet));

        return FALSE;
    }

    if( !__super::OnOpenDocument(lpszPathName) )
        return FALSE;

    AutomaticallyAssociateWithDocSet(csdoc_file_path);

    return TRUE;
}


void CSDocDoc::AutomaticallyAssociateWithDocSet(const std::string& csdoc_file_path)
{
    // 1)  check if the CSPro Document was opened via the Document Set tree
    CSDocumentApp& csdoc_app = *assert_cast<CSDocumentApp*>(AfxGetApp());

    if( csdoc_app.HasDocSetParametersForNextOpen(csdoc_file_path) )
    {
        std::tie(m_docSetSpec, std::ignore) = csdoc_app.ReleaseDocSetParametersForNextOpen();
        ASSERT(m_docSetSpec != nullptr && assert_cast<CMainFrame*>(AfxGetMainWnd())->IsCSDocPartOfDocSet(*m_docSetSpec, csdoc_file_path));
        return;
    }

    CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    auto test_doc_set = [&](const std::string& doc_set_file_path)
    {
        try
        {
            std::shared_ptr<DocSetSpec> doc_set_spec = main_frame->FindSharedDocSetSpec(doc_set_file_path, true);

            if( main_frame->IsCSDocPartOfDocSet(*doc_set_spec, csdoc_file_path) )
            {
                m_docSetSpec = std::move(doc_set_spec);
                return true;
            }
        }
        catch(...) { }

        return false;
    };

    // 2) check if a previous association exists
    const std::string* const previously_associated_doc_set_filename = m_settingsDb.Read<std::string*>(csdoc_file_path);

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
    directory_lister.SetNameFilter(FileExtensions::CreateWildcard(FileExtensions::CSDocumentSet));

    std::string test_directory = PortableFunctions::PathGetDirectory(csdoc_file_path);

    while( true )
    {
        ASSERT(test_directory.back() == Path::NativeSlashChar);

        for( const std::string& doc_set_file_path : directory_lister.GetPaths(test_directory) )
        {
            if( test_doc_set(doc_set_file_path) )
                return;
        }

        test_directory = PortableFunctions::PathGetDirectory(PortableFunctions::PathRemoveTrailingSlash(test_directory));
        const std::string test_directory_with_trailing_slash_removed = PortableFunctions::PathRemoveTrailingSlash(test_directory);

        if( test_directory == test_directory_with_trailing_slash_removed )
            return;
    }
}


void CSDocDoc::OnCloseDocument()
{
    // save the current Document Set association
    m_settingsDb.Write(TC::ToUtf8(GetPathName()), ( m_docSetSpec != nullptr ) ? m_docSetSpec->GetFilePath() :
                                                                                std::string());

    __super::OnCloseDocument();
}
