#include "stdafx.h"
#include "MessageEditView.h"


IMPLEMENT_DYNAMIC(MessageEditView, LogicView)


MessageEditView::MessageEditView() noexcept
    :   m_designerDoc(nullptr)
{
}


void MessageEditView::OnUpdate(CView* const pSender, const LPARAM lHint, CObject* const pHint)
{
    __super::OnUpdate(pSender, lHint, pHint);

    switch( lHint )
    {
        case Hint::DesignerDocLoaded:
        {
            m_designerDoc = assert_cast<DesignerDoc*>(pHint);
            OnDesignerDocLoaded();
            break;
        }

        case Hint::LogicSettingsChanged:
        {
            OnLogicSettingsChanged();
            break;
        }
    }
}


void MessageEditView::OnDesignerDocLoaded()
{
    ASSERT(m_designerDoc != nullptr);

    // get the text source for the messages
    const std::shared_ptr<TextSourceEditable> text_source = m_designerDoc->GetMessageTextSource();
    ASSERT(text_source != nullptr);

    // use a text source modifier to manage the modification flag and to keep the text source's buffer in sync
    RegisterTextSourceModifier(text_source);

    // set up the editor based on the version of messages being used
    const int lexer_language = Lexers::GetLexer_Message(m_designerDoc->GetLogicSettings());
    m_logicCtrl->BaseInitControl(lexer_language);

    // do not show line numbers
    m_logicCtrl->SetMargins(0);

    // set the message text
    m_logicCtrl->SetTextAndSetSavePoint(text_source->GetText());
    m_logicCtrl->EmptyUndoBuffer();
}


void MessageEditView::OnLogicSettingsChanged()
{
    ASSERT(m_designerDoc != nullptr);

    const int lexer_language = Lexers::GetLexer_Message(m_designerDoc->GetLogicSettings());
    m_logicCtrl->ToggleLexer(lexer_language);
    m_logicCtrl->ColorizeEntireDocument();
}
