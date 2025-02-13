#include "StdAfx.h"
#include "SelectDocsDlg.h"
#include "FakeDropDoc.h"


BEGIN_MESSAGE_MAP(SelectDocsDlg, CDialog)
    ON_BN_CLICKED(IDC_SELECTALL, OnSelectAll)
END_MESSAGE_MAP()


SelectDocsDlg::SelectDocsDlg(std::vector<CDocument*> documents, const TCHAR* dialog_title/* = nullptr*/, CWnd* pParent/* = nullptr*/)
    :   CDialog(SelectDocsDlg::IDD, pParent),
        m_documents(std::move(documents)),
        m_dialogTitle(dialog_title)
{
    ASSERT(!m_documents.empty() && dialog_title != nullptr);
}


void SelectDocsDlg::SetRelativePathAdjuster(const CString& filename)
{
    ASSERT(PortableFunctions::FileIsRegular(filename));

    m_relativePathFilename = filename;
}


BOOL SelectDocsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetWindowText(m_dialogTitle);

    // populate the list
    CListBox* pBox = (CListBox*)GetDlgItem(IDC_DOCS_LIST);

    for( const CDocument* document : m_documents )
    {
        CString description =
            document->IsKindOf(RUNTIME_CLASS(CDDDoc))           ? assert_cast<const CDDDoc*>(document)->GetDict()->GetName() :
            document->IsKindOf(RUNTIME_CLASS(FormFileBasedDoc)) ? assert_cast<const FormFileBasedDoc*>(document)->GetFormFile().GetName() :
            document->IsKindOf(RUNTIME_CLASS(CTabulateDoc))     ? assert_cast<const CTabulateDoc*>(document)->GetTableSpec()->GetName() :
            document->IsKindOf(RUNTIME_CLASS(CAplDoc))          ? assert_cast<const CAplDoc*>(document)->GetAppObject().GetLabel() :
            document->IsKindOf(RUNTIME_CLASS(FakeDropDoc))      ? assert_cast<const FakeDropDoc*>(document)->GetLabel() :
                                                                  ReturnProgrammingError(_T("Unknown Document"));

        CString filename = document->GetPathName();

        if( m_relativePathFilename.has_value() )
            filename = GetRelativeFNameForDisplay<CString>(*m_relativePathFilename, filename);

        ASSERT(!description.IsEmpty() && !filename.IsEmpty());

        pBox->AddString(FormatText(_T("%s: %s"), (LPCTSTR)description, (LPCTSTR)filename));
    }

    // select the first option
    pBox->SetSel(0, TRUE);

    return TRUE;
}


void SelectDocsDlg::OnSelectAll()
{
    CListBox* pBox = (CListBox*)GetDlgItem(IDC_DOCS_LIST);

    for( int i = 0 ; i < pBox->GetCount(); ++i )
        pBox->SetSel(i, TRUE);
}


void SelectDocsDlg::OnOK()
{
    CListBox* pBox = (CListBox*)GetDlgItem(IDC_DOCS_LIST);

    for( int i = pBox->GetCount() - 1; i >= 0; --i )
    {
        if( !pBox->GetSel(i) )
            m_documents.erase(m_documents.begin() + i);
    }

    CDialog::OnOK();
}
