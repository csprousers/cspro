#pragma once

#include <zEngineO/RuntimeEvent.h>
#include <zUtilO/TransactionManager.h>
#include <zLogicO/Symbol.h>

class CommonStore;
struct EngineData;


class PersistentVariableProcessor : public RuntimeEvent, public TransactionGenerator
{
    friend class RuntimeEventsProcessor;

private:
    PersistentVariableProcessor(EngineData& engine_data);

public:
    ~PersistentVariableProcessor();

    void AddSymbol(const Symbol& symbol);

    bool HasSymbolWithSameName(const Symbol& symbol) const;

protected:
    Type GetType() const override { return Type::PersistentVariableProcessor; }

    void serialize(Serializer& ar) override;

    void OnStart() override;
    void OnStop() override;

private:
    bool CommitTransactions() override;

    void SaveSymbols();

private:
    EngineData& m_engineData;
    std::shared_ptr<CommonStore> m_commonStore;
    std::vector<int> m_symbolIndices;
};
