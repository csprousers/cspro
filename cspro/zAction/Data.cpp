#include "stdafx.h"
#include <zUtilO/Versioning.h>
#include <zDictO/DDClass.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseBinaryDataVirtualFileMappingHandler.h>
#include <zDataO/CacheableCaseWrapperRepository.h>
#include <zDataO/ConnectionStringProperties.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DictionarySource.h>
#include <zDataO/ParadataWrapperRepository.h>
#include <zFormatterO/QuestionnaireContentCreator.h>
#include <zParadataO/ParadataDriver.h>


CREATE_JSON_KEY(dataId)
CREATE_JSON_KEY(openFlags)


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

    // Creates a QuestionnaireContentCreator (if passed a dictionary) and runs the callback function.
    // Before creating any content, call QuestionnaireContentCreator::SetCase.
    template<typename CF>
    static void WriteCaseWrapper(Runtime& runtime, const JsonNode& json_node,
                                 std::variant<std::shared_ptr<const CDataDict>, std::unique_ptr<QuestionnaireContentCreator>> dictionary_or_questionnaire_content_creator,
                                 const CF& callback_function);

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

    ASSERT(case_access != nullptr);

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
        ASSERT(!data_wrapper->IsActionInvokerOwned());
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


template<typename CF>
void ActionInvoker::Runtime::DataWrapper::WriteCaseWrapper(
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
                                                       const bool return_null_case_if_no_key_present)
{
    const char* const key = json_node.Contains(JK::uuid)     ? JK::uuid :
                            json_node.Contains(JK::position) ? JK::position :
                            json_node.Contains(JK::key)      ? JK::key :
                                                               nullptr;

    if( return_null_case_if_no_key_present && key == nullptr )
        return nullptr;

    std::unique_ptr<Case> data_case = data_repository.GetCaseAccess().CreateCase(true);

    if( key == JK::uuid )
    {
        data_repository.ReadCaseByUuid(*data_case, json_node.Get<std::string>(JK::uuid));
    }

    else if( key == JK::position )
    {
        data_repository.ReadCase(*data_case, json_node.Get<double>(JK::position));
    }

    else
    {
        ASSERT(key == JK::key);
        data_repository.ReadCase(*data_case, json_node.Get<std::string>(JK::key));
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

    DataWrapper::WriteCaseWrapper(*this, json_node, std::move(dictionary_or_questionnaire_content_creator),
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

    DataWrapper::WriteCaseWrapper(*this, json_node, data_wrapper->GetDictionary(),
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
    bool contains_case;

    if( json_node.Contains(JK::uuid) )
    {
        try
        {
            std::string key;
            std::string uuid = json_node.Get<std::string>(JK::uuid);
            double position_in_repository;

            data_repository.PopulateCaseIdentifiers(key, uuid, position_in_repository);
            contains_case = true;
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
            contains_case = false;
        }
    }

    else
    {
        contains_case = data_repository.ContainsCase(json_node.Get<std::string>(JK::key));
    }

    return Result::Bool(contains_case);
}
