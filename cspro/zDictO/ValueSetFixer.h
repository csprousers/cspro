#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DDClass.h>


// a class to fix value sets based on item attributes

class CLASS_DECL_ZDICTO ValueSetFixer
{
public:
    ValueSetFixer(const CDictItem& dict_item, TCHAR decimal_char = 0);

    void Fix(std::vector<DictValueSet>& dict_value_sets);
    void Fix(DictValueSet& dict_value_set);
    void Fix(DictValue& dict_value);
    void Fix(DictValuePair& dict_value_pair);

private:
    const CDictItem& m_dictItem;
    TCHAR m_decimalChar;
    std::unique_ptr<TCHAR[]> m_dtoaSpace;

    static std::optional<bool> m_zeroBeforeDecimal;
};
