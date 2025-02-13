#include "LexCSPro.h"
#include <optional>

using namespace Scintilla;
using namespace Lexilla;


namespace
{
    struct SpecialTag
    {
        std::string_view tag;
        bool use_cspro_logic_lexer;
    };

    static constexpr SpecialTag SpecialTags[] =
    {
        { "logic",       true  },
        { "logicsyntax", true  },
        { "logiccolor",  true  },
        { "action",      true  },
        { "message",     false },
        { "report",      false },
        { "color",       false },
        { "pff",         false },
        { "html",        false },
    };

    enum class State
    {
        Default,
        StartTag_InTagName,
        StartTag_ExpectingAttributeOrValue,
        StartTag_InAttributeOrValue,
        StartTag_InSpaceAfterAttributeOrValue,
        StartTag_InQuotedValue,
        StartTag_InQuotedValueEscape,
        EndTag,
    };

    struct LineState
    {
        State state = State::Default;
        const SpecialTag* special_tag = nullptr;
    };
}


class LexerCSProDocument : public LexCSPro
{
public:
    LexerCSProDocument();
    static ILexer5* CreateLexer();

    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;

private:
    const SpecialTag* GetSpecialTag(Accessor& styler, Sci_PositionU start_pos);

private:
    std::map<int, LineState> m_lineStates;
    char m_blockTagCheckBuffer[20];
};


LexerCSProDocument::LexerCSProDocument()
    :   LexCSPro(SCLEX_CSPRO_DOCUMENT),
        m_blockTagCheckBuffer{0}
{
}


ILexer5* LexerCSProDocument::CreateLexer()
{
    return new LexerCSProDocument();
}


const SpecialTag* LexerCSProDocument::GetSpecialTag(Accessor& styler, const Sci_PositionU start_pos)
{
    styler.GetRange(start_pos, start_pos + _countof(m_blockTagCheckBuffer), m_blockTagCheckBuffer, _countof(m_blockTagCheckBuffer));

    for( const SpecialTag& special_tag : SpecialTags )
    {
        assert(( special_tag.tag.size() + 1 ) <= _countof(m_blockTagCheckBuffer));

        // the !IsAlpha check makes sure that tags like "logiccolor" don't get matched with "logic"
        if( strncmp(special_tag.tag.data(), m_blockTagCheckBuffer, special_tag.tag.length()) == 0 &&
            !IsAlpha(m_blockTagCheckBuffer[special_tag.tag.length()]) )
        {
            return &special_tag;
        }
    }

    return nullptr;
}


void LexerCSProDocument::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess)
{
    Accessor styler(pAccess, nullptr);
    StyleContext sc(startPos, length, initStyle, styler);

    // because the line state is used by the CSPro lexer for the state of multiline comments, we use
    // our own object to keep track of line states rather than use the value of LexAccessor::GetLineState
    LineState line_state = m_lineStates[sc.currentLine];

    std::optional<Sci_Position> start_pos_for_cspro_logic_lexer =
        ( line_state.special_tag != nullptr && line_state.special_tag->use_cspro_logic_lexer ) ? std::make_optional(startPos) :
                                                                                                 std::nullopt;

    auto lex_cspro_logic_if_necessary = [&]()
    {
        if( !start_pos_for_cspro_logic_lexer.has_value() )
            return false;

        styler.Flush();
        LexCSPro::Lex(*start_pos_for_cspro_logic_lexer, sc.currentPos - *start_pos_for_cspro_logic_lexer, SCE_CSPRO_DEFAULT, pAccess);
        styler.StartSegment(sc.currentPos);

        start_pos_for_cspro_logic_lexer.reset();
        return true;
    };


    // the move_forward flag should be set to false when calling StyleContext::ForwardSetState
    int quotemark = 0;
    bool move_forward;

    for( ; sc.More(); move_forward ? sc.Forward() : void() )
    {
        move_forward = true;

        // when on a new line, save the current line state
        if( sc.atLineStart )
            m_lineStates[sc.currentLine] = line_state;


        // check 1: when in a quoted value, the only thing to do is wait for the end quotemark
        if( line_state.state == State::StartTag_InQuotedValue ||
            line_state.state == State::StartTag_InQuotedValueEscape )
        {
            // prevent leaky strings
            if( sc.atLineStart )
            {
                line_state.state = State::StartTag_ExpectingAttributeOrValue;
                sc.SetState(SCE_CSPRO_DOCUMENT_BOOLEAN_ATTRIBUTE);
            }

            // flip the state if the previous character was '\\'
            else if( line_state.state == State::StartTag_InQuotedValueEscape )
            {
                assert(sc.chPrev == '\\');
                line_state.state = State::StartTag_InQuotedValue;
            }

            // ignore escapes
            else if( sc.ch == '\\' )
            {
                line_state.state = State::StartTag_InQuotedValueEscape;
            }

            // end the quoted value
            else if( sc.ch == quotemark )
            {
                // default to the next state being another attribute or value
                sc.ForwardSetState(SCE_CSPRO_DOCUMENT_BOOLEAN_ATTRIBUTE);
                line_state.state = State::StartTag_ExpectingAttributeOrValue;
                move_forward = false;
            }

            continue;
        }


        // check 2: if a tag is starting, see if it is a tag that we should treat as a tag (and not, say, a < character in a logic block)
        bool is_end_tag;

        if( sc.ch == '<' && ( ( ( is_end_tag = ( sc.chNext == '/' ) ) == true ) ||
                              ( IsAlpha(sc.chNext) ) ) )
        {
            // if in a special tag, this should only be considered if this is the end tag for this special tag
            if( ( line_state.special_tag == nullptr ) ||
                ( is_end_tag && line_state.special_tag == GetSpecialTag(styler, sc.currentPos + 2) ) )
            {
                lex_cspro_logic_if_necessary();

                sc.SetState(SCE_CSPRO_DOCUMENT_TAG);

                if( is_end_tag )
                {
                    line_state.state = State::EndTag;
                    line_state.special_tag = nullptr;
                }

                else
                {
                    line_state.state = State::StartTag_InTagName;
                    line_state.special_tag = GetSpecialTag(styler, sc.currentPos + 1);
                }

                continue;
            }
        }


        // check 3: if in a section for logic lexing, keep processing characters until the section ends
        if( start_pos_for_cspro_logic_lexer.has_value() )
            continue;


        // check 4: check for a tag ending
        if( line_state.state != State::Default && sc.ch == '>' )
        {
            assert(!start_pos_for_cspro_logic_lexer.has_value());

            if( line_state.special_tag != nullptr )
            {
                assert(line_state.state != State::EndTag);

                // when the start tag for a special tag ends, store the position
                // from which this block should be lexed using the CSPro logic lexer
                if( line_state.special_tag->use_cspro_logic_lexer )
                    start_pos_for_cspro_logic_lexer = sc.currentPos + 1;
            }

            sc.SetState(SCE_CSPRO_DOCUMENT_TAG);
            sc.ForwardSetState(SCE_CSPRO_DEFAULT);
            line_state.state = State::Default;
            move_forward = false;

            continue;
        }


        // check 5: when ending a tag with />, color the / the same color as the tag
        if( line_state.state != State::EndTag && sc.Match('/', '>') )
        {
            sc.SetState(SCE_CSPRO_DOCUMENT_TAG);
            continue;
        }


        // check 6: when processing a tag name, wait for a space character to indicate the tag name is complete
        if( line_state.state == State::StartTag_InTagName )
        {
            if( isspacechar(sc.ch) )
            {
                sc.SetState(SCE_CSPRO_DEFAULT);
                line_state.state = State::StartTag_ExpectingAttributeOrValue;
            }

            continue;
        }


        // check 7: when expecting an attribute or value, process text as a boolean attribute, and a quotemark as a quoted value
        if( ( line_state.state == State::StartTag_ExpectingAttributeOrValue ) ||
            ( line_state.state == State::StartTag_InSpaceAfterAttributeOrValue && ( IsAlpha(sc.ch) || is_quotemark(sc.ch) ) ) )
        {
            if( IsAlpha(sc.ch) )
            {
                sc.SetState(SCE_CSPRO_DOCUMENT_BOOLEAN_ATTRIBUTE);
                line_state.state = State::StartTag_InAttributeOrValue;
            }

            else if( is_quotemark(sc.ch) )
            {
                sc.SetState(SCE_CSPRO_DOCUMENT_VALUE);
                line_state.state = State::StartTag_InQuotedValue;
                quotemark = sc.ch;
            }

            continue;
        }


        // check 8: when a = appears, modify the current state to an attribute (rather than a boolean attribute)
        if( sc.ch == '=' && ( line_state.state == State::StartTag_InAttributeOrValue ||
                              line_state.state == State::StartTag_InSpaceAfterAttributeOrValue ) )
        {
            sc.ChangeState(SCE_CSPRO_DOCUMENT_ATTRIBUTE);
            sc.ForwardSetState(SCE_CSPRO_DOCUMENT_BOOLEAN_ATTRIBUTE);
            line_state.state = State::StartTag_ExpectingAttributeOrValue;
            move_forward = false;

            continue;
        }


        // check 9: when in an attribute or value, wait for a space character to indicate the attribute / boolean attribute is complete
        if( line_state.state == State::StartTag_InAttributeOrValue )
        {
            if( isspacechar(sc.ch) )
                line_state.state = State::StartTag_InSpaceAfterAttributeOrValue;

            continue;
        }
    }

    // save the last line state
    if( sc.atLineStart )
        m_lineStates[sc.currentLine] = line_state;

    // complete the last segment
    if( !lex_cspro_logic_if_necessary() )
        sc.Complete();
}


LexerModule lmCSProDocument(SCLEX_CSPRO_DOCUMENT, LexerCSProDocument::CreateLexer, LexCSPro::Name(SCLEX_CSPRO_DOCUMENT));
