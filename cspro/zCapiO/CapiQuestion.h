#pragma once

#include <zCapiO/zCapiO.h>
#include <zCapiO/CapiCondition.h>

class JsonWriter;
class Serializer;
namespace YAML { template <typename T> struct convert; }


class CLASS_DECL_ZCAPIO CapiQuestion
{
    friend struct YAML::convert<CapiQuestion>;

public:
    CapiQuestion(const CString& item_name = CString());

    const CString& GetItemName() const         { return m_itemName; }
    void SetItemName(const CString& item_name) { m_itemName = item_name; }

    const std::vector<CapiCondition>& GetConditions() const { return m_conditions; }
    std::vector<CapiCondition>& GetConditions()             { return m_conditions; }

    const CapiCondition* GetCondition(const CString& logic, int min_occ = -1, int max_occ = -1) const;
    void SetCondition(CapiCondition condition);

    const std::map<CString, int>& GetFillExpressions() const         { return m_fillExpressions; }
    void SetFillExpressions(std::map<CString, int> fill_expressions) { m_fillExpressions = std::move(fill_expressions); }


    // serialization
    // --------------------------------------------------
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);


private:
    CString m_itemName;
    std::vector<CapiCondition> m_conditions;
    std::map<CString, int> m_fillExpressions;
};
