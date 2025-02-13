#include "StdAfx.h"
#include "GenerateVSDlg.h"


IMPLEMENT_DYNAMIC(GenerateVSDlg, CDialog)


BEGIN_MESSAGE_MAP(GenerateVSDlg, CDialog)
    ON_EN_KILLFOCUS(IDC_GET_VSET_INTERVAL, OnEnKillfocusGetVsetInterval)
    ON_EN_KILLFOCUS(IDC_GEN_VSET_NAME, OnEnKillfocusGenVsetName)
    ON_EN_KILLFOCUS(IDC_GEN_VSET_FROM, OnEnKillfocusGenVsetFrom)
    ON_EN_KILLFOCUS(IDC_GEN_VSET_TO, OnEnKillfocusGenVsetTo)
END_MESSAGE_MAP()


GenerateVSDlg::GenerateVSDlg(CWnd* pParent /* = nullptr*/)
    :   CDialog(IDD_GENERATE_VSET, pParent),
        m_dFrom(0),
        m_dTo(0),
        m_dInterval(0),
        m_bUseThousandsSeparator(TRUE)
{
}


void GenerateVSDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_GEN_VSET_LABEL, m_sLabel);
    DDV_MaxChars(pDX, m_sLabel, MAX_LABEL_LEN);
    DDX_Text(pDX, IDC_GEN_VSET_NAME, m_sName);
    DDX_Text(pDX, IDC_GEN_VSET_FROM, m_dFrom);
    DDX_Text(pDX, IDC_GEN_VSET_TO, m_dTo);
    DDX_Text(pDX, IDC_GET_VSET_INTERVAL, m_dInterval);
    DDX_Text(pDX, IDC_GEN_VSET_TEMPLATE, m_sTemplate);
    DDX_Check(pDX, IDC_GEN_VSET_USE_THOUSANDS_SEPARATOR, m_bUseThousandsSeparator);
}


BOOL GenerateVSDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Calculate Min and Max values and Min Interval
    UINT uLen = m_pItem->GetLen();
    if (m_pItem->GetDecimal() > 0 && !m_pItem->GetDecChar()) {
        uLen++;
    }
    CIMSAString sMinVal = CString(_T('9'),uLen);
    sMinVal.SetAt(0,'-');
    CIMSAString sMaxVal = CString(_T('9'),uLen);
    if (m_pItem->GetDecimal() > 0) {
        sMinVal.SetAt(uLen - m_pItem->GetDecimal() - 1,'.');
        sMaxVal.SetAt(uLen - m_pItem->GetDecimal() - 1,'.');
    }
    m_dMinVal = atod(sMinVal);
    m_dMaxVal = atod(sMaxVal);
    if (m_pItem->GetDecimal() > 0) {
        m_dMinInterval = pow(10.0,-((double) m_pItem->GetDecimal()));
    }
    else {
        m_dMinInterval = 1.0;
    }

    // Initialize Label and Name
    m_sName.Format(_T("%s_VS%d"), (LPCTSTR)m_pItem->GetName(), (int)m_pItem->GetNumValueSets() + 1);
    m_sLabel = m_pItem->GetLabel();

    m_sName = m_pDoc->GetDict()->GetUniqueName(m_sName);
    // Initialize From, To, and Interval
    m_dFrom = 0.0;
    m_dTo = m_dMaxVal;

    int dTemp = m_pItem->GetLen() - m_pItem->GetDecimal();
    m_dInterval = pow(10.0,(double) dTemp - 1);

    // Initialize Template
    if (m_dInterval == m_dMinInterval) {
        m_sTemplate = _T("%s");
    }
    else {
        m_sTemplate = _T("%s - %s");
    }
    UpdateData(FALSE);

    TCHAR sThousandSep[30];
    GetPrivateProfileString(_T("intl"), _T("sThousand"), _T(""), sThousandSep, 30, _T("WIN.INI"));
    CString sThousandSeparatorCheckboxLabel = FormatText(_T("Use 1000 separator (%s)"), sThousandSep);
    GetDlgItem(IDC_GEN_VSET_USE_THOUSANDS_SEPARATOR)->SetWindowText(sThousandSeparatorCheckboxLabel);

    return TRUE;  // return TRUE unless you set the focus to a control
}


void GenerateVSDlg::OnEnKillfocusGenVsetName()
{
    UpdateData(TRUE);
    if (SO::IsBlank(m_sName)) {
        AfxMessageBox(_T("A name must be given."));
    }
    m_sName.MakeName();
    m_sName = m_pDoc->GetDict()->GetUniqueName(m_sName);
    UpdateData(FALSE);
}


void GenerateVSDlg::OnEnKillfocusGenVsetFrom()
{
    UpdateData(TRUE);
    bool bError = false;
    // Normalize from value
    TCHAR pszTemp[30];
    UINT uDec = m_pItem->GetDecimal();
    UINT uLen = m_pItem->GetLen();
    if (uDec > 0 && !m_pItem->GetDecChar()) {
        uLen++;
    }
    CIMSAString sFrom = dtoa(m_dFrom, pszTemp, m_pItem->GetDecimal(), DOT, false);
    m_dFrom = atod(sFrom);
    // Check size of from value
    if ((UINT) sFrom.GetLength() > uLen) {
        AfxMessageBox(_T("From value is too large."));
        if (m_dFrom < 0.0) {
            m_dFrom = m_dMinVal;
        }
        else {
            m_dFrom = 0.0;
        }
        bError = true;
    }
    // Check if from less than to
    if (m_dFrom >= m_dTo) {
        AfxMessageBox(_T("From value must be less than To value."));
        if (m_dTo > 0.0) {
            m_dFrom = 0.0;
        }
        else {
            m_dFrom = m_dMinVal;
        }
        bError = true;
    }
    UpdateData(FALSE);
    if (bError) {
        CWnd* pWnd = GetDlgItem(IDC_GEN_VSET_FROM);
        pWnd->SetFocus();
    }
}


void GenerateVSDlg::OnEnKillfocusGenVsetTo()
{
    UpdateData(TRUE);
    bool bError = false;
    // Normalize to value
    TCHAR pszTemp[30];
    UINT uDec = m_pItem->GetDecimal();
    UINT uLen = m_pItem->GetLen();
    if (uDec > 0 && !m_pItem->GetDecChar()) {
        uLen++;
    }
    CIMSAString sTo = dtoa(m_dTo, pszTemp, m_pItem->GetDecimal(), DOT, false);
    m_dTo = atod(sTo);
    if ((UINT) sTo.GetLength() > uLen) {
        AfxMessageBox(_T("From value is too large."));
        m_dTo = m_dMaxVal;
        bError = true;
    }
    // Check if to is greater than from
    if (m_dTo <= m_dFrom) {
        AfxMessageBox(_T("To value must be greater than From value."));
        m_dTo = m_dMaxVal;
        bError = true;
    }
    UpdateData(FALSE);
    if (bError) {
        CWnd* pWnd = GetDlgItem(IDC_GEN_VSET_TO);
        pWnd->SetFocus();
    }
}


void GenerateVSDlg::OnEnKillfocusGetVsetInterval()
{
    UpdateData(TRUE);
    bool bError = false;
    // Normalize interval value
    TCHAR pszTemp[30];
    UINT uDec = m_pItem->GetDecimal();
    UINT uLen = m_pItem->GetLen();
    if (uDec > 0 && !m_pItem->GetDecChar()) {
        uLen++;
    }
    CIMSAString sInterval = dtoa(m_dInterval, pszTemp, m_pItem->GetDecimal(), DOT, false);
    m_dInterval = atod(sInterval);
    if (m_dInterval > m_dTo - m_dFrom + m_dMinInterval) {
        AfxMessageBox(_T("Interval is too large."));
        if (m_pItem->GetLen() < 4) {
            m_dInterval = 1.0;
        }
        else {
            m_dInterval = pow(10.0,(double) m_pItem->GetLen() - 3);
        }
        bError = true;
    }
    // Interval must be positive
    if (m_dInterval <= 0.0) {
        AfxMessageBox(_T("Interval must be greater than 0."));
        m_dInterval = 1.0;
        bError = true;
    }
    // Cannot be more than 9999 intervals
    if ((m_dTo - m_dFrom) / m_dInterval > 9999.0) {
        AfxMessageBox(_T("Too many intervals (more than 10000)."));
        if (m_pItem->GetLen() < 4) {
            m_dInterval = 1.0;
        }
        else {
            m_dInterval = pow(10.0,(double) m_pItem->GetLen() - 3);
        }
        bError = true;
    }
    if (m_dInterval == m_dMinInterval) {
        if (m_sTemplate == _T("%s - %s")) {
            m_sTemplate = _T("%s");
        }
    }
    else {
        if (m_sTemplate == _T("%s")) {
            m_sTemplate = _T("%s - %s");
        }
    }
    UpdateData(FALSE);
    if (bError) {
        CWnd* pWnd = GetDlgItem(IDC_GET_VSET_INTERVAL);
        pWnd->SetFocus();
    }
}


void GenerateVSDlg::OnOK()
{
    UpdateData(TRUE);
    if (SO::IsBlank(m_sLabel)) {
        AfxMessageBox(_T("Value Set label must be given."));
        return;
    }
    if (SO::IsBlank(m_sTemplate)) {
        AfxMessageBox(_T("Template must be given."));
        return;
    }
    CDialog::OnOK();
}
