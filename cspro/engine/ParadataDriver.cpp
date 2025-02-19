#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "ParadataDriver.h"
#include <zEngineO/AllSymbols.h>
#include <zToolsO/VarFuncs.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <zParadataO/Logger.h>
#include <zDictO/DDClass.h>
#include <zMessageO/MessageManager.h>
#include <ZBRIDGEO/npff.h>
#include <zCaseO/Case.h>
#include <Zissalib/CsDriver.h>

using namespace Paradata;


EngineParadataDriver::EngineParadataDriver(CIntDriver& interpreter)
    :   m_pIntDriver(&interpreter),
        m_pEngineArea(m_pIntDriver->m_pEngineArea),
        m_pEngineDriver(m_pIntDriver->m_pEngineDriver)
{
}


const Logic::SymbolTable& EngineParadataDriver::GetSymbolTable() const
{
    return m_pEngineArea->GetSymbolTable();
}


void EngineParadataDriver::ClearCachedObjects()
{
    m_cachedSymbolObjects.clear();
    m_cachedNonSymbolObjects.clear();
}


bool EngineParadataDriver::GetRecordIteratorLoadCases() const
{
    const ParadataProperties& paradata_properties = m_pEngineDriver->m_pPifFile->GetApplication()->GetApplicationProperties().GetParadataProperties();
    return paradata_properties.GetRecordIteratorLoadCases();
}


std::unique_ptr<Paradata::NamedObject> EngineParadataDriver::CreateObjectWorker(const Symbol& symbol)
{
    NamedObject::Type type;
    std::shared_ptr<NamedObject> parent;

    switch( symbol.GetType() )
    {
        case SymbolType::Dictionary:
        {
            type = NamedObject::Type::Dictionary;
            // ENGINECR_TODO(CreateObjectWorker) parent = GetPrimaryFlowObject();
            break;
        }

        case SymbolType::Flow:
        case SymbolType::Pre80Flow:
        {
            type = NamedObject::Type::Flow;
            break;
        }

        case SymbolType::Pre80Dictionary:
        {
            type = NamedObject::Type::Dictionary;
            parent = GetPrimaryFlowObject();
            break;
        }

        case SymbolType::Group:
        {
            const GROUPT* pGroupT = assert_cast<const GROUPT*>(&symbol);
            const FLOW* pFlow = pGroupT->GetFlow();

            type = ( pGroupT->GetGroupType() == GROUPT::Level ) ? NamedObject::Type::Level : NamedObject::Type::Group;
            parent = CreateObject(*pFlow);
            break;
        }

        case SymbolType::Record:
        {
            const EngineRecord& engine_record = assert_cast<const EngineRecord&>(symbol);

            type = NamedObject::Type::Record;
            parent = CreateObject(engine_record.GetEngineDictionary());
            break;
        }

        case SymbolType::Section:
        {
            const SECT* const pSecT = assert_cast<const SECT*>(&symbol);
            const DICT* const pDicT = pSecT->GetDicT();

            type = NamedObject::Type::Record;
            parent = CreateObject(*pDicT);
            break;
        }

        case SymbolType::Variable:
        {
            const VART* const pVarT = assert_cast<const VART*>(&symbol);
            const DICT* const pDicT = pVarT->GetDPT();

            type = NamedObject::Type::Item;
            parent = CreateObject(*pDicT);
            break;
        }

        case SymbolType::ValueSet:
        {
            const ValueSet* const value_set = assert_cast<const ValueSet*>(&symbol);
            const VART* const pVarT = value_set->GetVarT();

            type = NamedObject::Type::ValueSet;
            parent = CreateObject(*pVarT);
            break;
        }

        case SymbolType::Block:
        {
            const EngineBlock& engine_block = assert_cast<const EngineBlock&>(symbol);
            const GROUPT* const pGroupT = engine_block.GetGroupT();

            type = NamedObject::Type::Block;
            parent = CreateObject(*pGroupT);
            break;
        }

        default:
        {
            ASSERT(false);

            type = NamedObject::Type::Other;
            parent = GetPrimaryFlowObject();
            break;
        }
    }

    return std::make_unique<NamedObject>(type, symbol.GetName(), std::move(parent));
}


std::shared_ptr<Paradata::NamedObject> EngineParadataDriver::CreateObject(const Symbol& symbol)
{
    // some objects, like dynamic value sets, can't be cached
    if( symbol.GetSymbolIndex() <= 0 )
        return CreateObjectWorker(symbol);

    // use the cached symbol if available
    const size_t index = static_cast<size_t>(symbol.GetSymbolIndex());

    // otherwise use the cached symbol if available
    if( index >= m_cachedSymbolObjects.size() )
        m_cachedSymbolObjects.resize(index + 1);

    // create a new NamedObject and cache it
    if( m_cachedSymbolObjects[index] == nullptr )
        m_cachedSymbolObjects[index] = CreateObjectWorker(symbol);

    return m_cachedSymbolObjects[index];
}


std::shared_ptr<Paradata::NamedObject> EngineParadataDriver::GetPrimaryFlowObject()
{
    if( m_pEngineDriver->UseNewDriver() )
    {
        return CreateObject(*m_pEngineArea->GetEngineData().flows.front());
    }

    else
    {
        return CreateObject(*m_pEngineDriver->GetPrimaryFlow());
    }
}


std::shared_ptr<Paradata::NamedObject> EngineParadataDriver::CreateObject(const Paradata::NamedObject::Type type, const std::string_view name_sv)
{
    // there shouldn't be too many non-symbol objects so linearly search for a cached one
    const auto& lookup = std::find_if(m_cachedNonSymbolObjects.cbegin(), m_cachedNonSymbolObjects.cend(),
        [&](const std::shared_ptr<Paradata::NamedObject>& named_object)
        {
            return ( named_object->GetType() == type && named_object->GetName() == name_sv );
        });

    if( lookup != m_cachedNonSymbolObjects.cend() )
        return *lookup;

    // if not found, add one
    return m_cachedNonSymbolObjects.emplace_back(std::make_unique<NamedObject>(type, std::string(name_sv), GetPrimaryFlowObject()));
}



// --------------------------------------------------------------------------
//  Field methods
// --------------------------------------------------------------------------

std::unique_ptr<FieldInfo> EngineParadataDriver::CreateFieldInfo(const VART* const pVarT, const CNDIndexes& theIndex)
{
    ASSERT(!theIndex.isZeroBased()); // this is only for one-based indices

    ItemIndex item_index;
    m_pIntDriver->ConvertIndex(theIndex, item_index);

    return CreateFieldInfo(pVarT, item_index);
}


std::unique_ptr<FieldInfo> EngineParadataDriver::CreateFieldInfo(const VART* const pVarT, const double* const pdIndices)
{
    UserIndexesArray* const theArray = (UserIndexesArray*)(*(&pdIndices)); // a hack to cast to UserIndexesArray typedef
    C3DObject the3dObject;
    CsDriver::PassTo3D(&the3dObject, pVarT, *theArray);

    ItemIndex item_index;
    m_pIntDriver->ConvertIndex(the3dObject, item_index);

    return CreateFieldInfo(pVarT, item_index);
}


std::unique_ptr<FieldInfo> EngineParadataDriver::CreateFieldInfo(const DEFLD3* const pDeFld)
{
    VART* const pVarT = VPT(pDeFld->GetSymbol());
    return CreateFieldInfo(pVarT, pDeFld->GetIndexes());
}


std::unique_ptr<FieldInfo> EngineParadataDriver::CreateFieldInfo(const VART* const pVarT, const ItemIndex& item_index)
{
    return std::make_unique<FieldInfo>(CreateObject(*pVarT), pVarT->GetCaseItem()->GetItemIndexHelper().GetOneBasedOccurrences(item_index));
}


std::unique_ptr<FieldValueInfo> EngineParadataDriver::CreateFieldValueInfo(const VART* const pVarT, const CNDIndexes& theIndex)
{
    FieldValueInfo::SpecialType special_type = FieldValueInfo::SpecialType::NotSpecial;
    std::string value_text;

    VARX* const pVarX = pVarT->GetVarX();

    if( pVarT->IsNumeric() )
    {
        double value = m_pIntDriver->GetVarFloatValue(pVarX, theIndex);

        if( IsSpecial(value) )
        {
            if( value == NOTAPPL )
            {
                special_type = FieldValueInfo::SpecialType::Notappl;
            }

            else if( value == MISSING )
            {
                special_type = FieldValueInfo::SpecialType::Missing;
            }

            else if( value == DEFAULT )
            {
                special_type = FieldValueInfo::SpecialType::Default;
            }

            else if( value == REFUSED )
            {
                special_type = FieldValueInfo::SpecialType::Refused;
            }

            value = pVarX->varoutval(value);
        }

        const int numeric_length = pVarT->GetLength() + ( pVarT->GetDecChar() ? 0 : 1 );
        value_text = dvaltochar<std::string>(value, numeric_length, pVarT->GetDecimals());

        if( pVarT->GetDecimals() > 0 )
        {
            // right-trim any zeros
            SO::MakeTrimRight(value_text, '0');

            // prevent the text from ending in a decimal
            SO::MakeTrimRight(value_text, '.');
        }

        // left-trim any spaces
        SO::MakeTrimLeft(value_text);
    }

    else
    {
        auto wide_buffer = std::make_unique_for_overwrite<wchar_t[]>(pVarT->GetLength());
        _tmemcpy(wide_buffer.get(), m_pIntDriver->GetVarAsciiAddr(pVarX, theIndex), pVarT->GetLength());

        value_text = UTF8_TODO::GetUtf8(std::wstring_view(wide_buffer.get(), pVarT->GetLength()));

        // right-trim any spaces
        SO::MakeTrimRight(value_text);
    }

    return std::make_unique<FieldValueInfo>(CreateObject(*pVarT),
                                            special_type,
                                            std::move(value_text));
}


std::unique_ptr<FieldValidationInfo> EngineParadataDriver::CreateFieldValidationInfo(const VART* const pVarT)
{
    const ValueSet* const value_set = pVarT->GetCurrentValueSet();
    const bool notappl_allowed = ( ( pVarT->m_iBehavior & CANENTER_NOTAPPL ) != 0 );
    const bool notappl_confirmation = ( ( pVarT->m_iBehavior & CANENTER_NOTAPPL_NOCONFIRM ) == 0 );
    const bool out_of_range_allowed = ( ( pVarT->m_iBehavior & CANENTER_OUTOFRANGE ) != 0 );
    const bool out_of_range_confirmation = ( ( pVarT->m_iBehavior & CANENTER_OUTOFRANGE_NOCONFIRM ) == 0 );

    return std::make_unique<FieldValidationInfo>(CreateObject(*pVarT),
                                                 ( value_set != nullptr ) ? CreateObject(*value_set) : nullptr,
                                                 notappl_allowed,
                                                 notappl_confirmation,
                                                 out_of_range_allowed,
                                                 out_of_range_confirmation);
}


std::unique_ptr<Paradata::MessageEvent> EngineParadataDriver::CreateMessageEvent(const std::variant<MessageType, FunctionCode> message_type_or_function_code,
                                                                                 int message_number, SharableString message_text)
{
    MessageEvent::Source source;
    MessageType message_type;
    SharableString unformatted_message_text;
    std::string message_language;

    if( std::holds_alternative<MessageType>(message_type_or_function_code) )
    {
        source = MessageEvent::Source::System;
        message_type = std::get<MessageType>(message_type_or_function_code);
    }

    else
    {
        const FunctionCode& function_code = std::get<FunctionCode>(message_type_or_function_code);
        source = ( function_code == FNERRMSG_CODE )  ? Paradata::MessageEvent::Source::Errmsg :
                 ( function_code == FNWARNING_CODE ) ? Paradata::MessageEvent::Source::Warning :
                                                       Paradata::MessageEvent::Source::Logtext;
        message_type = MessageType::User;
    }

    // process unnumbered messages
    if( message_type == MessageType::User )
    {
        const MessageManager& user_message_manager = m_pEngineDriver->GetUserMessageManager();
        const int message_number_for_display = user_message_manager.GetMessageNumberForDisplay(message_number);

        if( message_number != message_number_for_display )
        {
            unformatted_message_text = user_message_manager.GetUnnumberedMessageText(message_number);
            message_number = message_number_for_display;
        }
    }

    // for numbered messages, we need to look up the unformatted text and the language name
    if( !unformatted_message_text.IsSet() )
    {
        const MessageFile& message_file = ( source == MessageEvent::Source::System ) ? m_pEngineDriver->GetSystemMessageManager().GetMessageFile() :
                                                                                       m_pEngineDriver->GetUserMessageManager().GetMessageFile();

        std::tie(unformatted_message_text, message_language) = message_file.GetMessageTextAndCurrentLanguage(message_number);
    }

    return std::make_unique<Paradata::MessageEvent>(source, message_type, message_number,
                                                    std::move(message_text), std::move(unformatted_message_text),
                                                    CreateObject(NamedObject::Type::Language, message_language));
}



// --------------------------------------------------------------------------
// Other methods
// --------------------------------------------------------------------------

void EngineParadataDriver::RegisterAndLogEvent(std::shared_ptr<Event> event, const void* const instance_object/* = nullptr*/)
{
    ASSERT(m_pIntDriver->m_iProgType == PROCTYPE_PRE ||
           m_pIntDriver->m_iProgType == PROCTYPE_ONFOCUS ||
           m_pIntDriver->m_iProgType == PROCTYPE_KILLFOCUS ||
           m_pIntDriver->m_iProgType == PROCTYPE_POST ||
           m_pIntDriver->m_iProgType == PROCTYPE_ONOCCCHANGE);

    if( m_pIntDriver->m_iExSymbol > 0 )
        event->SetProcInformation(CreateObject(NPT_Ref(m_pIntDriver->m_iExSymbol)), m_pIntDriver->m_iProgType);

    Logger::LogEvent(std::move(event), instance_object);
}


void EngineParadataDriver::LogEngineEvent(const ParadataEngineEvent engine_event)
{
    const Application* const application = m_pEngineDriver->m_pPifFile->GetApplication();
    const ParadataProperties& paradata_properties = application->GetApplicationProperties().GetParadataProperties();

    if( engine_event == ParadataEngineEvent::ApplicationStart )
    {
        if( paradata_properties.GetCollectionType() != ParadataProperties::CollectionType::No )
            Logger::Start(UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetParadataFilename()), application);
    }

    if( !Logger::IsOpen() )
        return;

    std::unique_ptr<Event> event;

    // application events
    if( engine_event == ParadataEngineEvent::ApplicationStart )
    {
        event = ApplicationEvent::CreateStartEvent(UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetAppFName()),
                                                   ToString(application->GetEngineAppType()),
                                                   application->GetName(),
                                                   application->GetVersion(),
                                                   UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetPifFileName()),
                                                   application->GetSerializerArchiveVersion());
    }

    else if( engine_event == ParadataEngineEvent::ApplicationStop )
    {
        event = ApplicationEvent::CreateStopEvent();
    }


    // session events
    else if( engine_event == ParadataEngineEvent::SessionStart )
    {
        if( Issamod == ModuleType::Entry )
        {
            CEntryDriver* const pEntryDriver = assert_cast<CEntryDriver*>(m_pEngineDriver);
            event = SessionEvent::CreateStartEvent(pEntryDriver->dedemode(), UTF8_TODO::GetUtf8(pEntryDriver->GetOperatorId()));
        }

        else
        {
            event = SessionEvent::CreateStartEvent();
        }
    }

    else if( engine_event == ParadataEngineEvent::SessionStop )
    {
        event = SessionEvent::CreateStopEvent();
    }


    // case events
    else if( engine_event == ParadataEngineEvent::CaseStart )
    {
        if( m_pEngineDriver->UseNewDriver() )
        {
            EngineDictionary& input_engine_dictionary = *m_pEngineArea->m_engineData->engine_dictionaries.front();
            Case& data_case = input_engine_dictionary.GetEngineCase().GetCase();

            // if there is no UUID (because it is a new case or a case from a text repository), then create one
            event = CaseEvent::CreateStartEvent(m_pIntDriver->m_paradataDriver->CreateObject(input_engine_dictionary), data_case.GetOrCreateUuid());
        }

        else
        {
            DICX* const pDicX = DIX(0);
            Case& data_case = pDicX->GetCase();

            // if there is no UUID (because it is a new case or a case from a text repository), then create one
            event = CaseEvent::CreateStartEvent(m_pIntDriver->m_paradataDriver->CreateObject(*pDicX->GetDicT()), data_case.GetOrCreateUuid());
        }

        Logger::UpdateBackgroundCollectionParameters(application);
    }

    else if( engine_event == ParadataEngineEvent::CaseStop )
    {
        event = CaseEvent::CreateStopEvent();
    }


    ASSERT(event != nullptr);

    // log the device state when logging any engine event in a data entry application, or when
    // logging the application events in a batch application
    bool log_device_state_after_event = ( engine_event == ParadataEngineEvent::ApplicationStart );
    bool log_device_state_before_event = ( engine_event == ParadataEngineEvent::ApplicationStop );

    if( Issamod == ModuleType::Entry )
    {
        log_device_state_after_event |= ( ( engine_event == ParadataEngineEvent::SessionStart ) || ( engine_event == ParadataEngineEvent::CaseStart ) );
        log_device_state_before_event |= ( ( engine_event == ParadataEngineEvent::SessionStop ) || ( engine_event == ParadataEngineEvent::CaseStop ) );
    }

    if( log_device_state_before_event )
        Logger::LogEvent(std::make_unique<DeviceStateEvent>());

    Logger::LogEvent(std::move(event));

    if( log_device_state_after_event )
        Logger::LogEvent(std::make_unique<DeviceStateEvent>());

    if( engine_event == ParadataEngineEvent::ApplicationStart )
    {
        if( paradata_properties.GetRecordInitialPropertyValues() )
            m_pIntDriver->m_paradataDriver->LogProperties();
    }

    else if( engine_event == ParadataEngineEvent::ApplicationStop )
    {
        Logger::Stop();
    }
}


void EngineParadataDriver::ProcessCachedEvents(const std::vector<std::string>& event_strings)
{
    for( const std::string_view event_string_sv : event_strings )
    {
        // the event string will appear as: <event type>;<timestamp>;<event information>
        const size_t semicolon_pos1 = event_string_sv.find(';');
        const size_t semicolon_pos2 = event_string_sv.find(';', semicolon_pos1 + 1);
        const std::string_view event_type_sv = event_string_sv.substr(0, semicolon_pos1);
        const std::string timestamp(event_string_sv.substr(semicolon_pos1 + 1, semicolon_pos2 - semicolon_pos1 - 1));
        const std::string_view event_information_sv = event_string_sv.substr(semicolon_pos2 + 1);

        std::unique_ptr<Event> event;

        if( SO::StartsWith(event_type_sv, "gps") )
        {
            event = m_pIntDriver->CreateParadataGpsEvent(event_type_sv, event_information_sv);
        }

        else
        {
            ASSERT(false);
        }

        event->SetTimestamp(atof(timestamp.c_str()));
        Logger::LogEvent(std::move(event));
    }
}
