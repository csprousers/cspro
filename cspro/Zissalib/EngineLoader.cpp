#include "StdAfx.h"
#include <engine/Engdrv.h>
#include <zEngineO/EngineDictionaryFactory.h>
#include <zEngineO/Flow.h>


// FLOW_TODO with time, more and more functionality can be moved into these loading methods


void CEngineDriver::IssueLoadException(int message_number, ...)
{
    va_list parg;
    va_start(parg, message_number);
    std::wstring message = m_pEngineDriver->GetSystemMessageIssuer().GetFormattedMessageVA(message_number, parg);
    va_end(parg);

    CREATE_CSPRO_EXCEPTION(EngineLoadException) // FLOW_TODO determine if this should be part of ApplicationLoadException, or even if it has to be a separate named exception

    ASSERT(Issamod == ModuleType::Batch);  // FLOW_TODO make sure that the designer, csentry, etc. catch the engine loading exceptions

    throw EngineLoadException(_T("There was an error loading the engine: %s"), message.c_str());
}


void CEngineDriver::LoadApplication()
{
    // FLOW_TODO for now this is called by CEngineDriver::exapplinit for the new driver as a substitute for CEngineDriver::attrload and CEngineDriver::LoadApplChildren and throws exceptions on error
    ASSERT(m_pApplication != nullptr);

    // FLOW_TODO eventually add more types and look at exapplinit to see how things like POSTCALC are set [if these even matter]
    ASSERT(m_pApplication->GetEngineAppType() == EngineAppType::Batch);
    Appl.ApplicationType = ModuleType::Batch;
    Appl.ApplicationTypeText = _T("BATCH");

    // insert the application into the symbol table
    m_engineData->AddSymbol(m_pEngineArea->m_Appl);

    ASSERT(GetSymbolTable().GetTableSize() == ( Logic::SymbolTable::FirstValidSymbolIndex + 1 )); // the first symbol is nullptr


    // load the form files (along with their dictionaries), turning them into flows
    SymbolSubType flow_subtype = SymbolSubType::Primary;

    for( const auto& form_file : m_pApplication->GetRuntimeFormFiles() )
    {
        LoadApplicationFlow(form_file, flow_subtype);
        flow_subtype = SymbolSubType::Secondary;
    }


    // load external, special output, and working dictionaries
    for( const auto& dictionary : m_pApplication->GetRuntimeExternalDictionaries() )
        LoadApplicationDictionary(dictionary);
}


std::shared_ptr<EngineDictionary> CEngineDriver::LoadApplicationDictionary(std::shared_ptr<const CDataDict> dictionary,
                                                                           std::optional<SymbolSubType> flow_subtype/* = std::nullopt*/)
{
    // make sure the dictionary name is unique
    if( GetSymbolTable().NameExists(dictionary->GetName()) )
        IssueLoadException(18000, _T("dictionary"), dictionary->GetName().GetString());

    // update the dictionary's pointers
    std::const_pointer_cast<CDataDict>(dictionary)->UpdatePointers(); // DD_STD_REFACTOR_TODO should not be necessary when finished


    // insert the dictionary into the symbol table
    std::shared_ptr<EngineDictionary> engine_dictionary = EngineDictionaryFactory::CreateDictionary(dictionary, *m_engineData);
    m_engineData->AddSymbol(engine_dictionary);

    std::const_pointer_cast<CDataDict>(dictionary)->SetSymbol(engine_dictionary->GetSymbolIndex());


    // process the dictionary type, setting the subtype (which, if invalid, will default to an external dictionary)
    DictionaryType dictionary_type = m_pApplication->GetDictionaryType(*dictionary);

    SymbolSubType subtype =
        ( dictionary_type == DictionaryType::Input )   ? SymbolSubType::Input :
        ( dictionary_type == DictionaryType::Output )  ? SymbolSubType::Output :
        ( dictionary_type == DictionaryType::Working ) ? SymbolSubType::Work :
                                                         SymbolSubType::External;
    engine_dictionary->SetSubType(subtype);

    if( subtype == SymbolSubType::Output )
        m_pEngineDriver->SetHasOutputDict(true);


    // check that the dictionary subtype matches with the flow's subtype (if applicable)
    auto check_flow_dictionary_match = [&](auto flow_subtype_to_check, auto required_dictionary_subtype)
    {
        if( flow_subtype == flow_subtype_to_check && subtype != required_dictionary_subtype )
        {
            IssueLoadException(18001, dictionary->GetName().GetString(), ToString(dictionary_type),
                ToString(flow_subtype_to_check), ToString(required_dictionary_subtype));
        }
    };

    check_flow_dictionary_match(SymbolSubType::Primary, SymbolSubType::Input);
    check_flow_dictionary_match(SymbolSubType::Secondary, SymbolSubType::External);

    // input dictionaries must be part of the primary flow
    if( subtype == SymbolSubType::Input && flow_subtype != SymbolSubType::Primary )
        IssueLoadException(18002, dictionary->GetName().GetString(), ToString(dictionary_type));


    return engine_dictionary;
}


void CEngineDriver::LoadApplicationFlow(std::shared_ptr<CDEFormFile> form_file, SymbolSubType flow_subtype)
{
    // make sure the form file name is unique
    if( GetSymbolTable().NameExists(form_file->GetName()) )
        IssueLoadException(18000, _T("form file"), form_file->GetName().GetString());

    // update the form file's pointers
    form_file->UpdatePointers();


    // add the form file's dictionary
    auto engine_dictionary = LoadApplicationDictionary(form_file->GetSharedDictionary(), flow_subtype);


    // insert the form file (as a flow) into the symbol table
    int flow_symbol_index = m_engineData->AddSymbol(std::make_unique<Flow>(form_file, flow_subtype, engine_dictionary));

    form_file->SetSymbol(flow_symbol_index);


    // BATCH_FLOW_TODO add forms
}
