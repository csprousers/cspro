#pragma once

#include <engine/StandardSystemIncludes.h>
#include <StandardIncludes/strict_errors.h>

#include "Controller.h"
#include "resource.h"
#include "UWM.h"
#include <zToolsO/Utf8.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/WindowHelpers.h>
#include <zUtilO/WindowsUtf8.h>
#include <zUtilO/WindowsWS.h>
#include <zGit/GitBranch.h>
#include <zGit/GitCommit.h>

#define GIT_DEPRECATE_HARD
#include <external/libgit2/include/git2.h>
