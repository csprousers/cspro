#include "stdafx.h"
#include "LogicFormatter.h"


namespace
{
    constexpr const char* ZeroTabWords[] =
    {
        "proc", "function", "preproc", "postproc", "onfocus", "killfocus", "onoccchange"
    };

    constexpr const char* AddTabWords[] =
    {
        "if", "do", "for", "while", "elseif", "else"
    };

    constexpr const char* RemoveTabWords[] =
    {
        "end", "enddo", "endfor", "endif"
    };
}


LogicFormatter::LogicFormatter(const LogicSettings::Version logic_version) noexcept
    :   m_multilineCommentStart_sv(( logic_version == LogicSettings::Version::V0 ) ? CommentStrings::MultilineOldStart_sv : CommentStrings::MultilineNewStart_sv),
        m_multilineCommentEnd_sv(( logic_version == LogicSettings::Version::V0 ) ? CommentStrings::MultilineOldEnd_sv : CommentStrings::MultilineNewEnd_sv)
{
}


std::string LogicFormatter::FormatLogic(const std::string& logic) const
{
    ASSERT(logic.find('\r') == std::string::npos);

    // routine from 20101208
    std::string formatted_logic;

    // format the logic, using these rules
    //      Indent code
    //      Trim trailing spaces
    //      Convert spaces at beginning into tabs
    //      Must be at least two blank lines between functions, procs

    int curTabPos = 0;
    int prevSpaceCnt = 0;
    int prevShift = 0;
    int prevBlankLines = 0;
    bool lastWordWasAnIndentation = false;
    bool firstKeyword = true;
    bool addedNewline = false;

    bool moreLines = true;
    size_t curPos = 0;

    int numMultilineComments = 0;
    bool inComment = false;

    while( moreLines )
    {
        std::string line;

        // get a line from the text
        while( curPos < logic.length() && logic[curPos] != '\n' )
        {
            line.push_back(logic[curPos]);
            curPos++;
        }

        // advance past the new line
        if( curPos < logic.length() && logic[curPos] == '\n' )
            curPos++;

        if( curPos >= logic.length() )
            moreLines = false;

        std::string tempLine = line;

        if( !SO::IsWhitespace(tempLine) )
        {
            // 20101209 i initially programmed this to only analyze the first word on the line, but i'm now changing it
            // to analyze all words on the line (because if .... endif; lines were getting formatted poorly)

            int numSpacesBeforeFirstWord = 0;

            size_t linePos = 0;

            for( ; linePos < line.length(); linePos++ )
            {
                if( line[linePos] == ' ' )
                {
                    numSpacesBeforeFirstWord++;
                }

                else if( line[linePos] == '\t' )
                {
                    numSpacesBeforeFirstWord += 4;
                }

                else
                {
                    break;
                }
            }

            auto match_multiline_comment = [&](const std::string_view multiline_comment_sv, const std::string_view line_sv, const size_t line_pos)
            {
                return ( ( line_pos + multiline_comment_sv.length() ) <= line_sv.length() &&
                         line_sv.substr(line_pos).starts_with(multiline_comment_sv) );
            };


            // remove the comments from the temp line
            if( inComment )
            {
                linePos = 0;

                while( linePos < tempLine.length() )
                {
                    if( match_multiline_comment(m_multilineCommentStart_sv, tempLine, linePos) )
                    {
                        inComment = true;
                        numMultilineComments++;
                        linePos += m_multilineCommentStart_sv.length();
                    }

                    else if( numMultilineComments > 0 && match_multiline_comment(m_multilineCommentEnd_sv, tempLine, linePos) )
                    {
                        numMultilineComments--;
                        linePos += m_multilineCommentEnd_sv.length();

                        if( numMultilineComments == 0 ) // comment has now ended
                        {
                            inComment = false;
                            break;
                        }
                    }

                    else
                    {
                        linePos++;
                    }
                }

                tempLine = SO::Trim(tempLine.substr(linePos));
            }

            // now get rid of any future comments in the string, as well as strings
            char prevChar = 0;
            bool inSingleQuote = false;
            bool inDoubleQuote = false;
            size_t startCommentPos = 0;
            size_t startQuotePos = 0;

            for( size_t i = 0; i < tempLine.length(); i++ )
            {
                if( !inComment && !inSingleQuote && !inDoubleQuote && prevChar == '/' && tempLine[i] == '/' )
                {
                    tempLine = tempLine.substr(0, i - 1);
                    break;
                }

                else if( !inSingleQuote && !inDoubleQuote && match_multiline_comment(m_multilineCommentStart_sv, tempLine, i) )
                {
                    inComment = true;
                    numMultilineComments++;

                    if( numMultilineComments == 1 )
                        startCommentPos = i;

                    i += ( m_multilineCommentStart_sv.length() - 1 );
                }

                else if( !inSingleQuote && !inDoubleQuote && numMultilineComments > 0 &&
                         match_multiline_comment(m_multilineCommentEnd_sv, tempLine, i) )
                {
                    numMultilineComments--;

                    i += ( m_multilineCommentEnd_sv.length() - 1 );

                    if( numMultilineComments == 0 ) // comment has now ended
                    {
                        inComment = false;
                        tempLine = tempLine.substr(0, startCommentPos) + tempLine.substr(i + 1);
                        i = static_cast<size_t>(-1);
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                else if( !inComment && tempLine[i] == '\'' )
                {
                    if( !inSingleQuote && !inDoubleQuote )
                    {
                        inSingleQuote = true;
                        startQuotePos = i;
                    }

                    else if( inSingleQuote )
                    {
                        inSingleQuote = false;
                        tempLine = tempLine.substr(0, startQuotePos) + tempLine.substr(i + 1);
                        i = static_cast<size_t>(-1);
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                else if( !inComment && tempLine[i] == '"' )
                {
                    if( !inSingleQuote && !inDoubleQuote )
                    {
                        inDoubleQuote = true;
                        startQuotePos = i;
                    }

                    else if( inDoubleQuote )
                    {
                        inDoubleQuote = false;
                        tempLine = tempLine.substr(0, startQuotePos) + tempLine.substr(i + 1);
                        i = static_cast<size_t>(-1);
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                prevChar = tempLine[i];
            }

            if( inComment )
            {
                tempLine = tempLine.substr(0, startCommentPos);
            }

            else if( inSingleQuote || inDoubleQuote ) // (this case wouldn't compile correctly)
            {
                tempLine = tempLine.substr(0, startQuotePos);
            }

            // now tempLine contains the line of code without any comments or strings

            std::string formattedLine;

            bool foundMatch = false;
            bool addTabAfterPrint = false;

            addedNewline = false;

            // process all potential keywords
            linePos = 0;

            char prevNonWhitespaceChar = 0; // 20101230

            while( linePos < tempLine.length() )
            {
                std::string word;

                if( linePos < tempLine.length() && isalpha(tempLine[linePos]) )
                {
                    // 20101216 modified because variable names like hello_for were getting read as for statements
                    while( linePos < tempLine.length() && ( isalpha(tempLine[linePos]) || tempLine[linePos] == '_' ) )
                 // while( linePos < tempLine.length() && isalpha(tempLine[linePos]) )
                    {
                        word.push_back(tempLine[linePos]);
                        linePos++;
                    }
                }

                else // read all non-word characters
                {
                    while( linePos < tempLine.length() && !isalpha(tempLine[linePos]) )
                    {
                        char thisChar = tempLine[linePos];

                        if( thisChar != ' ' && thisChar != '\t' && thisChar != '\n' ) // 20101230
                            prevNonWhitespaceChar = thisChar;

                        linePos++;
                    }
                }

                if( !word.empty() ) // see if this is a valid keyword
                {
                    for( int i = 0; i < _countof(ZeroTabWords); i++ )
                    {
                        if( SO::EqualsNoCase(word, ZeroTabWords[i]) )
                        {
                            curTabPos = 0;
                            foundMatch = true;
                            addTabAfterPrint = true;

                            if( i == 0 && tempLine.find("GLOBA") != std::string::npos ) // we're still in PROC GLOBAL (or the user unfortunately has a variable with GLOBAL in the name)
                                addTabAfterPrint = false;

                            if( firstKeyword )
                            {
                                firstKeyword = false;
                            }

                            // make sure that PROCs and functions (after the first one, PROC GLOBAL, have spacing above them)
                            else if( i == 0 || i == 1 )
                            {
                                for( int j = prevBlankLines; j < 2; j++ )
                                    formattedLine.push_back('\n');
                            }

                            // preproc, postproc, onfocus, and killfocus must also have spacing above them, though only 1 line will be required
                            else if( !prevBlankLines )
                            {
                                formattedLine.push_back('\n');
                            }

                            if( i != 1 ) // for everything but a function, force there to be at least one empty line between the start of the code
                                addedNewline = true;
                        }
                    }

                    for( int i = 0; i < _countof(AddTabWords); i++ )
                    {
                        if( SO::EqualsNoCase(word, AddTabWords[i]) )
                        {
                            if( i == 1 && prevNonWhitespaceChar == ',' )
                            {
                                // 20101230 format logic wasn't working with the do used in the userbar,
                                // which will have a comma preceding the do word
                            }

                            else
                            {
                                foundMatch = true;
                                addTabAfterPrint = true;

                                // for elseif and else we must remove the tab for this line but insert it on the next line
                                if( i == 4 || i == 5 )
                                    curTabPos = std::max(0, curTabPos - 1);
                            }
                        }
                    }

                    for( int i = 0; i < _countof(RemoveTabWords); i++ )
                    {
                        if( SO::EqualsNoCase(word, RemoveTabWords[i]) )
                        {
                            if( addTabAfterPrint )
                            {
                                addTabAfterPrint = false; // this makes sense for lines where if and endif are on the same line
                            }

                            else
                            {
                                curTabPos--;

                                if( curTabPos < 0 )
                                    curTabPos = 0;
                            }

                            foundMatch = true;
                        }
                    }

                    prevNonWhitespaceChar = word[word.length() - 1];
                }
            }

            if( foundMatch )
            {
                prevShift = 0;
                prevSpaceCnt = 0;
            }

            if( !foundMatch ) // then we see how the number of spaces compared to the previous line and we adjust accordingly
            {
                prevShift = prevShift + ( numSpacesBeforeFirstWord - prevSpaceCnt );

                if( prevShift < 0 )
                    prevShift = 0;

                prevSpaceCnt = numSpacesBeforeFirstWord;
            }

            if( lastWordWasAnIndentation )
                prevShift = 0;

            for( int i = 0; i < curTabPos; i++ )
                formattedLine.push_back('\t');

            for( int i = 0; i < ( prevShift / 4 ); i++ )
                formattedLine.push_back('\t');

            for( int i = 0; i < ( prevShift % 4 ); i++ )
                formattedLine.push_back(' ');

            formattedLine.append(SO::Trim(line));

            formatted_logic.append(formattedLine);
            formatted_logic.push_back('\n');

            if( !foundMatch && lastWordWasAnIndentation )
                prevSpaceCnt = numSpacesBeforeFirstWord;

            lastWordWasAnIndentation = false;

            if( foundMatch )
            {
                if( addTabAfterPrint )
                {
                    curTabPos++;
                    lastWordWasAnIndentation = true;
                }

                prevSpaceCnt = 4 * curTabPos;
            }

            if( addedNewline )
            {
                formatted_logic.push_back('\n');
                prevBlankLines = 1;
            }

            else
            {
                prevBlankLines = 0;
            }
        }


        else if( !formatted_logic.empty() ) // don't allow newlines at the very beginning of the buffer
        {
            if( !addedNewline )
            {
                formatted_logic.push_back('\n');
                prevBlankLines++;
            }

            addedNewline = false;
        }
    }

    if( prevBlankLines < 2 )
        formatted_logic.push_back('\n');

    return formatted_logic;
}
