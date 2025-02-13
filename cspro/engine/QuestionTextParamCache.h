#pragma once

#include <zCapiO/CapiText.h>

struct ParsedCapiParam
{
    enum class ParamType { GetOccLabel, GetValueLabel, VariableOrUserFunction };

    ParamType m_eParamType;
    CString m_csTextToReplace;
    int m_iSymbolVar;
    bool m_bIsOccSymbol;
    int m_iOccSymbolVarOrCte; // Symbol or Cte.

    ParsedCapiParam(ParamType eParamType)
        : m_eParamType(eParamType),
        m_iSymbolVar(0),
        m_bIsOccSymbol(false),
        m_iOccSymbolVarOrCte(0)
    {
    }

    ParsedCapiParam()
        : ParsedCapiParam(ParamType::VariableOrUserFunction)
    {
    }
};

class QuestionTextParamCache {
public:

    const std::vector<ParsedCapiParam>* Get(const CapiText& text) const;
    void Put(const CapiText& text, std::vector<ParsedCapiParam> params);

private:
    std::map<CString, std::vector<ParsedCapiParam>> m_cache;
};
