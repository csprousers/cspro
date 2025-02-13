// TlyVrDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "TlyVrDlg.h"
#include <zTableO/TllyStat.h>
#include <zUtilO/ArrUtil.h>

// CTallyVarDlg dialog

IMPLEMENT_DYNAMIC(CTallyVarDlg, CDialog)
CTallyVarDlg::CTallyVarDlg(CWnd* pParent /*=NULL*/)
    : m_pFmtSelected(NULL),
      m_bDisablePct(FALSE),
      CDialog(CTallyVarDlg::IDD, pParent)
{
}

CTallyVarDlg::~CTallyVarDlg()
{
}

void CTallyVarDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_AVAILABLE, m_listAvailable);
    DDX_Control(pDX, IDC_LIST_SELECTED, m_listSelected);

    if (pDX->m_bSaveAndValidate) {
        // create map from new stat pos to original stat pos
        const int iNumStats = m_pFmtSelected->GetStats().GetCount();
        m_aMapStatNewToOldPos.SetSize(iNumStats);
        for (int i = 0; i < iNumStats; ++i) {
            m_aMapStatNewToOldPos[i] = -1;
        }
        m_bOrderChanged = false;
        for (int i = 0; i < iNumStats; ++i) {
            m_aMapStatNewToOldPos[i] = FindInArray(m_aOrigStatPos, m_pFmtSelected->GetStats().GetAt(i));
            if (m_aMapStatNewToOldPos[i] != i) {
                m_bOrderChanged = true;
            }
        }
    }
}

BOOL CTallyVarDlg::OnInitDialog( )
{
    // Execute base class
    BOOL bRetVal = CDialog::OnInitDialog();
    if (!bRetVal) {
        return bRetVal;
    }

    m_listAvailable.AddString(_T("Counts"));
    m_listAvailable.AddString(_T("Total"));
    if (!m_bDisablePct) {
        m_listAvailable.AddString(_T("Percents"));
        m_listAvailable.AddString(_T("Total Percent"));
    }
    m_listAvailable.AddString(_T("Mean"));
    m_listAvailable.AddString(_T("Median"));
    m_listAvailable.AddString(_T("Mode"));
    m_listAvailable.AddString(_T("Standard Deviation"));
    m_listAvailable.AddString(_T("Variance"));
    m_listAvailable.AddString(_T("N-tiles"));
    m_listAvailable.AddString(_T("Minimum"));
    m_listAvailable.AddString(_T("Maximum"));
    m_listAvailable.AddString(_T("Proportion"));

    // copy stats from fmt to m_listSelected
    UpdateListSelected();

    // store off orig positions of stats so we can create new to old map later on
    for (int i = 0; i < m_pFmtSelected->GetStats().GetCount(); ++i) {
        m_aOrigStatPos.Add(m_pFmtSelected->GetStats().GetAt(i));
    }

    SetWindowText(m_sTitle);

    UpdateEnableDisable();

    return TRUE;
}

void CTallyVarDlg::OnOK( )
{
    if( !m_listSelected.GetCount() ) // GHM 20110914 tom-reported bug
    {
        AfxMessageBox(_T("You must select at least one tally attribute."));
        return;
    }

    bool bInvalidInterleave = false;
    for (int iPct = 0; iPct < m_pFmtSelected->GetStats().GetCount();++iPct) {

        CTallyVarStatFmt* pStat = m_pFmtSelected->GetStats().GetAt(iPct);
        if (!_tcscmp(pStat->GetType(), _T("Percents"))) {
            CTallyVarStatFmtPercent* pStatPercents =
                static_cast<CTallyVarStatFmtPercent*>(pStat);
            ASSERT(pStatPercents);
            if (pStatPercents->GetInterleaved()) {

                // only allow interleaved percents if there are counts above or below us in list
                if (iPct > 0 && !_tcscmp(m_pFmtSelected->GetStats().GetAt(iPct-1)->GetType(), _T("Counts"))) {
                    // counts before - interleave ok
                }
                else if (iPct < m_pFmtSelected->GetStats().GetCount()-1
                        && !_tcscmp(m_pFmtSelected->GetStats().GetAt(iPct+1)->GetType(), _T("Counts"))) {
                    // counts after - interleave ok
                }
                else {
                    // no counts before or after - not ok
                    bInvalidInterleave = true;
                }
            }
        }
    }
    if (bInvalidInterleave) {
        AfxMessageBox(IDS_NEED_CNTS_FOR_INTERLEAVE);
        return;
    }
    CDialog::OnOK();
}

BEGIN_MESSAGE_MAP(CTallyVarDlg, CDialog)
    ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
    ON_BN_CLICKED(IDC_REMOVE, OnBnClickedRemove)
    ON_LBN_SELCHANGE(IDC_LIST_AVAILABLE, OnLbnSelchangeListAvailable)
    ON_LBN_SELCHANGE(IDC_LIST_SELECTED, OnLbnSelchangeListSelected)
    ON_LBN_SETFOCUS(IDC_LIST_AVAILABLE, OnLbnSetfocusListAvailable)
    ON_LBN_SETFOCUS(IDC_LIST_SELECTED, OnLbnSetfocusListSelected)
    ON_BN_CLICKED(IDC_UP, OnBnClickedUp)
    ON_BN_CLICKED(IDC_DOWN, OnBnClickedDown)
    ON_BN_CLICKED(IDC_OPTIONS, OnBnClickedOptions)
    ON_BN_CLICKED(IDC_RESET, OnBnClickedReset)
END_MESSAGE_MAP()


// CTallyVarDlg message handlers


void CTallyVarDlg::OnBnClickedAdd()
{
    int nCount = m_listAvailable.GetSelCount();
    if (nCount > 0) {
        CArray<int> aSel;
        aSel.SetSize(nCount);
        m_listAvailable.GetSelItems(nCount, aSel.GetData());

        bool bAddedPercent = false;
        bool bAddedTotalPercent = false;
        for (int i = 0; i < nCount; ++i) {
            int iSel = aSel.GetAt(i);
            CIMSAString sLabel;
            m_listAvailable.GetText(iSel, sLabel);
            CTallyVarStatFmt* pStat = CTallyVarStatFmtFactory::GetInstance()->Create(sLabel, m_dVarMinDefault, m_dVarMaxDefault);
            m_pFmtSelected->AddStat(pStat);
            if (!_tcscmp(pStat->GetType(), _T("Percents"))) {
                bAddedPercent = true;
                // set pct type to row for row var, col for col var
                CTallyVarStatFmtPercent* pStatPercents = static_cast<CTallyVarStatFmtPercent*>(pStat);
                pStatPercents->SetPctType(m_pFmtSelected->GetID() == FMT_ID_TALLY_ROW ? PT_ROW : PT_COL);

            }
            else if (!_tcscmp(pStat->GetType(), _T("Total Percent"))) {
                bAddedTotalPercent = true;
                // by default pct type is "same as percent" but if there is no pct, determine based on
                // whether variable is in row or col
                bool bHasPercent = bAddedPercent || (m_pFmtSelected->FindFirstStatPos(_T("Percents")) != NONE);
                if (!bHasPercent) {
                    // make sure we aren't add percents later on in the loop
                    for (int j = i; j < nCount; ++j) {
                        CIMSAString sLab;
                        m_listAvailable.GetText(aSel.GetAt(j), sLab);
                        if (sLab == _T("Percents")) {
                            bHasPercent = true;
                        }
                    }
                }
                if (!bHasPercent) {
                    // set pct type to row for row var, col for col var
                    CTallyVarStatFmtTotalPercent* pStatTotalPercent = static_cast<CTallyVarStatFmtTotalPercent*>(pStat);
                    pStatTotalPercent->SetPctType(m_pFmtSelected->GetID() == FMT_ID_TALLY_ROW ? PT_ROW : PT_COL);
                    pStatTotalPercent->SetSameTypeAsPercents(false);
               }
            }
            else if (!_tcscmp(pStat->GetType(), _T("Proportion"))) {
                CTallyVarStatFmtProportion* pStatProportion = static_cast<CTallyVarStatFmtProportion*>(pStat);
                pStatProportion->SetRange(m_sDefaultPropRange);
            }
        }


        if (bAddedPercent) {
            PlacePercentsAndPercentTotal(bAddedTotalPercent);
        }
    }
    m_listSelected.ResetContent();
    UpdateListSelected();
}

// place percents next to counts if interleaved, add percent total if needed
void CTallyVarDlg::PlacePercentsAndPercentTotal(bool bAddedTotalPercent)
{
    // find out what stats are there and where they are
    int iCountsPos = NONE;
    int iTotalsPos = NONE;
    int iTotalPercentPos = NONE;
    int iPercentsPos = NONE;
    bool bTooManyTotalsOrCounts = false;
    for (int i = 0; i < m_pFmtSelected->GetStats().GetCount() && !bTooManyTotalsOrCounts; ++i) {
        CTallyVarStatFmt* pStat = m_pFmtSelected->GetStats().GetAt(i);
        if (!_tcscmp(pStat->GetType(), _T("Total"))) {
            if (iTotalsPos == NONE) {
                iTotalsPos = i;
            }
            else {
                bTooManyTotalsOrCounts = true;
            }
        }
        else if (!_tcscmp(pStat->GetType(), _T("Counts"))) {
            if (iCountsPos == NONE) {
                iCountsPos = i;
            }
            else {
                bTooManyTotalsOrCounts = true;
            }
        }
        else if (!_tcscmp(pStat->GetType(), _T("Percents"))) {
            iPercentsPos = i;
        }
        else if (!_tcscmp(pStat->GetType(), _T("Total Percent"))) {
            iTotalPercentPos = i;
         }
    }
    if (!bTooManyTotalsOrCounts && iCountsPos != NONE) {
        // have counts - are stats interleaved
        ASSERT(iPercentsPos != NONE);
        CTallyVarStatFmtPercent* pStatPercents =
            static_cast<CTallyVarStatFmtPercent*>(m_pFmtSelected->GetStats().GetAt(iPercentsPos));
        if (pStatPercents->GetInterleaved()) {
            // percents are interleaved - move percents to be under counts
            m_pFmtSelected->MoveStatTo(iPercentsPos, iCountsPos+1);

            // have interleaved percents and counts - check to see if we should add total percent
            if (iTotalPercentPos == NONE && iTotalsPos != NONE) {
                // have totals and no existing total percent so add in total percent

                // create new total percent with same settings as percent
                CTallyVarStatFmtTotalPercent* pStatTotalPercent =
                    static_cast<CTallyVarStatFmtTotalPercent*>(CTallyVarStatFmtFactory::GetInstance()->Create(_T("Total Percent"), m_dVarMinDefault, m_dVarMaxDefault));
                pStatTotalPercent->SetPctType(pStatPercents->GetPctType());
                m_pFmtSelected->AddStat(pStatTotalPercent);
                iTotalPercentPos = m_pFmtSelected->GetStats().GetCount() - 1;
                bAddedTotalPercent = true;
            }

            // if we add total percent, need to move it to be after total
            if (bAddedTotalPercent) {
                // need to update total pos since it may have shifted when we moved percents
                iTotalsPos = m_pFmtSelected->FindFirstStatPos(_T("Total"));
                ASSERT(iTotalsPos != NONE);

                // move percent totals to be after the totals
                m_pFmtSelected->MoveStatTo(iTotalPercentPos, iTotalsPos + 1);
            }

            m_listSelected.ResetContent();
            UpdateListSelected();
        }
    }
}

void CTallyVarDlg::UpdateListSelected()
{
    ASSERT(m_pFmtSelected);
    for (int i = 0; i < m_pFmtSelected->GetStats().GetCount(); ++i) {
        m_listSelected.AddString(m_pFmtSelected->GetStats().GetAt(i)->GetDisplayString());
    }
}

void CTallyVarDlg::OnBnClickedRemove()
{
    int nCount = m_listSelected.GetSelCount();
    if (nCount > 0) {
        CArray<int> aSel;
        aSel.SetSize(nCount);
        m_listSelected.GetSelItems(nCount, aSel.GetData());
        std::sort(aSel.GetData(), aSel.GetData() + aSel.GetSize());
        for (int i = nCount-1; i >= 0; --i) {
            int iSel = aSel.GetAt(i);
            m_pFmtSelected->RemoveStatAt(iSel);
            m_listSelected.DeleteString(iSel);
        }
    }
}


void CTallyVarDlg::OnBnClickedUp()
{
    int nCount = m_listSelected.GetSelCount();
    if (nCount == 1) {
        int iSel = -1;
        m_listSelected.GetSelItems(1,&iSel);
        if (iSel > 0) {
            CString sLabel;
            m_listSelected.GetText(iSel, sLabel);
            m_listSelected.DeleteString(iSel);
            int iNewPos = m_listSelected.InsertString(iSel-1, sLabel);
            m_listSelected.SetSel(iNewPos);
            CTallyVarStatFmt* pStat = m_pFmtSelected->GetStats().GetAt(iSel);
            m_pFmtSelected->MoveStatTo(iSel, iNewPos);
            UpdateEnableDisable();
        }
    }
}

void CTallyVarDlg::OnBnClickedDown()
{
    int nCount = m_listSelected.GetSelCount();
    if (nCount == 1) {
        int iSel = -1;
        m_listSelected.GetSelItems(1,&iSel);
        if (iSel < m_listSelected.GetCount() - 1) {
            CString sLabel;
            m_listSelected.GetText(iSel, sLabel);
            m_listSelected.DeleteString(iSel);
            int iNewPos = m_listSelected.InsertString(iSel+1, sLabel);
            m_listSelected.SetSel(iNewPos);
            CTallyVarStatFmt* pStat = m_pFmtSelected->GetStats().GetAt(iSel);
            m_pFmtSelected->MoveStatTo(iSel, iNewPos);
            UpdateEnableDisable();
        }
    }
}


void CTallyVarDlg::OnBnClickedReset()
{
    // copy default vals into current
    *m_pFmtSelected = *m_pDefaultFmt;

    // reset the list box
    m_listSelected.ResetContent();
    UpdateListSelected();
}

void CTallyVarDlg::OnLbnSelchangeListAvailable()
{
    UpdateEnableDisable();
}

void CTallyVarDlg::OnLbnSelchangeListSelected()
{
    UpdateEnableDisable();
}

void CTallyVarDlg::OnLbnSetfocusListAvailable()
{
    if (m_listSelected.GetCount() > 0) {
        m_listSelected.SelItemRange(FALSE, 0, m_listSelected.GetCount() - 1);
    }
    UpdateEnableDisable();
}

void CTallyVarDlg::OnLbnSetfocusListSelected()
{
    if (m_listAvailable.GetCount() > 0) {
        m_listAvailable.SelItemRange(FALSE, 0, m_listAvailable.GetCount() - 1);
    }
    UpdateEnableDisable();
}

void CTallyVarDlg::UpdateEnableDisable()
{
    if (m_listAvailable.GetSelCount() > 0) {
        GetDlgItem(IDC_ADD)->EnableWindow(TRUE);
    }
    else {
        GetDlgItem(IDC_ADD)->EnableWindow(FALSE);
    }

    if (m_listSelected.GetSelCount() > 0) {
        GetDlgItem(IDC_REMOVE)->EnableWindow(TRUE);
    }
    else {
        GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
    }

    if (m_listSelected.GetSelCount() == 1) {
        int iSel = -1;
        m_listSelected.GetSelItems(1,&iSel);
        GetDlgItem(IDC_UP)->EnableWindow(iSel > 0);
        GetDlgItem(IDC_DOWN)->EnableWindow(iSel < m_listSelected.GetCount() - 1);
        CTallyVarStatFmt* pStat = m_pFmtSelected->GetStats().GetAt(iSel);
        if (!_tcscmp(pStat->GetType(), _T("Percents")) || !_tcscmp(pStat->GetType(), _T("Total Percent")) ||
            !_tcscmp(pStat->GetType(), _T("Median")) || !_tcscmp(pStat->GetType(), _T("N-tiles")) ||
            !_tcscmp(pStat->GetType(), _T("Proportion"))) {
            GetDlgItem(IDC_OPTIONS)->EnableWindow(TRUE);
        }
        else {
            GetDlgItem(IDC_OPTIONS)->EnableWindow(FALSE);
        }
    }
    else {
        GetDlgItem(IDC_UP)->EnableWindow(FALSE);
        GetDlgItem(IDC_DOWN)->EnableWindow(FALSE);
        GetDlgItem(IDC_OPTIONS)->EnableWindow(FALSE);
    }
}

void CTallyVarDlg::OnBnClickedOptions()
{
    int nCount = m_listSelected.GetSelCount();
    if (nCount == 1) {
        int iSel = -1;
        m_listSelected.GetSelItems(1,&iSel);
        CTallyVarStatFmt* pStat = m_pFmtSelected->GetStats().GetAt(iSel);
        if (!_tcscmp(pStat->GetType(), _T("Percents"))) {
            OnPercentOptions(pStat);
        }
        else if (!_tcscmp(pStat->GetType(), _T("Total Percent"))) {
            OnTotalPercentOptions(pStat);
        }
        else if (!_tcscmp(pStat->GetType(), _T("Median"))) {
            OnMedianOptions(pStat);
        }
        else if (!_tcscmp(pStat->GetType(), _T("N-tiles"))) {
            OnNTilesOptions(pStat);
        }
        else if (!_tcscmp(pStat->GetType(), _T("Proportion"))) {
            OnProportionOptions(pStat);
        }
        m_listSelected.ResetContent();
       UpdateListSelected();
    }
}

void CTallyVarDlg::OnPercentOptions(CTallyVarStatFmt* pStat)
{
    ASSERT(!_tcscmp(pStat->GetType(), _T("Percents")));
    CTallyVarStatFmtPercent* pPctStat = static_cast<CTallyVarStatFmtPercent*>(pStat);

    CTallyVarPctDlg optDlg;
    optDlg.m_sPercentType = pPctStat->GetPctTypeString();
    optDlg.m_bInterleaved = pPctStat->GetInterleaved();

    if (optDlg.DoModal() == IDOK) {
        pPctStat->SetPctType(PctTypeFromString(optDlg.m_sPercentType));
        pPctStat->SetInterleaved(optDlg.m_bInterleaved ? TRUE : FALSE);
    }
}

void CTallyVarDlg::OnTotalPercentOptions(CTallyVarStatFmt* pStat)
{
    ASSERT(!_tcscmp(pStat->GetType(), _T("Total Percent")));
    CTallyVarStatFmtTotalPercent* pPctStat = static_cast<CTallyVarStatFmtTotalPercent*>(pStat);

    CTallyVarTotalPctDlg optDlg;
    if (pPctStat->GetSameTypeAsPercents()) {
        optDlg.m_sPercentType = _T("(same as percents)");
    }
    else {
        optDlg.m_sPercentType = pPctStat->GetPctTypeString();
    }

    // only allow same as percent if there is a percent on the variable
    optDlg.m_bAllowSameAsPercent = (m_pFmtSelected->FindFirstStatPos(_T("Percents")) != NONE);

    if (optDlg.DoModal() == IDOK) {
        if (optDlg.m_sPercentType == _T("(same as percents)")) {
            pPctStat->SetSameTypeAsPercents(true);
        }
        else {
            pPctStat->SetPctType(PctTypeFromString(optDlg.m_sPercentType));
            pPctStat->SetSameTypeAsPercents(false);
        }
    }
}


void CTallyVarDlg::OnNTilesOptions(CTallyVarStatFmt* pStat)
{
    ASSERT(!_tcscmp(pStat->GetType(), _T("N-tiles")));
    CTallyVarStatFmtNTiles* pNTilesStat = static_cast<CTallyVarStatFmtNTiles*>(pStat);

    CTallyVarNTilesDlg optDlg;
    optDlg.m_numTiles = pNTilesStat->GetNumTiles();
    optDlg.m_nUseValueSet = pNTilesStat->GetRangeProps().GetUseValueSet() ? BST_CHECKED : BST_UNCHECKED;

    CStatRangeProps& rangeProps = pNTilesStat->GetRangeProps();

    // check for invalid range props (happens with defaults since not set based on any particular
    // variable
    if (rangeProps.GetIntervalsMax() <= rangeProps.GetIntervalsMin()) {
        rangeProps.SetDefaults(m_dVarMinDefault, m_dVarMaxDefault);
    }

    optDlg.m_dMin = rangeProps.GetIntervalsMin();
    optDlg.m_dMax = rangeProps.GetIntervalsMax();
    optDlg.m_dInterval = rangeProps.GetIntervalsStep();

    optDlg.m_dMaxMax = m_dVarMax;
    optDlg.m_dMinMin = m_dVarMin;

    if (optDlg.DoModal() == IDOK) {
        pNTilesStat->SetNumTiles(optDlg.m_numTiles);
        rangeProps.SetUseValueSet(optDlg.m_nUseValueSet == BST_CHECKED);
        if (optDlg.m_nUseValueSet != BST_CHECKED) {
            rangeProps.SetIntervalsMin(optDlg.m_dMin);
            rangeProps.SetIntervalsMax(optDlg.m_dMax);
            rangeProps.SetIntervalsStep(optDlg.m_dInterval);
        }
    }
}


void CTallyVarDlg::OnMedianOptions(CTallyVarStatFmt* pStat)
{
    ASSERT(!_tcscmp(pStat->GetType(), _T("Median")));
    CTallyVarStatFmtMedian* pMedianStat = static_cast<CTallyVarStatFmtMedian*>(pStat);

    CTallyVarMedianDlg optDlg;
    optDlg.m_sType = pMedianStat->GetContinuous() ? _T("Continuous") : _T("Discrete");
    optDlg.m_nUseValueSet = pMedianStat->GetRangeProps().GetUseValueSet() ? BST_CHECKED : BST_UNCHECKED;

    CStatRangeProps& rangeProps = pMedianStat->GetRangeProps();
    // check for invalid range props (happens with defaults since not set based on any particular
    // variable
    if (rangeProps.GetIntervalsMax() <= rangeProps.GetIntervalsMin()) {
        rangeProps.SetDefaults(m_dVarMinDefault, m_dVarMaxDefault);
    }

    optDlg.m_dMin = rangeProps.GetIntervalsMin();
    optDlg.m_dMax = rangeProps.GetIntervalsMax();
    optDlg.m_dInterval = rangeProps.GetIntervalsStep();

    optDlg.m_dMaxMax = m_dVarMax;
    optDlg.m_dMinMin = m_dVarMin;

    if (optDlg.DoModal() == IDOK) {
        pMedianStat->SetContinuous(optDlg.m_sType == _T("Continuous"));
        rangeProps.SetUseValueSet(optDlg.m_nUseValueSet == BST_CHECKED);
        if (optDlg.m_nUseValueSet != BST_CHECKED) {
            rangeProps.SetIntervalsMin(optDlg.m_dMin);
            rangeProps.SetIntervalsMax(optDlg.m_dMax);
            rangeProps.SetIntervalsStep(optDlg.m_dInterval);
        }
    }
}


void CTallyVarDlg::OnProportionOptions(CTallyVarStatFmt* pStat)
{
    ASSERT(!_tcscmp(pStat->GetType(), _T("Proportion")));
    CTallyVarStatFmtProportion* pProportionStat = static_cast<CTallyVarStatFmtProportion*>(pStat);

    CTallyVarProportionDlg optDlg;
    switch (pProportionStat->GetProportionType()) {
        case CTallyVarStatFmtProportion::PRT_PERCENT:
            optDlg.m_iType = 0;
            break;
        case CTallyVarStatFmtProportion::PRT_PERCENT_AND_TOTAL:
            optDlg.m_iType = 1;
            break;
        case CTallyVarStatFmtProportion::PRT_RATIO:
            optDlg.m_iType = 2;
            break;
        case CTallyVarStatFmtProportion::PRT_RATIO_AND_TOTAL:
            optDlg.m_iType = 3;
            break;
        default:
            ASSERT(!_T("Invalid proportion type"));
    }
    optDlg.m_sRange = pProportionStat->GetRange();


    if (optDlg.DoModal() == IDOK) {
        switch (optDlg.m_iType) {
            case 0:
                pProportionStat->SetProportionType(CTallyVarStatFmtProportion::PRT_PERCENT);
                break;
            case 1:
                pProportionStat->SetProportionType(CTallyVarStatFmtProportion::PRT_PERCENT_AND_TOTAL);
                break;
            case 2:
                pProportionStat->SetProportionType(CTallyVarStatFmtProportion::PRT_RATIO);
                break;
            case 3:
                pProportionStat->SetProportionType(CTallyVarStatFmtProportion::PRT_RATIO_AND_TOTAL);
                break;
            default:
                ASSERT(!_T("Invalid proportion type"));
        }

        pProportionStat->SetRange(optDlg.m_sRange);
    }
}

// CTallyVarPctDlg dialog

IMPLEMENT_DYNAMIC(CTallyVarPctDlg, CDialog)
CTallyVarPctDlg::CTallyVarPctDlg(CWnd* pParent /*=NULL*/)
    : m_bInterleaved(true),
      m_sPercentType(_T("Row")),
      CDialog(CTallyVarPctDlg::IDD, pParent)
{
}

CTallyVarPctDlg::~CTallyVarPctDlg()
{
}

BOOL CTallyVarPctDlg::OnInitDialog( )
{
    // Execute base class
    BOOL bRetVal = CDialog::OnInitDialog();
    if (!bRetVal) {
        return bRetVal;
    }

    UpdateInterleavedImage();

    return bRetVal;
}

void CTallyVarPctDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_CBString(pDX, IDC_PERCENT_TYPE, m_sPercentType);
    DDX_Radio(pDX, IDC_SEPARATE, m_bInterleaved);
    CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTallyVarPctDlg, CDialog)
    ON_BN_CLICKED(IDC_SEPARATE, OnBnClickedSeparate)
    ON_BN_CLICKED(IDC_INTERLEAVED, OnBnClickedInterleaved)
END_MESSAGE_MAP()


// CTallyVarPctDlg message handlers

void CTallyVarPctDlg::OnBnClickedSeparate()
{
    m_bInterleaved = false;
    UpdateInterleavedImage();
}

void CTallyVarPctDlg::OnBnClickedInterleaved()
{
    m_bInterleaved = true;
    UpdateInterleavedImage();
}

void CTallyVarPctDlg::UpdateInterleavedImage()
{
    CStatic* pPic = (CStatic*) GetDlgItem(IDC_INTERLEAVE_PIC);
    CBitmap newImg;
    newImg.LoadBitmap(m_bInterleaved ? IDB_INTERLEAVE : IDB_SEPARATE);
    pPic->SetBitmap((HBITMAP) newImg.Detach());
}

// CTallyVarTotalPctDlg dialog

IMPLEMENT_DYNAMIC(CTallyVarTotalPctDlg, CDialog)
CTallyVarTotalPctDlg::CTallyVarTotalPctDlg(CWnd* pParent /*=NULL*/)
    : m_sPercentType(_T("Row")),
      CDialog(CTallyVarTotalPctDlg::IDD, pParent)
{
}

CTallyVarTotalPctDlg::~CTallyVarTotalPctDlg()
{
}

void CTallyVarTotalPctDlg::OnOK( )
{
    UpdateData();
    if (m_sPercentType == _T("(same as percents)") && !m_bAllowSameAsPercent) {
        AfxMessageBox(IDS_NEED_PERCENTS_FOR_SAME_AS_PERCENTS);
        return;
    }
    CDialog::OnOK();
}

void CTallyVarTotalPctDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_CBString(pDX, IDC_PERCENT_TYPE, m_sPercentType);
    CDialog::DoDataExchange(pDX);
}


// CTallyVarRangePropDlg dialog

IMPLEMENT_DYNAMIC(CTallyVarRangePropDlg, CDialog)
CTallyVarRangePropDlg::CTallyVarRangePropDlg(UINT nID, CWnd* pParent /*=NULL*/)
    : CDialog(nID, pParent)
{
}


void CTallyVarRangePropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_USE_VSET, m_nUseValueSet);
    DDX_Text(pDX, IDC_MIN, m_dMin);
    DDX_Text(pDX, IDC_MAX, m_dMax);
    DDX_Text(pDX, IDC_INTERVAL, m_dInterval);

    // verify minimum and maximum and interval settings
    if (pDX->m_bSaveAndValidate && m_nUseValueSet != BST_CHECKED) {
        if (m_dMin < m_dMinMin) {
            CString s;
            s.Format(_T("Minimum must be at least %f"), m_dMinMin);
            AfxMessageBox(s);
            pDX->Fail();
        }
        if (m_dMax > m_dMaxMax) {
            CString s;
            s.Format(_T("Maximum must be less than %f"), m_dMaxMax);
            AfxMessageBox(s);
            pDX->Fail();
        }

        if (m_dMin >= m_dMax) {
            AfxMessageBox(_T("Minimum must be less than the maximum"));
            pDX->Fail();
        }
        if (m_dInterval >= m_dMax - m_dMin) {
            AfxMessageBox(_T("The interval is too large for the current minimum and maximum"));
            pDX->Fail();
        }
    }
}


BEGIN_MESSAGE_MAP(CTallyVarRangePropDlg, CDialog)
    ON_BN_CLICKED(IDC_USE_VSET, OnBnClickedUseVset)
END_MESSAGE_MAP()

BOOL CTallyVarRangePropDlg::OnInitDialog( )
{
   if (CDialog::OnInitDialog()) {
        UpdateRangesEnabled();
        return TRUE;
    }

    return FALSE;
}

void CTallyVarRangePropDlg::OnBnClickedUseVset()
{
    UpdateRangesEnabled();
}

void CTallyVarRangePropDlg::UpdateRangesEnabled()
{
    bool bUseVSet = (((CButton*) GetDlgItem(IDC_USE_VSET))->GetCheck() == BST_CHECKED);
    GetDlgItem(IDC_MIN)->EnableWindow(!bUseVSet);
    GetDlgItem(IDC_MAX)->EnableWindow(!bUseVSet);
    GetDlgItem(IDC_INTERVAL)->EnableWindow(!bUseVSet);
}

// CTallyVarMedianDlg dialog

IMPLEMENT_DYNAMIC(CTallyVarMedianDlg, CTallyVarRangePropDlg)
CTallyVarMedianDlg::CTallyVarMedianDlg(CWnd* pParent /*=NULL*/)
    : m_sType(_T("Discrete")),
      CTallyVarRangePropDlg(CTallyVarMedianDlg::IDD, pParent)
{
}


void CTallyVarMedianDlg::DoDataExchange(CDataExchange* pDX)
{
    CTallyVarRangePropDlg::DoDataExchange(pDX);
    DDX_CBString(pDX, IDC_TYPE, m_sType);
}

// CTallyVarNTilesDlg dialog

IMPLEMENT_DYNAMIC(CTallyVarNTilesDlg, CTallyVarRangePropDlg)
CTallyVarNTilesDlg::CTallyVarNTilesDlg(CWnd* pParent /*=NULL*/)
    : m_numTiles(4),
      CTallyVarRangePropDlg(CTallyVarNTilesDlg::IDD, pParent)
{
}


void CTallyVarNTilesDlg::DoDataExchange(CDataExchange* pDX)
{
    CTallyVarRangePropDlg::DoDataExchange(pDX);
    int iIndex;
    if (!pDX->m_bSaveAndValidate) {
        iIndex = m_numTiles-2;
    }

    DDX_CBIndex(pDX, IDC_NUM_TILES, iIndex);

    if (pDX->m_bSaveAndValidate) {
        m_numTiles = iIndex + 2;
    }
}

// CTallyVarProportionDlg dialog

IMPLEMENT_DYNAMIC(CTallyVarProportionDlg, CDialog)
CTallyVarProportionDlg::CTallyVarProportionDlg(CWnd* pParent /*=NULL*/)
    : m_iType(0),
      m_sRange(_T("")),
      CDialog(CTallyVarProportionDlg::IDD, pParent)
{
}

void CTallyVarProportionDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_CBIndex(pDX, IDC_TYPE, m_iType);
    DDX_Text(pDX, IDC_RANGE, m_sRange);
    CDialog::DoDataExchange(pDX);
    if (pDX->m_bSaveAndValidate) {
        if (!IsRangeValid(m_sRange)) {
            CString s(_T("Invalid range.\nUse a single numeric value (e.g. 1), a range (e.g. 5:10) or multiple values/ranges separated by commas (e.g. 1, 2, 5:10)"));
            AfxMessageBox(s);
            pDX->Fail();
        }
    }
}

bool CTallyVarProportionDlg::IsRangeValid(const CIMSAString& sRangeIn)
{
    // make a copy so we don't modify real range with calls to GetToken
    CIMSAString sRange = sRangeIn;

    const TCHAR* const sSeparators = _T(",:");
    TCHAR pSepChar;
    CIMSAString sToken = sRange.GetToken(sSeparators, &pSepChar);
    sToken.Trim();
    if (!sToken.IsNumeric()) {
        return false;
    }
    while (pSepChar != EOS) {

        // first part of range must be a number
        if (!sToken.IsNumeric()) {
            return false;
        }

        if (pSepChar == ':') {
            // parsing a range of form a:b
            sToken = sRange.GetToken(sSeparators, &pSepChar);
            sToken.Trim();
            if (!sToken.IsNumeric()) {
                // second part of range (b) is not numeric
                return false;
            }
            if (pSepChar == ':') {
                // after range should have comma or end but not :
                // that would allow a:b:c
                return false;
            }
        }

        if (pSepChar != EOS) {
            sToken = sRange.GetToken(sSeparators, &pSepChar);
            sToken.Trim();
        }
    }

    return true;
}
