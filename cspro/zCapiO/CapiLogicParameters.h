#pragma once


struct CapiLogicLocation
{
    size_t condition_index;
    std::optional<std::string> language_label;
};


struct CapiLogicParameters
{
    enum class Type { Condition, Fill };

    Type type;
    std::variant<int, std::string> symbol_index_or_name;
    SharableString logic;
    CapiLogicLocation capi_logic_location;
};
