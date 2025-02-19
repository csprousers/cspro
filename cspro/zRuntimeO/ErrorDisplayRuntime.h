#pragma once

#include <zRuntimeO/zRuntimeO.h>
#include <zRuntimeO/Runtime.h>

class VirtualFileMappingHandler;


class ZRUNTIMEO_API ErrorDisplayRuntime : public Runtime
{
public:
    ErrorDisplayRuntime(SharableString message);
    ~ErrorDisplayRuntime();

    SharableString GetDescription() noexcept override;
    SharableString GetUrl() noexcept override;
    bool IsCloseable() noexcept override;
    void Start() override;
    SharableString OnMessage(std::string_view action_sv, const JsonNode& json_node) override;

private:
    SharableString m_message;
    SharableString m_url;
    std::unique_ptr<VirtualFileMappingHandler> m_virtualFileMappingHandler;
};
