#pragma once

#include <zParadataO/Event.h>


namespace Paradata
{
    class ZPARADATAO_API OperatorSelectionEvent : public Event
    {
        DECLARE_PARADATA_EVENT(OperatorSelectionEvent)

    public:
        enum class Source
        {
            Errmsg,
            Accept,
            Prompt,
            Userbar,
            SelCase,
            Show,
            ShowArray,
            ListShow,
            MapShow,
            ValueSetShow,
            BarcodeRead
        };

        OperatorSelectionEvent(Source source);

        void SetPostSelectionValues(std::optional<int> selection_number, std::optional<std::wstring> selection_text, bool set_display_duration);

    private:
        Source m_source;
        std::optional<int> m_selectionNumber;
        std::optional<std::wstring> m_selectionText;
        std::optional<double> m_displayDuration;
    };
}
