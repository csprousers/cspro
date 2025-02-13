#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Html
{
    const unsigned CloseDialog                               = UWM::Ranges::HtmlStart + 0;
    const unsigned ProcessDisplayOptions                     = UWM::Ranges::HtmlStart + 1;
    const unsigned ActionInvokerEngineProgramControlExecuted = UWM::Ranges::HtmlStart + 2;

    // unlike the above messages, the following messages are only used within the project
    const unsigned ExecuteSizeUpdate                         = UWM::Ranges::HtmlStart + 3;
    const unsigned ActionInvokerProcessAsyncMessage          = UWM::Ranges::HtmlStart + 4;

    CHECK_MESSAGE_NUMBERING(ExecuteSizeUpdate, UWM::Ranges::HtmlLast)
}
