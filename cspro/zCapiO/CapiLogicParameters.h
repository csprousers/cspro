#pragma once

class CapiCondition;
class CapiText;


constexpr const char* QuestionTextStringWriterName = "QSF";


struct CapiLogicLocation
{
    size_t condition_index;
    std::optional<std::string> language_label;
};


struct CapiLogicParameters
{
    std::variant<int, std::string> symbol_index_or_name;
    std::variant<const CapiCondition*, const CapiText*> condition_or_text; // non-null
    CapiLogicLocation capi_logic_location;
};
