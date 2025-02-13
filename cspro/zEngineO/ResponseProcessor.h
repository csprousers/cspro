#pragma once

#include <zEngineO/zEngineO.h>
#include <zToolsO/CSProException.h>
#include <zDictO/ValueSetResponse.h>

class ValueProcessor;


class ZENGINEO_API ResponseProcessor
{
public:
    CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(SelectionError, "Selection Error");

    ResponseProcessor(std::shared_ptr<const ValueProcessor> value_processor);

    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetResponses() const;
    const std::vector<std::shared_ptr<const ValueSetResponse>>& GetUnfilteredResponses() const { return m_responses; }

    bool ResponsesIncludeRefused() const { return m_valueSetContainsRefused; }
    bool IsRefusedValueHidden() const    { return ( m_valueSetContainsRefused && !m_filteredResponsesShowRefused ); }

    void ShowRefusedValue(bool show_refused_value);
    void AlwaysShowRefusedValue();

    void ApplyCaptureTypeProperties(CaptureType capture_type);

    void RemoveRangeResponses(bool remove_range_responses);

    void SetCanEnterNotAppl(bool add_notappl);

    // for radio buttons, drop downs, combo boxes, and toggle buttons
    size_t GetResponseIndex(const CString& value) const;

    CString GetInputFromResponseIndex(size_t index) const;
    double GetNumericInputFromResponseIndex(size_t index) const;

    // for checkboxes
    int GetCheckboxWidth() const;
    size_t GetCheckboxMaxSelections() const;

    std::vector<size_t> GetCheckboxResponseIndices(const CString& value) const;

    CString GetInputFromCheckboxIndices(const std::vector<size_t>& indices) const;

    // other operations
    void ResetResponses();

    void RandomizeResponses(const std::set<const DictValue*>& values_to_exclude);

    void SortResponses(bool ascending, bool sort_by_label);

private:
    void GenerateFilteredResponses();

    size_t GetValueSetResponseIndex(const DictValue& dict_value) const;
    std::shared_ptr<const ValueSetResponse> GetValueSetResponseFromIndex(size_t index) const;

    void DoCheckboxCalculations() const;

private:
    struct CheckboxCalculations
    {
        int checkbox_width;
        size_t max_selections;
    };

    std::shared_ptr<const ValueProcessor> m_valueProcessor;
    std::vector<std::shared_ptr<const ValueSetResponse>> m_responses;
    mutable std::optional<CheckboxCalculations> m_checkboxCalculations;

    bool m_valueSetContainsNotApplOrBlank;
    bool m_valueSetContainsRefused;
    std::optional<bool> m_valueSetContainsRanges;

    bool m_filteredResponsesShowRefused;
    bool m_filteredResponsesShowRefusedOverride;
    bool m_filteredResponsesHaveBlanksRemoved;
    bool m_filteredResponsesHaveRangesRemoved;
    std::shared_ptr<const ValueSetResponse> m_addedNotApplResponse;

    std::unique_ptr<std::vector<std::shared_ptr<const ValueSetResponse>>> m_filteredResponses;
    bool m_responsesNeedResetting;
};
