#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiCondition.h>

namespace YAML { template <typename T> struct convert; }


class CLASS_DECL_ZCAPIO CapiQuestion
{
    friend struct YAML::convert<CapiQuestion>;

public:
    CapiQuestion(std::string item_name = std::string());

    const std::string& GetItemName() const  { return m_itemName; }
    void SetItemName(std::string item_name) { m_itemName = std::move(item_name); }

    const std::vector<CapiCondition>& GetConditions() const { return m_conditions; }
    std::vector<CapiCondition>& GetConditions()             { return m_conditions; }

    const CapiCondition* GetCondition(const std::string& logic, int min_occ = -1, int max_occ = -1) const;
    void SetCondition(CapiCondition condition);

    const std::map<std::string, int>& GetFillExpressions() const         { return m_fillExpressions; }
    void SetFillExpressions(std::map<std::string, int> fill_expressions) { m_fillExpressions = std::move(fill_expressions); }

    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    std::string m_itemName;
    std::vector<CapiCondition> m_conditions;
    std::map<std::string, int> m_fillExpressions;
};
