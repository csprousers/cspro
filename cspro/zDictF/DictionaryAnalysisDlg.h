#pragma once


class DictionaryAnalysisDlg
{
public:
    DictionaryAnalysisDlg(const CDataDict& dictionary);

    void DoModal();

private:
    void RunDictAnalysis(const std::function<void(const CDictItem&)>& analysis_function,
                         const std::function<bool(std::string&, std::string&)>& summary_results_function);
    void RunDictAnalysisNoValueSets(bool numerics_only);

    void OnDictAnalysisItemsNoValueSets();
    void OnDictAnalysisNumericItemsNoValueSets();
    void OnDictAnalysisNumericItemsOverlappingValueSets();
    void OnDictAnalysisNumericMismatchedZeroFillDecChar();

private:
    const CDataDict& m_dictionary;
};
