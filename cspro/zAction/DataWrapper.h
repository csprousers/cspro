#pragma once

#include <zAction/ActionInvoker.h>
#include <zDataO/ISyncableDataRepository.h>
#include <engine/EngineDictionaryModifier.h>


// --------------------------------------------------------------------------
// DataWrapper
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

    // Creates an instance of the EngineDictionaryModifier for data sources connected to dictionaries and owned by the
    // interpreter. This object ensures that cases in memory from the data source are properly handled.
    // The method calls PrepareForModifications but the caller is responsible for calling FinishedWithModifications.
    std::unique_ptr<EngineDictionaryModifier> CreateEngineDictionaryModifier(Runtime& runtime);

    // Returns the data repository.
    virtual DataRepository& GetDataRepository() = 0;

    // Returns the ISyncableDataRepository object, throwing an exception if the repository does not support data synchronization.
    ISyncableDataRepository& GetSyncableDataRepository();

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
// inline implementations
// --------------------------------------------------------------------------

// Returns one of JK::uuid, JK::position, JK::key, or optionally nullptr (if default_to_key is false).
// If both a UUID and a key are specified, the UUID is prioritized.
// The position is also prioritized above the key.
inline const char* GetSpecifiedCaseIdentifier(const JsonNode& json_node, const bool default_to_key)
{
    return ( json_node.Contains(JK::uuid) )                  ? JK::uuid :
           ( json_node.Contains(JK::position) )              ? JK::position :
           ( default_to_key || json_node.Contains(JK::key) ) ? JK::key :
                                                               nullptr;
}
