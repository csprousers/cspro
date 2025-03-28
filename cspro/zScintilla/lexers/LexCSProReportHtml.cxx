#include "LexCSPro.h"

using namespace Scintilla;
using namespace Lexilla;


namespace
{
    enum class LexerMode
    {
        Literal, CSProLogic, LogicEscape, DoubleTilde, TripleTilde, CSProString
    };

    enum class HTMLMode
    {
        Literal, HTML, Option, Quote, Number
    };

    struct LexerToken
    {
        Sci_PositionU startPos;
        Sci_PositionU length;
        LexerMode mode;

        LexerToken(Sci_PositionU startPos_, LexerMode mode_)
            :   startPos(startPos_),
                length(0),
                mode(mode_)
        {
        }
    };
}


class LexCSProReportHtml : public LexCSPro
{
public:
    LexCSProReportHtml(int language);
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
};


LexCSProReportHtml::LexCSProReportHtml(const int language)
    :   LexCSPro(language)
{
}


void LexCSProReportHtml::Lex(Sci_PositionU startPos, Sci_Position length, int /*initStyle*/, IDocument* const pAccess)
{
    std::vector<LexerToken> tokens;
    //this is the easiest way to make multiline escapes work
    length += startPos;
    startPos = 0;

    tokens.emplace_back(startPos, LexerMode::Literal);

    Accessor styler(pAccess, NULL);
    styler.StartAt(startPos);
    LexerMode state = LexerMode::Literal;
    std::tuple<char, LexerMode> string_ch_and_prev_state(0, state);
    char html_string_ch = 0;
    Sci_Position lengthDoc = startPos + length;

    for (Sci_Position i = startPos; i < lengthDoc; i++)
    {
        char ch = styler.SafeGetCharAt(i);
        char nextch = styler.SafeGetCharAt(i + 1);
        char nextch2 = styler.SafeGetCharAt(i + 2);

        switch (state)
        {
        case LexerMode::Literal:
            if (ch == '<' && nextch == '?')
            {
                tokens.back().length = i - tokens.back().startPos;
                //skip over both < and ?
                tokens.emplace_back(i, LexerMode::LogicEscape);
                tokens.back().length = 2;
                tokens.emplace_back(i + 2, LexerMode::CSProLogic);
                state = LexerMode::CSProLogic;
                i++;
            }
            else if (ch == '~' && nextch == '~' && nextch2 == '~')
            {
                tokens.back().length = i - tokens.back().startPos;
                //skip over both < and ?
                tokens.emplace_back(i, LexerMode::TripleTilde);
                tokens.back().length = 3;
                tokens.emplace_back(i + 3, LexerMode::CSProLogic);
                state = LexerMode::TripleTilde;
                i+=2;
            }
            else if (ch == '~' && nextch == '~')
            {
                tokens.back().length = i - tokens.back().startPos;
                //skip over both < and ?
                tokens.emplace_back(i, LexerMode::DoubleTilde);
                tokens.back().length = 2;
                tokens.emplace_back(i + 2, LexerMode::CSProLogic);
                state = LexerMode::DoubleTilde;
                i++;
            }
            break;

        case LexerMode::DoubleTilde:
            if (ch == '~' && nextch == '~')
            {
                tokens.back().length = i - tokens.back().startPos;
                //skip over both < and ?
                tokens.emplace_back(i, LexerMode::DoubleTilde);
                tokens.back().length = 2;
                tokens.emplace_back(i + 2, LexerMode::Literal);
                state = LexerMode::Literal;
                i++;
            }
            else if (is_quotemark(ch))
            {
                string_ch_and_prev_state = { ch, state };
                state = LexerMode::CSProString;
            }
            break;

        case LexerMode::TripleTilde:
            if (ch == '~' && nextch == '~' && nextch2 == '~')
            {
                tokens.back().length = i - tokens.back().startPos;
                //skip over both < and ?
                tokens.emplace_back(i, LexerMode::TripleTilde);
                tokens.back().length = 3;
                tokens.emplace_back(i + 3, LexerMode::Literal);
                state = LexerMode::Literal;
                i+=2;
            }
            else if (is_quotemark(ch))
            {
                string_ch_and_prev_state = { ch, state };
                state = LexerMode::CSProString;
            }
            break;

        case LexerMode::CSProLogic:
            if (ch == '?' && nextch == '>')
            {
                tokens.back().length = i - tokens.back().startPos;
                //skip over both < and ?
                tokens.emplace_back(i, LexerMode::LogicEscape);
                tokens.back().length = 2;
                tokens.emplace_back(i + 2, LexerMode::Literal);
                state = LexerMode::Literal;
                i++;
            }
            else if (is_quotemark(ch))
            {
                string_ch_and_prev_state = { ch, state };
                state = LexerMode::CSProString;
            }

            break;

        case LexerMode::CSProString:
            if (ch == std::get<0>(string_ch_and_prev_state) && ( !IsV8_0() || styler.SafeGetCharAt(i - 1) != '\\' ))
            {
                state = std::get<1>(string_ch_and_prev_state);
            }
            break;
        }
    }


    tokens.back().length = lengthDoc - tokens.back().startPos;

    HTMLMode mode = HTMLMode::Literal;

    for (const LexerToken& cur_token : tokens)
    {
        Sci_Position pos = cur_token.startPos + cur_token.length;

        switch (cur_token.mode)
        {
        case LexerMode::CSProLogic:
            if (cur_token.length > 0)
            {
                styler.Flush();
                LexCSPro::Lex(cur_token.startPos, cur_token.length, SCE_CSPRO_DEFAULT, pAccess);
                styler.StartSegment(pos);
            }

            break;

        case LexerMode::LogicEscape:
            styler.ColourTo(pos - 1, SCE_CSPRO_REPORT_LOGIC_TAG);
            break;

        case LexerMode::Literal:
            for (int j = cur_token.startPos; j < pos; j++)
            {
                char ch = styler.SafeGetCharAt(j);
                char prevch = styler.SafeGetCharAt(j - 1);
                switch (mode)
                {
                case HTMLMode::Literal:
                    if (ch == '<')
                    {
                        mode = HTMLMode::HTML;
                        styler.ColourTo(j == pos-1 ? j : j-1, SCE_CSPRO_REPORT_HTML_DEFAULT);
                    }
                    break;
                case HTMLMode::HTML:
                    if (ch == '>')
                    {
                        mode = HTMLMode::Literal;
                        styler.ColourTo(j, SCE_CSPRO_REPORT_HTML_TAG);
                    }
                    else if (isspacechar(ch))
                    {
                        mode = HTMLMode::Option;
                        styler.ColourTo(j, SCE_CSPRO_REPORT_HTML_TAG);
                    }
                    break;
                case HTMLMode::Option:
                    if (ch == '>')
                    {
                        mode = HTMLMode::Literal;
                        styler.ColourTo(j-1, SCE_CSPRO_REPORT_HTML_ATTR);
                        styler.ColourTo(j, SCE_CSPRO_REPORT_HTML_TAG);
                    }
                    else if (ch == '=')
                    {
                        styler.ColourTo(j - 1, SCE_CSPRO_REPORT_HTML_ATTR);
                        styler.ColourTo(j, SCE_CSPRO_REPORT_HTML_TAG);
                    }
                    else if (is_quotemark(ch))
                    {
                        html_string_ch = ch;
                        styler.ColourTo(j - 1, SCE_CSPRO_REPORT_HTML_ATTR);
                        mode = HTMLMode::Quote;
                    }
                    else if ((isspacechar(prevch) || prevch == '=') && (ch >= '0' && ch <= '9'))
                    {
                        styler.ColourTo(j - 1, SCE_CSPRO_REPORT_HTML_ATTR);
                        mode = HTMLMode::Number;
                    }
                    break;
                case HTMLMode::Quote:
                    if (ch == html_string_ch)
                    {
                        styler.ColourTo(j, SCE_CSPRO_REPORT_HTML_QUOTE);
                        mode = HTMLMode::Option;
                    }
                    break;
                case HTMLMode::Number:
                    if (isspacechar(ch))
                    {
                        styler.ColourTo(j, SCE_CSPRO_REPORT_HTML_NUM);
                        mode = HTMLMode::Option;
                    }
                    else if (ch == '>')
                    {
                        styler.ColourTo(j -1 , SCE_CSPRO_REPORT_HTML_NUM);
                        styler.ColourTo(j, SCE_CSPRO_REPORT_HTML_TAG);
                        mode = HTMLMode::Literal;
                    }
                }
            }

            //final styles
            switch (mode)
            {
            case HTMLMode::Literal:
                styler.ColourTo(pos - 1, SCE_CSPRO_REPORT_HTML_DEFAULT);
                break;
            case HTMLMode::HTML:
                styler.ColourTo(pos - 1, SCE_CSPRO_REPORT_HTML_TAG);
                break;
            case HTMLMode::Option:
                styler.ColourTo(pos - 1, SCE_CSPRO_REPORT_HTML_ATTR);
                break;
            case HTMLMode::Quote:
                styler.ColourTo(pos - 1, SCE_CSPRO_REPORT_HTML_QUOTE);
                break;
            case HTMLMode::Number:
                styler.ColourTo(pos - 1, SCE_CSPRO_REPORT_HTML_NUM);
                mode = HTMLMode::Option;
                break;
            }

            break;

        case LexerMode::DoubleTilde:
            styler.ColourTo(pos - 1, SCE_CSPRO_REPORT_DOUBLE_TILDE);
            break;

        case LexerMode::TripleTilde:
            styler.ColourTo(pos - 1, SCE_CSPRO_REPORT_TRIPLE_TILDE);
            break;
        }
    }

    styler.Flush();
}



namespace
{
    ILexer5* CreateLexerCSProReportHtml_V0()   { return new LexCSProReportHtml(SCLEX_CSPRO_REPORT_HTML_V0);  }
    ILexer5* CreateLexerCSProReportHtml_V8_0() { return new LexCSProReportHtml(SCLEX_CSPRO_REPORT_HTML_V8_0);  }
}

LexerModule lmCSProReportHtml_V0(SCLEX_CSPRO_REPORT_HTML_V0, CreateLexerCSProReportHtml_V0, LexCSPro::Name(SCLEX_CSPRO_REPORT_HTML_V0));
LexerModule lmCSProReportHtml_V8_0(SCLEX_CSPRO_REPORT_HTML_V8_0, CreateLexerCSProReportHtml_V8_0, LexCSPro::Name(SCLEX_CSPRO_REPORT_HTML_V8_0));
