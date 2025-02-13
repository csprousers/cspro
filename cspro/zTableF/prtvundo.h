#pragma once

//***************************************************************************
//  File name: PrtVUndo.h
//
//  Description:
//       Undo commands for print view
//
//  History:    Date       Author     Comment
//              -----------------------------
//              March 2005 csc        CSPro 3.0
//
//***************************************************************************

#include <zTableO/Table.h>

class CTabPrtView;


/////////////////////////////////////////////////////////////////////////////
//
//                  CResizeCmdInfo
//
// Small class that contains column or stub information needed for undoing
// or redoing resize operations.
//
/////////////////////////////////////////////////////////////////////////////
class CResizeCmdInfo
{
public:
    int m_iIndex;   // index (colhead or stub/caption or stubhead)
    CArray<CPrtViewInfo, CPrtViewInfo&> m_aPrtViewInfo;

    CResizeCmdInfo() {
        m_aPrtViewInfo.RemoveAll();
        m_iIndex=-1;
    }
    CResizeCmdInfo(CResizeCmdInfo& c) {
        *this=c;
    }

    void operator=(CResizeCmdInfo& c) {
        m_iIndex=c.m_iIndex;
        m_aPrtViewInfo.RemoveAll();
        for (int i=0 ; i<c.m_aPrtViewInfo.GetSize() ; i++) {
            m_aPrtViewInfo.Add(c.m_aPrtViewInfo[i]);
        }
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                  CPrintFormatCmdInfo
//
// Small class that contains information needed for undoing
// or redoing print format operations.
//
/////////////////////////////////////////////////////////////////////////////
class CPrintFormatCmdInfo
{
public:
    HEADER_FREQUENCY    m_eHeaderFrequency;     // header frequency
    TBL_LAYOUT          m_eTblLayout;           // table layout
    int                 m_iStartPage;           // starting page number
    CRect               m_rcPageMargin;         // page margins
    CTblOb              m_tHeader[3];           // headers (left, center, right) -- text + formatting
    CTblOb              m_tFooter[3];           // footers (left, center, right) -- text + formatting

// access
public:
    HEADER_FREQUENCY GetHeaderFrequency(void) const { return m_eHeaderFrequency; }
    void SetHeaderFrequency(HEADER_FREQUENCY eHeaderFrequency) { m_eHeaderFrequency=eHeaderFrequency; }

    TBL_LAYOUT GetTblLayout(void) const { return m_eTblLayout; }
    void SetTblLayout(TBL_LAYOUT eTblLayout) { m_eTblLayout=eTblLayout; }

    int GetStartPage(void) const { return m_iStartPage; }
    void SetStartPage(int iStartPage) { m_iStartPage=iStartPage; }

    const CRect& GetPageMargin(void) const { return m_rcPageMargin; }
    void SetPageMargin(const CRect& rcPageMargin) { m_rcPageMargin=rcPageMargin; }

    CTblOb* GetHeader(int iIndex) { return &m_tHeader[iIndex]; }
    void SetHeader(int iIndex, CTblOb* pHeader) { m_tHeader[iIndex]=*pHeader; }

    CTblOb* GetFooter(int iIndex) { return &m_tFooter[iIndex]; }
    void SetFooter(int iIndex, CTblOb* pFooter) { m_tFooter[iIndex]=*pFooter; }

    CPrintFormatCmdInfo() {}
    CPrintFormatCmdInfo(CPrintFormatCmdInfo& c) {
        *this=c;
    }

    void operator=(CPrintFormatCmdInfo& c) {
        SetHeaderFrequency(c.GetHeaderFrequency());
        SetTblLayout(c.GetTblLayout());
        SetStartPage(c.GetStartPage());
        SetPageMargin(c.GetPageMargin());
        for (int i=0 ; i<3 ; i++) {
            SetHeader(i, c.GetHeader(i));
            SetFooter(i, c.GetFooter(i));
        }
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                  CPrtViewCommand
//
// Base class (abstract) command for undoing/redoing actions in prt view. See Command design pattern.
// Objects from classes derived from CPrtViewCommand can be put into an undo/redo stack (CUndoStack).
//
//  class                     function/usage
//  CPrtViewCommand           base class; abstract; not instantiable
//  CResizeCommand            resizing stuff (boxheads, stubs/captions, stubheads) (includes page breaks)
//  CRestoreDefaultsCommand   restore defaults command
//
//
// - tblprintfmt dlg stuff
// - restore default
// - auto-fit columns
//
/////////////////////////////////////////////////////////////////////////////
class CPrtViewCommand : public CObject
{
public:
    virtual ~CPrtViewCommand() {}
    virtual void Execute(bool bBuild=true)=0;

protected:
    CPrtViewCommand() {}
};


/////////////////////////////////////////////////////////////////////////////
//
//                  CResizeCommand
//
// Command for undoing/redoing resize actions in prt view, including those for:
//   - boxheads (CBoxheadResizeCommand)
//   - stubs/captions (CStubResizeCommand)
//   - stubheads (CStubHeadResizeCommand)
// These use derived classes.
//
// See base class CPrtViewCommand above.
//
/////////////////////////////////////////////////////////////////////////////
class CResizeCommand : public CPrtViewCommand
{
public:
    virtual ~CResizeCommand() {}
    virtual void Execute(bool bBuild=true)=0;
    CResizeCommand(CTabPrtView* pView, int iTbl);

// command-specific data
protected:
    CArray<CResizeCmdInfo, CResizeCmdInfo&> m_aResizeCmdInfo;    // resizing information for all of the table's boxheads
    CTabPrtView* m_pView;             // the prt view (our receiver)
    int          m_iTbl;              // the table to which this command applies

// access
public:
    void Add(CResizeCmdInfo& c) {
        m_aResizeCmdInfo.Add(c);
    }
    int GetTbl(void) const {
        return m_iTbl;
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                  CBoxheadResizeCommand
//
// Command for undoing/redoing boxhead resize actions in prt view.
//
// See base class CPrtViewCommand above.
//
/////////////////////////////////////////////////////////////////////////////
class CBoxheadResizeCommand : public CResizeCommand
{
DECLARE_DYNCREATE (CBoxheadResizeCommand);
public:
    virtual ~CBoxheadResizeCommand() {}
    virtual void Execute(bool bBuild=true);
    CBoxheadResizeCommand(CTabPrtView* pView=NULL, int iTbl=-1, bool bAutoFitColumns=true) : CResizeCommand(pView, iTbl) {
        m_bAutoFitColumns=bAutoFitColumns;
    }

// command-specific data
private:
    bool m_bAutoFitColumns;       // =true for "automatically fit columns across each page" (corresponds to CTabPrtView::m_bAutoFitColumns)

// access
public:
    bool GetAutoFitColumns(void) const {
        return m_bAutoFitColumns;
    }
    void SetAutoFitColumns(bool bAutoFitColumns) {
        m_bAutoFitColumns=bAutoFitColumns;
    }
};



/////////////////////////////////////////////////////////////////////////////
//
//                  CStubResizeCommand
//
// Command for undoing/redoing stub and caption resize actions in prt view.
//
// See base class CPrtViewCommand above.
//
/////////////////////////////////////////////////////////////////////////////
class CStubResizeCommand : public CResizeCommand
{
DECLARE_DYNCREATE (CStubResizeCommand);
public:
    virtual ~CStubResizeCommand() {}
    virtual void Execute(bool bBuild=true);
    CStubResizeCommand(CTabPrtView* pView=NULL, int iTbl=-1) : CResizeCommand(pView, iTbl) {}
};



/////////////////////////////////////////////////////////////////////////////
//
//                  CStubHeadResizeCommand
//
// Command for undoing/redoing stubhead resize actions in prt view.
//
// See base class CPrtViewCommand above.
//
/////////////////////////////////////////////////////////////////////////////
class CStubHeadResizeCommand : public CResizeCommand
{
DECLARE_DYNCREATE (CStubHeadResizeCommand);
public:
    virtual ~CStubHeadResizeCommand() {}
    virtual void Execute(bool bBuild=true);
    CStubHeadResizeCommand(CTabPrtView* pView=NULL, int iTbl=-1) : CResizeCommand(pView, iTbl) {}
};



/////////////////////////////////////////////////////////////////////////////
//
//                  CRestoreDefaultsCommand
//
// Command for undoing/redoing the "restore defaults" command, which involves
// resizing both stubs and boxheads.  Also used for restoring stubhead resizing
// commands.
//
// See base class CPrtViewCommand above.
//
/////////////////////////////////////////////////////////////////////////////
class CRestoreDefaultsCommand : public CResizeCommand
{
DECLARE_DYNCREATE (CRestoreDefaultsCommand);
public:
    virtual ~CRestoreDefaultsCommand() {}
    virtual void Execute(bool bBuild=true);
    CRestoreDefaultsCommand(CTabPrtView* pView=NULL, int iTbl=-1);

// command-specific data
protected:
    CTabPrtView*               m_pView;                 // the prt view (our receiver)
    int                        m_iTbl;                  // the table to which this command applies

    CBoxheadResizeCommand      m_cmdResizeBoxhead;      // resize boxheads command
    CStubResizeCommand         m_cmdResizeStub;         // resize stubs command
    CStubHeadResizeCommand     m_cmdResizeStubHead;     // resize stubheadss command

// access
public:
    void AddBoxhead(CResizeCmdInfo& c) {
        m_cmdResizeBoxhead.Add(c);
    }
    void AddStub(CResizeCmdInfo& c) {
        m_cmdResizeStub.Add(c);
    }
    void AddStubHead(CResizeCmdInfo& c) {
        m_cmdResizeStubHead.Add(c);
    }
};


/////////////////////////////////////////////////////////////////////////////
//
//                  CFormatPrintCommand
//
// Command for undoing/redoing format print dialog settings.
//
// See base class CPrtViewCommand above.
//
/////////////////////////////////////////////////////////////////////////////
class CFormatPrintCommand : public CPrtViewCommand
{
DECLARE_DYNCREATE (CFormatPrintCommand);
public:
    virtual ~CFormatPrintCommand() {}
    virtual void Execute(bool bBuild=true);
    CFormatPrintCommand(CTabPrtView* pView=NULL, int iTbl=-1);

// command-specific data
protected:
    CPrintFormatCmdInfo m_formatInfo;           // print format information
    CTabPrtView*        m_pView;                // the prt view (our receiver)
    int                 m_iTbl;                 // the table to which this command applies

// access
public:
    CPrintFormatCmdInfo& GetPrintFormatCmdInfo(void) {
        return m_formatInfo;  // lvalue!
    }
    int GetTbl(void) const {
        return m_iTbl;
    }
};
