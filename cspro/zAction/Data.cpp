#include "stdafx.h"
#include "CallerWrappingCaseConstructionReporter.h"
#include <zUtilO/Versioning.h>
#include <zDictO/DDClass.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseBinaryDataVirtualFileMappingHandler.h>
#include <zCaseO/CaseJsonSerializer.h>
#include <zDataO/CacheableCaseWrapperRepository.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/ConnectionStringProperties.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zDataO/DictionarySource.h>
#include <zDataO/ParadataWrapperRepository.h>
#include <zDataO/WriteCaseParameter.h>
#include <zFormatterO/QuestionnaireContentCreator.h>
#include <zParadataO/ParadataDriver.h>
#include <engine/EngineDictionaryModifier.h>


enum class ActionInvoker::DataQueryContentType { Count, Keys, Summaries, Cases };


CREATE_JSON_KEY(dataId)
CREATE_JSON_KEY(openFlags)
CREATE_JSON_KEY(replace)

CREATE_ENUM_JSON_SERIALIZER(ActionInvoker::DataQueryContentType,
    { ActionInvoker::DataQueryContentType::Count,       "count" },
    { ActionInvoker::DataQueryContentType::Keys,        "keys" },
    { ActionInvoker::DataQueryContentType::Summaries,   "summaries" },
    { ActionInvoker::DataQueryContentType::Cases,       "cases" })


// --------------------------------------------------------------------------
// DataWrapper declaration
// --------------------------------------------------------------------------

class ActionInvoker::Runtime::DataWrapper
{
public:
    // DataWrapper wraps a data repository owned by the Action Invoker (ActionInvokerOwned),
    // or the name of a dictionary is provided and then evaluated each time (InterpreterOwned).
    class ActionInvokerOwned;
    class InterpreterOwned;

    virtual ~DataWrapper() { }

    // Returns true if the Action Invoker owns the data repository.
    virtual bool IsActionInvokerOwned() const = 0;

    // Returns true if the interpreter owns the data repository.
    bool IsInterpreterOwned() const { return !IsActionInvokerOwned(); }

    // Returns the data repository.
    virtual DataRepository& GetDataRepository() = 0;

    // Returns the non-null dictionary.
    virtual std::shared_ptr<const CDataDict> GetDictionary() = 0;

    // Closes the data repository (when owned by the Action Invoker).
    virtual void Close() = 0;

    // Returns the data wrapper associated with the evaluated 'dataId' value.
    // If 'dataId' is a dictionary name, a new DataWrapper object is created.
    static std::shared_ptr<DataWrapper> GetDataWrapper(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    // Opens the data repository (when owned by the Action Invoker), or checks that the dictionary name is valid.
    static int Open(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    // Closes the data repository (when owned by the Action Invoker) and destroys the resource ID.
    static void Close(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    // Returns one of JK::uuid, JK::position, JK::key, or optionally nullptr (if default_to_key is false).
    // If both a UUID and a key are specified, the UUID is prioritized.
    // The position is also prioritized above the key.
    static const char* GetSpecifiedCaseIdentifier(const JsonNode& json_node, bool default_to_key);

    // Returns the position in the repository based on a UUID lookup.
    static double GetPositionFromUuid(DataRepository& data_repository, const JsonNode& json_node);

    // If "replace" is specified, creates a WriteCaseParameter object.
    // Exceptions are thrown:
    //   - If the specified replacement case does not exist.
    //   - If, for data repositories that do not support duplicates, replacing the case would
    //     result in a duplicate case. CSEntry, the other user of WriteCaseParameter, does this
    //     instead of relying on the data repository to do it, so we implement that check here.
    static std::unique_ptr<WriteCaseParameter> ProcessWriteCaseReplace(DataRepository& data_repository, const Case& data_case,
                                                                       const JsonNode& json_node);

    // Creates a QuestionnaireContentCreator (if passed a dictionary) and runs the callback function.
    // Before creating any content, call QuestionnaireContentCreator::SetCase.
    template<typename CF>
    static void CreateCaseContentWrapper(Runtime& runtime, const JsonNode& json_node,
                                         std::variant<std::shared_ptr<const CDataDict>, std::unique_ptr<QuestionnaireContentCreator>> dictionary_or_questionnaire_content_creator,
                                         const CF& callback_function);

    // Routines to serialize case objects to a JSON writer where an array has already been started.
    static void FillArray_CaseKeys(JsonWriter& json_writer, CaseIterator& iterator);
    static void FillArray_CaseSummaries(JsonWriter& json_writer, CaseIterator& iterator);
    static void FillArray_Cases(Runtime& runtime, const JsonNode& json_node, DataWrapper& data_wrapper,
                                JsonWriter& json_writer, CaseIterator& iterator);

private:
    using EvaluateType = std::variant<std::unique_ptr<DataWrapper>,
                                      std::map<int, std::shared_ptr<DataWrapper>>::iterator>;

    // Evaluates the 'dataId' value.
    // If a string, the data repository is returned.
    // If a resource ID number, it is evaluated and a lookup into m_dataWrappers is returned.
    static EvaluateType EvaluateDataId(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    static std::unique_ptr<ActionInvokerOwned> OpenDataRepository(Runtime& runtime, const JsonNode& json_node);
};



// --------------------------------------------------------------------------
// DataWrapper::ActionInvokerOwned
// --------------------------------------------------------------------------

class ActionInvoker::Runtime::DataWrapper::ActionInvokerOwned : public ActionInvoker::Runtime::DataWrapper
{
public:
    ActionInvokerOwned(std::shared_ptr<DataRepository> data_repository, std::shared_ptr<const CDataDict> dictionary)
        :   m_dataRepository(std::move(data_repository)),
            m_dictionary(std::move(dictionary))
    {
        ASSERT(m_dataRepository != nullptr && m_dictionary != nullptr);
    }

    bool IsActionInvokerOwned() const override
    {
        return true;
    }

    DataRepository& GetDataRepository() override
    {
        return *m_dataRepository;
    }

    std::shared_ptr<const CDataDict> GetDictionary() override
    {
        return m_dictionary;
    }

    void Close() override
    {
        m_dataRepository->Close();
    }

private:
    std::shared_ptr<DataRepository> m_dataRepository;
    std::shared_ptr<const CDataDict> m_dictionary;
};



// --------------------------------------------------------------------------
// DataWrapper::InterpreterOwned
// --------------------------------------------------------------------------

class ActionInvoker::Runtime::DataWrapper::InterpreterOwned : public ActionInvoker::Runtime::DataWrapper
{
public:
    InterpreterOwned(Runtime& runtime, std::string dictionary_name)
        :   m_runtime(runtime),
            m_dictionaryName(std::move(dictionary_name))
    {
    }


    bool IsActionInvokerOwned() const override
    {
        return false;
    }

    DataRepository& GetDataRepository() override
    {
        return m_runtime.GetInterpreterAccessor().GetDataRepository(m_dictionaryName, false);
    }

    std::shared_ptr<const CDataDict> GetDictionary() override
    {
        if( m_dictionary == nullptr )
            m_dictionary = m_runtime.GetInterpreterAccessor().GetDictionary(m_dictionaryName);

        return m_dictionary;
    }

    void Close() override
    {
    }

private:
    Runtime& m_runtime;
    std::string m_dictionaryName;
    std::shared_ptr<const CDataDict> m_dictionary;
};



// --------------------------------------------------------------------------
// DataWrapper
// --------------------------------------------------------------------------

ActionInvoker::Runtime::DataWrapper::EvaluateType ActionInvoker::Runtime::DataWrapper::EvaluateDataId(
    Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    // when specified as a string, it is a dictionary name and the interpreter can supply the dictionary
    if( json_node.Contains(JK::dataId) )
    {
        const JsonNode data_id_json_node = json_node.Get(JK::dataId);

        if( data_id_json_node.IsString() )
            return std::make_unique<InterpreterOwned>(runtime, data_id_json_node.Get<std::string>());
    }

    // otherwise evaluate the resource ID
    const int data_id = runtime.GetResourceId(
        Resource::Data, json_node, caller, JK::dataId,
        "You must specify which data source to access using '%s'.",
        "Multiple data sources are open so you must specify which one to access using '%s'."
    );

    const auto& lookup = runtime.m_dataWrappers.find(data_id);

    if( lookup == runtime.m_dataWrappers.cend() )
        throw CSProException("No data source is associated with the ID '%d'.", data_id);

    return lookup;
}



std::shared_ptr<ActionInvoker::Runtime::DataWrapper> ActionInvoker::Runtime::DataWrapper::GetDataWrapper(
    Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    EvaluateType evaluate_value = EvaluateDataId(runtime, json_node, caller);

    if( std::holds_alternative<std::unique_ptr<DataWrapper>>(evaluate_value) )
    {
        return std::move(std::get<std::unique_ptr<DataWrapper>>(evaluate_value));
    }

    else
    {
        return std::get<std::map<int, std::shared_ptr<DataWrapper>>::iterator>(evaluate_value)->second;
    }
}


int ActionInvoker::Runtime::DataWrapper::Open(Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    const char* const input_type = GetUniqueKeyFromChoices(json_node, JK::connection, JK::name);
    std::unique_ptr<DataWrapper> data_wrapper;

    if( input_type == JK::connection )
    {
        data_wrapper = OpenDataRepository(runtime, json_node);
    }

    else if( input_type == JK::name )
    {
        std::string dictionary_name = json_node.Get<std::string>(JK::name);

        // ensure that the dictionary name is valid
        runtime.GetInterpreterAccessor().GetDataRepository(dictionary_name, false);

        data_wrapper = std::make_unique<InterpreterOwned>(runtime, std::move(dictionary_name));
    }

    ASSERT(data_wrapper != nullptr);

    // add the data source using a unique ID
    return runtime.m_dataWrappers.try_emplace(
        runtime.CreateResourceId(Resource::Data, caller),
        std::move(data_wrapper)
    ).first->first;
}


std::unique_ptr<ActionInvoker::Runtime::DataWrapper::ActionInvokerOwned>
    ActionInvoker::Runtime::DataWrapper::OpenDataRepository(Runtime& runtime, const JsonNode& json_node)
{
    const ConnectionString connection_string = json_node.Get<ConnectionString>(JK::connection);

    // default to read-only
    const size_t open_flags_index = json_node.Contains(JK::openFlags) ?
        json_node.GetFromStringOptions(JK::openFlags, { "read", "readWrite", "readWriteCreate", "readWriteTruncate" }) :
        0;

    const DataRepositoryAccess access_type =
        ( open_flags_index == 0 ) ? DataRepositoryAccess::ReadOnly :
                                    DataRepositoryAccess::ReadWrite;


    const DataRepositoryOpenFlag open_flag =
        ( open_flags_index == 2 ) ? DataRepositoryOpenFlag::OpenOrCreate :
        ( open_flags_index == 3 ) ? DataRepositoryOpenFlag::CreateNew :
                                    DataRepositoryOpenFlag::OpenMustExist;

    const std::optional<JsonNode> dictionary_json_node = json_node.GetOptional<JsonNode>(JK::dictionary);
    std::shared_ptr<const CDataDict> dictionary;

    // if no dictionary is specified, the data source must have an embedded dictionary
    // or the dictionary file path must be specified in the connection string's dictionaryPath override.
    if( !dictionary_json_node.has_value() )
    {
        dictionary = DictionarySource::GetAssociatedDictionary(connection_string);

        if( dictionary == nullptr )
        {
            throw CSProException("You must specify a dictionary that describes the data in: " +
                                 connection_string.GetName(DataRepositoryNameType::Concise));
        }
    }

    // if not a string, it should be a JSON object specifying the dictionary
    else if( !dictionary_json_node->IsString() )
    {
        dictionary = CDataDict::CreateFromJson<std::unique_ptr<CDataDict>>(*dictionary_json_node);
    }

    // otherwise evaluate the string as either a dictionary name that the interpreter can evaluate,
    // or as a file path to a dictionary
    else
    {
        // use a dictionary owned by the interpreter
        if( CIMSAString::IsName(dictionary_json_node->Get<std::string_view>()) )
        {
            dictionary = runtime.GetInterpreterAccessor().GetDictionary(dictionary_json_node->Get<std::string_view>());
        }

        // or read it from the disk
        else
        {
            dictionary = CDataDict::InstantiateAndOpen(dictionary_json_node->GetAbsolutePath());
        }
    }

    ASSERT(dictionary != nullptr);

    const std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

    // case construction messages will be forwarded to the current caller
    case_access->SetCaseConstructionReporter(
        std::make_unique<CallerWrappingCaseConstructionReporter>(runtime, runtime.m_currentCaller)
    );

    std::shared_ptr<DataRepository> data_repository = DataRepository::Create(
        case_access,
        connection_string,
        access_type
    );

    // wrap the repository if using paradata
    if( Paradata::Logger::IsOpen() )
    {
        Paradata::ParadataDriver* const paradata_driver = runtime.GetInterpreterAccessor().GetParadataDriver();

        if( paradata_driver != nullptr )
        {
            data_repository = std::make_shared<ParadataWrapperRepository>(
                std::move(data_repository),
                paradata_driver,
                paradata_driver->CreateObject(Paradata::NamedObject::Type::Dictionary, case_access->GetDataDict().GetName())
            );
        }
    }

    // see if the cases should be cached
    if( connection_string.HasProperty(CSProperty::cache, CSValue::true_, true) )
        data_repository = CacheableCaseWrapperRepository::CreateCacheableCaseWrapperRepository(std::move(data_repository));

    data_repository->Open(connection_string, open_flag);

    return std::make_unique<ActionInvokerOwned>(std::move(data_repository), std::move(dictionary));
}


void ActionInvoker::Runtime::DataWrapper::Close(Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    EvaluateType evaluate_value = EvaluateDataId(runtime, json_node, caller);
    std::shared_ptr<DataWrapper> data_wrapper;

    // when calling close on a repository specified by name (to be retrieved from the interpreter),
    // we don't have to do anything as the interpreter controls when the repository should be closed;
    // Close, called at the end of the method, will do nothing
    if( std::holds_alternative<std::unique_ptr<DataWrapper>>(evaluate_value) )
    {
        data_wrapper = std::move(std::get<std::unique_ptr<DataWrapper>>(evaluate_value));
        ASSERT(data_wrapper->IsInterpreterOwned());
    }

    // for wrappers with resource IDs, destroy the wrapper before closing the data repository
    // in case an exception is thrown while closing
    else
    {
        auto& wrapper_lookup = std::get<std::map<int, std::shared_ptr<DataWrapper>>::iterator>(evaluate_value);
        data_wrapper = wrapper_lookup->second;

        runtime.DestroyResourceId(wrapper_lookup->first);
        runtime.m_dataWrappers.erase(wrapper_lookup);
    }

    ASSERT(data_wrapper != nullptr);

    data_wrapper->Close();
}


const char* ActionInvoker::Runtime::DataWrapper::GetSpecifiedCaseIdentifier(const JsonNode& json_node, const bool default_to_key)
{
    return ( json_node.Contains(JK::uuid) )                  ? JK::uuid :
           ( json_node.Contains(JK::position) )              ? JK::position :
           ( default_to_key || json_node.Contains(JK::key) ) ? JK::key :
                                                               nullptr;
}


double ActionInvoker::Runtime::DataWrapper::GetPositionFromUuid(DataRepository& data_repository, const JsonNode& json_node)
{
    std::string uuid = json_node.Get<std::string>(JK::uuid);

    if( uuid.empty() )
        throw DataRepositoryException::CaseNotFound();

    std::string key;
    double position_in_repository;

    data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);

    return position_in_repository;
}


std::unique_ptr<WriteCaseParameter> ActionInvoker::Runtime::DataWrapper::ProcessWriteCaseReplace(
    DataRepository& data_repository, const Case& data_case, const JsonNode& json_node)
{
    if( !json_node.Contains(JK::replace) )
        return nullptr;

    const JsonNode replace_json_node = json_node.Get(JK::replace);
    const char* const identifier = DataWrapper::GetSpecifiedCaseIdentifier(replace_json_node, true);

    std::string key = ( identifier == JK::key ) ? replace_json_node.Get<std::string>(JK::key) : std::string();
    std::string uuid = ( identifier == JK::uuid ) ? replace_json_node.Get<std::string>(JK::uuid) : std::string();
    double position_in_repository = ( identifier == JK::position ) ? replace_json_node.Get<double>(JK::position) : -1;

    try
    {
        data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
        throw CSProException("No case exists to replace that is identified by '%s': %s",
                             identifier,
                             replace_json_node.Get<std::string>(identifier).c_str());
    }

    // if the data repository does not support duplicate cases, make sure that this case will not result in duplicates
    if( key != data_case.GetKey() &&
        !DataRepositoryHelpers::TypeSupportsDuplicates(data_repository.GetRepositoryType()) &&
        data_repository.ContainsCase(data_case.GetKey()) )
    {
        throw CSProException("The case '%s' cannot be replaced by '%s' because it would result in two cases "
                             "with the same key in a data source that cannot contain duplicate cases.",
                             key.c_str(),
                             data_case.GetKey().c_str());
    }

    WriteCaseParameter write_case_parameter = WriteCaseParameter::CreateModifyParameter(
        CaseKey(std::move(key), position_in_repository)
    );

    // because we don't know the contents of the original case, set the notes as modified
    // to ensure that the data repository updates the notes
    write_case_parameter.SetNotesModified();

    return std::make_unique<WriteCaseParameter>(std::move(write_case_parameter));
}


template<typename CF>
void ActionInvoker::Runtime::DataWrapper::CreateCaseContentWrapper(
    Runtime& runtime, const JsonNode& json_node,
    std::variant<std::shared_ptr<const CDataDict>, std::unique_ptr<QuestionnaireContentCreator>> dictionary_or_questionnaire_content_creator,
    const CF& callback_function)
{
    std::unique_ptr<QuestionnaireContentCreator> questionnaire_content_creator;

    if( std::holds_alternative<std::unique_ptr<QuestionnaireContentCreator>>(dictionary_or_questionnaire_content_creator) )
    {
        questionnaire_content_creator = std::move(std::get<std::unique_ptr<QuestionnaireContentCreator>>(dictionary_or_questionnaire_content_creator));
        ASSERT(questionnaire_content_creator != nullptr);
    }

    else
    {
        questionnaire_content_creator = std::make_unique<QuestionnaireContentCreator>();

        ASSERT(std::get<std::shared_ptr<const CDataDict>>(dictionary_or_questionnaire_content_creator) != nullptr);
        questionnaire_content_creator->SetDictionary(std::move(std::get<std::shared_ptr<const CDataDict>>(dictionary_or_questionnaire_content_creator)));
    }

    questionnaire_content_creator->SetWriteCasePositions();

    if( json_node.Contains(JK::serializationOptions) )
        questionnaire_content_creator->SetSerializationOptions(json_node.Get(JK::serializationOptions));

    callback_function(*questionnaire_content_creator);

    // QuestionnaireContentCreator may write out the binary data in a case using a virtual file mapping handler;
    // if so add it to the Action Invoker's handlers
    std::shared_ptr<CaseBinaryDataVirtualFileMappingHandler> case_binary_data_virtual_file_mapping_handler = questionnaire_content_creator->GetCaseBinaryDataVirtualFileMappingHandler();

    if( case_binary_data_virtual_file_mapping_handler != nullptr )
        runtime.m_localHostKeyBasedVirtualFileMappingHandlers.emplace_back(std::move(case_binary_data_virtual_file_mapping_handler));
}


void ActionInvoker::Runtime::DataWrapper::FillArray_CaseKeys(JsonWriter& json_writer, CaseIterator& iterator)
{
    CaseKey case_key;

    while( iterator.NextCaseKey(case_key) )
        json_writer.Write(case_key.GetKey());
}


void ActionInvoker::Runtime::DataWrapper::FillArray_CaseSummaries(JsonWriter& json_writer, CaseIterator& iterator)
{
    CaseSummary case_summary;

    while( iterator.NextCaseKey(case_summary) )
    {
        json_writer.BeginObject()
                   .Write(JK::key, case_summary.GetKey())
                   .Write(JK::position, case_summary.GetPositionInRepository())
                   .Write(JK::label, case_summary.GetCaseLabel())
                   .Write(JK::deleted, case_summary.GetDeleted())
                   .Write(JK::verified, case_summary.GetVerified())
                   .Write(JK::caseNote, case_summary.GetCaseNote());

        if( case_summary.IsPartial() )
        {
            json_writer.BeginObject(JK::partialSave)
                       .Write(JK::mode, case_summary.GetPartialSaveMode())
                       .EndObject();
        }

        else
        {
            json_writer.WriteNull(JK::partialSave);
        }

        json_writer.EndObject();
    }
}


void ActionInvoker::Runtime::DataWrapper::FillArray_Cases(Runtime& runtime, const JsonNode& json_node, DataWrapper& data_wrapper,
                                                          JsonWriter& json_writer, CaseIterator& iterator)
{
    CreateCaseContentWrapper(runtime, json_node, data_wrapper.GetDictionary(),
        [&](QuestionnaireContentCreator& questionnaire_content_creator)
        {
            const std::shared_ptr<Case> data_case = data_wrapper.GetDataRepository().GetCaseAccess().CreateCase(true);
            questionnaire_content_creator.SetCase(data_case);

            while( iterator.NextCase(*data_case) )
                questionnaire_content_creator.WriteCaseContent(json_writer);
        });
}



// --------------------------------------------------------------------------
// Data actions
// --------------------------------------------------------------------------

ActionInvoker::Result ActionInvoker::Runtime::Data_open(const JsonNode& json_node, Caller& caller)
{
    const int data_id = ActionInvoker::Runtime::DataWrapper::Open(*this, json_node, caller);
    ASSERT(m_dataWrappers.find(data_id) != m_dataWrappers.cend());
    return Result::Number(data_id);
}


ActionInvoker::Result ActionInvoker::Runtime::Data_close(const JsonNode& json_node, Caller& caller)
{
    ActionInvoker::Runtime::DataWrapper::Close(*this, json_node, caller);
    return Result::Undefined();
}


std::unique_ptr<Case> ActionInvoker::Runtime::ReadCase(const JsonNode& json_node, DataRepository& data_repository,
                                                       const bool return_null_if_no_case_identifier_present)
{
    const char* const identifier = DataWrapper::GetSpecifiedCaseIdentifier(json_node, !return_null_if_no_case_identifier_present);

    if( identifier == nullptr )
    {
        ASSERT(return_null_if_no_case_identifier_present);
        return nullptr;
    }

    std::unique_ptr<Case> data_case = data_repository.GetCaseAccess().CreateCase(true);

    if( identifier == JK::key )
    {
        data_repository.ReadCase(*data_case, json_node.Get<std::string>(JK::key));
    }

    else if( identifier == JK::uuid )
    {
        data_repository.ReadCaseByUuid(*data_case, json_node.Get<std::string>(JK::uuid));
    }

    else
    {
        ASSERT(identifier == JK::position);
        data_repository.ReadCase(*data_case, json_node.Get<double>(JK::position));
    }

    return data_case;
}


ActionInvoker::Result ActionInvoker::Runtime::GetQuestionnaireContentWithCaseData(
    std::variant<std::shared_ptr<const CDataDict>, std::unique_ptr<QuestionnaireContentCreator>> dictionary_or_questionnaire_content_creator,
    std::unique_ptr<Case> data_case, const JsonNode& json_node,
    const bool write_all_content, const bool case_content_is_from_current_case)
{
    ASSERT(data_case != nullptr);

    std::string content;

    DataWrapper::CreateCaseContentWrapper(*this, json_node, std::move(dictionary_or_questionnaire_content_creator),
        [&](QuestionnaireContentCreator& questionnaire_content_creator)
        {
            questionnaire_content_creator.SetCase(std::move(data_case));

            if( case_content_is_from_current_case )
            {
                try
                {
                    questionnaire_content_creator.SetFieldStatusRetriever(GetInterpreterAccessor().CreateFieldStatusRetriever());
                }
                catch(...) { }
            }


            content = write_all_content ? questionnaire_content_creator.GetContent() :
                                          questionnaire_content_creator.GetCaseContent();
        });

    return Result::JsonText(std::move(content));
}


ActionInvoker::Result ActionInvoker::Runtime::Data_getCase(const JsonNode& json_node, Caller& caller)
{
    static_assert(Versioning::Number <= 8.1, "Start adding runtime warnings when using Data.getCase as opposed to Data.getCurrentCase or Data.readCase");

    std::shared_ptr<const CDataDict> dictionary = std::get<2>(GetApplicationComponents<std::shared_ptr<const CDataDict>>(json_node.GetOptional<std::string_view>(JK::name)));
    ASSERT(dictionary != nullptr);

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::dataId, dictionary->GetName())
                .WriteIfHasValue(JK::serializationOptions, json_node.GetOptional<JsonNode>(JK::serializationOptions));

    bool case_content_is_from_current_case;

    // get content for a specific case...
    if( bool using_key = json_node.Contains(JK::key); using_key || json_node.Contains(JK::uuid) )
    {
        const char* const key = using_key ? JK::key : JK::uuid;
        json_writer->Write(key, json_node.Get<std::string>(key));
        case_content_is_from_current_case = false;
    }

    // ...or the current case
    else
    {
        case_content_is_from_current_case = true;
    }

    json_writer->EndObject();

    const JsonNode& reformatted_json_node = Json::Parse(json_writer->GetString());

    return case_content_is_from_current_case ? Data_getCurrentCase(reformatted_json_node, caller) :
                                               Data_readCase(reformatted_json_node, caller);
}


ActionInvoker::Result ActionInvoker::Runtime::Data_getCurrentCase(const JsonNode& json_node, Caller& caller)
{
    const std::shared_ptr<DataWrapper> data_wrapper = DataWrapper::GetDataWrapper(*this, json_node, caller);
    std::shared_ptr<const CDataDict> dictionary = data_wrapper->GetDictionary();

    if( data_wrapper->IsActionInvokerOwned() )
    {
        throw CSProException("There is no current case for '%s' because it is not associated with an engine dictionary.",
                             dictionary->GetName().c_str());
    }

    std::unique_ptr<Case> data_case = GetInterpreterAccessor().GetCurrentCase(dictionary->GetName());

    return GetQuestionnaireContentWithCaseData(
        std::move(dictionary),
        std::move(data_case),
        json_node,
        false, // write only case content
        true // the case content is from the current case
    );
}


ActionInvoker::Result ActionInvoker::Runtime::Data_readCase(const JsonNode& json_node, Caller& caller)
{
    const std::shared_ptr<DataWrapper> data_wrapper = DataWrapper::GetDataWrapper(*this, json_node, caller);
    DataRepository& data_repository = data_wrapper->GetDataRepository();

    std::unique_ptr<Case> data_case = ReadCase(json_node, data_repository, false);
    ASSERT(data_case != nullptr);

    std::string case_content;

    DataWrapper::CreateCaseContentWrapper(*this, json_node, data_wrapper->GetDictionary(),
        [&](QuestionnaireContentCreator& questionnaire_content_creator)
        {
            questionnaire_content_creator.SetCase(std::move(data_case));
            case_content = questionnaire_content_creator.GetCaseContent();
        });

    return Result::JsonText(std::move(case_content));
}


ActionInvoker::Result ActionInvoker::Runtime::Data_contains(const JsonNode& json_node, Caller& caller)
{
    const std::shared_ptr<DataWrapper> data_wrapper = DataWrapper::GetDataWrapper(*this, json_node, caller);
    DataRepository& data_repository = data_wrapper->GetDataRepository();

    const char* const identifier = DataWrapper::GetSpecifiedCaseIdentifier(json_node, true);
    bool contains_case;

    if( identifier == JK::key )
    {
        contains_case = data_repository.ContainsCase(json_node.Get<std::string>(JK::key));
    }

    else
    {
        try
        {
            if( json_node.Contains(JK::uuid) )
            {
                DataWrapper::GetPositionFromUuid(data_repository, json_node);
            }

            else
            {
                std::string key;
                std::string uuid;
                double position_in_repository = json_node.Get<double>(JK::position);
                data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);
            }

            contains_case = true;
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
            contains_case = false;
        }
    }

    return Result::Bool(contains_case);
}


ActionInvoker::Result ActionInvoker::Runtime::Data_countCases(const JsonNode& json_node, Caller& caller)
{
    return QueryDataRepository(json_node, caller, DataQueryContentType::Count);
}


ActionInvoker::Result ActionInvoker::Runtime::Data_query(const JsonNode& json_node, Caller& caller)
{
    return QueryDataRepository(json_node, caller, std::nullopt);
}


ActionInvoker::Result ActionInvoker::Runtime::Data_queryCases(const JsonNode& json_node, Caller& caller)
{
    return QueryDataRepository(json_node, caller, DataQueryContentType::Cases);
}


ActionInvoker::Result ActionInvoker::Runtime::Data_queryKeys(const JsonNode& json_node, Caller& caller)
{
    return QueryDataRepository(json_node, caller, DataQueryContentType::Keys);
}


ActionInvoker::Result ActionInvoker::Runtime::QueryDataRepository(const JsonNode& json_node, Caller& caller,
                                                                  std::optional<DataQueryContentType> content_type)
{
    const std::shared_ptr<DataWrapper> data_wrapper = DataWrapper::GetDataWrapper(*this, json_node, caller);
    DataRepository& data_repository = data_wrapper->GetDataRepository();

    // parse "content"
    const std::optional<DataQueryContentType> specified_content_type = json_node.GetOptional<DataQueryContentType>(JK::content);

    if( content_type.has_value() )
    {
        // if here via an action like Data.queryKeys, make sure that a different content type is not specified
        if( specified_content_type.has_value() && *content_type != *specified_content_type )
        {
            throw CSProException("The action does not support querying content of the type '%s'",
                                 Json::ToJson(*specified_content_type).c_str());
        }
    }

    else if( specified_content_type.has_value() )
    {
        content_type = *specified_content_type;
    }

    else
    {
        throw CSProException("You must specify the type of content to query.");
    }

    // parse any filters
    const CaseIteratorSettings iterator_settings = CaseIteratorSettings::CreateFromJson(json_node, CaseIterationCaseStatus::NotDeletedOnly);

    // if requesting a count, we can now run the query
    if( *content_type == DataQueryContentType::Count )
    {
        return Result::Number(
            data_repository.GetNumberCases(iterator_settings.GetStatus(), iterator_settings.GetParameters())
        );
    }

    // otherwise we will use an iterator to process the query
    const CaseIterationContent iteration_content =
        ( *content_type == DataQueryContentType::Keys )      ? CaseIterationContent::CaseKey :
        ( *content_type == DataQueryContentType::Summaries ) ? CaseIterationContent::CaseSummary:
                                                               CaseIterationContent::Case;

    // create the iterator, parsing "offset" and "limit"
    const std::unique_ptr<CaseIterator> iterator = data_repository.CreateIterator(
        iteration_content,
        iterator_settings,
        json_node.Contains(JK::offset) ? json_node.Get<size_t>(JK::offset) : 0,
        json_node.Contains(JK::limit) ? json_node.Get<size_t>(JK::limit) : std::numeric_limits<size_t>::max()
    );

    // return an array with the contents of the query
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();
    json_writer->SetVerbose();

    json_writer->BeginArray();

    switch( iteration_content )
    {
        case CaseIterationContent::CaseKey:
            DataWrapper::FillArray_CaseKeys(*json_writer, *iterator);
            break;

        case CaseIterationContent::CaseSummary:
            DataWrapper::FillArray_CaseSummaries(*json_writer, *iterator);
            break;

        default:
            ASSERT(iteration_content == CaseIterationContent::Case);
            DataWrapper::FillArray_Cases(*this, json_node, *data_wrapper, *json_writer, *iterator);
            break;
    }

    json_writer->EndArray();

    return Result::JsonText(*json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::Data_deleteCase(const JsonNode& json_node, Caller& caller)
{
    const std::shared_ptr<DataWrapper> data_wrapper = DataWrapper::GetDataWrapper(*this, json_node, caller);
    DataRepository& data_repository = data_wrapper->GetDataRepository();

    const char* const identifier = DataWrapper::GetSpecifiedCaseIdentifier(json_node, true);

    // make sure that data sources connected to dictionaries owned by the interpreter and properly updated
    std::unique_ptr<EngineDictionaryModifier> engine_dictionary_modifier;

    if( data_wrapper->IsInterpreterOwned() )
    {
        engine_dictionary_modifier = GetInterpreterAccessor().CreateEngineDictionaryModifier(data_wrapper->GetDictionary()->GetName());
        engine_dictionary_modifier->PrepareForModifications();
    }

    if( identifier == JK::key )
    {
        data_repository.DeleteCase(json_node.Get<std::string>(JK::key));
    }

    else
    {
        const double position_in_repository =
            ( identifier == JK::uuid ) ? DataWrapper::GetPositionFromUuid(data_repository, json_node) :
                                         json_node.Get<double>(JK::position);

        data_repository.DeleteCase(position_in_repository);
    }

    if( engine_dictionary_modifier != nullptr )
        engine_dictionary_modifier->FinishedWithModifications();

    return Result::Undefined();
}


ActionInvoker::Result ActionInvoker::Runtime::Data_writeCase(const JsonNode& json_node, Caller& caller)
{
    const std::shared_ptr<DataWrapper> data_wrapper = DataWrapper::GetDataWrapper(*this, json_node, caller);
    DataRepository& data_repository = data_wrapper->GetDataRepository();

    std::shared_ptr<const CaseAccess> case_access = data_repository.GetSharedCaseAccess();
    const std::shared_ptr<Case> data_case = case_access->CreateCase(true);

    CaseJsonParserHelper case_json_parser_helper(std::move(case_access));
    case_json_parser_helper.ParseJson(*data_case, json_node.Get(JK::case_));

     // make sure the case is valid
    std::vector<std::string> added_record_names;
    data_case->AddRequiredRecords(false, &added_record_names);

    if( !added_record_names.empty() )
        throw CSProException("The case is missing required records: " + SO::CreateSingleString(added_record_names));

    // if this is going to replace an existing case, create a modification parameter
    const std::unique_ptr<const WriteCaseParameter> write_case_parameter = DataWrapper::ProcessWriteCaseReplace(
        data_repository, *data_case, json_node
    );

    // make sure that data sources connected to dictionaries owned by the interpreter and properly updated
    std::unique_ptr<EngineDictionaryModifier> engine_dictionary_modifier;

    if( data_wrapper->IsInterpreterOwned() )
    {
        engine_dictionary_modifier = GetInterpreterAccessor().CreateEngineDictionaryModifier(data_wrapper->GetDictionary()->GetName());
        engine_dictionary_modifier->PrepareForModifications();
    }

    data_repository.WriteCase(*data_case, write_case_parameter.get());

    if( engine_dictionary_modifier != nullptr )
        engine_dictionary_modifier->FinishedWithModifications();

    return ( data_case->GetPositionInRepository() != -1 ) ? Result::Number(data_case->GetPositionInRepository()) :
                                                            Result::Undefined();
}
