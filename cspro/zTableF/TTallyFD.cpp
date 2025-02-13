#include "StdAfx.h"
#include "TTallyFD.h"
#include "TTallyFD.h"
#include "LogicDlg.h"
#include "TblUtDlg.h"
#include "TabDoc.h"
#include "TabView.h"
#include <zInterfaceF/UniverseDlg.h>


// CTblTallyFmtDlg dialog

IMPLEMENT_DYNAMIC(CTblTallyFmtDlg, CDialog)
CTblTallyFmtDlg::CTblTallyFmtDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CTblTallyFmtDlg::IDD, pParent)
    , m_sUnivString(_T(""))
    , m_sWeight(_T(""))
    , m_sValue(_T(""))
    , m_sSubTable(_T(""))
    , m_bUseCustomSpecVal(FALSE)
    , m_bUseSpecValNotAppl(FALSE)
    , m_bUseSpecValMissing(FALSE)
    , m_bUseSpecValRefused(FALSE)
    , m_bUseSpecValDefault(FALSE)
    , m_bUseSpecValUndefined(FALSE)

    , m_sTabLogic(_T(""))
    , m_sPostCalc(_T(""))
{
    m_iBreakLevel = -1;
    m_pConsolidate = NULL;
    m_iCurSubTable =0;
}

CTblTallyFmtDlg::~CTblTallyFmtDlg()
{
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblTallyFmtDlg::DoDataExchange(CDataExchange* pDX)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblTallyFmtDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_WEIGHT, m_sWeight);
    DDX_Text(pDX, IDC_VALUE, m_sValue);
    DDX_Control(pDX, IDC_SUBTABLES, m_cSubTables);
    DDX_Control(pDX, IDC_UNIT, m_cUnitName);
    DDX_CBString(pDX, IDC_SUBTABLES, m_sSubTable);
    DDX_Control(pDX, IDC_BREAKLEVEL, m_cBreakLevel);
    DDX_Check(pDX, IDC_USE_CUSTOM_SPECIAL_VALUES, m_bUseCustomSpecVal);
    DDX_Check(pDX, IDC_SPECIAL_VAL_NOTAPPL, m_bUseSpecValNotAppl);
    DDX_Check(pDX, IDC_SPECIAL_VAL_MISSING, m_bUseSpecValMissing);
    DDX_Check(pDX, IDC_SPECIAL_VAL_REFUSED, m_bUseSpecValRefused);
    DDX_Check(pDX, IDC_SPECIAL_VAL_DEFAULT, m_bUseSpecValDefault);
    DDX_Check(pDX, IDC_SPECIAL_VAL_UNDEFINED, m_bUseSpecValUndefined);
    DDX_Control(pDX, IDC_UNIVSTRING, m_editUniv);
    DDX_Control(pDX, IDC_TABLOGIC, m_editTabLogic);
    DDX_Control(pDX, IDC_POSTCALC, m_editPostCalc);
}


BEGIN_MESSAGE_MAP(CTblTallyFmtDlg, CDialog)
    ON_BN_CLICKED(IDC_UNIVERSE, OnBnClickedUniverse)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_CBN_SELCHANGE(IDC_SUBTABLES, OnCbnSelchangeSubtables)
    ON_CBN_SELENDOK(IDC_SUBTABLES, OnCbnSelendokSubtables)
    ON_CBN_SELCHANGE(IDC_BREAKLEVEL, OnCbnSelchangeBreaklevel)
    ON_BN_CLICKED(IDC_USE_CUSTOM_SPECIAL_VALUES, OnBnClickedUseCustSpecVal)
    ON_BN_CLICKED(IDC_WGHT_APPLYALL, OnBnClickedWghtApplyall)
    ON_BN_CLICKED(IDC_UNIV_APPLYALL, OnBnClickedUnivApplyall)
    ON_BN_CLICKED(IDC_BTN_TABLOGIC, OnBnClickedBtnTablogic)
    ON_BN_CLICKED(IDC_BTN_POSTCALC, OnBnClickedBtnPostcalc)
    ON_BN_CLICKED(IDC_APPLYALL_UNIT, OnBnClickedApplyallUnit)
END_MESSAGE_MAP()
// CTblTallyFmtDlg message handlers


namespace
{
    class CrosstabUniverseDlgActionResponder : public UniverseDlg::ActionResponder
    {
    public:
        CrosstabUniverseDlgActionResponder(CTblTallyFmtDlg& dlg)
            :   m_dlg(dlg)
        {
        }

        bool CheckSyntax(const std::wstring& universe) override
        {
            return m_dlg.CheckUniverseSyntax(WS2CS(universe));
        }

        void ToggleNamesInTree() override
        {
            CFrameWnd* pMainWnd = (CFrameWnd*)m_dlg.GetParent();
            pMainWnd->SendMessage(WM_COMMAND, ID_VIEW_NAMES);
        }

    private:
        CTblTallyFmtDlg& m_dlg;
    };
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblTallyFmtDlg::OnBnClickedUniverse()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblTallyFmtDlg::OnBnClickedUniverse()
{
    CTabulateDoc* pDoc = m_pTabView->GetDocument();
    std::wstring universe = m_editUniv.GetText();
    
    CrosstabUniverseDlgActionResponder universe_dlg_action_responder(*this);

    UniverseDlg universe_dlg(pDoc->GetTableSpec()->GetSharedDictionary(), std::move(universe), universe_dlg_action_responder);

    if( universe_dlg.DoModal() == IDOK )
        m_editUniv.SetText(universe_dlg.GetUniverse());
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblTallyFmtDlg::CheckSyntaxAll()
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblTallyFmtDlg::CheckSyntaxAll()
{
    CTabulateDoc* pDoc = m_pTabView->GetDocument();
    // make sure that if tbl has units, subtables don't and vice versa
    if(!CheckSyntax(0, XTABSTMENT_POSTCALC_ONLY)){
        CIMSAString sMsg = _T("PostCalc\r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
        return false;
    }
    if(!CheckSyntax(0, XTABSTMENT_TABLOGIC_ONLY)){
        CIMSAString sMsg = _T("Tablogic\r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
        return false;
    }

    bool bSubTblHasUnits = false;
    if (m_iCurSubTable == 0) {
        // table is selected, check all subtables
        for (int i = 1; i < m_cSubTables.GetCount(); ++i) {
            CUnitSpec* pSubTblSpec = (CUnitSpec*)m_cSubTables.GetItemDataPtr(i);
            if (pSubTblSpec->IsUnitPresent()) {
                bSubTblHasUnits = true;
                break;
            }
        }
    }
    else {
        // just check current subtable
        CUnitSpec* pSubTblSpec = (CUnitSpec*)m_cSubTables.GetItemDataPtr(m_iCurSubTable);
        bSubTblHasUnits = pSubTblSpec->IsUnitPresent();
    }

    /*if (bTblHasUnits && bSubTblHasUnits) {
        AfxMessageBox("You may not have units, weights or universe for both the table and subtable");
        return false;
    }*/

    if(!CheckSyntaxSubtable(m_iCurSubTable)){
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  bool CTblTallyFmtDlg::CheckSyntaxSubtable(int iSubtable)
//
/////////////////////////////////////////////////////////////////////////////////
bool CTblTallyFmtDlg::CheckSyntaxSubtable(int iSubtable)
{
    CTabulateDoc* pDoc = m_pTabView->GetDocument();
    if(!CheckSyntax(iSubtable, XTABSTMENT_UNITS_ONLY)){
        CIMSAString sMsg = _T("*** Unit ***\r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
        return false;
    }
    if(!CheckSyntax(iSubtable, XTABSTMENT_WGHT_ONLY)){
        CIMSAString sMsg = _T("*** Value or Weight ***\r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
        return false;
    }
    if(!CheckSyntax(iSubtable, XTABSTMENT_UNIV_ONLY)){
        CIMSAString sMsg = _T("*** Universe ***\r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
        return false;
    }
    if(!CheckSyntax(iSubtable, XTABSTMENT_TABLOGIC_ONLY)){
        CIMSAString sMsg = _T("*** Tablogic ***\r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblTallyFmtDlg::OnBnClickedOk()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblTallyFmtDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    UpdateData(TRUE);

    m_sUnivString = WS2CS(m_editUniv.GetText());
    m_sTabLogic = WS2CS(m_editTabLogic.GetText());
    m_sPostCalc = WS2CS(m_editPostCalc.GetText());


    m_iBreakLevel = m_cBreakLevel.GetItemData(m_cBreakLevel.GetCurSel());

    CUnitSpec* pUnitSpec = (CUnitSpec*)m_cSubTables.GetItemDataPtr(m_cSubTables.GetCurSel());
    if(m_cUnitName.IsWindowEnabled()){
        SetLoopingVar(pUnitSpec);
    }

    m_sWeight.Trim();
    pUnitSpec->SetWeightExpr(m_sWeight);

    m_sValue.Trim();
    pUnitSpec->SetValue(m_sValue);

    m_sUnivString.Trim();
    pUnitSpec->SetUniverse(m_sUnivString);

    CStringArray& arrTabLogic = pUnitSpec->GetTabLogicArray();
    arrTabLogic.RemoveAll();
    MakeTabLogicArray(m_sTabLogic,arrTabLogic);

    m_arrPostCalc.RemoveAll();
    MakeTabLogicArray(m_sPostCalc,m_arrPostCalc);

    //To do check postcalc syntax;
    if(!CheckSyntaxAll()){
        return;
    }
    OnOK();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CTblTallyFmtDlg::OnInitDialog()
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CTblTallyFmtDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //create scintilla control for universe
    DWORD dwStyle = WS_CHILDWINDOW | WS_VISIBLE | WS_MAXIMIZEBOX;
    RECT rect;
    m_editUniv.GetWindowRect(&rect);
    this->ScreenToClient(&rect);
    int nControlID = m_editUniv.GetDlgCtrlID();
    m_editUniv.DestroyWindow();
    m_editUniv.Create(dwStyle, rect, this, nControlID, WS_EX_CLIENTEDGE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
    m_editUniv.InitLogicControl(false, false);


    //create scintilla control for tablogic
    m_editTabLogic.GetWindowRect(&rect);
    this->ScreenToClient(&rect);
    nControlID = m_editTabLogic.GetDlgCtrlID();
    m_editTabLogic.DestroyWindow();
    m_editTabLogic.Create(dwStyle, rect, this, nControlID, WS_EX_CLIENTEDGE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
    m_editTabLogic.InitLogicControl(false, false);

    //create scintilla control for postcalc
    m_editPostCalc.GetWindowRect(&rect);
    this->ScreenToClient(&rect);
    nControlID = m_editPostCalc.GetDlgCtrlID();
    m_editPostCalc.DestroyWindow();
    m_editPostCalc.Create(dwStyle, rect, this, nControlID, WS_EX_CLIENTEDGE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
    m_editPostCalc.InitLogicControl(false, false);

    UpdateDlgContent();
    CTabulateDoc* pDoc = m_pTabView->GetDocument();
    CTabSet* pTabSet = pDoc->GetTableSpec();
    int iNumTables =pTabSet->GetNumTables();
    if(m_iCurSubTable ==0 && iNumTables > 1){
        GetDlgItem(IDC_UNIV_APPLYALL)->EnableWindow(TRUE);
        GetDlgItem(IDC_WGHT_APPLYALL)->EnableWindow(TRUE);
    }
    else {
        GetDlgItem(IDC_UNIV_APPLYALL)->EnableWindow(FALSE);
        GetDlgItem(IDC_WGHT_APPLYALL)->EnableWindow(FALSE);
    }
    GetDlgItem(IDC_APPLYALL_UNIT)->EnableWindow(FALSE);
    bool bMorethanOneSubtable = m_aUnitSpec.GetSize() > 2;
    if(bMorethanOneSubtable){
        GetDlgItem(IDC_APPLYALL_UNIT)->EnableWindow(TRUE);
    }
    if(m_cSubTables.GetCurSel() > 0){
        SetWindowText(_T("Tally Attributes (Subtable)"));
    }
    else {
        SetWindowText(_T("Tally Attributes (Table)"));
    }


    //to refresh scintilla horizontal scroll bar removed when not required
    m_editUniv.SetScrollWidth(1);
    m_editUniv.SetScrollWidthTracking(TRUE);

    m_editTabLogic.SetScrollWidth(1);
    m_editTabLogic.SetScrollWidthTracking(TRUE);

    m_editPostCalc.SetScrollWidth(1);
    m_editPostCalc.SetScrollWidthTracking(TRUE);

    m_editUniv.SetText(m_sUnivString); //call base class instead of window text
    m_editTabLogic.SetText(m_sTabLogic); //call base class instead of window text
    m_editPostCalc.SetText(m_sPostCalc); //call base class instead of window text

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblTallyFmtDlg::UpdateDlgContent()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblTallyFmtDlg::UpdateDlgContent()
{
    // Tables /Subtables
    int iCount = m_aUnitSpec.GetSize();
    iCount ==  2 ? iCount = iCount-1 : iCount = iCount;
    m_iCurSubTable > iCount ? m_iCurSubTable = 0 : m_iCurSubTable =m_iCurSubTable;
    for (int iIndex =0; iIndex <iCount; iIndex++){
        if(iIndex==0  && m_aUnitSpec[iIndex].GetSubTableString().IsEmpty()){
            m_cSubTables.AddString(_T("Entire Table"));
        }
        else{
            if(m_aUnitSpec[iIndex].GetAltSubTableString().IsEmpty()){
                m_cSubTables.AddString(m_aUnitSpec[iIndex].GetSubTableString());
            }
            else {
                m_cSubTables.AddString(m_aUnitSpec[iIndex].GetAltSubTableString());
            }
        }
        m_cSubTables.SetItemDataPtr(iIndex,&m_aUnitSpec[iIndex]);
    }

    m_cSubTables.SetCurSel(m_iCurSubTable);

    UpdateUnitsCombo();

    //Now Set the value for the controls.
    if(m_aUnitSpec.GetSize() >1){
        CUnitSpec& unitSpec = m_aUnitSpec[m_iCurSubTable];
        m_sValue = unitSpec.GetValue();
        m_sWeight = unitSpec.GetWeightExpr();
        m_sUnivString = unitSpec.GetUniverse();
        m_sTabLogic =_T("");
        CStringArray& arrTabLogic = unitSpec.GetTabLogicArray();
        m_sTabLogic = MakeStringFromTabLogicArray(arrTabLogic);
        UpdateData(FALSE);
    }

    // BreakLevel
    if(m_pConsolidate) {
        if(m_pConsolidate->GetNumAreas() ==0){
            m_cBreakLevel.EnableWindow(FALSE);
        }
        else {
            int iNumAreas = m_pConsolidate->GetNumAreas();
            CIMSAString sLowest(_T("Lowest"));
            sLowest += _T(" (") + m_pConsolidate->GetArea(iNumAreas-1)+_T(")");
            m_cBreakLevel.AddString(sLowest);
            m_cBreakLevel.SetItemData(0, static_cast<DWORD_PTR>(-1));
            m_cBreakLevel.AddString(_T("TOTAL"));
            m_cBreakLevel.SetItemData(1,0);

            for (int iIndex = 0 ; iIndex < iNumAreas; iIndex++) {
                m_cBreakLevel.AddString(m_pConsolidate->GetArea(iIndex));
                m_cBreakLevel.SetItemData(iIndex+2,iIndex+1);
            }
        }
    }
    m_cBreakLevel.SetCurSel(m_iBreakLevel + 1);

    UpdateSpecialValuesCheckBoxes();
    if(m_iCurSubTable ==0 && m_cSubTables.GetCount()> 1){
        //Disable looping var
        m_cUnitName.SetCurSel(-1);
        if(!m_sCommonUnitName.IsEmpty()){
            m_cUnitName.InsertString(0,m_sCommonUnitName);
            m_cUnitName.SetCurSel(0);
        }
        m_cUnitName.EnableWindow(FALSE);
    }
    else {
        m_cUnitName.EnableWindow(TRUE);
    }
    //Doing post Calc
    m_sPostCalc = MakeStringFromTabLogicArray(m_arrPostCalc);
    UpdateData(FALSE);
}

void CTblTallyFmtDlg::UpdateUnitsCombo()
{
    if(m_arrUnitNames.GetSize() ==0){
        return;
    }
    int iSelSubtable = m_cSubTables.GetCurSel();
    ASSERT(iSelSubtable < m_arrUnitNames.GetSize());

    CStringArray& asSubTableUnits = m_arrUnitNames.GetAt(iSelSubtable);

    m_cUnitName.ResetContent();

    if (asSubTableUnits.GetSize() > 0) {
        m_cUnitName.AddString(_T("Default (") + asSubTableUnits[0] + _T(")"));
    }
    for (int iIndex =0; iIndex < asSubTableUnits.GetSize(); iIndex++){
        m_cUnitName.AddString(asSubTableUnits[iIndex]);
    }

    CUnitSpec& unitSpec = m_aUnitSpec[iSelSubtable];
    if (unitSpec.GetUseDefaultLoopingVar()) {
        m_cUnitName.SetCurSel(0);
    }
    else {
        int iIndex =0;
        for (iIndex =0; iIndex < asSubTableUnits.GetSize(); iIndex++){
            if (unitSpec.GetLoopingVarName().CompareNoCase(asSubTableUnits[iIndex]) == 0) {
                m_cUnitName.SetCurSel(iIndex+1);
                break;
            }
        }
        if (iIndex == asSubTableUnits.GetSize()) {
            m_cUnitName.SetCurSel(-1);
            m_cUnitName.SetWindowText(unitSpec.GetLoopingVarName());
        }
    }

    // enable/disable other windows if curr subtable has no valid units
    // (should only occur for entire table subtables do not allow a single unit for table).
    if (asSubTableUnits.GetCount() == 0) {
        // no valid units - don't allow user to change
        ((CWnd*)GetDlgItem(IDC_UNIT))->EnableWindow(FALSE);
        ((CWnd*)GetDlgItem(IDC_WEIGHT))->EnableWindow(FALSE);
        ((CWnd*)GetDlgItem(IDC_UNIVSTRING))->EnableWindow(FALSE);
        ((CWnd*)GetDlgItem(IDC_UNIVERSE))->EnableWindow(FALSE);
        ((CWnd*)GetDlgItem(IDC_VALUE))->EnableWindow(FALSE);
        CUnitSpec* pUnitSpec=(CUnitSpec*) m_cSubTables.GetItemDataPtr(iSelSubtable);
        pUnitSpec->SetWeightExpr(_T(""));
        pUnitSpec->SetUniverse(_T(""));
        pUnitSpec->SetValue(_T(""));
    }
    else {
        ((CWnd*)GetDlgItem(IDC_UNIT))->EnableWindow(TRUE);
        ((CWnd*)GetDlgItem(IDC_WEIGHT))->EnableWindow(TRUE);
        ((CWnd*)GetDlgItem(IDC_UNIVSTRING))->EnableWindow(TRUE);
        ((CWnd*)GetDlgItem(IDC_UNIVERSE))->EnableWindow(TRUE);
        ((CWnd*)GetDlgItem(IDC_VALUE))->EnableWindow(TRUE);
    }
    if(m_iCurSubTable ==0 && m_cSubTables.GetCount()> 1){
        //Disable looping var
        m_cUnitName.SetCurSel(-1);
        if(!m_sCommonUnitName.IsEmpty()){
            m_cUnitName.InsertString(0,m_sCommonUnitName);
            m_cUnitName.SetCurSel(0);
        }
        m_cUnitName.EnableWindow(FALSE);
    }
    else {
        m_cUnitName.EnableWindow(TRUE);
    }
}

void CTblTallyFmtDlg::SetLoopingVar(CUnitSpec* pUnitSpec)
{
    int iSelLoopingVar = m_cUnitName.GetCurSel();
    if (iSelLoopingVar < 0) {
        /*// user typed in text instead of picking from list
        CIMSAString sString;
        ((CWnd*)GetDlgItem(IDC_UNIT))->GetWindowText(sString);
        sString.Trim();
        pUnitSpec->SetLoopingVarName(sString);
        pUnitSpec->SetUseDefaultLoopingVar(false);    */
    }
    else if (iSelLoopingVar == 0) {
        // user chose default (always first item)
        pUnitSpec->SetUseDefaultLoopingVar(true);
        const CString& sDefaultLoopingVar = m_arrUnitNames.GetAt(m_iCurSubTable).GetAt(0);
        pUnitSpec->SetLoopingVarName(sDefaultLoopingVar);
    }
    else {
        // chose other item from list
        pUnitSpec->SetUseDefaultLoopingVar(false);
        const CString& sLoopingVar = m_arrUnitNames.GetAt(m_iCurSubTable).GetAt(iSelLoopingVar-1);
        pUnitSpec->SetLoopingVarName(sLoopingVar);
    }
}

void CTblTallyFmtDlg::OnCbnSelchangeSubtables()
{
    const int iSelSubtable = m_cSubTables.GetCurSel();
    ASSERT(iSelSubtable < m_arrUnitNames.GetSize());

    if(iSelSubtable == m_iCurSubTable) {
        return; // stops potential inf recursion from call to SetCurSel when syntax check failes
    }
    CTabulateDoc* pDoc = m_pTabView->GetDocument();
    CTabSet* pTabSet = pDoc->GetTableSpec();
    int iNumTables =pTabSet->GetNumTables();
    if(iSelSubtable ==0 && iNumTables > 1){
        GetDlgItem(IDC_UNIV_APPLYALL)->EnableWindow(TRUE);
        GetDlgItem(IDC_WGHT_APPLYALL)->EnableWindow(TRUE);
    }
    else {
        GetDlgItem(IDC_UNIV_APPLYALL)->EnableWindow(FALSE);
        GetDlgItem(IDC_WGHT_APPLYALL)->EnableWindow(FALSE);
    }

    //Update the current stuff from the controls to unitspec
    CUnitSpec* pUnitSpec=(CUnitSpec*) m_cSubTables.GetItemDataPtr(m_iCurSubTable);
    //weight
    CIMSAString sString;
    ((CWnd*)GetDlgItem(IDC_WEIGHT))->GetWindowText(sString);
    sString.Trim();
    pUnitSpec->SetWeightExpr(sString);

    //universe
    sString = WS2CS(m_editUniv.GetText());
    sString.Trim();
    pUnitSpec->SetUniverse(sString);

    //value
    ((CWnd*)GetDlgItem(IDC_VALUE))->GetWindowText(sString);
    sString.Trim();
    pUnitSpec->SetValue(sString);

    // looping var
    SetLoopingVar(pUnitSpec);

    //TabLogic
    sString = WS2CS(m_editTabLogic.GetText());
    CStringArray& arrTabLogic = pUnitSpec->GetTabLogicArray();
    arrTabLogic.RemoveAll();
    MakeTabLogicArray(sString,arrTabLogic);

    if (!CheckSyntaxAll()){
        m_cSubTables.SetCurSel(m_iCurSubTable); // bad syntax - don't let them change subtable
        return;
    }

    m_iCurSubTable = m_cSubTables.GetCurSel();

    //Update the current stuff from the newly selected unitspec to the controls
    pUnitSpec=(CUnitSpec*) m_cSubTables.GetItemDataPtr(iSelSubtable);

    UpdateUnitsCombo();

    //weight
    ((CWnd*)GetDlgItem(IDC_WEIGHT))->SetWindowText(pUnitSpec->GetWeightExpr());
     m_editUniv.SetText(pUnitSpec->GetUniverse());
    ((CWnd*)GetDlgItem(IDC_VALUE))->SetWindowText(pUnitSpec->GetValue());
    m_editTabLogic.SetText(MakeStringFromTabLogicArray(pUnitSpec->GetTabLogicArray()));
    if(m_cSubTables.GetCurSel() > 0){
        SetWindowText(_T("Tally Attributes (Subtable)"));
    }
    else {
        SetWindowText(_T("Tally Attributes (Table)"));
    }

}


void CTblTallyFmtDlg::OnCbnSelendokSubtables()
{

}

void CTblTallyFmtDlg::OnCbnSelchangeBreaklevel()
{
    // TODO: Add your control notification handler code here
}

bool CTblTallyFmtDlg::CheckSyntax(int iSubtable, XTABSTMENT_TYPE eStatementType)
{
    bool bRet = false;
    CTabulateDoc* pDoc = m_pTabView->GetDocument();
    pDoc->SetErrorString();
    CTable* pTable = m_pTabView->GetGrid()->GetTable();
    //Store current table units in temp
    CArray<CUnitSpec, CUnitSpec&>  arrTempUnitSpec;
    arrTempUnitSpec.Append(pTable->GetUnitSpecArr());
    CUnitSpec tempTblUnitSpec = pTable->GetTableUnit();

    CStringArray arrTempPostCalc;
    arrTempPostCalc.Append(pTable->GetPostCalcLogic());
    CUnitSpec* pUnitSpec = (CUnitSpec*)m_cSubTables.GetItemDataPtr(iSubtable);
    ASSERT(pUnitSpec);
    CUnitSpec* pTableUnitSpec = (CUnitSpec*)m_cSubTables.GetItemDataPtr(0);
    ASSERT(pUnitSpec);

    CUnitSpec unitSpec = *pUnitSpec;

    SetLoopingVar(&unitSpec);

    if(eStatementType == XTABSTMENT_UNIV_ONLY){
        CIMSAString sUnivString = WS2CS(m_editUniv.GetText());
        sUnivString.Trim();

        unitSpec.SetUniverse(sUnivString);
        unitSpec.SetWeightExpr(_T(""));
        unitSpec.SetValue(_T(""));

        if(sUnivString.IsEmpty()){
            return true;
        }
    }
    else if(eStatementType == XTABSTMENT_WGHT_ONLY){
        unitSpec.SetUniverse(_T(""));

        CIMSAString sWeight,sValue;
        ((CWnd*)GetDlgItem(IDC_WEIGHT))->GetWindowText(sWeight);
        sWeight.Trim();
        unitSpec.SetWeightExpr(sWeight);

        //value
        ((CWnd*)GetDlgItem(IDC_VALUE))->GetWindowText(sValue);
        sValue.Trim();
        unitSpec.SetValue(sValue);

        if(sWeight.IsEmpty() && sValue.IsEmpty()){
            return true;
        }
    }
    else if(eStatementType == XTABSTMENT_TABLOGIC_ONLY){
        unitSpec.SetUniverse(_T(""));
        unitSpec.SetWeightExpr(_T(""));
        unitSpec.SetValue(_T(""));

        CIMSAString sTabLogic = WS2CS(m_editTabLogic.GetText());
        sTabLogic.Trim();

        CStringArray& arrTabLogic  = unitSpec.GetTabLogicArray();
        arrTabLogic.RemoveAll();
        if(sTabLogic.IsEmpty()){
            return true;
        }
        MakeTabLogicArray(sTabLogic,arrTabLogic);
    }
    else if(eStatementType == XTABSTMENT_POSTCALC_ONLY){
        unitSpec.SetUniverse(_T(""));
        unitSpec.SetWeightExpr(_T(""));
        unitSpec.SetValue(_T(""));
        unitSpec.GetTabLogicArray().RemoveAll();

        //value
        CIMSAString sPostCalc = WS2CS(m_editPostCalc.GetText());
        sPostCalc.Trim();
        pTable->GetPostCalcLogic().RemoveAll();
        if(sPostCalc.IsEmpty()){
            return true;
        }
        MakeTabLogicArray(sPostCalc,pTable->GetPostCalcLogic());

    }
   else if(eStatementType == XTABSTMENT_UNITS_ONLY){
        unitSpec.SetUniverse(_T(""));
        unitSpec.SetWeightExpr(_T(""));
        unitSpec.SetValue(_T(""));
    }
    else {
        ASSERT(FALSE);
    }

    //use the dialog's unit specs for compilation
    pTable->GetUnitSpecArr().RemoveAll();

    if (iSubtable == 0) {
       // entire table
       pTable->GetTableUnit() = unitSpec;
    }
    else {
        // current subtable
        pTable->GetUnitSpecArr().Add(unitSpec);
        pTable->GetTableUnit() = *pTableUnitSpec; // use tbl unit spec from dlg
    }

    //compile the table with new unit stuff
    //Make the crosstab statement with just the universe .
    //Send for compile .If the compile fails . Bring the dialog back ?
    //Follow the flow in OnTblCompile;
    TableElementTreeNode table_element_tree_node(pDoc, pTable);

    if(AfxGetMainWnd()->SendMessage(UWM::Table::CheckSyntax, static_cast<WPARAM>(eStatementType), reinterpret_cast<LPARAM>(&table_element_tree_node)) == -1){
//        AfxMessageBox("Invalid Universe Syntax");
        bRet = false;
    }
    else {
        bRet = true;
    }
    //reset the unit stuff of table back to its original unit specs
    pTable->GetUnitSpecArr().RemoveAll();
    pTable->GetUnitSpecArr().Append(arrTempUnitSpec);
    arrTempUnitSpec.RemoveAll();
    pTable->GetTableUnit() = tempTblUnitSpec;
    pTable->GetPostCalcLogic().RemoveAll();
    pTable->GetPostCalcLogic().Append(arrTempPostCalc);
    return bRet;
}

bool CTblTallyFmtDlg::CheckUniverseSyntax(const CString& sUniverseStatement)
{
    CTabulateDoc* pDoc = m_pTabView->GetDocument();
    CString sOldUniv = WS2CS(m_editUniv.GetText());
    m_editUniv.SetText(sUniverseStatement);
    bool bRet = CheckSyntax(m_iCurSubTable, XTABSTMENT_UNIV_ONLY);
    m_editUniv.SetText(sOldUniv);
    if (!bRet) {
        CIMSAString sMsg = _T("Invalid Universe Syntax \r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
    }
    return bRet;
}

afx_msg void CTblTallyFmtDlg::OnBnClickedUseCustSpecVal()
{
    UpdateSpecialValuesCheckBoxes();
}

void CTblTallyFmtDlg::UpdateSpecialValuesCheckBoxes()
{
    BOOL bEnable = ((CButton*) GetDlgItem(IDC_USE_CUSTOM_SPECIAL_VALUES))->GetCheck();
    GetDlgItem(IDC_SPECIAL_VAL_NOTAPPL)->EnableWindow(bEnable);
    GetDlgItem(IDC_SPECIAL_VAL_MISSING)->EnableWindow(bEnable);
    GetDlgItem(IDC_SPECIAL_VAL_REFUSED)->EnableWindow(bEnable);
    GetDlgItem(IDC_SPECIAL_VAL_DEFAULT)->EnableWindow(bEnable);
    GetDlgItem(IDC_SPECIAL_VAL_UNDEFINED)->EnableWindow(bEnable);
}



void CTblTallyFmtDlg::OnBnClickedWghtApplyall()
{
    if (AfxMessageBox(_T("The Weight will be applied to all the Tables.\n\nDo you want to continue?"), MB_YESNO) != IDYES) {
        return;
    }

    CTabulateDoc* pDoc = m_pTabView->GetDocument();

    //First compile weight and for the entire table . If it compiles apply
    //to all tables
    bool bRet = CheckSyntax(0, XTABSTMENT_WGHT_ONLY);
    if(!bRet) {
        CIMSAString sMsg = _T("Invalid Weight - unable to apply to all Tables \r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
        return;
    }
    else {
        CIMSAString sWeight;
        bool bApplied = false;
        GetDlgItem(IDC_WEIGHT)->GetWindowText(sWeight);
        CTabSet* pTabSet = pDoc->GetTableSpec();
        for(int iIndex = 0; iIndex < pTabSet->GetNumTables(); iIndex++){
            CTable* pTable = pTabSet->GetTable(iIndex);
            if(pTable == m_pTabView->GetGrid()->GetTable()){
                continue;
            }
            else {
                pTable->GetTableUnit().SetWeightExpr(sWeight);
                bApplied = true;
            }
        }
//         if(bApplied){
//             AfxMessageBox("Applied universe to all tables");
//         }
    }
}

void CTblTallyFmtDlg::OnBnClickedUnivApplyall()
{
    if (AfxMessageBox(_T("The Universe will be applied to all the Tables.\n\nDo you want to continue?"), MB_YESNO) != IDYES) {
        return;
    }

    CTabulateDoc* pDoc = m_pTabView->GetDocument();

    //First compile univ and for the entire table . If it compiles apply
    //to all tables
    bool bRet = CheckSyntax(0, XTABSTMENT_UNIV_ONLY);
    if(!bRet) {
        CIMSAString sMsg = _T("Invalid Universe - unable to apply to all Tables \r\n");
        sMsg += pDoc->GetErrorString();
        AfxMessageBox(sMsg);
        return;
    }
    else {
         CString sUniverse = WS2CS(m_editUniv.GetText());
         CTabSet* pTabSet = pDoc->GetTableSpec();
         bool bApplied = false;
         for(int iIndex = 0; iIndex < pTabSet->GetNumTables(); iIndex++){
             CTable* pTable = pTabSet->GetTable(iIndex);
             if(pTable == m_pTabView->GetGrid()->GetTable()){
                 continue;
             }
             else {
                 pTable->GetTableUnit().SetUniverse(sUniverse);
                 bApplied = true;
             }
         }
//         if(bApplied){
//             AfxMessageBox("Applied universe to all tables");
//         }
    }
}

void CTblTallyFmtDlg::MakeTabLogicArray(CIMSAString sTabLogic , CStringArray& arrTabLogic)
{
    if(sTabLogic.IsEmpty()){
        return;
    }
    int p = sTabLogic.Find(_T("\r\n"));
    if(p == -1 ) {
        arrTabLogic.Add(sTabLogic);
    }
    int iStart =0;
    while (p >= 0) {
        CIMSAString sLine;
        sLine = sTabLogic.Mid(iStart,p-iStart+1);
        sLine.Trim(_T("\r\n"));
        arrTabLogic.Add(sLine);
        p= p+2;
        iStart = p;
        p = sTabLogic.Find(_T("\r\n"),p);
        if(p ==-1){
            arrTabLogic.Add(sTabLogic.Mid(iStart));
            break;
        }
    }
}

CIMSAString CTblTallyFmtDlg::MakeStringFromTabLogicArray(CStringArray& arrTabLogic)
{
    CIMSAString sRet;
    for(int iLine =0; iLine < arrTabLogic.GetSize();iLine++){
        sRet +=arrTabLogic[iLine];
        if(iLine != arrTabLogic.GetSize()-1){
            sRet += _T("\r\n");
        }
    }
    return sRet;
}
void CTblTallyFmtDlg::OnBnClickedBtnTablogic()
{
    CEdtLogicDlg dlg;
    dlg.m_bIsPostCalc = false;    // BMD 05 Jun 2006
    CIMSAString sTemp ;
    m_editTabLogic.GetWindowText(sTemp);
    CStringArray arrTabLogic;
    MakeTabLogicArray(sTemp,arrTabLogic);
    sTemp = MakeStringFromTabLogicArray(arrTabLogic);
    dlg.m_sLogic =sTemp;
    if(dlg.DoModal() == IDOK){
        m_editTabLogic.SetText(dlg.m_sLogic);
    }
}

void CTblTallyFmtDlg::OnBnClickedBtnPostcalc()
{
    CEdtLogicDlg dlg;
    dlg.m_bIsPostCalc = true;    // BMD 05 Jun 2006
    m_editPostCalc.GetWindowText(dlg.m_sLogic);
    if(dlg.DoModal() == IDOK){
        m_editPostCalc.SetText(dlg.m_sLogic);
    }
}

void CTblTallyFmtDlg::OnBnClickedApplyallUnit()
{
    //If the units intersection is null Then just put up a message box
    CStringArray arrIntersectionUnitNames;
    arrIntersectionUnitNames.Append(m_arrUnitNames[0]) ;
    if(arrIntersectionUnitNames.GetSize() == 0){
        AfxMessageBox(_T("No units available to apply to all subtables"));
        return;
    }
    else {

        arrIntersectionUnitNames.InsertAt(0,_T("Select Unit -To Apply To All Subtables"));
        CTblUnitDlg tblUnitDlg;
        tblUnitDlg.m_arrUnitNames.Append(arrIntersectionUnitNames);
        if(tblUnitDlg.DoModal() == IDOK) {
            for(int iUnit = 1; iUnit < m_aUnitSpec.GetSize() ; iUnit++) {
                CUnitSpec& unitSpec = m_aUnitSpec[iUnit];
                unitSpec.SetUseDefaultLoopingVar(false);
                unitSpec.SetLoopingVarName(tblUnitDlg.m_sUnitName);
            }
            CIMSAString sCurUnitName;
            m_cUnitName.GetWindowText(sCurUnitName);
            if(m_cSubTables.GetCurSel() != 0 && sCurUnitName.CompareNoCase(tblUnitDlg.m_sUnitName) != 0){
                CIMSAString sText;
                for(int iIndex =0; iIndex < m_cUnitName.GetCount(); iIndex++){
                    m_cUnitName.GetLBText(iIndex,sText);
                    if(sText.CompareNoCase(tblUnitDlg.m_sUnitName) ==0){
                        m_cUnitName.SetCurSel(iIndex);
                        break;
                    }
                }
            }
            m_sCommonUnitName = tblUnitDlg.m_sUnitName;
            if(m_iCurSubTable ==0 && m_cSubTables.GetCount()> 1){
                //Disable looping var
                m_cUnitName.SetCurSel(-1);
                if(!m_sCommonUnitName.IsEmpty()){
                    m_cUnitName.InsertString(0,m_sCommonUnitName);
                    m_cUnitName.SetCurSel(0);
                }
                m_cUnitName.EnableWindow(FALSE);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
// CTblTallyFmtDlg::PreTranslateMessage
//
////////////////////////////////////////////////////////////////////////////

BOOL CTblTallyFmtDlg::PreTranslateMessage(MSG* pMsg)
{
    // pass CTRL-T on main window to toggle names in dictionary tree
    if (pMsg->message == WM_KEYDOWN) {

        bool bCtrl = GetKeyState(VK_CONTROL) < 0;
        bool bT = (pMsg->wParam == _T('t') || pMsg->wParam == _T('T'));

        if (bCtrl && bT) {
            CFrameWnd* pMainWnd = (CFrameWnd*) GetParent();
            pMainWnd->SendMessage(WM_COMMAND, ID_VIEW_NAMES);
            return TRUE;
        }
    }

    return CDialog::PreTranslateMessage(pMsg);
}


