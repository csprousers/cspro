#include "stdafx.h"
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
// DataWrapper
// --------------------------------------------------------------------------

class ActionInvoker::Runtime::DataWrapper
{
public:
    // The data repository is wrapped, or the name of a dictionary is provided and then evaluated each time.
    using WrapperType = std::variant<std::shared_ptr<DataRepository>, std::string>;
    DataWrapper(WrapperType wrapper, std::unique_ptr<const CDataDict> dictionary);

    // Returns the data repository associated with the evaluated 'dataId' value.
    static DataRepository& GetDataRepository(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    // Opens the data repository (when owned by the wrapper), or checks that the dictionary name is valid.
    static int Open(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    // Closes the data repository (when owned by the wrapper) and destroys the resource ID.
    static void Close(Runtime& runtime, const JsonNode& json_node, Caller& caller);

private:
    using EvaluateType = std::variant<std::reference_wrapper<DataRepository>,
                                      std::map<int, std::shared_ptr<DataWrapper>>::iterator>;

    // Evaluates the 'dataId' value.
    // If a string, the data repository is returned.
    // If a resource ID number, it is evaluated and a lookup into m_dataWrappers is returned.
    static EvaluateType EvaluateDataId(Runtime& runtime, const JsonNode& json_node, Caller& caller);

    static DataRepository& GetDataRepository(Runtime& runtime, std::shared_ptr<DataRepository>& data_repository);
    static DataRepository& GetDataRepository(Runtime& runtime, const std::string& dictionary_name);

    static std::unique_ptr<DataWrapper> OpenDataRepository(Runtime& runtime, const JsonNode& json_node);

private:
    WrapperType m_wrapper;
    std::unique_ptr<const CDataDict> m_dictionary;
};


ActionInvoker::Runtime::DataWrapper::DataWrapper(WrapperType wrapper, std::unique_ptr<const CDataDict> dictionary)
    :   m_wrapper(std::move(wrapper)),
        m_dictionary(std::move(dictionary))
{
}


ActionInvoker::Runtime::DataWrapper::EvaluateType ActionInvoker::Runtime::DataWrapper::EvaluateDataId(
    Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    // when specified as a string, it is a dictionary name and the interpreter can supply the dictionary
    if( json_node.Contains(JK::dataId) )
    {
        const JsonNode data_id_json_node = json_node.Get(JK::dataId);

        if( data_id_json_node.IsString() )
            return GetDataRepository(runtime, data_id_json_node.Get<std::string>());
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


DataRepository& ActionInvoker::Runtime::DataWrapper::GetDataRepository(Runtime& /*runtime*/, std::shared_ptr<DataRepository>& data_repository)
{
    ASSERT(data_repository != nullptr);
    return *data_repository;
}


DataRepository& ActionInvoker::Runtime::DataWrapper::GetDataRepository(Runtime& runtime, const std::string& dictionary_name)
{
    return runtime.GetInterpreterAccessor().GetDataRepository(dictionary_name, true);
}


DataRepository& ActionInvoker::Runtime::DataWrapper::GetDataRepository(Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    const EvaluateType evaluate_value = EvaluateDataId(runtime, json_node, caller);

    if( evaluate_value.index() == 0 )
    {
        return std::get<0>(evaluate_value);
    }

    else
    {
        return std::visit(
            [&runtime](auto& value) -> DataRepository& { return GetDataRepository(runtime, value); },
            std::get<1>(evaluate_value)->second->m_wrapper
        );
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

        data_wrapper = std::make_unique<DataWrapper>(std::move(dictionary_name), nullptr);
    }

    ASSERT(data_wrapper != nullptr);

    // add the data source using a unique ID
    return runtime.m_dataWrappers.try_emplace(
        runtime.CreateResourceId(Resource::Data, caller),
        std::move(data_wrapper)
    ).first->first;
}


std::unique_ptr<ActionInvoker::Runtime::DataWrapper>
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
    std::unique_ptr<const CDataDict> dictionary;
    std::shared_ptr<CaseAccess> case_access;

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
        // create a copy of an existing dictionary
        if( CIMSAString::IsName(dictionary_json_node->Get<std::string_view>()) )
        {
            const DataRepository& interpreter_data_repository = runtime.GetInterpreterAccessor().GetDataRepository(
                dictionary_json_node->Get<std::string_view>(), false
            );

            case_access = CaseAccess::CreateAndInitializeFullCaseAccess(
                interpreter_data_repository.GetCaseAccess().GetDataDict()
            );
        }

        // or read it from the disk
        else
        {
            dictionary = CDataDict::InstantiateAndOpen(dictionary_json_node->GetAbsolutePath());
        }
    }

    ASSERT(( dictionary != nullptr ) != ( case_access != nullptr ));

    if( dictionary != nullptr )
        case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

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

    return std::make_unique<DataWrapper>(std::move(data_repository), std::move(dictionary));
}


void ActionInvoker::Runtime::DataWrapper::Close(Runtime& runtime, const JsonNode& json_node, Caller& caller)
{
    const EvaluateType evaluate_value = EvaluateDataId(runtime, json_node, caller);

    // when calling close on a repository specified by name (to be retrieved from the interpreter),
    // we don't have to do anything as the interpreter controls when the repository should be closed
    if( evaluate_value.index() == 0 )
        return;

    const auto wrapper_lookup = std::get<1>(evaluate_value);
    const WrapperType wrapper = std::exchange(wrapper_lookup->second->m_wrapper, std::shared_ptr<DataRepository>(nullptr));

    // destroy the wrapper before closing the data repository in case an exception is thrown while closing
    runtime.DestroyResourceId(wrapper_lookup->first);
    runtime.m_dataWrappers.erase(wrapper_lookup);

    // we only need to close data sources opened using the Action Invoker
    if( std::holds_alternative<std::shared_ptr<DataRepository>>(wrapper) )
        std::get<std::shared_ptr<DataRepository>>(wrapper)->Close();
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


ActionInvoker::Result ActionInvoker::Runtime::GetQuestionnaireContentWithCaseData(QuestionnaireContentCreator& questionnaire_content_creator, std::unique_ptr<Case> data_case,
                                                                                  const JsonNode& json_node, const bool write_all_content, const bool case_content_is_from_current_case)
{
    ASSERT(data_case != nullptr);

    questionnaire_content_creator.SetCase(std::move(data_case));

    if( json_node.Contains(JK::serializationOptions) )
        questionnaire_content_creator.SetSerializationOptions(json_node.Get(JK::serializationOptions));

    if( case_content_is_from_current_case )
    {
        try
        {
            questionnaire_content_creator.SetFieldStatusRetriever(GetInterpreterAccessor().CreateFieldStatusRetriever());
        }
        catch(...) { }
    }

    std::string content = write_all_content ? questionnaire_content_creator.GetContent() :
                                              questionnaire_content_creator.GetCaseContent();

    // QuestionnaireContentCreator may write out the binary data in a case using a virtual file mapping handler;
    // if so add it to the Action Invoker's handlers
    std::shared_ptr<CaseBinaryDataVirtualFileMappingHandler> case_binary_data_virtual_file_mapping_handler = questionnaire_content_creator.GetCaseBinaryDataVirtualFileMappingHandler();

    if( case_binary_data_virtual_file_mapping_handler != nullptr )
        m_localHostKeyBasedVirtualFileMappingHandlers.emplace_back(std::move(case_binary_data_virtual_file_mapping_handler));

    return Result::JsonText(std::move(content));
}


ActionInvoker::Result ActionInvoker::Runtime::Data_getCase(const JsonNode& json_node, Caller& /*caller*/)
{
    std::shared_ptr<const CDataDict> dictionary = std::get<2>(GetApplicationComponents<std::shared_ptr<const CDataDict>>(json_node.GetOptional<std::string_view>(JK::name)));
    ASSERT(dictionary != nullptr);

    std::unique_ptr<Case> data_case;
    bool case_content_is_from_current_case;

    // get content for a specific case...
    if( json_node.Contains(JK::key) || json_node.Contains(JK::uuid) )
    {
        data_case = GetInterpreterAccessor().GetCase(dictionary->GetName(),
                                                     json_node.GetOptional<std::string>(JK::uuid),
                                                     json_node.GetOptional<std::string>(JK::key));
        case_content_is_from_current_case = false;
    }

    // ...or the current case
    else
    {
        data_case = GetInterpreterAccessor().GetCurrentCase(dictionary->GetName());
        case_content_is_from_current_case = true;
    }

    ASSERT(data_case != nullptr);

    QuestionnaireContentCreator questionnaire_content_creator;
    questionnaire_content_creator.SetDictionary(std::move(dictionary));

    return GetQuestionnaireContentWithCaseData(questionnaire_content_creator, std::move(data_case), json_node, false, case_content_is_from_current_case);
}
