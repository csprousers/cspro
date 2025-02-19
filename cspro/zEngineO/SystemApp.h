#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>


class ZENGINEO_API SystemApp : public Symbol
{
public:
    struct Argument
    {
        std::string name;
        std::optional<std::variant<double, SharableString>> value;
    };

    struct Result
    {
        std::string name;
        SharableString value;
    };

private:
    SystemApp(const SystemApp& system_app);

public:
    SystemApp(std::string system_app_name);

    const std::vector<Argument>& GetArguments() const { return m_arguments; }
    void SetArgument(std::string argument_name, std::optional<std::variant<double, SharableString>> value);

    SharableString GetResult(const std::string& result_name) const;
    void SetResult(std::string result_name, SharableString value);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void WriteValueToJson(JsonWriter& json_writer) const override;
    void SetValueFromJson(const JsonNode& json_node) override;

private:
    std::vector<Argument> m_arguments;
    std::vector<Result> m_results;
};
