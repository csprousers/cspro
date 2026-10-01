#pragma once

#include <engine/StandardSystemIncludes.h>
#include <StandardIncludes/strict_errors.h>

#include <zDocument/CacheableCalculator.h>
#include <zDocument/DocSetCompiler.h>
#include <zDocument/GenerateTask.h>
#include <zDocument/GlobalSettings.h>
#include <CSDocument/UWM.h>
#include <zToolsO/CSProExecutables.h>
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Encoders.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/Hash.h>
#include <zToolsO/TextFile.h>
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8.h>
#include <zToolsO/VectorHelpers.h>
#include <zJson/Json.h>
#include <zUtilO/FileExtensions.h>
#include <zUtilO/MimeType.h>
#include <zUtilO/TextSourceEditable.h>
#include <zHtml/HtmlWriter.h>


// some preprocessor definitions to perform stricter error checking:

// CHECK_PATH_CASE: if defined, the path of documents/images/etc. must match the path as exists on the disk
// #define CHECK_PATH_CASE

// STRICT_CHECKING_FOR_DOT_NOTATION: if defined, dot-notation logic validity is checked more thoroughly (search for the use to see why it's disabled by default)
// #define STRICT_CHECKING_FOR_DOT_NOTATION
