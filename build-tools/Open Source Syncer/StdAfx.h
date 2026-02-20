#pragma once

#include <engine/StandardSystemIncludes.h>
#include <StandardIncludes/strict_errors.h>

#include "Controller.h"
#include "resource.h"
#include "SettingsKeys.h"
#include "UWM.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/Utf8.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/Viewers.h>
#include <zUtilO/WindowHelpers.h>
#include <zUtilO/WindowsUtf8.h>
#include <zUtilO/WindowsWS.h>
#include <zGit/GitBlob.h>
#include <zGit/GitBranch.h>
#include <zGit/GitCommit.h>
#include <zGit/GitDiff.h>
#include <zGit/GitIndex.h>
#include <zGit/GitRevisionWalker.h>
#include <zGit/GitTag.h>
#include <zGit/GitTree.h>
#include <regex>

#define GIT_DEPRECATE_HARD
#include <external/libgit2/include/git2.h>
