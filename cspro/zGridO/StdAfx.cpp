// stdafx.cpp : source file that includes just the standard includes
//  zGridO.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "StdAfx.h"

///////////////////////////////////////////////
//
//      changes to the grid
//
//  - CUGCtrl::GetAbsoluteCellFromPoint()
//
// #1 CUGGrid::Moved(): forced the redraw; the code is attempting to check, for both vert/horz
//          scrolling, if the screen needs to be redrawn based on the size of the current
//          col/row, not taking into consideration as in our case that a modeless dialog box
//          could be sitting on top of the grid, necessitating a redraw; also, in the update,
//          they added at the beg of the func   "if(m_ctrl->m_findDialogRunning)" which i think
//          is their attempt to fix this prob, but it doesn't work
//
//
// Add 2 functions:     void VScrollAlwaysPresent (BOOL bFlag)
//                      void HScrollAlwaysPresent (BOOL bFlag)
//
//          to force the presents of scroll bars on the grid regardless of the size of grid.
//          By default, these are both initialized to FALSE, thus removing the scroll bars when
//          not needed.     --BMD
//
//
//
//
//
// GSF Add 2 functions:
//      BOOL CUGCtrl::GetTopCellFromPoint(int px,int py,int *ptcol)
//      BOOL CUGCtrl::GetSideCellFromPoint(int px,int py,long *ptrow)
//
//      to distinguish top and side headers from 0th row/col in hit testing
//
// Savy 10/21/98
// GetCurrentTab() gives the current index into the Array of TabIDs.
// As the functionality required to Get the current Tab ID , I added 2 public
// functions to meet the requirement.
// 1. int CUGCtrl::GetCurrentTabID()  (UGCtrl.h && UGCtrl.cpp)
// 2. int CUGTab::GetCurrentTabID()   (UGTab.h && UGTab.cpp)

// Savy 10/22/98
// GetTabID(int iIndex) gives the ID of the tab at iIndex .This was required for
// implementing the delete Tabs in the "correct" way ( Excel Style)
// 1. int CUGCtrl::GetTabID(int iIndex)  (UGCtrl.h && UGCtrl.cpp)
// 2. int CUGTab::GetTabID(int iIndex)    (UGTab.h && UGTab.cpp)

// Savy 10/23/98
//  Added function to Change the TabText
// SetTabText(long ID , CString sString) sets the text of the tab at iIndex .This was required for
//  the resetting tab text after deletion/insertion of the tabs
// 1. int CUGCtrl::SetTabText(long ID , CString sString)  (UGCtrl.h && UGCtrl.cpp)
// 2. int CUGTab::SetTabText(long ID , CString sString)   (UGTab.h && UGTab.cpp)
// Savy 10/27/98
// Added function to CUGTab for Enabling Tool tips
//  1. int CUGTab::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
//  2. Added EnableToolTips(TRUE) to int CUGTab::OnCreate(LPCREATESTRUCT lpCreateStruct) function
//  3. Changed the GetTabItemWidth to get the width of string upto 20 characters
//  4. changed the Textout thing to get only the first twenty characters of the tab string

//
// smg 09/01/99
//
// 1. CUGCtrl::SetTH_RowHeight(); func ends prematurely, did not have proper init code;
//      see SetSH_ColWidth() for proper invocation (unless they hose it up on subsequent release)
// 2. CUGCtrl::SetRowHeight(); was not calling SetTH_RowHeight() if row was negative
// 3. CUGCnrBtn::OnLButtonUp();if you tried to size the side/top heading col/row in the
//      corner button area, the code plum wasn't there to call either OnSideHdgSized() or
//      OnTopHdgSized()
// 4. CUGCnrBtn::OnMouseMove(); if the user is sizing the col or row, it was calling
//      m_ctrl->SetSH_Width/SetTH_Height instead of m_GI->(appropriate func)

//
// gsf 09/13/99
//
// Make it so you can turn off clicks on the tabs
// 1. ugtabl.h:  added new member, bool CUGTab::m_bCanClick
// 2. ugtab.cpp: check the member in lbuttonup, lbuttondown, rbuttondown, hscroll

// bmd 09/14/99
// Modified CUGCellType::DrawText in ugceltyp.cpp to correctly handle multiline UG_ALIGNVCENTER and
//     UG_ALIGNBOTTOM.
//
// bmd 10/21/99
// Added CUGCtrl::VScrollEnable(UINT uFlag) to enable or disable the vertical scroll bar
// Added CUGCtrl::VScrollRedraw() to force redraw of vertical scroll bar after enable
