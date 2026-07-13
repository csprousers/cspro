#pragma once

#include <zUtilO/ResizableDlg.h>
#include <zEditO/ReadOnlyEditCtrl.h>

class ValueSetResponse;


class DictionaryAnalysisDlg : public DynamicLayoutResizableDlg
{
public:
    DictionaryAnalysisDlg(const CDataDict& dictionary, CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    std::vector<std::tuple<CWnd*, SizingDirection>> GetDynamicLayoutControls() override;

    void OnAnalysisTypeChange(NMHDR* pNMHDR, LRESULT* pResult);
    void OnAnalysisOrderChange();

    void OnCopyToClipboard();

private:
    HTREEITEM PopulateAnalysisTypes();

    void SetResultsText(const std::string& results_text);

    void RunAnalysis();
    void RunAnalysis(const std::function<void(const CDictItem&)>& analysis_function,
                     const std::function<std::string()>& get_header_function);

    static std::string GetValueSetDisplayText(const CDictItem& dict_item, const DictValueSet& dict_value_set);

    void OnWithoutValueSets(bool numerics_only);

    static void ThrowIfDiscreteValueFallsWithinRange(double discrete_value, const ValueSetResponse& range);
    static bool DoesValueSetHaveOverlappingRanges(const CDictItem& dict_item, const DictValueSet& dict_value_set);
    void OnNumericItemsOverlappingValueSets();

    void OnMismatchedDecCharZeroFill(bool dec_char); // vs. zero_fill

    void OnLinkedValueSets();
    void OnLinkedValueSetsCandidates();
    static bool ValueSetValuesMatch(const DictValueSet& dict_value_set1, const DictValueSet& dict_value_set2);

    void OnValueSetsUsingSpecials(std::optional<double> special_value);

private:
    const CDataDict& m_dictionary;

    CTreeCtrl m_analysisTypeTreeCtrl;
    int m_analysisType;
    int m_analysisOrder;
    ReadOnlyEditCtrl m_resultsEditCtrl;

    std::vector<std::string> m_resultRows;
    std::string m_resultsTextForClipboard;
};
