//***************************************************************************
//  File name: PrtVUndo.cpp
//
//  Description:
//       Implementation of undo commands for print view
//
//  History:    Date       Author     Comment
//              -----------------------------
//              March 2005 csc        CSPro 3.0
//
//***************************************************************************

#include "StdAfx.h"
#include "prtvundo.h"
#include "PrtView.h"

IMPLEMENT_DYNCREATE(CBoxheadResizeCommand, CResizeCommand)
IMPLEMENT_DYNCREATE(CStubResizeCommand, CResizeCommand)
IMPLEMENT_DYNCREATE(CStubHeadResizeCommand, CResizeCommand)
IMPLEMENT_DYNCREATE(CRestoreDefaultsCommand, CResizeCommand)
IMPLEMENT_DYNCREATE(CFormatPrintCommand, CPrtViewCommand)


/////////////////////////////////////////////////////////////////////////////
//
//                  CResizeCommand
//
//  Class implementation.
//
/////////////////////////////////////////////////////////////////////////////
CResizeCommand::CResizeCommand(CTabPrtView* pView, int iTbl)
{
    m_aResizeCmdInfo.RemoveAll();
    m_pView=pView;
    m_iTbl=iTbl;
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CBoxheadResizeCommand
//
//  Class implementation.
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CBoxheadResizeCommand::Execute(bool bBuild /*=true*/)
{
    ASSERT(m_pView);
    m_pView->UndoBoxheadResize(m_aResizeCmdInfo, m_iTbl);

    if (bBuild) {

        // build operation must have AutoFitColumn setting consistent with what was active when the
        // undo/redo push was performed.  We stored that as m_bAutoFitColumns
        bool bBackupAutoFitColumns=m_pView->GetAutoFitColumns();
        m_pView->SetAutoFitColumns(m_bAutoFitColumns);
        m_pView->Build(true);
        m_pView->SetAutoFitColumns(bBackupAutoFitColumns);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CStubResizeCommand
//
//  Class implementation.
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CStubResizeCommand::Execute(bool bBuild /*=true*/)
{
    ASSERT(m_pView);
    m_pView->UndoStubResize(m_aResizeCmdInfo, m_iTbl);

    if (bBuild) {
        m_pView->Build(true);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                  CStubHeadResizeCommand
//
//  Class implementation.
//
/////////////////////////////////////////////////////////////////////////////
/*V*/ void CStubHeadResizeCommand::Execute(bool bBuild /*=true*/)
{
    ASSERT(m_pView);
    m_pView->UndoStubHeadResize(m_aResizeCmdInfo, m_iTbl);

    if (bBuild) {
        m_pView->Build(true);
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//                  CRestoreDefaultsCommand
//
//  Class implementation.
//
/////////////////////////////////////////////////////////////////////////////
CRestoreDefaultsCommand::CRestoreDefaultsCommand(CTabPrtView* pView, int iTbl) :
    CResizeCommand(pView, iTbl),
    m_cmdResizeBoxhead(pView, iTbl),
    m_cmdResizeStub(pView, iTbl),
    m_cmdResizeStubHead(pView, iTbl)
{
    m_pView=pView;
    m_iTbl=iTbl;
}


/*V*/ void CRestoreDefaultsCommand::Execute(bool bBuild /*=true*/)
{
    ASSERT(m_pView);

    m_cmdResizeBoxhead.Execute(false);
    m_cmdResizeStub.Execute(false);
    m_cmdResizeStubHead.Execute();

}


/////////////////////////////////////////////////////////////////////////////
//
//                  CFormatPrintCommand
//
//  Class implementation.
//
/////////////////////////////////////////////////////////////////////////////
CFormatPrintCommand::CFormatPrintCommand(CTabPrtView* pView, int iTbl)
{
    m_pView=pView;
    m_iTbl=iTbl;
}


/*V*/ void CFormatPrintCommand::Execute(bool bBuild /*=true*/)
{
    ASSERT(m_pView);

    m_pView->UndoFormatPrint(m_formatInfo, m_iTbl);

    if (bBuild) {
        m_pView->Build(true);
    }


}


