#include "StdAfx.h"
#include "DictionaryAnalysisDlg.h"
#include <zToolsO/WinSettings.h>
#include <zUtilO/DynamicLayoutControlResizer.h>
#include <zUtilO/TreeCtrlHelpers.h>
#include <zUtilO/WindowHelpers.h>
#include <zDictO/ValueSetResponse.h>


namespace Analysis
{
    constexpr int ItemsWithoutValueSets                = 0;
    constexpr int NumericItemsWithoutValueSets         = 1;
    constexpr int NumericItemsWithOverlappingValueSets = 2;
    constexpr int ItemsWithMismatchedDecCharOptions    = 3;
    constexpr int ItemsWithMismatchedZeroFillOptions   = 4;
}


namespace Order
{
    constexpr int Dictionary   = 0;
    constexpr int Alphabetical = 1;
}


BEGIN_MESSAGE_MAP(DictionaryAnalysisDlg, DynamicLayoutResizableDlg)
    ON_NOTIFY(TVN_SELCHANGED, IDC_ANALYSIS_TYPE, OnAnalysisTypeChange)
    ON_BN_CLICKED(IDC_DICTIONARY, OnAnalysisOrderChange)
    ON_BN_CLICKED(IDC_ALPHABETICAL, OnAnalysisOrderChange)
    ON_COMMAND(IDC_COPY_TO_CLIPBOARD, OnCopyToClipboard)
END_MESSAGE_MAP()


DictionaryAnalysisDlg::DictionaryAnalysisDlg(const CDataDict& dictionary, CWnd* const pParent/* = nullptr*/)
    :   DynamicLayoutResizableDlg(IDD_DICTIONARY_ANALYSIS, pParent),
        m_dictionary(dictionary),
        m_analysisType(WinSettings::Read<DWORD>(WinSettings::Type::DictionaryAnalysisType, Analysis::NumericItemsWithoutValueSets)),
        m_analysisOrder(WinSettings::Read<DWORD>(WinSettings::Type::DictionaryAnalysisOrder, Order::Dictionary))
{
    SerializeDialogSize("DictionaryAnalysisDlg");
}


void DictionaryAnalysisDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_ANALYSIS_TYPE, m_analysisTypeTreeCtrl);
    DDX_Radio(pDX, IDC_DICTIONARY, m_analysisOrder);
    DDX_Control(pDX, IDC_RESULTS, m_resultsEditCtrl);
}


BOOL DictionaryAnalysisDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    // add the analysis types
    HTREEITEM initial_node_to_select = PopulateAnalysisTypes();
    TreeCtrlHelpers::ExpandAllNodes(m_analysisTypeTreeCtrl);

    // set up the read-only Scintilla control to show the results
    m_resultsEditCtrl.ReplaceCEdit(this, false, false, SCLEX_NULL);
    m_resultsEditCtrl.SetWrapMode(Scintilla::Wrap::WhiteSpace);

    // run the analysis for the selected item
    if( initial_node_to_select != nullptr )
        m_analysisTypeTreeCtrl.SelectItem(initial_node_to_select);

    return TRUE;
}


std::vector<std::tuple<CWnd*, SizingDirection>> DictionaryAnalysisDlg::GetDynamicLayoutControls()
{
    return { { &m_resultsEditCtrl, SizingDirection::XY } };
}


HTREEITEM DictionaryAnalysisDlg::PopulateAnalysisTypes()
{
    HTREEITEM initial_node_to_select = nullptr;

    TV_INSERTSTRUCT tvi { };
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.hInsertAfter = TVI_LAST;

    auto add_type = [&](const wchar_t* const text, const int analysis_type)
    {
        tvi.item.pszText = const_cast<wchar_t*>(text);
        tvi.item.lParam = analysis_type;

        HTREEITEM hTreeItem = m_analysisTypeTreeCtrl.InsertItem(&tvi);

        if( m_analysisType == analysis_type )
            initial_node_to_select = hTreeItem;

        return hTreeItem;
    };

    tvi.hParent = TVI_ROOT;
    tvi.hParent = add_type(L"Items", -1);
    add_type(L"Items with mismatched DecChar options", Analysis::ItemsWithMismatchedDecCharOptions);
    add_type(L"Items with mismatched ZeroFill options", Analysis::ItemsWithMismatchedZeroFillOptions);

    tvi.hParent = TVI_ROOT;
    tvi.hParent = add_type(L"Value Sets", -1);
    add_type(L"Items without value sets", Analysis::ItemsWithoutValueSets);
    add_type(L"Numeric items without value sets", Analysis::NumericItemsWithoutValueSets);
    add_type(L"Numeric items with overlapping value sets", Analysis::NumericItemsWithOverlappingValueSets);

    return initial_node_to_select;
}


void DictionaryAnalysisDlg::OnAnalysisTypeChange(NMHDR* const pNMHDR, LRESULT* const pResult)
{
    const NM_TREEVIEW* const pNMTreeView = reinterpret_cast<NM_TREEVIEW*>(pNMHDR);

    m_analysisType = ( pNMTreeView->itemNew.hItem != nullptr )
        ? static_cast<int>(m_analysisTypeTreeCtrl.GetItemData(pNMTreeView->itemNew.hItem))
        : -1;

    if( m_analysisType != -1 )
        WinSettings::Write<DWORD>(WinSettings::Type::DictionaryAnalysisType, m_analysisType);

    RunAnalysis();

    *pResult = 0;
}


void DictionaryAnalysisDlg::OnAnalysisOrderChange()
{
    m_analysisOrder = IsDlgButtonChecked(IDC_DICTIONARY) ? Order::Dictionary : Order::Alphabetical;
    WinSettings::Write<DWORD>(WinSettings::Type::DictionaryAnalysisOrder, m_analysisOrder);

    RunAnalysis();
}


void DictionaryAnalysisDlg::OnCopyToClipboard()
{
    ASSERT(!m_resultsTextForClipboard.empty());

    WinClipboard::PutText(this, m_resultsTextForClipboard);
}


void DictionaryAnalysisDlg::SetResultsText(const std::string& results_text)
{
    m_resultsEditCtrl.SetText(results_text);

    GetDlgItem(IDC_COPY_TO_CLIPBOARD)->EnableWindow(!m_resultsTextForClipboard.empty());
}


void DictionaryAnalysisDlg::RunAnalysis()
{
    m_resultRows.clear();
    m_resultsTextForClipboard.clear();

    switch( m_analysisType )
    {
        case Analysis::ItemsWithoutValueSets:
            return OnWithoutValueSets(false);

        case Analysis::NumericItemsWithoutValueSets:
            return OnWithoutValueSets(true);

        case Analysis::NumericItemsWithOverlappingValueSets:
            return OnNumericItemsOverlappingValueSets();

        case Analysis::ItemsWithMismatchedDecCharOptions:
            return OnMismatchedDecCharZeroFill(true);

        case Analysis::ItemsWithMismatchedZeroFillOptions:
            return OnMismatchedDecCharZeroFill(false);

        default:
            ASSERT(m_analysisType == -1);
            return SetResultsText(SO::Empty_string);
    }
}


void DictionaryAnalysisDlg::RunAnalysis(const std::function<void(const CDictItem&)>& analysis_function,
                                        const std::function<std::string()>& get_header_function)
{
    ASSERT(m_resultRows.empty());
    ASSERT(m_resultsTextForClipboard.empty());

    std::string results_text;

    try
    {
        // call the analysis function for each item
        DictionaryIterator::Foreach<CDictItem>(
            m_dictionary,
            [&](const CDictItem& dict_item) { analysis_function(dict_item); }
        );

        // if the callback function fills in m_resultRows, convert it to m_resultsTextForClipboard
        if( !m_resultRows.empty() )
        {
            ASSERT(m_resultsTextForClipboard.empty());

            if( m_analysisOrder == Order::Alphabetical )
            {
                std::sort(m_resultRows.begin(), m_resultRows.end(),
                          [&](const std::string& r1, const std::string& r2) { return ( SO::CompareNoCase(r1, r2) < 0 ); });
            }

            m_resultsTextForClipboard = SO::CreateSingleString(m_resultRows, "\n");
        }


        // get and the header and construct the results (header and the list of items)
        results_text = get_header_function();

        if( !m_resultsTextForClipboard.empty() )
        {
            results_text.append("\n\n")
                        .append(m_resultsTextForClipboard);
        }
    }

    catch( const CSProException& exception )
    {
        results_text = SO::Concatenate("There was an error running the analysis:\n\n", exception.what());
    }

    SetResultsText(results_text);
}


void DictionaryAnalysisDlg::OnWithoutValueSets(const bool numerics_only)
{
    const std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item)
        {
            if( !DictionaryRules::CanHaveValueSet(dict_item) ||
                ( numerics_only && dict_item.GetContentType() != ContentType::Numeric ) )
            {
                return;
            }

            if( !dict_item.HasValueSets() )
                m_resultRows.emplace_back(dict_item.GetName());
        };

    const std::function<std::string()> get_header_function =
        [&]()
        {
            if( m_resultRows.empty() )
            {
                return FormatText(
                    "No %s items exist that do not have a value set defined.",
                    numerics_only ? "numeric" : "eligible"
                );
            }

            return FormatText(
                "There %s %zu %s item%s without a value set:",
                PluralizeWord(m_resultRows.size(), "is", "are"),
                m_resultRows.size(),
                numerics_only ? "numeric" : "eligible",
                PluralizeWord(m_resultRows.size())
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
    const std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item)
        {
            if( dict_item.GetContentType() != ContentType::Numeric )
                return;

            for( const DictValueSet& dict_value_set : dict_item.GetValueSets() )
            {
                if( DoesValueSetHaveOverlappingRanges(dict_item, dict_value_set) )
                {
                    m_resultRows.emplace_back(SO::CreateParentheticalExpression(
                        dict_item.GetName().c_str(),
                        dict_value_set.GetName().c_str()
                    ));
                }
            }
        };

    const std::function<std::string()> get_header_function =
        [&]() -> std::string
        {
            if( m_resultRows.empty() )
                return "There are no numeric value sets with overlapping ranges.";

            return FormatText(
                "There %s %zu numeric value set%s with overlapping ranges:",
                PluralizeWord(m_resultRows.size(), "is", "are"),
                m_resultRows.size(),
                PluralizeWord(m_resultRows.size())
            );
        };

    RunAnalysis(analysis_function, get_header_function);
}


void DictionaryAnalysisDlg::OnMismatchedDecCharZeroFill(const bool dec_char)
{
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
                m_resultRows.emplace_back(dict_item.GetName());
        };

    const std::function<std::string()> get_header_function =
        [&]() -> std::string
        {
            const char* const option_type = dec_char ? "DecChar" : "ZeroFill";

            if( m_resultRows.empty() )
                return FormatText("There are no numeric items with mismatched %s options.", option_type);

            return FormatText(
                "There %s %zu item%s with a mismatched %s option:",
                PluralizeWord(m_resultRows.size(), "is", "are"),
                m_resultRows.size(),
                PluralizeWord(m_resultRows.size()),
                option_type
            );
        };

    RunAnalysis(analysis_function, get_header_function);
}
