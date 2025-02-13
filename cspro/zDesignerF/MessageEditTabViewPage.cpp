#include "StdAfx.h"
#include "MessageEditTabViewPage.h"


MessageEditTabViewPage::MessageEditTabViewPage(ApplicationChildWnd* application_child_wnd)
    :   m_applicationChildWnd(application_child_wnd)
{
    ASSERT(application_child_wnd != nullptr);
}


MessageEditTabViewPage::~MessageEditTabViewPage()
{
    if( m_textSourceEditable != nullptr )
        m_textSourceEditable->SetSourceModifier(nullptr);
}


void MessageEditTabViewPage::OnTabChange()
{
    // get the message text source if it hasn't already been retrieved
    if( m_textSourceEditable != nullptr )
        return;

    CDocument* active_document = m_applicationChildWnd->GetActiveDocument();

    if( active_document != nullptr &&
        WindowsDesktopMessage::Send(UWM::Designer::GetMessageTextSource, active_document, &m_textSourceEditable) == 1 )
    {
        SetText(m_textSourceEditable->GetText());
        EmptyUndoBuffer();

        m_textSourceEditable->SetSourceModifier(this);
    }
}


void MessageEditTabViewPage::SetModified(bool modified/* = true*/)
{
    MessageEditCtrl::SetModified(modified);

    if( modified )
        m_textSourceEditable->SetModified();
}


void MessageEditTabViewPage::SyncTextSource()
{
    m_textSourceEditable->SetText(GetText());
}


void MessageEditTabViewPage::OnTextSourceSave()
{
    SetModified(false);
}
