#include "StdAfx.h"
#include "CoreEntryPageField.h"
#include "CoreEntryEngineInterface.h"
#include "Runaple.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zDictO/ValueProcessor.h>
#include <ZBRIDGEO/npff.h>
#include <engine/IntDrive.h>
#include <zEngineO/ResponseProcessor.h>
#include <zEngineO/ValueSet.h>


CoreEntryPageField::CoreEntryPageField(CoreEntryEngineInterface* core_entry_engine_interface, CDEField* pField)
    :   m_coreEntryEngineInterface(core_entry_engine_interface),
        m_pRunAplEntry(core_entry_engine_interface->GetRunAplEntry()),
        m_pField(pField),
        m_pOriginalField(pField),
        m_valueSet(nullptr),
        m_responseProcessor(nullptr),
        m_numericEngineValue(0),
        m_numericDisplayValue(0),
        m_pEntryDriver(m_pRunAplEntry->GetEntryDriver()),
        m_pEngineArea(m_pEntryDriver->getEngineAreaPtr()),
        m_pIntDriver(m_pEntryDriver->m_pIntDriver.get())
{
    // set up the attributes for all fields
    m_readOnly = ( m_pField->IsProtected() || m_pField->IsMirror() );

    if( m_pRunAplEntry->GetPifFile()->GetApplication()->GetShowFieldLabels() )
        m_label = m_pField->GetCDEText().GetText();

    // if a mirror field, use the main field for all properties except for the above ones
    if( m_pField->IsMirror() )
    {
        VART* pVarT = VPT(m_pField->GetDictItem()->GetSymbol());
        m_pOriginalField = m_pField;
        m_pField = m_pIntDriver->GetCDEFieldFromVART(pVarT);
    }

    int symbol_index = m_pField->GetSymbol();

    m_DeFld = *m_pEntryDriver->GetCsDriver()->GetCurDeFld();
    m_DeFld.SetSymbol(symbol_index);

    m_pVarT = VPT(symbol_index);
    m_valueProcessor = &m_pVarT->GetCurrentValueProcessor();

    m_valueSet = m_pVarT->GetCurrentValueSet();
    m_evaluatedCaptureInfo = m_pVarT->GetEvaluatedCaptureInfo();
    m_responseProcessor = m_pEntryDriver->GetResponseProcessor(&m_DeFld);

    // set up some attributes for all non-mirror fields
    if( !m_pOriginalField->IsMirror() )
    {
        const CCapi* capi = m_pRunAplEntry->GetCapi();
        CapiContent field_capi_content;
        capi->GetCapiContent(&field_capi_content, symbol_index, CCapi::CapiContentType::All);

        m_capiContentVirtualFileMapping.SetCapiContent(std::move(field_capi_content), *m_pEntryDriver);
    }

    RefreshValues();
}


const Logic::SymbolTable& CoreEntryPageField::GetSymbolTable() const
{
    return m_pEngineArea->GetSymbolTable();
}


void CoreEntryPageField::RefreshValues()
{
    // get and process the current value
    m_dataBuffer = m_pRunAplEntry->GetVal(m_pField);

    if( IsNumeric() )
    {
        const NumericValueProcessor* numeric_value_processor = assert_cast<const NumericValueProcessor*>(m_valueProcessor);
        m_numericEngineValue = numeric_value_processor->GetNumericFromInput(m_dataBuffer);

        // in the portable environment we will only use NOTAPPL
        if( m_numericEngineValue == MASKBLK )
            m_numericEngineValue = NOTAPPL;

        m_numericDisplayValue = numeric_value_processor->ConvertNumberFromEngineFormat(m_numericEngineValue);
    }

    RefreshSelectedResponses();
}


const CString& CoreEntryPageField::GetName() const
{
    return m_pOriginalField->GetName();
}


bool CoreEntryPageField::IsMirror() const
{
    return m_pOriginalField->IsMirror();
}


int CoreEntryPageField::GetSymbol() const
{
    return m_pField->GetSymbol();
}


const C3DIndexes& CoreEntryPageField::GetIndexes() const
{
    return m_DeFld.GetIndexes();
}


bool CoreEntryPageField::IsNumeric() const
{
    return m_pVarT->IsNumeric();
}


int CoreEntryPageField::GetIntegerPartLength() const
{
    ASSERT(IsNumeric());
    return m_pVarT->GetLength() - m_pVarT->GetDecimals() - ( m_pVarT->GetDecChar() ? 1 : 0 );
}


int CoreEntryPageField::GetFractionalPartLength() const
{
    ASSERT(IsNumeric());
    return m_pVarT->GetDecimals();
}


int CoreEntryPageField::GetAlphaLength() const
{
    ASSERT(IsAlpha());
    return m_pVarT->GetLength();
}


void CoreEntryPageField::SetCaptureTypeForSingleFieldDisplay()
{
    if( m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::DropDown )
    {
        m_evaluatedCaptureInfo = CaptureType::RadioButton;

        ASSERT(m_valueSet != nullptr);
        ASSERT(m_evaluatedCaptureInfo == m_evaluatedCaptureInfo.MakeValid(*m_pVarT->GetDictItem(), &m_valueSet->GetDictValueSet()));
    }
}


bool CoreEntryPageField::IsUpperCase() const
{
    ASSERT(IsAlpha());
    ASSERT(m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::TextBox ||
           m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::ComboBox);

    return m_pField->IsUpperCase();
}


bool CoreEntryPageField::IsMultiline() const
{
    ASSERT(IsAlpha());
    ASSERT(m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::TextBox);

    return m_pField->AllowMultiLine();
}


int CoreEntryPageField::GetMaxCheckboxSelections() const
{
    ASSERT(m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::CheckBox);

    return static_cast<int>(m_responseProcessor->GetCheckboxMaxSelections());
}


void CoreEntryPageField::GetSliderProperties(double& min_value, double& max_value, double& step) const
{
    ASSERT(m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::Slider);

    const NumericValueProcessor* numeric_value_processor = assert_cast<const NumericValueProcessor*>(m_valueProcessor);
    min_value = numeric_value_processor->GetMinValue();
    max_value = numeric_value_processor->GetMaxValue();
    step = std::pow(10, -1 * m_pVarT->GetDecimals());
}


const std::vector<std::shared_ptr<const ValueSetResponse>>& CoreEntryPageField::GetResponses() const
{
    ASSERT(m_responseProcessor != nullptr);
    return m_responseProcessor->GetResponses();
}


double CoreEntryPageField::GetNumericValue() const
{
    ASSERT(IsNumeric());

    return m_numericDisplayValue;
}


void CoreEntryPageField::SetNumericValue(double value)
{
    ASSERT(IsNumeric() && !IsReadOnly());

    const NumericValueProcessor* numeric_value_processor = assert_cast<const NumericValueProcessor*>(m_valueProcessor);
    value = numeric_value_processor->ConvertNumberToEngineFormat(value);

    CString buffer_value = numeric_value_processor->GetOutput(value);

    m_pRunAplEntry->PutVal(m_pField, buffer_value, m_coreEntryEngineInterface->GetCaseModifiedFlagIfNotModified());

    RefreshValues();
}


CString CoreEntryPageField::GetAlphaValue() const
{
    ASSERT(IsAlpha());
    ASSERT(m_dataBuffer.GetLength() == GetAlphaLength());
    ASSERT(m_dataBuffer.Find('\r') == -1);

    // if not on a multiline field, turn newlines to spaces
    return m_pField->AllowMultiLine() ? m_dataBuffer :
                                        NewlineSubstitutor::NewlineToSpace(m_dataBuffer);
}


void CoreEntryPageField::SetAlphaValue(const CString& value)
{
    ASSERT(IsAlpha() && !IsReadOnly());

    ASSERT(!SO::ContainsNewlineCharacter(value) || IsMultiline());

    CString buffer_value = m_valueProcessor->GetOutput(value);

    m_pRunAplEntry->PutVal(m_pField, buffer_value, m_coreEntryEngineInterface->GetCaseModifiedFlagIfNotModified());

    RefreshValues();
}


void CoreEntryPageField::SetSelectedResponses(const std::vector<size_t>& indices, bool clear_existing_indices/* = true*/)
{
    ASSERT(!IsReadOnly());
    ASSERT(m_responseProcessor != nullptr);

    if( clear_existing_indices )
    {
        m_selectedIndices = indices;
    }

    else
    {
        ASSERT(m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::CheckBox);

        m_selectedIndices.insert(m_selectedIndices.end(), indices.begin(), indices.end());
    }

    // if there are multiple selections, remove any duplicates and make sure that they are in sorted order
    if( m_selectedIndices.size() > 1 )
    {
        ASSERT(m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::CheckBox);

        std::set<size_t> sort_indices(m_selectedIndices.begin(), m_selectedIndices.end());
        m_selectedIndices.assign(sort_indices.begin(), sort_indices.end());
    }


    // process the selections
    if( m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::CheckBox )
    {
        ASSERT(!IsNumeric());

        CString value = m_responseProcessor->GetInputFromCheckboxIndices(m_selectedIndices);
        SetAlphaValue(value);
    }

    else if( IsNumeric() )
    {
        double numeric_value = NOTAPPL;

        if( !indices.empty() )
        {
            try
            {
                numeric_value = m_responseProcessor->GetNumericInputFromResponseIndex(indices.front());
            }

            catch( const ResponseProcessor::SelectionError& )
            {
                // this will occur if the user clicks on a range, something which shouldn't be passed to CoreEntryPageField
                ASSERT(false);
            }
        }

        SetNumericValue(numeric_value);
    }

    else
    {
        CString alpha_value = m_responseProcessor->GetInputFromResponseIndex(indices.front());
        SetAlphaValue(alpha_value);
    }
}


void CoreEntryPageField::RefreshSelectedResponses()
{
    if( m_responseProcessor == nullptr )
        return;

    if( m_evaluatedCaptureInfo.GetCaptureType() == CaptureType::CheckBox )
    {
        m_selectedIndices = m_responseProcessor->GetCheckboxResponseIndices(m_dataBuffer);
    }

    else
    {
        m_selectedIndices.clear();

        size_t index = m_responseProcessor->GetResponseIndex(m_dataBuffer);

        if( index != SIZE_MAX )
            m_selectedIndices.push_back(index);
    }
}


CString CoreEntryPageField::GetNote() const
{
    return WS2CS(m_pIntDriver->ConvertV0Escapes(CS2WS(m_pEntryDriver->GetNoteContent(m_DeFld)), CIntDriver::V0_EscapeType::NewlinesToSlashN_Backslashes));
}


void CoreEntryPageField::SetNote(CString note)
{
    ASSERT(!IsMirror());

    m_pEntryDriver->SetNote(m_DeFld, WS2CS(m_pIntDriver->ApplyV0Escapes(CS2WS(note), CIntDriver::V0_EscapeType::NewlinesToSlashN_Backslashes)));
}


void CoreEntryPageField::EditNote()
{
    ASSERT(!IsMirror());

    if( m_pEntryDriver->EditNote(false, &m_DeFld) )
        m_coreEntryEngineInterface->SetCaseModified();
}


bool CoreEntryPageField::IsFieldFilled() const
{
    return IsNumeric() ? ( m_numericDisplayValue != NOTAPPL ) :
                         !SO::IsBlank(m_dataBuffer);
}


#ifdef _CONSOLE

std::vector<CString> CoreEntryPageField::GetVerboseFieldInformation() const
{
    CString text;

    // field properties ... initially put in a set and then add alphabetically to the vector
    std::set<CString> properties_set;
    std::set<int> symbols_set;
    symbols_set.insert(m_pField->GetSymbol());

    for( int i = 0; i < 2; i++ )
    {
        ParameterManager::Parameter thisProperty = ( i == 0 ) ?
            ParameterManager::Parameter::Property_CanEnterNotAppl : ParameterManager::Parameter::Property_AllowMultiLine;

        ParameterManager::Parameter lastProperty = ( i == 0 ) ?
            ParameterManager::Parameter::Property_UseEnterKey : ParameterManager::Parameter::Property_ZeroFill;

        while( thisProperty <= lastProperty )
        {
            text.Format(_T("%s: %s"), ParameterManager::GetDisplayName(thisProperty), m_pIntDriver->GetProperty(thisProperty, &symbols_set));
            properties_set.insert(text);
            thisProperty = static_cast<ParameterManager::Parameter>(static_cast<int>(thisProperty) + 1);
        }
    }

    // add the properties in alphabetical order
    std::vector<CString> field_information;

    for( const CString& property_text : properties_set )
        field_information.push_back(property_text);

    return field_information;
}


CString CoreEntryPageField::GetValueSetName() const
{
    return ( m_valueSet != nullptr ) ? WS2CS(m_valueSet->GetName()) : _T("<undefined>");
}

#endif // ending _CONSOLE
