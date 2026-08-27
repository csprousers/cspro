#pragma once


struct RuntimeMessage
{
    struct SelectButtons
    {
        std::vector<SharableString> button_texts;
        std::optional<size_t> default_button_number; // zero-based
    };

    int message_number;
    int message_number_for_display; // the message number, or the negative line number for unnumbered messages
    SharableString message_text;
    std::unique_ptr<SelectButtons> select_buttons;
};
