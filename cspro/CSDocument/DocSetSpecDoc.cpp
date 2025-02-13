#include "StdAfx.h"
#include "DocSetSpecDoc.h"


IMPLEMENT_DYNCREATE(DocSetSpecDoc, TextEditDoc)


void DocSetSpecDoc::CreateNewDocument()
{
    const std::wstring filter = FormatTextCS2WS(_T("CSPro Document Sets (%s)|%s|All Files (*.*)|*.*||"),
                                                FileExtensions::Wildcard::CSDocumentSet, FileExtensions::Wildcard::CSDocumentSet);

    CIMSAFileDialog file_dlg(FALSE, FileExtensions::CSDocumentSet, nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter.c_str(), nullptr, CFD_NO_DIR);
    file_dlg.m_ofn.lpstrTitle = _T("Create Document Set");

    if( file_dlg.DoModal() != IDOK )
        return;

    try
    {
        const std::wstring filename = CS2WS(file_dlg.GetPathName());

        DocSetSpec::WriteNewDocumentSetShell(filename);

        AfxGetApp()->OpenDocumentFile(filename.c_str());
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


BOOL DocSetSpecDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    // the primary document type is .csdoc so we should only be here if opening a .csdocset
    ASSERT(SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(lpszPathName), FileExtensions::CSDocumentSet));

    if( !__super::OnOpenDocument(lpszPathName) )
        return FALSE;

    m_docSetSpec = assert_cast<CMainFrame*>(AfxGetMainWnd())->FindSharedDocSetSpec(lpszPathName, true);
    ASSERT(m_docSetSpec != nullptr);

    return TRUE;
}
