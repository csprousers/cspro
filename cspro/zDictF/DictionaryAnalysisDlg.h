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

    void OnAnalysisTypeChange();

    void OnCopyToClipboard();

private:
    void RunAnalysis(const std::function<void(const CDictItem&)>& analysis_function,
                     const std::function<std::string()>& get_header_function);

    void OnWithoutValueSets(bool numerics_only);

    static void ThrowIfDiscreteValueFallsWithinRange(double discrete_value, const ValueSetResponse& range);
    static bool DoesValueSetHaveOverlappingRanges(const CDictItem& dict_item, const DictValueSet& dict_value_set);
    void OnNumericItemsOverlappingValueSets();

    void OnMismatchedDecCharZeroFill(bool dec_char); // vs. zero_fill

private:
    const CDataDict& m_dictionary;

    CListBox m_analysisTypeListBox;
    ReadOnlyEditCtrl m_resultsEditCtrl;

    std::string m_resultsForClipboard;
};
