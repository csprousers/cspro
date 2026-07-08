#include "StdAfx.h"
#include "DictionaryAnalysisDlg.h"
#include <zUtilO/DynamicLayoutControlResizer.h>
#include <zUtilO/WindowHelpers.h>
#include <zDictO/ValueSetResponse.h>


namespace Selection
{
    constexpr int ItemsWithoutValueSets = 0;
    constexpr int NumericItemsWithoutValueSets = 1;
    constexpr int NumericItemsWithOverlappingValueSets = 2;
    constexpr int ItemsWithMismatchedDecCharOptions = 3;
    constexpr int ItemsWithMismatchedZeroFillOptions = 4;
}


BEGIN_MESSAGE_MAP(DictionaryAnalysisDlg, DynamicLayoutResizableDlg)
    ON_LBN_SELCHANGE(IDC_ANALYSIS_TYPE, OnAnalysisTypeChange)
    ON_COMMAND(IDC_COPY_TO_CLIPBOARD, OnCopyToClipboard)
END_MESSAGE_MAP()


DictionaryAnalysisDlg::DictionaryAnalysisDlg(const CDataDict& dictionary, CWnd* const pParent/* = nullptr*/)
    :   DynamicLayoutResizableDlg(IDD_DICTIONARY_ANALYSIS, pParent),
        m_dictionary(dictionary)
{
    SerializeDialogSize("DictionaryAnalysisDlg");
}


void DictionaryAnalysisDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_ANALYSIS_TYPE, m_analysisTypeListBox);
    DDX_Control(pDX, IDC_RESULTS, m_resultsEditCtrl);
}


BOOL DictionaryAnalysisDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    // add the analysis types
    m_analysisTypeListBox.AddString(L"Items without value sets");
    m_analysisTypeListBox.AddString(L"Numeric items without value sets");
    m_analysisTypeListBox.AddString(L"Numeric items with overlapping value sets");
    m_analysisTypeListBox.AddString(L"Items with mismatched DecChar options");
    m_analysisTypeListBox.AddString(L"Items with mismatched ZeroFill options");

    // set up the read-only Scintilla control to show the results
    m_resultsEditCtrl.ReplaceCEdit(this, false, false, SCLEX_NULL);
    m_resultsEditCtrl.SetWrapMode(Scintilla::Wrap::WhiteSpace);

    // run the analysis for the first item
    m_analysisTypeListBox.SetCurSel(Selection::ItemsWithoutValueSets);
    PostMessage(WM_COMMAND,
                MAKEWPARAM(IDC_ANALYSIS_TYPE, LBN_SELCHANGE),
                reinterpret_cast<LPARAM>(m_analysisTypeListBox.m_hWnd));

    return TRUE;
}


std::vector<std::tuple<CWnd*, SizingDirection>> DictionaryAnalysisDlg::GetDynamicLayoutControls()
{
    return { { &m_resultsEditCtrl, SizingDirection::XY } };
}


void DictionaryAnalysisDlg::OnAnalysisTypeChange()
{
    switch( m_analysisTypeListBox.GetCurSel() )
    {
        case Selection::ItemsWithoutValueSets:
            return OnWithoutValueSets(false);

        case Selection::NumericItemsWithoutValueSets:
            return OnWithoutValueSets(true);

        case Selection::NumericItemsWithOverlappingValueSets:
            return OnNumericItemsOverlappingValueSets();

        case Selection::ItemsWithMismatchedDecCharOptions:
            return OnMismatchedDecCharZeroFill(true);

        case Selection::ItemsWithMismatchedZeroFillOptions:
            return OnMismatchedDecCharZeroFill(false);

        default:
            ASSERT(false);
    }
}


void DictionaryAnalysisDlg::OnCopyToClipboard()
{
    ASSERT(!m_resultsForClipboard.empty());

    WinClipboard::PutText(this, m_resultsForClipboard);
}


void DictionaryAnalysisDlg::RunAnalysis(const std::function<void(const CDictItem&)>& analysis_function,
                                        const std::function<std::string()>& get_header_function)
{
    std::string results;

    m_resultsForClipboard.clear();

    try
    {
        // call the analysis function for each item
        DictionaryIterator::Foreach<CDictItem>(
            m_dictionary,
            [&](const CDictItem& dict_item) { analysis_function(dict_item); }
        );

        // get and the header and construct the results (header and the list of items)
        results = get_header_function();

        if( !m_resultsForClipboard.empty() )
        {
            results.append("\n\n")
                   .append(m_resultsForClipboard);
        }
    }

    catch( const CSProException& exception )
    {
        results = SO::Concatenate("There was an error running the analysis:\n\n", exception.what());
    }

    m_resultsEditCtrl.SetText(results);

    const bool has_results = !m_resultsForClipboard.empty();
    GetDlgItem(IDC_COPY_TO_CLIPBOARD)->EnableWindow(has_results);
}


void DictionaryAnalysisDlg::OnWithoutValueSets(const bool numerics_only)
{
    size_t number_items_without_value_sets = 0;

    const std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item)
        {
            if( !DictionaryRules::CanHaveValueSet(dict_item) ||
                ( numerics_only && dict_item.GetContentType() != ContentType::Numeric ) )
            {
                return;
            }

            if( !dict_item.HasValueSets() )
            {
                ++number_items_without_value_sets;
                m_resultsForClipboard.append(dict_item.GetName())
                                     .push_back('\n');
            }
        };

    const std::function<std::string()> get_header_function =
        [&]()
        {
            if( number_items_without_value_sets == 0 )
            {
                return FormatText(
                    "No %s items exist that do not have a value set defined.",
                    numerics_only ? "numeric" : "eligible"
                );
            }

            return FormatText(
                "There %s %zu %s item%s without a value set:",
                PluralizeWord(number_items_without_value_sets, "is", "are"),
                number_items_without_value_sets,
                numerics_only ? "numeric" : "eligible",
                PluralizeWord(number_items_without_value_sets)
            );
        };

    RunAnalysis(analysis_function, get_header_function);
}


inline void DictionaryAnalysisDlg::ThrowIfDiscreteValueFallsWithinRange(const double discrete_value, const ValueSetResponse& range)
{
    if( discrete_value >= range.GetMinimumValue() &&
        discrete_value <= range.GetMaximumValue() )
    {
        throw std::exception();
    }
}


bool DictionaryAnalysisDlg::DoesValueSetHaveOverlappingRanges(const CDictItem& dict_item, const DictValueSet& dict_value_set)
{
    std::set<double> discretes;
    std::vector<std::shared_ptr<const ValueSetResponse>> ranges;
    bool has_overlapping_ranges = false;

    try
    {
        for( const DictValue& dict_value : dict_value_set.GetValues() )
        {
            for( const DictValuePair& dict_value_pair : dict_value.GetValuePairs() )
            {
                // use ValueSetResponse to easily parse each pair
                auto value_set_response = std::make_unique<const ValueSetResponse>(dict_item, dict_value, dict_value_pair);

                if( value_set_response->IsDiscrete() )
                {
                    const double discrete_value = value_set_response->GetMinimumValue();

                    if( discretes.find(discrete_value) != discretes.end() )
                        throw std::exception();

                    discretes.insert(discrete_value);
                }

                else
                {
                    ranges.emplace_back(std::move(value_set_response));
                }
            }
        }

        // in the above loop, any duplicate discrete values will be detected; now check for range overlaps

        // first check if the discretes are in any of the ranges
        for( const double discrete_value : discretes )
        {
            for( const std::shared_ptr<const ValueSetResponse>& range : ranges )
                ThrowIfDiscreteValueFallsWithinRange(discrete_value, *range);
        }

        // now check if any of the ranges overlap any of the other ranges
        for( const std::shared_ptr<const ValueSetResponse>& range1 : ranges )
        {
            for( const std::shared_ptr<const ValueSetResponse>& range2 : ranges )
            {
                if( range1 != range2 )
                {
                    ThrowIfDiscreteValueFallsWithinRange(range1->GetMinimumValue(), *range2);
                    ThrowIfDiscreteValueFallsWithinRange(range1->GetMaximumValue(), *range2);
                }
            }
        }
    }

    catch(...)
    {
        has_overlapping_ranges = true;
    }

    return has_overlapping_ranges;
}


void DictionaryAnalysisDlg::OnNumericItemsOverlappingValueSets()
{
    size_t number_overlapping_value_sets = 0;

    const std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item)
        {
            if( dict_item.GetContentType() != ContentType::Numeric )
                return;

            for( const DictValueSet& dict_value_set : dict_item.GetValueSets() )
            {
                if( DoesValueSetHaveOverlappingRanges(dict_item, dict_value_set) )
                {
                    ++number_overlapping_value_sets;
                    m_resultsForClipboard.append(FormatText(
                        "%s (%s)\n",
                        dict_item.GetName().c_str(),
                        dict_value_set.GetName().c_str())
                    );
                }
            }
        };

    const std::function<std::string()> get_header_function =
        [&]() -> std::string
        {
            if( number_overlapping_value_sets == 0 )
                return "There are no numeric value sets with overlapping ranges.";

            return FormatText(
                "There %s %zu numeric value set%s with overlapping ranges:",
                PluralizeWord(number_overlapping_value_sets, "is", "are"),
                number_overlapping_value_sets,
                PluralizeWord(number_overlapping_value_sets)
            );
        };

    RunAnalysis(analysis_function, get_header_function);
}


void DictionaryAnalysisDlg::OnMismatchedDecCharZeroFill(const bool dec_char)
{
    size_t number_mismatched_items = 0;

    const bool dict_default_value = dec_char ? m_dictionary.IsDecChar() : m_dictionary.IsZeroFill();

    const std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item)
        {
            bool item_value;

            if( dict_item.GetContentType() != ContentType::Numeric )
            {
                return;
            }

            else if( dec_char )
            {
                if( dict_item.GetDecimal() == 0 )
                    return;

                item_value = dict_item.GetDecChar();
            }

            else
            {
                item_value = dict_item.GetZeroFill();
            }

            if( dict_default_value != item_value )
            {
                ++number_mismatched_items;
                m_resultsForClipboard.append(dict_item.GetName())
                                     .push_back('\n');
            }
        };

    const std::function<std::string()> get_header_function =
        [&]() -> std::string
        {
            const char* const option_type = dec_char ? "DecChar" : "ZeroFill";

            if( number_mismatched_items == 0 )
            {
                return FormatText(
                    "There are no numeric items with mismatched %s options.",
                    option_type
                );
            }

            return FormatText(
                "There %s %zu item%s with a mismatched %s option:",
                PluralizeWord(number_mismatched_items, "is", "are"),
                number_mismatched_items,
                PluralizeWord(number_mismatched_items),
                option_type
            );
        };

    RunAnalysis(analysis_function, get_header_function);
}
