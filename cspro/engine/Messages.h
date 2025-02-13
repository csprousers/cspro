#pragma once

#include <zMessageO/SystemMessageIssuer.h>

#define issaerror m_pEngineDriver->GetSystemMessageIssuer().Issue
#define issaerror_or_throw m_pEngineDriver->GetSystemMessageIssuer().IssueOrThrow
