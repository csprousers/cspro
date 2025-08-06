#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiCondition.h>

namespace YAML { template<typename T> struct convert; }


class CLASS_DECL_ZCAPIO CapiQuestion
{
    friend struct YAML::convert<CapiQuestion>;

public:
    CapiQuestion(std::string item_name = std::string());

    const std::string& GetItemName() const  { return m_itemName; }
    void SetItemName(std::string item_name) { m_itemName = std::move(item_name); }

    const std::vector<CapiCondition>& GetConditions() const { return m_conditions; }
    std::vector<CapiCondition>& GetConditions()             { return m_conditions; }
    void SetCondition(CapiCondition condition);

    bool IsDefined() const;

    const std::map<std::string, int>* GetPre81FillExpressions() const { return m_pre81FillExpressions.get(); }

    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);

private:
    std::string m_itemName;
    std::vector<CapiCondition> m_conditions;
    std::shared_ptr<std::map<std::string, int>> m_pre81FillExpressions;
};
