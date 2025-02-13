#pragma once


struct CapiLogicLocation
{
    size_t condition_index;
    std::optional<std::wstring> language_label;
};


struct CapiLogicParameters
{
    enum class Type { Condition, Fill };

    Type type;
    std::variant<int, CString> symbol_index_or_name;
    std::wstring logic;
    CapiLogicLocation capi_logic_location;
};
