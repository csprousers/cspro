#include "StdAfx.h"
#include "GenerateVSDlg.h"
#include <zDictO/ValueProcessor.h>


BEGIN_MESSAGE_MAP(GenerateVSDlg, CDialog)
    ON_EN_KILLFOCUS(IDC_GET_VSET_INTERVAL, OnKillFocusInterval)
END_MESSAGE_MAP()


namespace
{
    constexpr double MaxNumIntervals = 10000;

    constexpr std::string_view DefaultTemplateFrom_sv   = "%s";
    constexpr std::string_view DefaultTemplateFromTo_sv = "%s - %s";
}


GenerateVSDlg::GenerateVSDlg(const CDataDict& dictionary, const CDictItem& dict_item, CWnd* const pParent /* = nullptr*/)
    :   CDialog(IDD_GENERATE_VSET, pParent),
        m_dictionary(dictionary),
        m_dictItem(dict_item),
        m_from(0),
        m_thousandsSeparator(GetLocaleInformation(LOCALE_STHOUSAND)),
        m_useThousandsSeparator(( m_dictItem.GetIntegerLen() > 3 ) ? std::make_optional(TRUE) : std::nullopt),
        m_valueOrder(0)
{
    ASSERT(IsNumeric(m_dictItem));
    const std::shared_ptr<const ValueProcessor> value_processor = ValueProcessor::CreateValueProcessor(m_dictItem);
    const NumericValueProcessor* const numeric_value_processor = assert_cast<const NumericValueProcessor*>(value_processor.get());

    // Initialize Label and Name
    m_label = UTF8_TODO::GetUtf8(m_dictItem.GetLabel());

    m_name = FormatText("%s_VS%d", m_dictItem.GetName().c_str(), static_cast<int>(m_dictItem.GetNumValueSets() + 1));
    m_name = m_dictionary.GetUniqueName(m_name);

    // Calculate Min and Max values and Min Interval
    m_minValue = numeric_value_processor->GetMinValue();
    m_maxValue = numeric_value_processor->GetMaxValue();
    m_minInterval = pow(10.0, -1.0 * m_dictItem.GetDecimal());

    // Initialize To and Interval
    m_to = m_maxValue;
    m_interval = pow(10.0, m_dictItem.GetIntegerLen() - 1);

    // Initialize Template
    m_template = ( m_interval == m_minInterval ) ? DefaultTemplateFrom_sv :
                                                   DefaultTemplateFromTo_sv;
}


void GenerateVSDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_GEN_VSET_LABEL, m_label);
    DDV_MaxChars(pDX, m_label, MAX_LABEL_LEN);
    DDX_Text(pDX, IDC_GEN_VSET_NAME, m_name);
    DDX_Text(pDX, IDC_GEN_VSET_FROM, m_from);
    DDX_Text(pDX, IDC_GEN_VSET_TO, m_to);
    DDX_Text(pDX, IDC_GET_VSET_INTERVAL, m_interval);
    DDX_Text(pDX, IDC_GEN_VSET_TEMPLATE, m_template);

    if( m_useThousandsSeparator.has_value() )
        DDX_Check(pDX, IDC_GEN_VSET_USE_THOUSANDS_SEPARATOR, *m_useThousandsSeparator);

    DDX_CBIndex(pDX, IDC_GEN_VSET_VALUE_ORDER, m_valueOrder);
}


BOOL GenerateVSDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    CWnd* const thousands_wnd = GetDlgItem(IDC_GEN_VSET_USE_THOUSANDS_SEPARATOR);
    WindowsUtf8::SetText(thousands_wnd, FormatText("Use 1000 separator (%s)", m_thousandsSeparator.c_str()));

    if( !m_useThousandsSeparator.has_value() )
        thousands_wnd->EnableWindow(FALSE);

    return result;
}


void GenerateVSDlg::OnKillFocusInterval()
{
    // if using the default formatter, modify the template if the interval changes accordingly
    UpdateData(TRUE);

    const std::string_view* const template_to_use =
        UsingToValues() ? ( ( m_template == DefaultTemplateFrom_sv   ) ? &DefaultTemplateFromTo_sv : nullptr ) :
                          ( ( m_template == DefaultTemplateFromTo_sv ) ? &DefaultTemplateFrom_sv   : nullptr );

    if( template_to_use != nullptr )
    {
        m_template = *template_to_use;
        UpdateData(FALSE);
    }
}


void GenerateVSDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( SO::IsBlank(m_label) )
            throw CSProException("You must specify a value set label.");

        if( SO::IsBlank(m_name) )
            throw CSProException("You must specify a value set name.");

        if( !ValidateDefinedName() )
            return;

        ValidateFromTo(m_from, "From");
        ValidateFromTo(m_to, "To");

        ValidateInterval();

        ValidateTemplate();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return;
    }

    __super::OnOK();
}


bool GenerateVSDlg::ValidateDefinedName()
{
    ASSERT(!SO::IsBlank(m_name));

    const std::string valid_name = CIMSAString::MakeName(m_name);
    std::string valid_and_unique_name = m_dictionary.GetUniqueName(valid_name);

    if( m_name == valid_and_unique_name )
        return true;

    std::string message = ( m_name != valid_name ) ?
        FormatText("The name '%s' is not a valid name.", m_name.c_str()) :
        FormatText("The name '%s' is not unique in the dictionary '%s.", m_name.c_str(), m_dictionary.GetName().c_str());

    message.append(FormatText(" Do you want to use the suggested name '%s'?", valid_and_unique_name.c_str()));

    if( AfxMessageBox(message, MB_YESNO) != IDYES )
        return false;

    m_name = std::move(valid_and_unique_name);

    CDataExchange dx(this, FALSE);
    DDX_Text(&dx, IDC_GEN_VSET_NAME, m_name);

    return true;
}


void GenerateVSDlg::ValidateFromTo(double& value, const char* const value_name) const
{
    ASSERT(&value == &m_from || &value == &m_to);

    if( value < m_minValue )
    {
        throw CSProException("%s value (%s) is too small. The minimum value is: %s",
                             value_name, DoubleToString(value).c_str(), DoubleToString(m_minValue).c_str());
    }

    if( value > m_maxValue )
    {
        throw CSProException("%s value (%s) is too large. The maximum value is: %s",
                             value_name, DoubleToString(value).c_str(), DoubleToString(m_maxValue).c_str());
    }

    if( m_from >= m_to )
    {
        if( &value == &m_from )
        {
            throw CSProException("From value (%s) must be less than To value (%s).",
                                 DoubleToString(m_from).c_str(), DoubleToString(m_to).c_str());
        }

        else
        {
            throw CSProException("To value (%s) must be greater than From value (%s).",
                                 DoubleToString(m_to).c_str(), DoubleToString(m_from).c_str());
        }
    }
}


void GenerateVSDlg::ValidateInterval() const
{
    ASSERT(m_from < m_to);

    if( m_interval <= 0 )
    {
        throw CSProException("The interval (%s) must be greater than 0.",
                             DoubleToString(m_interval).c_str());
    }

    if( m_interval < m_minInterval )
    {
        throw CSProException("The interval (%s) is too small. The minimum value is: %s",
                             DoubleToString(m_interval).c_str(),
                             DoubleToString(m_minInterval).c_str());
    }

    const double max_calculated_value = m_to - m_from + m_minInterval;

    if( m_interval > max_calculated_value )
    {
        throw CSProException("The interval (%s) is too large, resulting in no values between %s and %s.",
                             DoubleToString(m_interval).c_str(),
                             DoubleToString(m_from).c_str(),
                             DoubleToString(m_to).c_str());
    }

    const double num_intervals = max_calculated_value / m_interval;

    if( num_intervals > MaxNumIntervals )
    {
        throw CSProException("The interval (%s) results in too many values (" Formatter_uint64_t "). The maximum number of values is %0.f.",
                             DoubleToString(m_interval).c_str(),
                             static_cast<uint64_t>(std::ceil(num_intervals)),
                             MaxNumIntervals);
    }
}


void GenerateVSDlg::ValidateTemplate() const
{
    if( SO::IsBlank(m_template) )
        throw CSProException("You must specify a value label template.");

    std::string_view template_sv = m_template;
    const int max_percent_s_formatters = UsingToValues() ? 2 : 1;
    int percent_s_formatters_remaining = max_percent_s_formatters;
    size_t percent_pos;

    while( ( percent_pos = template_sv.find('%') ) != std::string_view::npos )
    {
        template_sv.remove_prefix(percent_pos + 1);

        const char formatter = !template_sv.empty() ? template_sv.front() : '\0';

        if( formatter == 's' )
        {
            if( percent_s_formatters_remaining-- == 0 )
            {
                throw CSProException("The value label template is invalid because it can only use %d %%s formatter%s.",
                                     max_percent_s_formatters, PluralizeWord(max_percent_s_formatters));
            }
        }

        else if( formatter != '%' )
        {
            throw CSProException("The value label template formatter is invalid starting at: %%%s",
                                 std::string(template_sv).c_str());
        }

        template_sv.remove_prefix(1);
    }
}


struct GenerateVSDlg::FormattingOptions
{
    bool use_leading_zero;
    char decimal_ch;
    bool indic_groupings;
    wchar_t buffer[30];
};


DictValueSet GenerateVSDlg::CreateValueSet() const
{
    DictValueSet dict_value_set;
    dict_value_set.SetName(m_name);
    dict_value_set.SetLabel(UTF8_TODO::GetCString(m_label));

    // format values based on the locale
    FormattingOptions options
    {
        ( GetLocaleInformation(LOCALE_ILZERO) == "1" ),
        CIMSAString::GetDecChar(),
        ( GetLocaleInformation(LOCALE_SGROUPING) == "3;2;0" )
    };

    const bool using_to_values = UsingToValues();

    for( double lower = m_from; lower <= m_to; lower += m_interval )
    {
        std::tuple<std::string, std::string> from_value_and_label = GetFormattedValueAndLabel(options, lower);
        std::optional<std::tuple<std::string, std::string>> to_value_and_label;

        if( using_to_values )
        {
            const double upper = std::min(m_to, lower + m_interval - m_minInterval);
            to_value_and_label = GetFormattedValueAndLabel(options, upper);
        }

        DictValue dict_value;
        dict_value.SetLabel(UTF8_TODO::GetCString(FormatText(m_template.c_str(),
                                                             std::get<1>(from_value_and_label).c_str(),
                                                             to_value_and_label.has_value() ? std::get<1>(*to_value_and_label).c_str() : "")));

        dict_value.AddValuePair(DictValuePair(std::move(std::get<0>(from_value_and_label)),
                                              to_value_and_label.has_value() ? std::move(std::get<0>(*to_value_and_label)) : std::string()));

        dict_value_set.AddValue(std::move(dict_value));
    }

    if( m_valueOrder == 1 )
        dict_value_set.ReverseValues();

    auto temp = dict_value_set.GetMinMax();temp;
    ASSERT(dict_value_set.GetMinMax() == std::make_tuple(m_from, m_to));

    return dict_value_set;
}


std::tuple<std::string, std::string> GenerateVSDlg::GetFormattedValueAndLabel(FormattingOptions& options, const double value) const
{
    std::string text_value = UTF8_TODO::GetUtf8(dtoa(value, options.buffer, m_dictItem.GetDecimal(), options.decimal_ch, options.use_leading_zero));
    std::string label = text_value;

    if( m_useThousandsSeparator == TRUE )
    {
        ASSERT(!label.empty() && label.length() == SO::WideLength(label));

        // start processing at the integer portion
        const size_t first_digit_pos = ( label.front() == '-' ) ? 1 : 0;
        size_t digit_itr_pos = std::min(label.length(), label.find('.')) - 1;
        int digits_until_next_separator = 3;

        ASSERT(digit_itr_pos >= first_digit_pos);
        ASSERT(std::isdigit(label[digit_itr_pos]));

        for( ; digit_itr_pos != first_digit_pos; --digit_itr_pos )
        {
            ASSERT(std::isdigit(label[digit_itr_pos]));

            if( --digits_until_next_separator == 0 )
            {
                label.insert(digit_itr_pos, m_thousandsSeparator);
                digits_until_next_separator = options.indic_groupings ? 2 : 3;
            }
        }
    }

    return { std::move(text_value), std::move(label) };
}
