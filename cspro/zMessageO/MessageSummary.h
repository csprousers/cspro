#pragma once


struct MessageSummary
{
    enum class Type { System, UserNumbered, UserUnnumbered };

    Type type;
    int message_number;
    std::wstring message_text;
    size_t frequency;
    std::optional<double> denominator;
};
