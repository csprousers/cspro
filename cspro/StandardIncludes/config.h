#pragma once


// --------------------------------------------------------------------------
// APPLICATION SETTINGS
// --------------------------------------------------------------------------

// On Windows, GetAppDataPath will return: %AppData%/APPLICATION_DATA_PATH
#ifdef _CONSOLE
#  define APPLICATION_DATA_PATH "CSEntryConsole"
#else
#  define APPLICATION_DATA_PATH "CSPro"
#endif



// --------------------------------------------------------------------------
// OPTIONAL COMPONENTS
// --------------------------------------------------------------------------

// If not enabled, the Action Invoker will be inaccessible.
// TODO: This is not fully implemented; it is only used for Goose Nest harmonization.
#define ENABLE_ACTION_INVOKER
