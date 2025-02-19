#pragma once


class GenerateVSDlg : public CDialog
{
public:
    GenerateVSDlg(const CDataDict& dictionary, const CDictItem& dict_item, CWnd* pParent = nullptr);

    DictValueSet CreateValueSet() const;

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnKillFocusInterval();

    void OnOK() override;

private:
    bool UsingToValues() const { return ( m_interval > m_minInterval ); }

    bool ValidateDefinedName();
    void ValidateFromTo(double& value, const char* const value_name) const;
    void ValidateInterval() const;
    void ValidateTemplate() const;

    struct FormattingOptions;
    std::tuple<std::string, std::string> GetFormattedValueAndLabel(FormattingOptions& options, double value) const;

private:
    const CDataDict& m_dictionary;
    const CDictItem& m_dictItem;
    std::string m_label;
    std::string m_name;
    double m_minValue;
    double m_maxValue;
    double m_minInterval;
    double m_from;
    double m_to;
    double m_interval;
    std::string m_template;
    const std::string& m_thousandsSeparator;
    std::optional<BOOL> m_useThousandsSeparator;
    int m_valueOrder;
};
