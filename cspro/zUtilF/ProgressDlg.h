#pragma once

#include <zUtilF/zUtilF.h>
#include <zUtilF/ProgressDlgFactory.h>

//***************************************************************************
//  Description:
//       Header for ProgressDlg
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Jan 98   BMD     Created from IMPS 4.1
//
//***************************************************************************
//
//  class ProgressDlg : public CDialog
//
//  Description:
//      Progress bar dialog box.  Uses CProgressCtrl in the dialog box.
//
//  Construction
//      ProgressDlg            Contruct the dialog box with a particular caption.
//      Create                  Create the dialog box for a given parent.
//
//  Checking for Cancel button
//      CheckCancelButton       Has cancel button been pressed.
//
//  Progress Dialog manipulation
//      SetStatus               Set the status messsage text in the dialog box.
//      SetRange                Set the upper and lower limits for stepping.
//      SetStep                 Set the step size.
//      SetPos                  Set the progress control position.
//      OffsetPos               Offset the progress control position from it current setting.
//      StepIt                  Increase the control by one step.
//      GetUpperRange           Get the upper range of the control.
//      DestroyWindow           Destroy the window.
//
//***************************************************************************
//
//  ProgressDlg::ProgressDlg(UINT uID = 0);
//
//      Parameter
//          uID                 Resource id of the text string to use as the Window title.
//                              If zero (0), use default title "IMPS 4.2".
//
//      Remarks
//          Construct ProgressDlg on the heap.
//
//---------------------------------------------------------------------------
//
//  BOOL ProgressDlg::Create(CWnd *pParent = NULL);
//
//      Parameters
//          pParent             Points to the parent window object (of type CWnd) to
//                              which the dialog object belongs. If it is NULL,
//                              he dialog object's parent window is set to
//                              the main application window.
//
//      Remarks
//           Call to create as a modeless dialog box.
//
//***************************************************************************
//
//  BOOL ProgressDlg::CheckCancelButton();
//
//      Return value
//          TRUE if cancel button was pressed, otherwise FALSE;
//
//      Remarks
//          Calling the function resets cancel to FALSE.
//
//***************************************************************************
//
//  void ProgressDlg::SetStatus(LPCTSTR lpszMessage);
//
//      Parameter
//          lpszMessage         String containing the status line message.
//
//---------------------------------------------------------------------------
//
//  void SetStatus(UINT uID);
//
//      Parameter
//          uID                 Resource id containing the status line message.
//
//---------------------------------------------------------------------------
//
//  void ProgressDlg::SetRange(int iLower,int iUpper);
//
//      Parameters
//          iLower              Lower limit of progress bar range.
//          iUpper              Upper limit of progress bar range.
//
//
//---------------------------------------------------------------------------
//
//  int ProgressDlg::SetStep(int iStep);
//
//      Parameters
//          iStep               The size of the bar step taken by StepIt().
//
//---------------------------------------------------------------------------
//
//  int ProgressDlg::SetPos(int iPos);
//
//      Parameters
//          iPos                New position of the progress bar.
//
//      Return value
//          Old position of the progress bar.
//
//---------------------------------------------------------------------------
//
//  int ProgressDlg::OffsetPos(int iPos);
//
//      Parameters
//          iPos                New position of the progress bar relative to the old position.
//
//      Return value
//          Old position of the progress bar.
//
//---------------------------------------------------------------------------
//
//  int ProgressDlg::StepIt();
//
//      Return value
//          Old position of the progress bar.
//
//      Remark
//          Increase bar by the size of value given by SetStep().
//
//---------------------------------------------------------------------------
//
//  int ProgressDlg::GetUpperRange() const;
//
//      Return value
//          Upper limit of range of the progress bar.
//
//---------------------------------------------------------------------------
//
//  virtual BOOL ProgressDlg::DestroyWindow();
//
//      Return value
//          TRUE if successful, otherwise FALSE.
//
//***************************************************************************
//***************************************************************************
//***************************************************************************


#ifdef WIN_DESKTOP

class CLASS_DECL_ZUTILF ProgressDlg : public CDialog
{
    friend class ProgressDlgFactory;

private:
    ProgressDlg();

public:
    // Construction/Destruction
    ~ProgressDlg();

    BOOL Create(CWnd *pParent=NULL);

    // Checking for Cancel button
    BOOL CheckCancelButton();

    // Progress Dialog manipulation
    void SetCaption(UINT caption_id);
    void SetStatus(LPCTSTR lpszMessage);
    void SetStatus(UINT uID);
    void SetRange(int iLower,int iUpper);
    int SetStep(int iStep);
    int SetPos(int iPos);
    int OffsetPos(int iPos);
    int StepIt();
    int GetUpperRange() const { return m_iUpper; }

    BOOL DestroyWindow() override;

protected:
    void DoDataExchange(CDataExchange* pDX) override;

    void ReEnableParent();
    void OnOK() override { };
    void OnCancel() override;
    void UpdatePercent(int iCurrent);
    void PumpMessages();

    BOOL OnInitDialog() override;

protected:
    CProgressCtrl m_progress;
    int m_iLower;
    int m_iUpper;
    int m_iStep;
    BOOL m_bCancel;
    bool m_bParentDisabled;
};


#else

// Progress dialog is stubbed out on Android
class ProgressDlg
{
    friend class ProgressDlgFactory;

private:
    ProgressDlg(UINT /*nCaptionID*/ = 0)
    {
    }

public:
    BOOL Create()
    {
        return TRUE;
    }

    void SetCaption(UINT /*caption_id*/) { }

    void SetStatus(LPCTSTR /*lpszMessage*/) { }

    void SetWindowText(LPCTSTR /*lpszMessage*/) { }

    void SetStatus(UINT /*uID*/) { }

    void SetRange(int /*iLower*/, int /*iUpper*/) { }

    int SetStep(int /*iStep*/)
    {
        return 0;
    }

    int SetPos(int /*iPos*/)
    {
        return 0;
    }

    int OffsetPos(int /*iPos*/)
    {
        return 0;
    }

    int StepIt()
    {
        return 0;
    }

    BOOL CheckCancelButton()
    {
        return FALSE;
    }
};
#endif
