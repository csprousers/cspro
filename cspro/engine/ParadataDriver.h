#pragma once

#include <zParadataO/IParadataDriver.h>
#include <zParadataO/NamedObject.h>

class CDataDict;
class CEngineArea;
class CIntDriver;

namespace Paradata
{
    class Event;
    class FieldInfo;
    class FieldValueInfo;
    class FieldValidationInfo;
};


enum class ParadataEngineEvent
{
    ApplicationStart,
    ApplicationStop,
    SessionStart,
    SessionStop,
    CaseStart,
    CaseStop
};


class ParadataDriver : public Paradata::IParadataDriver
{
public:
    ParadataDriver(CIntDriver* pIntDriver);

    void ClearCachedObjects();

    bool GetRecordIteratorLoadCases() const override;

    std::shared_ptr<Paradata::NamedObject> CreateObject(const Symbol& symbol);
    std::shared_ptr<Paradata::NamedObject> CreateObject(Paradata::NamedObject::Type type, wstring_view name) override;

    std::shared_ptr<Paradata::FieldInfo> CreateFieldInfo(const VART* pVarT, const CNDIndexes& theIndex);
    std::shared_ptr<Paradata::FieldInfo> CreateFieldInfo(const VART* pVarT, const double* pdIndices);
    std::shared_ptr<Paradata::FieldInfo> CreateFieldInfo(const DEFLD3* pDeFld);
    std::shared_ptr<Paradata::FieldInfo> CreateFieldInfo(const VART* pVarT, const ItemIndex& item_index);

    std::shared_ptr<Paradata::FieldValueInfo> CreateFieldValueInfo(const VART* pVarT, const CNDIndexes& theIndex);
    std::shared_ptr<Paradata::FieldValidationInfo> CreateFieldValidationInfo(const VART* pVarT);

    std::unique_ptr<Paradata::MessageEvent> CreateMessageEvent(std::variant<MessageType, FunctionCode> message_type_or_function_code,
                                                               int message_number, std::wstring message_text) override;

    void RegisterAndLogEvent(std::shared_ptr<Paradata::Event> event, void* instance_object = nullptr) override;

    void LogEngineEvent(ParadataEngineEvent engine_event);

    void LogProperties();

    void ProcessCachedEvents(const std::vector<CString>& event_strings);

private:
    const Logic::SymbolTable& GetSymbolTable() const;

    std::shared_ptr<Paradata::NamedObject> CreateObjectWorker(const Symbol& symbol);
    std::shared_ptr<Paradata::NamedObject> GetPrimaryFlowObject();

private:
    CIntDriver* m_pIntDriver;
    CEngineArea* m_pEngineArea;
    CEngineDriver* m_pEngineDriver;

    std::vector<std::shared_ptr<Paradata::NamedObject>> m_cachedSymbolObjects;
    std::vector<std::shared_ptr<Paradata::NamedObject>> m_cachedNonSymbolObjects;
};
