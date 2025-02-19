#pragma once

#include <zEngineF/zEngineF.h>
#include <zToolsO/ApplicationInterface.h>


class CLASS_DECL_ZENGINEF WindowsApplicationInterface : public ApplicationInterface
{
public:
    SharableString BarcodeRead(const std::string& message_text) override;

    bool IsNetworkConnected(bool wifi, bool mobile) override;
};
