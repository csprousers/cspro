#include "StdAfx.h"
#include "CoreEntryPage.h"
#include "CoreEntryEngineInterface.h"
#include "CoreEntryPageField.h"
#include "Runaple.h"
#include <engine/IntDrive.h>


CoreEntryPage::CoreEntryPage(CoreEntryEngineInterface* core_entry_engine_interface, CDEField* pField)
    :   m_pField(pField)
{
    ASSERT(!m_pField->IsProtected());
    ASSERT(!m_pField->IsMirror());

    CRunAplEntry* pRunAplEntry = core_entry_engine_interface->GetRunAplEntry();
    CEntryDriver* m_pEngineDriver = pRunAplEntry->GetEntryDriver();
    CEngineArea* m_pEngineArea = m_pEngineDriver->getEngineAreaPtr();
    CIntDriver* m_pIntDriver = m_pEngineDriver->m_pIntDriver.get();

    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return m_pEngineArea->GetSymbolTable(); };

    int symbol_index = m_pField->GetSymbol();
    const VART* field_vart = VPT(symbol_index);

    // construct a combined occurrence label (the function will return record and item occurrence labels)
    std::vector<std::wstring> occurrence_labels = m_pEngineDriver->GetOccurrenceLabels(field_vart);
    occurrence_labels.erase(std::remove(occurrence_labels.begin(), occurrence_labels.end(), SO::EmptyString), occurrence_labels.end());
    m_occurrenceLabel = SO::CreateSingleString(occurrence_labels, _T(" - "));

    // get the block information (if applicable)
    const EngineBlock* engine_block = field_vart->GetEngineBlock();

    if( engine_block != nullptr )
    {
        // setup the block details
        const CDEBlock& form_block = engine_block->GetFormBlock();
        m_blockName = form_block.GetName();
        m_blockLabel = form_block.GetLabel();

        // evaluate the block's question/help text
        const CCapi* capi = pRunAplEntry->GetCapi();
        CapiContent block_capi_content;
        capi->GetCapiContent(&block_capi_content, engine_block->GetSymbolIndex(), CCapi::CapiContentType::All);

        m_blockCapiContentVirtualFileMapping.SetCapiContent(std::move(block_capi_content), *m_pEngineDriver);
    }

    // the field is not in a block or should not be displayed with other block fields
    if( engine_block == nullptr || !engine_block->GetFormBlock().GetDisplayTogether() )
    {
        m_currentPageField = &m_pageFields.emplace_back(core_entry_engine_interface, m_pField);
    }

    // otherwise setup all of the fields on the block, adding all protected/mirror fields
    // and all fields starting from m_pField
    else
    {
        bool reached_this_field = false;

        for( CDEField* this_field : engine_block->GetFields() )
        {
            reached_this_field |= ( this_field == m_pField );

            if( reached_this_field || this_field->IsProtected() || this_field->IsMirror() )
            {
                m_pageFields.emplace_back(core_entry_engine_interface, this_field);

                if( this_field == m_pField )
                    m_currentPageField = &m_pageFields.back();
            }
        }
    }

    if( m_pageFields.size() == 1 )
        m_pageFields.front().SetCaptureTypeForSingleFieldDisplay();
}


CoreEntryPageField* CoreEntryPage::GetPageField(const std::wstring& field_name)
{
    auto lookup = std::find_if(m_pageFields.begin(), m_pageFields.end(),
                               [&](const CoreEntryPageField& page_field) { return SO::EqualsNoCase(field_name, page_field.GetName()); });

    return ( lookup != m_pageFields.end() ) ? &(*lookup) :
                                              nullptr;
}


bool CoreEntryPage::AreAnyFieldsFilled() const
{
    for( const CoreEntryPageField& page_field : m_pageFields )
    {
        if( page_field.IsFieldFilled() )
            return true;
    }

    return false;
}
