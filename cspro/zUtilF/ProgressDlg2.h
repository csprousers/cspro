#pragma once

#include <zUtilF/zUtilF.h>

//***************************************************************************
//  Description:
//       Progress dialog with 2 progress indicators: current and overall.
//       Useful for multi file operations where current progress indicator
//       can be used for current file and overall can be used for all files.
//
//***************************************************************************


// this dialog was only used by TRS, so when those tools were removed,
// this class was no longer exported; however, it's being kept around
// in case we want to use it at some point

class ProgressDlg2 : public CDialog
{
public:
    ProgressDlg2(CWnd* pParent = NULL);   // standard constructor
    ~ProgressDlg2();

    BOOL Create(CWnd *pParent=NULL);

    // Checking for Cancel button
    BOOL CheckCancelButton();

    // Progress Dialog manipulation
    enum WhichProgressControls
    {
        CURRENT = 0,
        OVERALL
    };

    void SetGroupBoxText(WhichProgressControls w, LPCTSTR lpszMessage);
    void SetStatus(WhichProgressControls w, LPCTSTR lpszMessage);
    void SetRange(WhichProgressControls w, int iLower,int iUpper);
    int GetUpperRange(WhichProgressControls w);
    int SetStep(WhichProgressControls w, int iStep);
    int SetPos(WhichProgressControls w, int iPos);
    int OffsetPos(WhichProgressControls w, int iPos);
    int StepIt(WhichProgressControls w);

    // turn off the overall progress controls (show only current)
    // must call after create (assumes hWnd is valid)
    void HideOverallIndicators();

    BOOL DestroyWindow() override;

protected:
    void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

    BOOL OnInitDialog() override;

    void OnOK()  override { }
    void OnCancel() override;

    void PumpMessages();
    void ReEnableParent();

protected:
    // helper class for dealing with the grouping of progress
    // control, status label and percent, one for current and one
    // for overall
    class ProgressControls
    {
        friend class ProgressDlg2;

    public:
        ProgressControls() :
          m_iLower(0),
          m_iUpper(100),
          m_iStep(1)
          {
          }

        void SetGroupBoxText(LPCTSTR lpszMessage);
        void SetStatus(LPCTSTR lpszMessage);
        void SetRange(int iLower,int iUpper);
        int GetUpperRange();
        int  SetStep(int iStep);
        int  SetPos(int iPos);
        int  OffsetPos(int iPos);
        int  StepIt();

    private:
        void UpdatePercent(int iCurrent);
        int m_iLower;
        int m_iUpper;
        int m_iStep;

        void SetControls(CWnd* pStatus, CWnd* pGroupBox,
                         CWnd* pPercent, CProgressCtrl* pProgCtrl)
        {
            m_pWndStatus = pStatus;
            m_pWndGroupBox = pGroupBox;
            m_pWndPercent = pPercent;
            m_pProgressCtrl = pProgCtrl;
            m_pProgressCtrl->SetRange((short) m_iLower, (short) m_iUpper);
            m_pProgressCtrl->SetStep(m_iStep);
            m_pProgressCtrl->SetPos(m_iLower);
        }
        CWnd* m_pWndStatus;
        CWnd* m_pWndGroupBox;
        CWnd* m_pWndPercent;
        CProgressCtrl* m_pProgressCtrl;
    };

    CProgressCtrl m_progressOverall;
    CProgressCtrl m_progressCurrent;

    ProgressControls m_controls[2]; // overall and current
    bool m_bCancel;
    bool m_bParentDisabled;
};
