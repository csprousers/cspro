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
