#pragma once

#include <zEditO/LogicView.h>
#include <zUtilO/TextSourceEditable.h>


class LogicView::TextSourceModifier : public TextSourceEditable::SourceModifier
{
public:
    // The text source modifier will be set to this class and then unregistered on destruction.
    TextSourceModifier(LogicView& logic_view, std::shared_ptr<TextSourceEditable> text_source)
        :   m_logicView(logic_view),
            m_textSourceEditable(std::move(text_source))
    {
        ASSERT(m_textSourceEditable != nullptr);
        m_textSourceEditable->SetSourceModifier(this);
    }

    ~TextSourceModifier() noexcept
    {
        m_textSourceEditable->SetSourceModifier(nullptr);
    }

    void SetModified(const bool modified)
    {
        m_textSourceEditable->SetModified(modified);
    }

protected:
    void SyncTextSource() override
    {
        m_textSourceEditable->SetText(m_logicView.GetLogicCtrl()->GetText());
    }

    void OnTextSourceSave() override
    {
        m_logicView.GetLogicCtrl()->SetSavePoint();
    }

private:
    LogicView& m_logicView;
    std::shared_ptr<TextSourceEditable> m_textSourceEditable;
};
