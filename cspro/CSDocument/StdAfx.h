#pragma once

#include <engine/StandardSystemIncludes.h>
#include <engine/StrictCompilerErrors.h>

#include <afxcontrolbars.h>
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zToolsO/PointerClasses.h>
#include <zToolsO/VectorHelpers.h>
#include <zToolsO/WinClipboard.h>
#include <zUtilO/Filedlg.h>
#include <zUtilO/Interapp.h>
#include <zUtilO/MimeType.h>
#include <zUtilO/TextSourceEditable.h>
#include <zUtilO/WindowHelpers.h>
#include <zUtilO/WindowsWS.h>
#include <zUtilF/DocViewIterators.h>
#include <zUtilF/DynamicMenuBuilder.h>
#include <zHtml/HtmlWriter.h>
#include <zScintilla/include/SciLexer.h>
#include <zDesignerF/resource_shared.h>
#include <zDesignerF/UWM.h>
#include <CSDocument/CacheableCalculator.h>
#include <CSDocument/GenerateTask.h>
#include <CSDocument/GlobalSettings.h>
#include <CSDocument/MainFrame.h>
#include <CSDocument/resource.h>
#include <CSDocument/TextEditView.h>
#include <CSDocument/UWM.h>


// some preprocessor definitions to perform stricter error checking:

// CHECK_PATH_CASE: if defined, the path of documents/images/etc. must match the path as exists on the disk
// #define CHECK_PATH_CASE

// STRICT_CHECKING_FOR_DOT_NOTATION: if defined, dot-notation logic validity is checked more thoroughly (search for the use to see why it's disabled by default)
// #define STRICT_CHECKING_FOR_DOT_NOTATION
