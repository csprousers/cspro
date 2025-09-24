#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepositoryDefines.h>


// --------------------------------------------------------------------------
// CaseIteratorSettings
// --------------------------------------------------------------------------

class ZDATAO_API CaseIteratorSettings
{
public:
    CaseIteratorSettings();

    CaseIteratorSettings(CaseIterationCaseStatus status,
                         std::optional<CaseIterationMethod> method = std::nullopt,
                         std::optional<CaseIterationOrder> order = std::nullopt,
                         std::optional<CaseIteratorParameters> parameters = std::nullopt);

    CaseIteratorSettings(CaseIterationCaseStatus status,
                         std::optional<CaseIterationMethod> method,
                         std::optional<CaseIterationOrder> order,
                         const CaseIteratorParameters* parameters);

    CaseIteratorSettings(const CaseIteratorSettings& rhs) = default;
    CaseIteratorSettings(CaseIteratorSettings&& rhs) noexcept = default;

    CaseIteratorSettings& operator=(const CaseIteratorSettings& rhs) = default;
    CaseIteratorSettings& operator=(CaseIteratorSettings&& rhs) noexcept = default;

    CaseIterationCaseStatus GetStatus() const      { return m_status; }
    void SetStatus(CaseIterationCaseStatus status) { m_status = status; }

    std::optional<CaseIterationMethod> GetMethod() const { return m_method; }
    CaseIterationMethod GetEvaluatedMethod() const       { return m_method.value_or(CaseIterationMethod::SequentialOrder); }
    void SetMethod(CaseIterationMethod method)           { m_method = method; }
    void ToggleMethod();

    std::optional<CaseIterationOrder> GetOrder() const { return m_order; }
    CaseIterationOrder GetEvaluatedOrder() const       { return m_order.value_or(CaseIterationOrder::Ascending); }
    void SetOrder(CaseIterationOrder order)            { m_order = order; }

    const CaseIteratorParameters* GetParameters() const                  { return m_parameters.has_value() ? &(*m_parameters) : nullptr; }
    void SetParameters(std::optional<CaseIteratorParameters> parameters) { m_parameters = std::move(parameters); }

    static CaseIteratorSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true) const;

private:
    CaseIterationCaseStatus m_status;
    std::optional<CaseIterationMethod> m_method;
    std::optional<CaseIterationOrder> m_order;
    std::optional<CaseIteratorParameters> m_parameters;
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

    bool GetViewFilters() const            { return m_viewFilters; }
    void SetViewFilters(bool view_filters) { m_viewFilters = view_filters; }

    static ViewableCaseIteratorSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

private:
    bool m_viewCaseLabel;
    bool m_viewFilters;
};
