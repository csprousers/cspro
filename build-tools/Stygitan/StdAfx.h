#pragma once

#include <engine/StandardSystemIncludes.h>
#include <StandardIncludes/strict_errors.h>

#include <Stygitan/Helpers.h>
#include <Stygitan/resource.h>
#include <Stygitan/UWM.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/Utf8.h>
#include <zUtilO/FileUtil.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/SettingsDb.h>
#include <zUtilO/WindowsUtf8.h>
#include <zUtilO/WindowsWS.h>
#include <zUtilF/DocViewIterators.h>
#include <zUtilF/UIThreadRunner.h>
#include <zGit/GitBranch.h>
#include <zGit/GitRepository.h>
#include <zGit/GitRevisionWalker.h>
#include <external/libgit2/include/git2/diff.h>
#include <thread>
