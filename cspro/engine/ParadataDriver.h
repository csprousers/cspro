#pragma once

#include <engine/DEFLD.H>
#include <zParadataO/NamedObject.h>
#include <zParadataO/ParadataDriver.h>

class CDataDict;
class CEngineArea;
class CIntDriver;
class ItemIndex;
namespace Paradata { class Event; class FieldInfo; class FieldValueInfo; class FieldValidationInfo; }


enum class ParadataEngineEvent
{
    ApplicationStart,
    ApplicationStop,
    SessionStart,
    SessionStop,
    CaseStart,
    CaseStop
};


class EngineParadataDriver : public Paradata::ParadataDriver
{
public:
    EngineParadataDriver(CIntDriver& interpreter);

    void ClearCachedObjects();

    bool GetRecordIteratorLoadCases() const override;

    std::shared_ptr<Paradata::NamedObject> CreateObject(const Symbol& symbol);
    std::shared_ptr<Paradata::NamedObject> CreateObject(Paradata::NamedObject::Type type, std::string_view name_sv) override;

    std::unique_ptr<Paradata::FieldInfo> CreateFieldInfo(const VART* pVarT, const CNDIndexes& theIndex);
    std::unique_ptr<Paradata::FieldInfo> CreateFieldInfo(const VART* pVarT, const double* pdIndices);
    std::unique_ptr<Paradata::FieldInfo> CreateFieldInfo(const DEFLD3* pDeFld);
    std::unique_ptr<Paradata::FieldInfo> CreateFieldInfo(const VART* pVarT, const ItemIndex& item_index);

    std::unique_ptr<Paradata::FieldValueInfo> CreateFieldValueInfo(const VART* pVarT, const CNDIndexes& theIndex);
    std::unique_ptr<Paradata::FieldValidationInfo> CreateFieldValidationInfo(const VART* pVarT);

    std::unique_ptr<Paradata::MessageEvent> CreateMessageEvent(std::variant<MessageType, FunctionCode> message_type_or_function_code,
                                                               int message_number, SharableString message_text) override;

    void RegisterAndLogEvent(std::shared_ptr<Paradata::Event> event, const void* instance_object = nullptr) override;

    void LogEngineEvent(ParadataEngineEvent engine_event);

    void LogProperties();

    void ProcessCachedEvents(const std::vector<std::string>& event_strings);

private:
    const Logic::SymbolTable& GetSymbolTable() const;

    std::unique_ptr<Paradata::NamedObject> CreateObjectWorker(const Symbol& symbol);
    std::shared_ptr<Paradata::NamedObject> GetPrimaryFlowObject();

private:
    CIntDriver* const m_pIntDriver;
    CEngineArea* const m_pEngineArea;
    CEngineDriver* const m_pEngineDriver;

    std::vector<std::shared_ptr<Paradata::NamedObject>> m_cachedSymbolObjects;
    std::vector<std::shared_ptr<Paradata::NamedObject>> m_cachedNonSymbolObjects;
};
