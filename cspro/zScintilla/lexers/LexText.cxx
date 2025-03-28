#include "LexCSPro.h"

using namespace Scintilla;
using namespace Lexilla;


// This lexer uses the same code as LexNull but is used by LexCSProReportGeneric 
// to properly style segments. LexNull's implementation assumes that all styles 
// are 0 so it is not suitable for use by CSPro reports.

static void ColouriseTextDoc(const Sci_PositionU startPos, const Sci_Position length, int /*initStyle*/, 
                             WordList** /*keywordlists*/, Accessor& styler) 
{
    styler.StartAt(startPos);
    styler.StartSegment(startPos);
    styler.ColourTo(startPos + length - 1, 0);
    styler.Flush();
}


LexerModule lmText(SCLEX_NULL, ColouriseTextDoc, "null");
