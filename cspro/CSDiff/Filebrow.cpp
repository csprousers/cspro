#include "StdAfx.h"
#include "Filebrow.h"
#include "Csdfdoc.h"
#include <zUtilO/Filedlg.h>
#include <ZBRIDGEO/DataFileDlg.h>


BEGIN_MESSAGE_MAP(CFilesBrow, CDialog)
    ON_BN_CLICKED(IDC_LISTBROW, OnListbrow)
    ON_BN_CLICKED(IDC_INPBROW, OnInpbrow)
    ON_BN_CLICKED(IDC_REFBROW, OnRefbrow)
    ON_EN_CHANGE(IDC_INPUTFILE, OnChangeInputfile)
    ON_EN_CHANGE(IDC_LISTFILE, OnChangeListfile)
    ON_EN_CHANGE(IDC_REFERENCEFILE, OnChangeReferencefile)
END_MESSAGE_MAP()


CFilesBrow::CFilesBrow(CCSDiffDoc* pDoc, PFF& pff, CWnd* pParent/* = nullptr*/)
    :   CDialog(CFilesBrow::IDD, pParent),
        m_pDoc(pDoc),
        m_diffSpec(m_pDoc->GetDiffSpec()),
        m_pff(pff)
{
    ASSERT(m_diffSpec.IsDictionaryDefined());

    if( m_pff.GetSingleInputDataConnectionString().IsDefined() )
        m_inputFilename = WS2CS(m_pff.GetSingleInputDataConnectionString().ToString());

    if( m_pff.GetReferenceDataConnectionString().IsDefined() )
        m_referenceFilename = WS2CS(m_pff.GetReferenceDataConnectionString().ToString());

    m_listingFilename = CS2WS(m_pff.GetListingFName());

    m_roneway = ( m_diffSpec.GetDiffMethod() == DiffSpec::DiffMethod::BothWays );
    m_indexseq = ( m_diffSpec.GetDiffOrder() == DiffSpec::DiffOrder::Sequential );
}


void CFilesBrow::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_INPUTFILE, m_inputFilename);
    DDX_Text(pDX, IDC_REFERENCEFILE, m_referenceFilename);
    DDX_Text(pDX, IDC_LISTFILE, m_listingFilename);
    DDX_Radio(pDX, IDC_ONEWAY, m_roneway);
    DDX_Radio(pDX, IDC_INDEXED, m_indexseq);
}


void CFilesBrow::OnListbrow()
{
    UpdateData(TRUE);

    CIMSAFileDialog dlg(FALSE, NULL, m_listingFilename.c_str(), OFN_HIDEREADONLY,
                        FileFilters::Listing, AfxGetApp()->GetMainWnd(), CFD_NO_DIR);
    dlg.m_ofn.lpstrTitle = _T("Select Listing File for Compare");

    if( dlg.DoModal() != IDOK )
        return;

    m_listingFilename = CS2WS(dlg.GetPathName());

    UpdateData(FALSE);
    EnableDisable();
}


void CFilesBrow::OnInpbrow()
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, ConnectionString(m_inputFilename));
    data_file_dlg.SetDictionaryFilename(m_diffSpec.GetDictionary().GetFullFileName())
                 .SuggestMatchingDataRepositoryType(ConnectionString(m_referenceFilename));

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_inputFilename = data_file_dlg.GetConnectionString().ToString();

    UpdateData(FALSE);
    EnableDisable();
}


void CFilesBrow::OnRefbrow()
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, ConnectionString(m_referenceFilename));
    data_file_dlg.SetDictionaryFilename(m_diffSpec.GetDictionary().GetFullFileName())
                 .SuggestMatchingDataRepositoryType(ConnectionString(m_inputFilename));

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_referenceFilename = data_file_dlg.GetConnectionString().ToString();

    UpdateData(FALSE);
    EnableDisable();
}


BOOL CFilesBrow::OnInitDialog()
{
    CDialog::OnInitDialog();

    UpdateData(FALSE);
    GetDlgItem(IDOK)->EnableWindow(FALSE);
    EnableDisable();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CFilesBrow::OnOK()
{
    UpdateData();

    m_pff.SetSingleInputDataConnectionString(WS2CS(m_inputFilename));
    m_pff.SetReferenceDataConnectionString(WS2CS(m_referenceFilename));
    m_pff.SetListingFName(WS2CS(m_listingFilename));

    const DiffSpec::DiffMethod diff_method = m_roneway ? DiffSpec::DiffMethod::BothWays : DiffSpec::DiffMethod::OneWay;
    const DiffSpec::DiffOrder diff_order = m_indexseq ? DiffSpec::DiffOrder::Sequential : DiffSpec::DiffOrder::Indexed;

    if( m_diffSpec.GetDiffMethod() != diff_method || m_diffSpec.GetDiffOrder() != diff_order )
    {
        m_diffSpec.SetDiffMethod(diff_method);
        m_diffSpec.SetDiffOrder(diff_order);
        m_pDoc->SetModifiedFlag();
    }

    CDialog::OnOK();
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
    GetDlgItem(IDOK)->EnableWindow(( !SO::IsBlank(m_inputFilename) &&
                                     !SO::IsBlank(m_referenceFilename) &&
                                     !SO::IsBlank(m_listingFilename) ));
}
