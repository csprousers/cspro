#pragma once

#include <zUtilF/zUtilF.h>

class ProgressDlg;


// ---------------------------
// --- ProgressDlgFactory
// ---------------------------
class CLASS_DECL_ZUTILF ProgressDlgFactory
{
private:
    ProgressDlgFactory() { }

public:
    static ProgressDlgFactory& Instance() { return m_instance; }

    std::shared_ptr<ProgressDlg> Create(UINT caption_id = 0);

    void StartSharing();
    void StopSharing();

private:
    std::shared_ptr<ProgressDlg> m_currentShared;

    static ProgressDlgFactory m_instance;
};


// ---------------------------
// --- ProgressDlgSharing
// ---------------------------
class CLASS_DECL_ZUTILF ProgressDlgSharing
{
public:
    ProgressDlgSharing();
    ~ProgressDlgSharing();
};
