#include "LexCSPro.h"
#include <stack>

using namespace Scintilla;
using namespace Lexilla;


namespace 
{
    enum class FoldState { Word, NotWord, String, Comment, BlockComment };

    struct FoldRule
    {
        std::string start;
        std::vector<std::string> ends;
    };

    // defines all the fold rules
    const FoldRule foldRules[] = {
        { "function", { "end"             } },
        { "if",       { "endif"           } },
        { "do",       { "enddo"           } },
        { "while",    { "enddo"           } },
        { "forcase",  { "endfor", "enddo" } },
        { "for",      { "endfor", "enddo" } },
        { "recode",   { "endrecode"       } },
        { "when",     { "endwhen"         } },
    };


    //Options that can be set for the lexer externally
    struct OptionsCSPro 
    {
        bool fold = false;
        bool foldBlocks = false;
        bool foldProcs = false;
    };

    //Initialise the lexer options
    struct OptionSetCSPro : public OptionSet<OptionsCSPro> 
    {
        OptionSetCSPro() 
        {
            DefineProperty("fold", &OptionsCSPro::fold);
            DefineProperty("fold.blocks", &OptionsCSPro::foldBlocks);
            DefineProperty("fold.procs", &OptionsCSPro::foldProcs);
        }
    };


    std::string toLowerCase(std::string str)
    {
        for( char& ch : str )
        {
            if (ch >= 'A' && ch <= 'Z')
                ch += 32;
        }

        return str;
    }
}


class LexerCSProLogic : public LexCSPro
{
    OptionsCSPro options;
    OptionSetCSPro osCSPro;

public:
    LexerCSProLogic(int language)
        :   LexCSPro(language)
    {
    }

    virtual ~LexerCSProLogic() 
    { 
    }

    void SCI_METHOD Release() override 
    {
        delete this;
    }

    int SCI_METHOD Version() const override 
    {
        return lvRelease4;
    }

    const char * SCI_METHOD PropertyNames() override 
    {
        return osCSPro.PropertyNames();
    }

    int SCI_METHOD PropertyType(const char *name) override 
    {
        return osCSPro.PropertyType(name);
    }

    const char * SCI_METHOD DescribeProperty(const char *name) override 
    {
        return osCSPro.DescribeProperty(name);
    }

    Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override
    {
        if (osCSPro.PropertySet(&options, key, val)) {
            return 0;
        }

        return -1;
    }

    const char * SCI_METHOD DescribeWordListSets() override 
    {
        return osCSPro.DescribeWordListSets();
    }

    int SCI_METHOD PrimaryStyleFromStyle(int style) override 
    {
        return style;
    }

    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
};


void SCI_METHOD LexerCSProLogic::Fold(Sci_PositionU startPos, Sci_Position length, int, IDocument *pAccess)
{
    if (!options.fold) {
        return;
    }

    Accessor styler(pAccess, NULL);

    // always fold from the start of the document
    Sci_Position lengthDoc = startPos + length;
    startPos = 0;

    Sci_Position lineCurrent = styler.GetLine(startPos);
    int levelCurrent = SC_FOLDLEVELBASE;
    int levelStartLine = levelCurrent;
    FoldState mode = FoldState::NotWord;
    char chPrev;
    char ch = styler.SafeGetCharAt(startPos - 1);
    char chNext = styler.SafeGetCharAt(startPos);
    std::string word;
    std::string prevWord;
    std::stack<FoldRule> endStack;
    int commentNestLevel = 0;
    bool firstWordOnLine = true;
    int procCount = 0;
    bool onProcLine = false;
    bool lineRequiresHeader = false;
    char stringEnd = 0;
    bool ignoreDo = false;
    bool ignoreWhile = false;
    bool ignoreFunction = false;
    Sci_Position previousLineWithNonWhitespaceChars = lineCurrent;
    bool lineHasNonWhitespaceChars = false;

    for (Sci_Position i = startPos; i < lengthDoc; i++)
    {
        chPrev = ch;
        ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);

        if( !lineHasNonWhitespaceChars )
            lineHasNonWhitespaceChars = !isspace(ch);

        switch (mode)
        {
        case FoldState::NotWord:
            if (IsCSProWordChar(ch))
            {
                word += std::tolower(ch);
                mode = FoldState::Word;
            }
            break;
        case FoldState::Word:
            if (IsCSProWordChar(ch))
            {
                word += std::tolower(ch);
            }
            else
            {
                //we have the full word, check for any matches
                assert(word == toLowerCase(word));
                bool foundMatch = false;

                // process PROC lines
                if (onProcLine)
                {
                     // ignore anything else on the line
                    foundMatch = true;
                }

                else if (options.foldProcs && firstWordOnLine && word == "proc")
                {
                    foundMatch = true;
                    onProcLine = true;
                    ++procCount;

                    if (procCount >= 2)
                    {
                        lineRequiresHeader = true;

                        // increase the level for the first PROC following PROC GLOBAL
                        if (procCount == 2)
                        {
                            levelCurrent++;
                        }

                        // for subsequent PROCS, mark the previous level as ending at the previous non-whitespace line
                        else
                        {
                            levelStartLine--;

                            for( Sci_Position line = previousLineWithNonWhitespaceChars + 1; line < lineCurrent; line++ )
                            {
                                styler.SetLevel(line, levelStartLine);
                            }
                        }
                    }
                }

                // check if this is the end of a block
                else if (!endStack.empty())
                {
                    const FoldRule& rule = endStack.top();
                    if (std::find(rule.ends.cbegin(), rule.ends.cend(), word) != rule.ends.cend())
                    {
                        foundMatch = true;
                        levelCurrent--;
                        ignoreFunction = false;
                        endStack.pop();
                    }
                }

                // check if this is the start of a block
                if( !foundMatch )
                {
                    // if not folding blocks of code, only check for functions
                    size_t fold_rules_to_check = options.foldBlocks ? _countof(foldRules) : 1;
                    assert(foldRules[0].start == "function");

                    for (size_t j = 0; j < fold_rules_to_check; j++)
                    {
                        const FoldRule& rule = foldRules[j];

                        if (word == rule.start)
                        {
                            //loops are a really annoying special case since they reuse the same keywords for different things
                            //like while do vs do while
                            if (ignoreWhile && (word == "while" || word == "until"))
                            {
                                ignoreWhile = false;
                                break;
                            }
                            else if (word == "while" || word == "for" || word == "forcase")
                            {
                                ignoreDo = true;
                            }
                            else if (word == "do")
                            {
                                if (ignoreDo)
                                {
                                    ignoreDo = false;
                                    break;
                                }
                                else
                                {
                                    ignoreWhile = true;
                                }

                            }
                            else if (word == "if" && prevWord == "ask" )
                            {
                                // ignore ask if
                                break;
                            }
                            else if (word == "function")
                            {
                                // ignore function pointers
                                if (ignoreFunction)
                                {
                                    break;
                                }

                                ignoreFunction = true;
                            }

                            endStack.push(rule);
                            levelCurrent++;
                            lineRequiresHeader = true;
                            break;
                        }
                    }
                }

                prevWord = word;
                word.clear();
                mode = FoldState::NotWord;
                firstWordOnLine = false;
            }
            break;
        case FoldState::Comment:
            break;
        case FoldState::BlockComment:
            if (IsV8_0() ? ( ch == '*' && chNext == '/' ) : ( ch == '}' ))
            {
                if (commentNestLevel > 0)
                {
                    commentNestLevel--;
                }
            }
            if (commentNestLevel == 0)
            {
                mode = FoldState::NotWord;
            }
            break;
        case FoldState::String:
            break;
        }
        //special cases (like strings and comments
        if (mode != FoldState::BlockComment && mode != FoldState::String && ch == '/' && chNext == '/')
        {
            mode = FoldState::Comment;
        }
        else if (mode != FoldState::Comment && mode != FoldState::String && ( IsV8_0() ? ( ch == '/' && chNext == '*' ) : ( ch == '{' ) ))
        {
            mode = FoldState::BlockComment;
            commentNestLevel++;
        }
        else if (mode != FoldState::Comment && mode != FoldState::BlockComment && is_quotemark(ch))
        {
            if (mode == FoldState::String)
            {
                if (ch == stringEnd && ( !IsV8_0() || chPrev != '\\' ))
                {
                    mode = FoldState::NotWord;
                }
            }
            else
            {
                stringEnd = ch;
                mode = FoldState::String;
            }

        }
        else if ((ch == '\r' && chNext != '\n') || (ch == '\n') || i == (lengthDoc - 1))
        {
            int levelStyle = levelStartLine;

            if (lineRequiresHeader && levelCurrent != levelStartLine)
            {
                levelStyle |= SC_FOLDLEVELHEADERFLAG;
            }

            if (levelStyle != styler.LevelAt(lineCurrent))
            {
                styler.SetLevel(lineCurrent, levelStyle);
            }

            if (lineHasNonWhitespaceChars)
            {
                previousLineWithNonWhitespaceChars = lineCurrent;
                lineHasNonWhitespaceChars = false;
            }

            lineCurrent++;
            levelStartLine = levelCurrent;
            word.clear();
            firstWordOnLine = true;
            onProcLine = false;
            lineRequiresHeader = false;

            if (mode == FoldState::Comment || mode == FoldState::String)
            {
                mode = FoldState::NotWord;
            }
        }
    }
}


namespace
{
    ILexer5* LexerFactoryCSPro_V0()   { return new LexerCSProLogic(SCLEX_CSPRO_LOGIC_V0); }
    ILexer5* LexerFactoryCSPro_V8_0() { return new LexerCSProLogic(SCLEX_CSPRO_LOGIC_V8_0); }
}

LexerModule lmCSProLogic_V0(SCLEX_CSPRO_LOGIC_V0, LexerFactoryCSPro_V0, LexCSPro::Name(SCLEX_CSPRO_LOGIC_V0));
LexerModule lmCSProLogic_V8_0(SCLEX_CSPRO_LOGIC_V8_0, LexerFactoryCSPro_V8_0, LexCSPro::Name(SCLEX_CSPRO_LOGIC_V8_0));
