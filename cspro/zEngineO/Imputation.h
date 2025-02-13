#pragma once

#include <zEngineO/zEngineO.h>

struct EngineData;
class Serializer;
class ValueSet;


class ZENGINEO_API Imputation
{
public:
    Imputation(const std::wstring& compilation_unit, size_t line_number);

    const std::wstring& GetCompilationUnit() const { return m_compilationUnit; }
                                                
    size_t GetLineNumber() const { return m_lineNumber; }
                                                        
    const VART* GetVariable() const        { return m_variable; }
    void SetVariable(const VART* variable) { m_variable = variable; }
                                                
    const std::optional<std::wstring>& GetTitle() const { return m_title; }
    void SetTitle(std::wstring title)                   { m_title = std::move(title); }
                                                                          
    bool GetSpecific() const { return m_specific; }
    void SetSpecific()       { m_specific = true; }
                                                        
    const ValueSet* GetValueSet() const         { return m_valueSet; }
    void SetValueSet(const ValueSet* value_set) { m_valueSet = value_set; }

    bool GetUsingStat() const { return m_usingStat; }
    void SetUsingStat()       { m_usingStat = true; }

    const std::vector<const VART*>& GetStatVariables() const       { return m_statVariables; }
    void SetStatVariables(std::vector<const VART*> stat_variables) { m_statVariables = std::move(stat_variables); }


    // runtime variables
    size_t GetImputationFrequencyIndex() const     { return m_imputationFrequencyIndex; }
    void SetImputationFrequencyIndex(size_t index) { m_imputationFrequencyIndex = index; }

    size_t GetStatRecordIndex() const     { return m_statRecordIndex; }
    void SetStatRecordIndex(size_t index) { m_statRecordIndex = index; }


    static void serialize(Serializer& ar, EngineData& engine_data);

private:
    std::wstring m_compilationUnit;
    size_t m_lineNumber;

    const VART* m_variable;

    std::optional<std::wstring> m_title;
    bool m_specific;
    const ValueSet* m_valueSet;
    bool m_usingStat;
    std::vector<const VART*> m_statVariables;

    size_t m_imputationFrequencyIndex;
    size_t m_statRecordIndex;
};
