#pragma once

#include <zEngineO/RuntimeEvent.h>
#include <zLogicO/Symbol.h>
#include <zToolsO/Encryption.h>


class PreinitializedVariable : public RuntimeEvent
{
    friend class RuntimeEventsProcessor;

private:
    PreinitializedVariable(const EngineData& engine_data);

public:
    enum class SpecialProcessing: int { None, EncryptedString = 4, RepeatingArray = 6 };

    PreinitializedVariable(const EngineData& engine_data, Symbol& symbol,
                           std::vector<int> data, SpecialProcessing special_processing = SpecialProcessing::None);

    static Encryptor::Type GetEncryptionType() { return Encryptor::Type::RijndaelHex; }

protected:
    Type GetType() const override { return Type::PreinitializedVariable; }

    void serialize(Serializer& ar) override;

    void OnStart() override;

private:
    const EngineData* m_engineData;
    Symbol* m_symbol;
    std::vector<int> m_data;
    SpecialProcessing m_specialProcessing;
};
