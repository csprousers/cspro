#include "stdafx.h"
#include "ResponseProcessor.h"
#include <zToolsO/Special.h>
#include <zToolsO/VectorHelpers.h>
#include <zUtilO/Randomizer.h>
#include <zMessageO/Messages.h>
#include <zDictO/ValueProcessor.h>
#include <zDictO/ValueSetResponse.h>


ResponseProcessor::ResponseProcessor(std::shared_ptr<const ValueProcessor> value_processor)
    :   m_valueProcessor(std::move(value_processor)),
        m_responses(m_valueProcessor->GetResponses()),
        m_valueSetContainsNotApplOrBlank(false),
        m_valueSetContainsRefused(false),
        m_filteredResponsesShowRefused(true),
        m_filteredResponsesShowRefusedOverride(false),
        m_filteredResponsesHaveBlanksRemoved(false),
        m_filteredResponsesHaveRangesRemoved(false),
        m_responsesNeedResetting(false)
{
    switch( m_valueProcessor->GetDictItem().GetContentType() )
    {
        case ContentType::Numeric:
            m_valueSetContainsNotApplOrBlank = m_valueProcessor->IsValid(NOTAPPL);
            m_valueSetContainsRefused = m_valueProcessor->IsValid(REFUSED);
            break;

        case ContentType::Alpha:
            m_valueSetContainsNotApplOrBlank = m_valueProcessor->IsValid(CString());
            break;

        default:
            ASSERT(false);
            break;
    }
}


const std::vector<std::shared_ptr<const ValueSetResponse>>& ResponseProcessor::GetResponses() const
{
    return ( m_filteredResponses != nullptr ) ? *m_filteredResponses :
                                                m_responses;
}


void ResponseProcessor::ShowRefusedValue(bool show_refused_value)
{
    ASSERT(m_valueProcessor->GetDictItem().GetContentType() == ContentType::Numeric);

    if( m_filteredResponsesShowRefusedOverride )
    {
        ASSERT(m_filteredResponsesShowRefused);
    }

    else if( show_refused_value != m_filteredResponsesShowRefused )
    {
        if( m_valueSetContainsRefused )
        {
            m_filteredResponsesShowRefused = show_refused_value;
            GenerateFilteredResponses();
        }
    }
}

void ResponseProcessor::AlwaysShowRefusedValue()
{
    ASSERT(IsRefusedValueHidden());
    ShowRefusedValue(true);
    m_filteredResponsesShowRefusedOverride = true;
    m_responsesNeedResetting = true;
}


void ResponseProcessor::ApplyCaptureTypeProperties(CaptureType capture_type)
{
    bool saved_filtered_responses_have_blanks_removed = m_filteredResponsesHaveBlanksRemoved;

    m_filteredResponsesHaveBlanksRemoved = ( capture_type == CaptureType::ToggleButton && m_valueSetContainsNotApplOrBlank );

    if( saved_filtered_responses_have_blanks_removed != m_filteredResponsesHaveBlanksRemoved )
        GenerateFilteredResponses();
}


void ResponseProcessor::RemoveRangeResponses(bool remove_range_responses)
{
    ASSERT(m_valueProcessor->GetDictItem().GetContentType() == ContentType::Numeric);

    if( remove_range_responses != m_filteredResponsesHaveRangesRemoved )
    {
        // see if there are even ranges to remove
        if( !m_valueSetContainsRanges.has_value() )
        {
            m_valueSetContainsRanges = false;

            for( const auto& response : m_responses )
            {
                if( !response->IsDiscrete() )
                {
                    m_valueSetContainsRanges = true;
                    break;
                }
            }
        }

        if( *m_valueSetContainsRanges )
        {
            m_filteredResponsesHaveRangesRemoved = remove_range_responses;
            GenerateFilteredResponses();
        }
    }
}


void ResponseProcessor::SetCanEnterNotAppl(bool add_notappl)
{
    ASSERT(m_valueProcessor->GetDictItem().GetContentType() == ContentType::Numeric);

    // if notappl is already in the value set, there is no need to add it
    if( m_valueSetContainsNotApplOrBlank )
        return;

    if( add_notappl != ( m_addedNotApplResponse != nullptr ) )
    {
        if( m_addedNotApplResponse == nullptr )
        {
            const std::wstring& notappl_text = MGF::GetMessageText(110005, _T("Not Applicable"));
            m_addedNotApplResponse = std::make_shared<ValueSetResponse>(WS2CS(notappl_text), NOTAPPL);
        }

        else
        {
            m_addedNotApplResponse.reset();
        }

        GenerateFilteredResponses();
    }
}


size_t ResponseProcessor::GetValueSetResponseIndex(const DictValue& dict_value) const
{
    const auto& responses = GetResponses();

    for( size_t i = 0; i < responses.size(); ++i )
    {
        if( responses[i]->GetDictValue() == &dict_value )
            return i;
    }

    // a value should be found unless the blanks or ranges have been removed
    ASSERT(m_filteredResponsesHaveBlanksRemoved || m_filteredResponsesHaveRangesRemoved);
    return SIZE_MAX;
}

size_t ResponseProcessor::GetResponseIndex(const CString& value) const
{
    const DictValue* response_dict_value = m_valueProcessor->GetDictValueFromInput(value);

    // if found, get the index of the response
    if( response_dict_value != nullptr )
        return GetValueSetResponseIndex(*response_dict_value);

    // if the value is an added notappl, return the index of that
    if( m_addedNotApplResponse != nullptr )
    {
        if( SO::IsBlank(value) )
        {
            const auto& responses = GetResponses();
            return responses.size() - 1;
        }
    }

    // otherwise the value isn't in the list of responses
    return SIZE_MAX;
}


std::shared_ptr<const ValueSetResponse> ResponseProcessor::GetValueSetResponseFromIndex(size_t index) const
{
    const auto& responses = GetResponses();
    ASSERT(index < responses.size());

    const auto& response = responses[index];

    if( !response->IsDiscrete() )
        throw ResponseProcessor::SelectionError();

    return response;
}

CString ResponseProcessor::GetInputFromResponseIndex(size_t index) const
{
    const auto& response = GetValueSetResponseFromIndex(index);
    ContentType content_type = m_valueProcessor->GetDictItem().GetContentType();

    if( content_type == ContentType::Numeric )
    {
        return m_valueProcessor->GetOutput(response->GetMinimumValue());
    }

    else if( content_type == ContentType::Alpha )
    {
        return m_valueProcessor->GetOutput(response->GetCode());
    }

    else
    {
        CONTENT_TYPE_REFACTOR::LOOK_AT_AND_THROW();
    }
}

double ResponseProcessor::GetNumericInputFromResponseIndex(size_t index) const
{
    ASSERT(m_valueProcessor->GetDictItem().GetContentType() == ContentType::Numeric);
    const auto& response = GetValueSetResponseFromIndex(index);
    return response->GetMinimumValue();
}


void ResponseProcessor::DoCheckboxCalculations() const
{
    if( m_checkboxCalculations.has_value() )
        return;

    m_checkboxCalculations = CheckboxCalculations { 1, 0 };

    for( const auto& response : GetResponses() )
        m_checkboxCalculations->checkbox_width = std::max(m_checkboxCalculations->checkbox_width, response->GetCode().GetLength());

    m_checkboxCalculations->max_selections = m_valueProcessor->GetDictItem().GetLen() / m_checkboxCalculations->checkbox_width;
}


int ResponseProcessor::GetCheckboxWidth() const
{
    DoCheckboxCalculations();
    return m_checkboxCalculations->checkbox_width;
}

size_t ResponseProcessor::GetCheckboxMaxSelections() const
{
    DoCheckboxCalculations();
    return m_checkboxCalculations->max_selections;
}

std::vector<size_t> ResponseProcessor::GetCheckboxResponseIndices(const CString& value) const
{
    DoCheckboxCalculations();
    ASSERT(value.GetLength() <= (int)m_valueProcessor->GetDictItem().GetLen());

    std::vector<size_t> indices;
    bool sort_indices = false;
    size_t last_index = 0;

    for( int i = 0; i < value.GetLength(); i += m_checkboxCalculations->checkbox_width )
    {
        CString this_value = value.Mid(i, m_checkboxCalculations->checkbox_width);

        const DictValue* response_dict_value = m_valueProcessor->GetDictValueFromInput(this_value);

        if( response_dict_value != nullptr )
        {
            size_t index = GetValueSetResponseIndex(*response_dict_value);

            // make sure that the indices are in sorted order
            if( index <= last_index && !indices.empty() )
            {
                // prevent duplicate checkbox codes from resulting in multiple entries in indices
                if( std::find(indices.begin(), indices.end(), index) != indices.end() )
                    continue;

                sort_indices = true;
            }

            last_index = index;

            indices.emplace_back(index);
        }
    }

    // make sure that the selected indices are in sorted order
    if( sort_indices )
        std::sort(indices.begin(), indices.end());

    return indices;
}

CString ResponseProcessor::GetInputFromCheckboxIndices(const std::vector<size_t>& indices) const
{
    DoCheckboxCalculations();
    ASSERT(indices.size() <= m_checkboxCalculations->max_selections);

    const auto& responses = GetResponses();

    CString checkbox_string;

    for( const size_t& index : indices )
    {
        CIMSAString this_value = responses[index]->GetCode();
        this_value.MakeExactLength(m_checkboxCalculations->checkbox_width);
        checkbox_string = checkbox_string + this_value;
    }

    return checkbox_string;
}


void ResponseProcessor::ResetResponses()
{
    // this is called every time a session is started; if the responses
    // have been randomized or sorted, this resets them
    if( m_responsesNeedResetting )
    {
        m_responses = m_valueProcessor->GetResponses();
        GenerateFilteredResponses();
        m_filteredResponsesShowRefusedOverride = false;
        m_responsesNeedResetting = false;
    }
}


void ResponseProcessor::RandomizeResponses(const std::set<const DictValue*>& values_to_exclude)
{
    std::default_random_engine random_engine(Randomizer::NextSeed());

    // if randomizing the whole value set, we can do this simply
    if( values_to_exclude.empty() )
    {
        std::shuffle(m_responses.begin(), m_responses.end(), std::move(random_engine));
    }

    // otherwise get the list of indices to randomize
    else
    {
        std::vector<size_t> indices_to_randomize;

        for( size_t i = 0; i < m_responses.size(); ++i )
        {
            if( values_to_exclude.find(m_responses[i]->GetDictValue()) == values_to_exclude.cend() )
                indices_to_randomize.emplace_back(i);
        }

        VectorHelpers::Randomize(m_responses, std::move(indices_to_randomize), std::move(random_engine));
    }

    GenerateFilteredResponses();

    m_responsesNeedResetting = true;
}


void ResponseProcessor::SortResponses(bool ascending, bool sort_by_label)
{
    ContentType content_type = m_valueProcessor->GetDictItem().GetContentType();

    std::sort(m_responses.begin(), m_responses.end(),
        [&](const auto& response1, const auto& response2)
        {
            double comparison;

            if( sort_by_label )
            {
                comparison = response1->GetLabel().CompareNoCase(response2->GetLabel());
            }

            else if( content_type == ContentType::Numeric )
            {
                comparison = response1->GetMinimumValue() - response2->GetMinimumValue();
            }

            else if( content_type == ContentType::Alpha )
            {
                comparison = response1->GetCode().Compare(response2->GetCode());
            }

            else
            {
                CONTENT_TYPE_REFACTOR::LOOK_AT_AND_THROW();
            }

            return ascending ? ( comparison < 0 ) : ( comparison > 0 );
        });

    GenerateFilteredResponses();

    m_responsesNeedResetting = true;
}


void ResponseProcessor::GenerateFilteredResponses()
{
    m_filteredResponses.reset();

    // don't filter responses unless there are changes from the default options
    if( m_filteredResponsesShowRefused && !m_filteredResponsesHaveBlanksRemoved &&
        !m_filteredResponsesHaveRangesRemoved && m_addedNotApplResponse == nullptr )
    {
        return;
    }


    // numeric items
    if( m_valueProcessor->GetDictItem().GetContentType() == ContentType::Numeric )
    {
        m_filteredResponses = std::make_unique<std::vector<std::shared_ptr<const ValueSetResponse>>>();

        for( const auto& response : m_responses )
        {
            // filter refused
            if( !m_filteredResponsesShowRefused && response->GetMinimumValue() == REFUSED )
                continue;

            // filter blanks
            if( m_filteredResponsesHaveBlanksRemoved && response->GetMinimumValue() == NOTAPPL )
                continue;

            // filter ranges
            if( m_filteredResponsesHaveRangesRemoved && !response->IsDiscrete() )
                continue;

            m_filteredResponses->emplace_back(response);
        }

        // add the notappl
        if( !m_filteredResponsesHaveBlanksRemoved && m_addedNotApplResponse != nullptr )
            m_filteredResponses->emplace_back(m_addedNotApplResponse);
    }


    // string items
    else if( m_valueProcessor->GetDictItem().GetContentType() == ContentType::Alpha )
    {
        m_filteredResponses = std::make_unique<std::vector<std::shared_ptr<const ValueSetResponse>>>();

        for( const auto& response : m_responses )
        {
            // filter blanks
            if( m_filteredResponsesHaveBlanksRemoved && SO::IsBlank(response->GetCode()) )
                continue;

            m_filteredResponses->emplace_back(response);
        }
    }
}
