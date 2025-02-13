#pragma once

#define HELP_OPENVARCHAR       _T('%')
#define HELP_CLOSEVARCHAR      _T('%')
#define HELP_OPENPAREN          _T('(')
#define HELP_CLOSEPAREN         _T(')')

#define HELP_MAXREPLACETEXTLEN  255

class CReplace
{
public:
    CString csIn;
    CString csOut;
    csprochar*   pszOutBuff;

    CReplace() { pszOutBuff = NULL; }
};
