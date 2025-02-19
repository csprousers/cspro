#pragma once

#include <zUtilF/zUtilF.h>

class ProgressDlg;
class ProgressDlgSharing;


// --------------------------------------------------------------------------
// ProgressDlgFactory
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILF ProgressDlgFactory
{
    friend ProgressDlgSharing;

private:
    ProgressDlgFactory() { }

public:
    static ProgressDlgFactory& Instance() { return m_instance; }

    std::shared_ptr<ProgressDlg> Create(UINT caption_id = 0);

private:
    void StartSharing();
    void StopSharing();

private:
    std::shared_ptr<ProgressDlg> m_currentShared;

    static ProgressDlgFactory m_instance;
};



// --------------------------------------------------------------------------
// ProgressDlgSharing
//
// This RAII class results in a single progress bar dialog showing even if
// routines try to create multiple progress bar dialogs.
// --------------------------------------------------------------------------

class ProgressDlgSharing
{
public:
    ProgressDlgSharing()  { ProgressDlgFactory::Instance().StartSharing(); }
    ~ProgressDlgSharing() { ProgressDlgFactory::Instance().StopSharing(); }
};
