#pragma once


constexpr const TCHAR* CSEntryLanguageOverrideFile = _T("CSEntry.menu");

void ActivateDynamicMenus(const CString& override_filename, BCMenu& menu); // 20111125
