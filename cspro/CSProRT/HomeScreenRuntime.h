#pragma once

#include <zRuntimeO/Runtime.h>


class HomeScreenRuntime : public Runtime
{
public:
    HomeScreenRuntime();

    // Runtime overrides
protected:
    SharableString GetDescription() noexcept override;
    SharableString GetUrl() noexcept override;
    bool IsCloseable() noexcept override;
    void Start() override;
    SharableString OnMessage(std::string_view action_sv, const JsonNode& json_node) override;

private:
    SharableString m_url;
};
