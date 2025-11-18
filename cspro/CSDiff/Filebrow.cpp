#include "StdAfx.h"
#include "Filebrow.h"
#include "Csdfdoc.h"
#include <zUtilO/WindowHelpers.h>
#include <zBridgeO/DataFileDlg.h>


BEGIN_MESSAGE_MAP(CFilesBrow, ResizableDlg)
    ON_BN_CLICKED(IDC_LISTBROW, OnListbrow)
    ON_BN_CLICKED(IDC_INPBROW, OnInpbrow)
    ON_BN_CLICKED(IDC_REFBROW, OnRefbrow)
    ON_EN_CHANGE(IDC_INPUTFILE, OnChangeInputfile)
    ON_EN_CHANGE(IDC_LISTFILE, OnChangeListfile)
    ON_EN_CHANGE(IDC_REFERENCEFILE, OnChangeReferencefile)
END_MESSAGE_MAP()


CFilesBrow::CFilesBrow(CCSDiffDoc* const pDoc, PFF& pff, CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_FILEBROW, pParent),
        m_pDoc(pDoc),
        m_diffSpec(m_pDoc->GetDiffSpec()),
        m_pff(pff),
        m_inputConnectionString(m_pff.GetSingleInputDataConnectionString()),
        m_referenceConnectionString(m_pff.GetReferenceDataConnectionString()),
        m_listingFilePath(UTF8_TODO::GetUtf8(m_pff.GetListingFName())),
        m_diffMethod(m_diffSpec.GetDiffMethod()),
        m_diffMethodRadioEnumHelper({ DiffSpec::DiffMethod::OneWay,
                                      DiffSpec::DiffMethod::BothWays }),
        m_diffOrder(m_diffSpec.GetDiffOrder()),
        m_diffOrderRadioEnumHelper({ DiffSpec::DiffOrder::Indexed,
                                     DiffSpec::DiffOrder::Sequential })
{
    ASSERT(m_diffSpec.IsDictionaryDefined());

    SerializeDialogSize("CFilesBrow");
}


void CFilesBrow::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_INPUTFILE, m_inputConnectionString);
    DDX_Text(pDX, IDC_REFERENCEFILE, m_referenceConnectionString);
    DDX_Text(pDX, IDC_LISTFILE, m_listingFilePath);
    DDX_Radio(pDX, IDC_ONEWAY, m_diffMethodRadioEnumHelper, m_diffMethod);
    DDX_Radio(pDX, IDC_INDEXED, m_diffOrderRadioEnumHelper, m_diffOrder);
}


void CFilesBrow::OnListbrow()
{
    UpdateData(TRUE);

    OpenFileDlg open_file_dlg(0, nullptr, m_listingFilePath, FileFilters::Listing, this);
    open_file_dlg.SetTitle(L"Select Listing File for Compare");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_listingFilePath = open_file_dlg.GetFilePath();

    UpdateData(FALSE);
    EnableDisable();
}


void CFilesBrow::OnDataBrowse(ConnectionString& connection_string, const ConnectionString& other_connection_string)
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, connection_string);
    data_file_dlg.SetDictionaryFilePath(m_diffSpec.GetDictionary().GetFilePath())
                 .SuggestMatchingDataRepositoryType(other_connection_string);

    if( data_file_dlg.DoModal() != IDOK )
        return;

    connection_string = data_file_dlg.GetConnectionString();

    UpdateData(FALSE);
    EnableDisable();
}


void CFilesBrow::OnInpbrow()
{
    OnDataBrowse(m_inputConnectionString, m_referenceConnectionString);
}


void CFilesBrow::OnRefbrow()
{
    OnDataBrowse(m_referenceConnectionString, m_inputConnectionString);
}


BOOL CFilesBrow::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    UpdateData(FALSE);
    GetDlgItem(IDOK)->EnableWindow(FALSE);
    EnableDisable();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CFilesBrow::OnOK()
{
    UpdateData(TRUE);

    m_pff.SetSingleInputDataConnectionString(m_inputConnectionString);
    m_pff.SetReferenceDataConnectionString(m_referenceConnectionString);
    m_pff.SetListingFName(UTF8_TODO::GetCString(m_listingFilePath));

    if( m_diffSpec.GetDiffMethod() != m_diffMethod || m_diffSpec.GetDiffOrder() != m_diffOrder )
    {
        m_diffSpec.SetDiffMethod(m_diffMethod);
        m_diffSpec.SetDiffOrder(m_diffOrder);
        m_pDoc->SetModifiedFlag();
    }

    __super::OnOK();
}


void CFilesBrow::OnChangeInputfile()
{
    EnableDisable();
}


void CFilesBrow::OnChangeListfile()
{
    EnableDisable();
}


void CFilesBrow::OnChangeReferencefile()
{
    EnableDisable();
}


void CFilesBrow::EnableDisable()
{
    UpdateData(TRUE);
    GetDlgItem(IDOK)->EnableWindow(( m_inputConnectionString.IsDefined() &&
                                     m_referenceConnectionString.IsDefined() &&
                                     !SO::IsWhitespace(m_listingFilePath) ));
}
