#pragma once

#include <engine/StandardSystemIncludes.h>

#include <zToolsO/FileIO.h>
#include <zToolsO/SpanHelpers.h>
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8.h>
#include <zUtilO/Interapp.h>
#include <zJson/Json.h>
#include <zZip/ZLib.h>
#include <zNetwork/ConnectResponse.h>
#include <zNetwork/FileInfo.h>
#include <zNetwork/LoginCredentials.h>
#include <zNetwork/SyncException.h>
#include <zNetwork/SyncLogSyncListener.h>
#include <zCaseO/Case.h>
#include <zSyncF/SyncLoginAccessor.h>

// Headers for CppUnitTest
#include <CppUnitTest.h>

#include <tests/TestSync/ToString.h>

#define _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING
#include <fakeit.hpp>
