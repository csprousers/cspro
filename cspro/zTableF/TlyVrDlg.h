#pragma once

class CTallyVarStatFmt;

// CTallyVarDlg dialog

class CTallyVarDlg : public CDialog
{
    DECLARE_DYNAMIC(CTallyVarDlg)

public:
    CTallyVarDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTallyVarDlg();

    CString m_sTitle;
    CTallyFmt* m_pFmtSelected;
    CTallyFmt* m_pDefaultFmt;
    CDWordArray  m_aMapStatNewToOldPos; // maps from new index of stat to old index
                                        // this is updated in OK to be used for reconcile
    bool m_bOrderChanged;               // true if order of stats changed
    bool m_bDisablePct;                 // true to allow percents
    double m_dVarMin, m_dVarMax;        // hard limits for ranges in ntiles and median
    double m_dVarMinDefault, m_dVarMaxDefault; // default limits for ranges in ntiles and median
    CIMSAString m_sDefaultPropRange;    // default range for proportion

// Dialog Data
    enum { IDD = IDD_TALLY_VAR_NEW };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    virtual  BOOL OnInitDialog( );
    virtual void OnOK( );

    void UpdateEnableDisable();

    CPtrArray  m_aOrigStatPos; // orig positions of stats to calculate m_aMapStatNewToOldPos

    void PlacePercentsAndPercentTotal(bool bAddedTotalPercent);
    void UpdateListSelected();

    void OnPercentOptions(CTallyVarStatFmt* pStat);
    void OnTotalPercentOptions(CTallyVarStatFmt* pStat);
    void OnMedianOptions(CTallyVarStatFmt* pStat);
    void OnNTilesOptions(CTallyVarStatFmt* pStat);
    void OnProportionOptions(CTallyVarStatFmt* pStat);

    DECLARE_MESSAGE_MAP()
public:
    CListBox m_listAvailable;
    CListBox m_listSelected;
    afx_msg void OnBnClickedAdd();
    afx_msg void OnBnClickedRemove();
    afx_msg void OnLbnSelchangeListAvailable();
    afx_msg void OnLbnSelchangeListSelected();
    afx_msg void OnLbnSetfocusListAvailable();
    afx_msg void OnLbnSetfocusListSelected();
    afx_msg void OnBnClickedUp();
    afx_msg void OnBnClickedDown();
    afx_msg void OnBnClickedOptions();
    afx_msg void OnBnClickedReset();
};


// CTallyVarPctDlg dialog

class CTallyVarPctDlg : public CDialog
{
    DECLARE_DYNAMIC(CTallyVarPctDlg)

public:
    CTallyVarPctDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTallyVarPctDlg();
    CString m_sPercentType;
    BOOL m_bInterleaved;

// Dialog Data
    enum { IDD = IDD_TALLY_VAR_PERCENT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual  BOOL OnInitDialog( );

    void UpdateInterleavedImage();

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedSeparate();
    afx_msg void OnBnClickedInterleaved();
};

// CTallyVarTotalPctDlg dialog

class CTallyVarTotalPctDlg : public CDialog
{
    DECLARE_DYNAMIC(CTallyVarTotalPctDlg)

public:
    CTallyVarTotalPctDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CTallyVarTotalPctDlg();
    virtual void OnOK( );

    CString m_sPercentType;
    bool m_bAllowSameAsPercent;

// Dialog Data
    enum { IDD = IDD_TALLY_VAR_TOTAL_PERCENT };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
};


// CTallyVarRangePropDlg dialog - base class for dialogs with range props (median and ntile)

class CTallyVarRangePropDlg : public CDialog
{
    DECLARE_DYNAMIC(CTallyVarRangePropDlg)

public:
    CTallyVarRangePropDlg(UINT nId, CWnd* pParent = NULL);   // standard constructor
    virtual  BOOL OnInitDialog( );

    int m_nUseValueSet;
    double m_dMin, m_dMax, m_dInterval;
    double m_dMinMin, m_dMaxMax;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    void UpdateRangesEnabled();

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedUseVset();
};

// CTallyVarMedianDlg dialog

class CTallyVarMedianDlg : public CTallyVarRangePropDlg
{
    DECLARE_DYNAMIC(CTallyVarMedianDlg)

public:
    CTallyVarMedianDlg(CWnd* pParent = NULL);   // standard constructor

    CString m_sType;

// Dialog Data
    enum { IDD = IDD_TALLY_VAR_MEDIAN };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
};

// CTallyVarNTilesDlg dialog

class CTallyVarNTilesDlg : public CTallyVarRangePropDlg
{
    DECLARE_DYNAMIC(CTallyVarNTilesDlg)

public:
    CTallyVarNTilesDlg(CWnd* pParent = NULL);   // standard constructor

    int m_numTiles;

// Dialog Data
    enum { IDD = IDD_TALLY_VAR_NTILES };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
};

// CTallyVarProportionDlg dialog

class CTallyVarProportionDlg : public CDialog
{
    DECLARE_DYNAMIC(CTallyVarProportionDlg)

public:
    CTallyVarProportionDlg(CWnd* pParent = NULL);   // standard constructor

    int m_iType;
    CString m_sRange;

// Dialog Data
    enum { IDD = IDD_TALLY_VAR_PROPORTION };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    bool IsRangeValid(const CIMSAString& sRange);
};
