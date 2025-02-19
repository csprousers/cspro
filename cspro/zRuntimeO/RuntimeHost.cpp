#include "StdAfx.h"
#include "RuntimeHost.h"
#include <zJson/ValidJsonAsserter.h>


void RuntimeHost::PostActionMessageAsync(const std::string_view action_sv) noexcept
{
    PostActionMessageAsync(action_sv, Json::Text::EmptyObject_sv);
}


void RuntimeHost::PostActionMessageAsync(std::string_view action_sv, std::string_view data_sv) noexcept
{
    AssertValidJson(data_sv);

    PostWebMessageAsync(SO::Concatenate("{\"action\":",
                                        Encoders::ToJsonString(action_sv),
                                        ",\"data\":",
                                        data_sv,
                                        "}"));
}
