#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>


class ZENGINEO_API SystemApp : public Symbol
{
public:
    struct Argument
    {
        std::wstring name;
        std::optional<std::variant<double, std::wstring>> value;
    };

    struct Result
    {
        std::wstring name;
        std::wstring value;
    };

private:
    SystemApp(const SystemApp& system_app);

public:
    SystemApp(std::wstring system_app_name);

    const std::vector<Argument>& GetArguments() const { return m_arguments; }
    void SetArgument(std::wstring argument_name, std::optional<std::variant<double, std::wstring>> value);

    const std::wstring& GetResult(const std::wstring& result_name) const;
    void SetResult(std::wstring result_name, std::wstring value);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

	void WriteValueToJson(JsonWriter& json_writer) const override;
	void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

private:
    std::vector<Argument> m_arguments;
    std::vector<Result> m_results;
};
