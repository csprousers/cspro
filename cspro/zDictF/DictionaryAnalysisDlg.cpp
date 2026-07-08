#include "StdAfx.h"
#include <zUtilF/TextReportDlg.h>
#include <zDictO/ValueSetResponse.h>


void CDictChildWnd::RunDictAnalysis(const std::function<void(const CDictItem&)>& analysis_function,
                                    const std::function<bool(std::string&, std::string&)>& summary_results_function)
{
    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());

    // call the analysis function for each item
    DictionaryIterator::Foreach<CDictItem>(*pDoc->GetDict(),
        [&](const CDictItem& dict_item)
        {
            analysis_function(dict_item);
        });

    // get and display the report text
    std::string heading;
    std::string contents;

    if( summary_results_function(heading, contents) )
    {
        TextReportDlg text_report_dialog(std::move(heading), std::move(contents));
        text_report_dialog.DoModal();
    }

    else
    {
        AfxMessageBox(L"No items exist that meet the selected criteria.");
    }
}


void CDictChildWnd::RunDictAnalysisNoValueSets(const bool numerics_only)
{
    int number_items_without_value_sets = 0;
    std::string items_without_value_sets;

    const std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item)
        {
            if( !DictionaryRules::CanHaveValueSet(dict_item) || ( numerics_only && dict_item.GetContentType() != ContentType::Numeric ) )
                return;

            if( !dict_item.HasValueSets() )
            {
                ++number_items_without_value_sets;
                items_without_value_sets.append(dict_item.GetName())
                                        .append(SO::Newline_crlf_sv);
            }
        };

    const std::function<bool(std::string&, std::string&)> summary_results_function =
        [&](std::string& heading, std::string & contents)
        {
            if( number_items_without_value_sets == 0 )
                return false;

            heading = FormatText("There are %d numeric%s item%s without value sets:", number_items_without_value_sets,
                                 numerics_only ? "" : "/alpha", PluralizeWord(number_items_without_value_sets));

            contents = items_without_value_sets;

            return true;
        };

    RunDictAnalysis(analysis_function, summary_results_function);
}


void CDictChildWnd::OnDictAnalysisItemsNoValueSets()
{
    RunDictAnalysisNoValueSets(false);
}


void CDictChildWnd::OnDictAnalysisNumericItemsNoValueSets()
{
    RunDictAnalysisNoValueSets(true);
}


namespace
{
    inline void ThrowIfDiscreteValueFallsWithinRange(double discrete_value, const ValueSetResponse& range)
    {
        if( discrete_value >= range.GetMinimumValue() && discrete_value <= range.GetMaximumValue() )
            throw std::exception();
    }

    bool DoesValueSetHaveOverlappingRanges(const CDictItem& dict_item, const DictValueSet& dict_value_set)
    {
        std::set<double> discretes;
        std::vector<std::shared_ptr<const ValueSetResponse>> ranges;
        bool has_overlapping_ranges = false;

        try
        {
            for( const auto& dict_value : dict_value_set.GetValues() )
            {
                for( const auto& dict_value_pair : dict_value.GetValuePairs() )
                {
                    // use ValueSetResponse to easily parse each pair
                    auto value_set_response = std::make_shared<const ValueSetResponse>(dict_item, dict_value, dict_value_pair);

                    if( value_set_response->IsDiscrete() )
                    {
                        double discrete_value = value_set_response->GetMinimumValue();

                        if( discretes.find(discrete_value) != discretes.end() )
                            throw std::exception();

                        discretes.insert(discrete_value);
                    }

                    else
                    {
                        ranges.emplace_back(value_set_response);
                    }
                }
            }

            // in the above loop, any duplicate discrete values will be detected; now check for range overlaps

            // first check if the discretes are in any of the ranges
            for( double discrete_value : discretes )
            {
                for( const auto& range : ranges )
                    ThrowIfDiscreteValueFallsWithinRange(discrete_value, *range);
            }

            // now check if any of the ranges overlap any of the other ranges
            for( const auto& range1 : ranges )
            {
                for( const auto& range2 : ranges )
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
}

void CDictChildWnd::OnDictAnalysisNumericItemsOverlappingValueSets()
{
    int number_overlapping_value_sets = 0;
    std::string overlapping_value_sets;

    const std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item)
        {
            if( dict_item.GetContentType() != ContentType::Numeric )
                return;

            for( const auto& dict_value_set : dict_item.GetValueSets() )
            {
                if( DoesValueSetHaveOverlappingRanges(dict_item, dict_value_set) )
                {
                    ++number_overlapping_value_sets;
                    overlapping_value_sets.append(FormatText("%s (%s)\r\n", dict_item.GetName().c_str(),
                                                                            dict_value_set.GetName().c_str()));
                }
            }
        };

    const std::function<bool(std::string&, std::string&)> summary_results_function =
        [&](std::string& heading, std::string & contents)
        {
            if( number_overlapping_value_sets == 0 )
                return false;

            heading = FormatText("There are %d numeric value set%s with overlapping ranges:",
                                 number_overlapping_value_sets, PluralizeWord(number_overlapping_value_sets));

            contents = overlapping_value_sets;

            return true;
        };

    RunDictAnalysis(analysis_function, summary_results_function);
}


void CDictChildWnd::OnDictAnalysisNumericMismatchedZeroFillDecChar()
{
    int number_mismatched_items = 0;
    std::string mismatched_items;

    CDDDoc* pDoc = assert_cast<CDDDoc*>(GetActiveDocument());
    const CDataDict* pDict = pDoc->GetDict();
    bool default_zero_fill = pDict->IsZeroFill();
    bool default_dec_char = pDict->IsDecChar();

    const std::function<void(const CDictItem&)> analysis_function =
        [&](const CDictItem& dict_item)
        {
            if( dict_item.GetContentType() != ContentType::Numeric )
                return;

            bool zero_fill_mismatch = ( dict_item.GetZeroFill() != default_zero_fill );
            bool dec_char_mismatch = ( dict_item.GetDecimal() > 0 && dict_item.GetDecChar() != default_dec_char );

            if( zero_fill_mismatch || dec_char_mismatch )
            {
                ++number_mismatched_items;
                mismatched_items.append(FormatText("%s (%s%s%s)\r\n",
                    dict_item.GetName().c_str(),
                    zero_fill_mismatch ? "zero fill" : "",
                    ( zero_fill_mismatch && dec_char_mismatch ) ? " / " : "",
                    dec_char_mismatch ? "decimal character" : ""));
            }
        };

    const std::function<bool(std::string&, std::string&)> summary_results_function =
        [&](std::string& heading, std::string & contents)
        {
            if( number_mismatched_items == 0 )
                return false;

            heading = FormatText("There are %d item%s with mismatched options:",
                                 number_mismatched_items, PluralizeWord(number_mismatched_items));

            contents = mismatched_items;

            return true;
        };

    RunDictAnalysis(analysis_function, summary_results_function);
}
