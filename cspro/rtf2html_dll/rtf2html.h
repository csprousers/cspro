#ifndef __RTF2HTML_H__
#define __RTF2HTML_H__

/* This file was added to the original rtf2html 0.2.0 code by
   Josh Handley for OECD/Paris21/IHSN.
   Copyright (c) 2009 OECD.
   Released under original rtf2html license (LGPL 2.1)
   See COPYING.LESSER for details.
*/

#include "rtf2html_dll.h"

RTF2HTML_DLL_API void rtf2html(std::istream &file_in,
                               std::ostream &file_out,
                               bool fragmentOnly);

#endif

