#include "StdAfx.h"
#include "ProgressDlgFactory.h"
#include "ProgressDlg.h"


// ---------------------------
// --- ProgressDlgFactory
// ---------------------------
ProgressDlgFactory ProgressDlgFactory::m_instance;

std::shared_ptr<ProgressDlg> ProgressDlgFactory::Create(UINT caption_id/* = 0*/)
{
    std::shared_ptr<ProgressDlg> dlg = m_currentShared;

    if( dlg == nullptr )
    {
        dlg = std::shared_ptr<ProgressDlg>(new ProgressDlg());
        dlg->Create();
    }

    if( caption_id != 0 )
        dlg->SetCaption(caption_id);

    return dlg;
}

void ProgressDlgFactory::StartSharing()
{
    m_currentShared = std::shared_ptr<ProgressDlg>(new ProgressDlg());
    m_currentShared->Create();
}

void ProgressDlgFactory::StopSharing()
{
    m_currentShared.reset();
}


// ---------------------------
// --- ProgressDlgSharing
// ---------------------------
ProgressDlgSharing::ProgressDlgSharing()
{
    ProgressDlgFactory::Instance().StartSharing();
}

ProgressDlgSharing::~ProgressDlgSharing()
{
    ProgressDlgFactory::Instance().StopSharing();
}
