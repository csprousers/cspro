#pragma once

#include <engine/StandardSystemIncludes.h>
#include <StandardIncludes/strict_errors.h>

#include "resource.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/Utf8.h>
#include <zUtilO/Interapp.h>
#include <zGit/GitBranch.h>

#define GIT_DEPRECATE_HARD
#include <external/libgit2/include/git2.h>
