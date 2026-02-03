#pragma once

#include <engine/StandardSystemIncludes.h>
#include <StandardIncludes/strict_errors.h>

#include "resource.h"
#include <zToolsO/Utf8.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/SettingsDb.h>
#include <zUtilF/LoggingListBox.h>
#include <zGit/GitBranch.h>
#include <zGit/GitCommit.h>

#define GIT_DEPRECATE_HARD
#include <external/libgit2/include/git2.h>
