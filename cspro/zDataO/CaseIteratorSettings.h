#pragma once

#include <zDataO/zDataO.h>


// --------------------------------------------------------------------------
// CaseIteratorSettings
// --------------------------------------------------------------------------

class ZDATAO_API CaseIteratorSettings
{
public:
    CaseIteratorSettings();

    CaseIteratorSettings(const CaseIteratorSettings& rhs);
    CaseIteratorSettings(CaseIteratorSettings&& rhs) = default;

    CaseIteratorSettings& operator=(const CaseIteratorSettings& rhs);
    CaseIteratorSettings& operator=(CaseIteratorSettings&& rhs) = default;

    CaseIterationCaseStatus GetStatus() const      { return m_status; }
    void SetStatus(CaseIterationCaseStatus status) { m_status = status; }

    std::optional<CaseIterationMethod> GetMethod() const { return m_method; }
    CaseIterationMethod GetEvaluatedMethod() const       { return m_method.value_or(CaseIterationMethod::SequentialOrder); }
    void SetMethod(CaseIterationMethod method)           { m_method = method; }
    void ToggleMethod();

    std::optional<CaseIterationOrder> GetOrder() const { return m_order; }
    CaseIterationOrder GetEvaluatedOrder() const       { return m_order.value_or(CaseIterationOrder::Ascending); }
    void SetOrder(CaseIterationOrder order)            { m_order = order; }

    const CaseIteratorParameters* GetParameters() const { return m_parameters.get(); }

    static CaseIteratorSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;

private:
    CaseIterationCaseStatus m_status;
    std::optional<CaseIterationMethod> m_method;
    std::optional<CaseIterationOrder> m_order;
    std::unique_ptr<CaseIteratorParameters> m_parameters;
};



// --------------------------------------------------------------------------
// ViewableCaseIteratorSettings
// --------------------------------------------------------------------------

class ZDATAO_API ViewableCaseIteratorSettings : public CaseIteratorSettings
{
public:
    ViewableCaseIteratorSettings();
    ViewableCaseIteratorSettings(CaseIteratorSettings settings);

    bool GetViewCaseKey() const                    { return !m_viewCaseLabel; }
    bool GetViewCaseLabel() const                  { return m_viewCaseLabel; }
    void SetViewCaseKeyLabel(bool view_case_label) { m_viewCaseLabel = view_case_label; }
    void ToggleViewCaseKeyLabel()                  { m_viewCaseLabel = !m_viewCaseLabel; }

    static ViewableCaseIteratorSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

private:
    bool m_viewCaseLabel;
};
