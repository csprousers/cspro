#include "StdAfx.h"
#include "FrqOptV.h"
#include <zInterfaceF/UniverseDlg.h>


namespace
{
    class FreqUniverseDlgActionResponder : public UniverseDlg::ActionResponder
    {
    public:
        FreqUniverseDlgActionResponder(CFrqOptionsView& view)
            :   m_view(view)
        {
        }

        bool CheckSyntax(const std::string& universe) override
        {
            return m_view.CheckUniverseSyntax(universe);
        }

        void ToggleNamesInTree() override
        {
            AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_TOGGLE);
        }

    private:
        CFrqOptionsView& m_view;
    };
}


IMPLEMENT_DYNCREATE(CFrqOptionsView, CFormView)


BEGIN_MESSAGE_MAP(CFrqOptionsView, CFormView)
    ON_BN_CLICKED(IDC_USE_VSET, OnDataChange)
    ON_BN_CLICKED(IDC_TRUE_FRQ, OnDataChange)
    ON_BN_CLICKED(IDC_NOSTATS, OnDataChange)
    ON_BN_CLICKED(IDC_GENSTATS, OnDataChange)
    ON_CBN_SELCHANGE(IDC_PERCENTILES, OnDataChange)
    ON_BN_CLICKED(IDC_SORT_ORDER, OnDataChange)
    ON_CBN_SELCHANGE(IDC_SORT_TYPE, OnDataChange)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_FREQ_FORMAT_TABLE, IDC_FREQ_FORMAT_EXCEL, OnDataChange)
    ON_EN_CHANGE(IDC_FRQ_UNIV, OnDataChange)
    ON_EN_CHANGE(IDC_WEIGHT, OnDataChange)
    ON_BN_CLICKED(ID_EDIT_UNIVERSE, OnBnClickedEditUniverse)
END_MESSAGE_MAP()


CFrqOptionsView::CFrqOptionsView()
    :   CFormView(CFrqOptionsView::IDD),
        m_iUseVSet(0),
        m_iNoStats(0),
        m_percentilesIndex(0),
        m_sortOrderAscending(TRUE),
        m_sortTypeIndex(1),
        m_outputFormat(OutputFormat::Table)
{
    ASSERT(strcmp(SortTypeNames[m_sortTypeIndex], "Code") == 0);
}


void CFrqOptionsView::OnInitialUpdate()
{
    CFormView::OnInitialUpdate();

    m_cEditUniverse.ReplaceCEdit(this, false, false);

    FromDoc();
}


void CFrqOptionsView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_USE_VSET, m_iUseVSet);
    DDX_Radio(pDX, IDC_NOSTATS, m_iNoStats);
    DDX_CBIndex(pDX, IDC_PERCENTILES, m_percentilesIndex);
    DDX_Radio(pDX, IDC_FREQ_FORMAT_TABLE, reinterpret_cast<int&>(m_outputFormat));
    DDX_Check(pDX, IDC_SORT_ORDER, m_sortOrderAscending);
    DDX_CBIndex(pDX, IDC_SORT_TYPE, m_sortTypeIndex);
    DDX_Control(pDX, IDC_FRQ_UNIV, m_cEditUniverse);
    DDX_Text(pDX, IDC_WEIGHT, m_weight);

    if( m_cEditUniverse.AlreadyReplacedCEdit() )
    {
        if( pDX->m_bSaveAndValidate )
        {
            m_universe = m_cEditUniverse.GetText();
        }

        else
        {
            m_cEditUniverse.SetText(m_universe);
        }
    }

    UpdateControlVisibility();
}


void CFrqOptionsView::OnDataChange()
{
    if( UpdateData() )
        GetDocument()->SetModifiedFlag();

    auto write_to_registry = [](const wchar_t* const key_name, const wchar_t* const value)
    {
        AfxGetApp()->WriteProfileString(L"Settings", key_name, value);
    };

    write_to_registry(L"TypeValueSet", ( m_iUseVSet == 0 ) ? L"Yes" : L"No");
    write_to_registry(L"GenerateStats", ( m_iNoStats == 1 ) ? L"Yes" : L"No");
    write_to_registry(L"SortOrder", ( m_sortOrderAscending == 1 ) ? L"Ascending" : L"Descending");
    write_to_registry(L"SortType", TC::ToWide(SortTypeNames[m_sortTypeIndex]).c_str());
    write_to_registry(L"OutputFormat", TC::ToWide(OutputFormatNames[static_cast<size_t>(m_outputFormat)]).c_str());
}


void CFrqOptionsView::OnDataChange(UINT /*nID*/)
{
    OnDataChange();
}


void CFrqOptionsView::UpdateControlVisibility()
{
    const int percentiles_show_window = ( m_iNoStats == 1 ) ? SW_SHOW : SW_HIDE ;
    GetDlgItem(IDC_PERCENTILES_TEXT)->ShowWindow(percentiles_show_window);
    GetDlgItem(IDC_PERCENTILES)->ShowWindow(percentiles_show_window);
}


void CFrqOptionsView::ToDoc()
{
    UpdateData();

    CSFreqDoc* pDoc = GetDocument();

    pDoc->m_bUseVset = ( m_iUseVSet == 0 );
    pDoc->m_bHasFreqStats = ( m_iNoStats == 1 );
    pDoc->m_percentiles = ( m_percentilesIndex == 0 ) ? std::nullopt : std::make_optional(m_percentilesIndex + 1);
    pDoc->m_sortOrderAscending = ( m_sortOrderAscending == 1 );
    pDoc->m_sortType = static_cast<FrequencyPrinterOptions::SortType>(m_sortTypeIndex);
    pDoc->m_outputFormat = m_outputFormat;
    pDoc->m_universe = m_universe;
    pDoc->m_weight = m_weight;
}


void CFrqOptionsView::FromDoc()
{
    CSFreqDoc* pDoc = GetDocument();

    m_iUseVSet = pDoc->m_bUseVset ? 0 : 1;
    m_iNoStats = pDoc->m_bHasFreqStats ? 1 : 0;
    m_percentilesIndex = !pDoc->m_percentiles.has_value() ? 0 : ( *pDoc->m_percentiles - 1 );
    m_sortOrderAscending = pDoc->m_sortOrderAscending ? TRUE : FALSE;
    m_sortTypeIndex = static_cast<int>(pDoc->m_sortType);
    m_outputFormat = pDoc->m_outputFormat;
    m_universe = pDoc->m_universe;
    m_weight = pDoc->m_weight;

    UpdateData(FALSE);
}


bool CFrqOptionsView::CheckWeightSyntax(const std::string& weight)
{
    WindowsUtf8::SetText(this, IDC_WEIGHT, weight);

    if( !GetDocument()->CompileApp(XTABSTMENT_WGHT_ONLY) )
    {
        AfxMessageBox(L"Invalid Weight Syntax");
        return false;
    }

    return true;
}


bool CFrqOptionsView::CheckUniverseSyntax(const std::string& universe)
{
    std::string saved_universe = m_universe;

    // CompileApp calls UpdateData(TRUE) so update the text
    m_cEditUniverse.SetText(universe);

    const bool success = GetDocument()->CompileApp(XTABSTMENT_UNIV_ONLY);

    m_universe = saved_universe;
    UpdateData(FALSE);

    if( !success )
        AfxMessageBox(L"Invalid Universe");

    return success;
}


void CFrqOptionsView::RefreshLexer()
{
    m_cEditUniverse.PostMessage(UWM::Edit::RefreshLexer);
}


void CFrqOptionsView::OnBnClickedEditUniverse()
{
    UpdateData(TRUE);

    CSFreqDoc* pDoc = GetDocument();

    // 20111228 for tom
    if( !pDoc->IsAtLeastOneItemSelected() )
    {
        AfxMessageBox(L"You must select at least one item to tabulate before you add a universe");
        return;
    }

    FreqUniverseDlgActionResponder universe_dlg_action_responder(*this);

    UniverseDlg universe_dlg(pDoc->GetSharedDictionary(), m_universe, universe_dlg_action_responder);

    if( universe_dlg.DoModal() == IDOK )
    {
        m_universe = universe_dlg.GetUniverse();
        UpdateData(FALSE);
    }
}
