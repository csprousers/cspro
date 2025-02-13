#pragma once
// RelGrid.h: interface for the CLangGrid class.
//
//////////////////////////////////////////////////////////////////////

#include <zDictF/zDictF.h>
#include <zGridO/Ugctrl.h>

class CLabelEdit2;
class CNameEdit2;

enum class eLANGINFO { DEFAULT_INFO = 0, NEW_INFO, MODIFIED_INFO, DELETED_INFO };


class CLASS_DECL_ZDICTF CLangInfo : public CObject {
    public:
    CLangInfo() { m_eLangInfo = eLANGINFO::DEFAULT_INFO; }
    virtual ~CLangInfo(){};
    bool operator== (const CLangInfo& langInfo ) const {
        bool bRet = false;
        if(true) {
            CIMSAString sThis = m_sLangName;
            CIMSAString sCompare = langInfo.m_sLangName;
            sThis.Trim();
            sCompare.Trim();

            CIMSAString sThisLabel = m_sLabel;
            CIMSAString sCompareLabel = langInfo.m_sLabel;
            sThisLabel.Trim();
            sCompareLabel.Trim();

            if((sThis.CompareNoCase(sCompare) == 0) && (sThisLabel.CompareNoCase(sCompareLabel) == 0) ) {
                bRet = true;
            }
        }
        return bRet;

    }
    CLangInfo (CLangInfo& langInfo)
    {
        m_sLangName = langInfo.m_sLangName ;
        m_sLabel = langInfo.m_sLabel;
        m_eLangInfo = langInfo.m_eLangInfo;
    }       // copy constructor

    void operator= (CLangInfo& langInfo){
        m_sLangName = langInfo.m_sLangName ;
        m_sLabel = langInfo.m_sLabel;
        m_eLangInfo = langInfo.m_eLangInfo;
    }

public :
    CIMSAString m_sLangName;
    CIMSAString m_sLabel;
    eLANGINFO m_eLangInfo;
};


class CLASS_DECL_ZDICTF CLangGrid : public CUGCtrl
{
private:
    int                 m_iOldStart;
    int                 m_iOldLen;

    CLabelEdit2*         m_pLabelEdit;
    CNameEdit2*          m_pNameEdit;

protected:
    bool            m_bCanEdit;
    bool            m_bEditing;
    bool            m_bAdding;

    LOGFONT*        m_plf;
    CFont           m_font;

public:
    public:
    int         m_iEditRow;
    int         m_iEditCol;
    int         m_iMaxCol;
    int         m_iMinCol;
    bool        m_bChanged;

    CArray<CLangInfo,CLangInfo&>   m_aLangInfo;
    CArray<CWnd*, CWnd*>    m_aEditControl;

public:
    CLangGrid() {m_bChanged = false;}
    ~CLangGrid() {}

    void Size(CRect rect);
    void Resize(CRect rect);
    void EditChange(UINT uChar);

    void SetGridFont(LOGFONT* plf) {m_plf = plf; m_font.CreateFontIndirect(plf);}

    void Update();
    bool IsEditing() const { return m_bEditing; }
    void EditContinue();
    void ResetGrid();

    int IsDuplicate(CLangInfo& langInfo ,int iIgnoreRow =-1);

    afx_msg void OnEditAdd();
    afx_msg void OnEditDelete();
    afx_msg void OnEditModify();

private:
    void UpdateLang(CLangInfo* pLangInfo, int row, COLORREF rgb);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSetFocus(CWnd* pOldWnd);

public:
    //Row edits
    void EditBegin(int col, long row, UINT vcKey);
    bool EditEnd(bool bSilent = false);
    void EditQuit();

    //***** Over-ridable Notify Functions *****
    void OnSetup() override;

    //movement and sizing
    int OnCanMove(int oldcol, long oldrow, int newcol, long newrow) override;
    void OnColSized(int col, int* width) override;

    //mouse and key strokes
    void OnLClicked(int col, long row, int updn, RECT* rect, POINT* point, int processed) override;
    void OnTH_RClicked(int col, long row, int updn, RECT* rect, POINT* point, BOOL processed = 0) override;
    void OnCB_RClicked(int updn, RECT* rect, POINT* point, BOOL processed = 0) override;
    void OnDClicked(int col, long row, RECT* rect, POINT* point, BOOL processed) override;
    void OnKeyDown(UINT* vcKey, BOOL processed) override;

    //cell type notifications
    int OnCellTypeNotify(long ID, int col, long row, long msg, long param) override;

    //focus rect setup
    void OnKillFocus(int section) override;

    // Movement and sizing (derived from CUGCtrl)
    int OnCanSizeTopHdg() override;
    int OnCanSizeSideHdg() override;
    int OnCanSizeCol(int col) override;
    void OnColSizing(int col, int* width) override;

    int  OnCanSizeRow(long row) override;

    void OnRowChange(long oldrow,long newrow) override;

public:
    void OnCharDown(UINT* vcKey,BOOL processed) override;
};
