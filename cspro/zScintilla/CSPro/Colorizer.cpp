#include "CSPro.h"

#include <cassert>
#include <forward_list>
#include <chrono>

#include "Debugging.h"
#include "Geometry.h"
#include "ScintillaTypes.h"
#include "Platform.h"
#include "CharacterCategoryMap.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"

#include "CharacterSet.h"
#include "CharacterCategory.h"
#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "PerLine.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"

#include "LexerModule.h"


namespace
{
    class CSProLexInterface : public Scintilla::Internal::LexInterface 
    {
    public:
        CSProLexInterface(Scintilla::Internal::Document* doc, int lexer_language, const std::vector<std::string>& keywords)
            :   LexInterface(doc)
        {
            Scintilla::ILexer5* lexer = CSProScintilla::CreateLexer(lexer_language);
            assert(lexer != nullptr);

            SetInstance(lexer);

            for( size_t i = 0; i < keywords.size(); ++i )
                instance->WordListSet(i, keywords[i].c_str());
        }
    };
}


std::unique_ptr<char[]> CSProScintilla::GetStyledText(int lexer_language, const std::vector<std::string>& keywords, std::string_view text_sv)
{
    Scintilla::Internal::Document doc(Scintilla::DocumentOption::Default);
    doc.InsertString(0, text_sv.data(), text_sv.length());

    CSProLexInterface lex(&doc, lexer_language, keywords);
    lex.Colourise(0, doc.Length());

    const Sci_Position doc_length = doc.Length();
    const size_t buffer_size_with_nulls = ( doc_length + 1 ) * 2;
    auto chars_and_styles = std::make_unique_for_overwrite<char[]>(buffer_size_with_nulls);
    char* chars_and_styles_itr = chars_and_styles.get();

    for( Sci_Position i = 0; i < doc_length; ++i )
    {
        *(chars_and_styles_itr++) = doc.CharAt(i);
        *(chars_and_styles_itr++) = doc.StyleAt(i);
    }

    *(chars_and_styles_itr++) = 0;
    *chars_and_styles_itr++ = 0;

    assert(( chars_and_styles_itr - buffer_size_with_nulls ) == chars_and_styles.get());

    return chars_and_styles;
}
