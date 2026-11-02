#include "stdafx.h"
#include "LogicFormatter.h"


CString LogicFormatter::FormatLogic(const int lexer, const std::wstring& logic)
{
    // routine from 20101208
    CString formattedLogic;

    // format the logic, using these rules
    //      Indent code
    //      Trim trailing spaces
    //      Convert spaces at beginning into tabs
    //      Must be at least two blank lines between functions, procs

    const TCHAR* const zeroTabWords[] = { _T("proc"), _T("function"), _T("preproc"), _T("postproc"), _T("onfocus"), _T("killfocus"), _T("onoccchange") };
    const TCHAR* const addTabWords[] = { _T("if"), _T("do"), _T("for"), _T("while"), _T("elseif"), _T("else") };
    const TCHAR* const removeTabWords[] = { _T("end"), _T("enddo"), _T("endfor"), _T("endif") };
    const int numZeroTabWords = 7, numAddTabWords = 6, numRemoveTabWords = 4;

    const bool use_old_multiline_comments = ( lexer == SCLEX_CSPRO_LOGIC_V0 );
    const std::string_view multiline_comment_start_sv = use_old_multiline_comments ? CommentStrings::MultilineOldStart_sv : CommentStrings::MultilineNewStart_sv;
    const std::string_view multiline_comment_end_sv = use_old_multiline_comments ? CommentStrings::MultilineOldEnd_sv : CommentStrings::MultilineNewEnd_sv;

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

    while (moreLines)
    {
        CString line;

        // get a line from the text
        while (curPos < logic.length() && logic[curPos] != '\r')
        {
            line.AppendChar(logic[curPos]);
            curPos++;
        }

        // advance past the new line
        if (curPos < logic.length() && logic[curPos] == '\r')
            curPos++;

        if (curPos < logic.length() && logic[curPos] == '\n')
            curPos++;

        if (curPos >= logic.length())
            moreLines = false;

        CString tempLine = line;

        if (!tempLine.Trim().IsEmpty())
        {
            // 20101209 i initially programmed this to only analyze the first word on the line, but i'm now changing it
            // to analyze all words on the line (because if .... endif; lines were getting formatted poorly)

            int numSpacesBeforeFirstWord = 0;

            int linePos = 0;

            for (; linePos < line.GetLength(); linePos++)
            {
                if (line[linePos] == ' ')
                    numSpacesBeforeFirstWord++;

                else if (line[linePos] == '\t')
                    numSpacesBeforeFirstWord += 4;

                else
                    break;
            }

            auto match_multiline_comment = [&](const std::string_view multiline_comment_sv, const wstring_view line_sv, const int line_pos)
            {
                return ( ( line_pos + multiline_comment_sv.length() ) <= line_sv.length() &&
                         line_sv.substr(line_pos).starts_with(UTF8_TODO::GetWide(multiline_comment_sv)) );
            };


            // remove the comments from the temp line
            if (inComment)
            {
                linePos = 0;

                while (linePos < tempLine.GetLength())
                {
                    if (match_multiline_comment(multiline_comment_start_sv, tempLine, linePos))
                    {
                        inComment = true;
                        numMultilineComments++;
                        linePos += multiline_comment_start_sv.length();
                    }

                    else if (numMultilineComments > 0 && match_multiline_comment(multiline_comment_end_sv, tempLine, linePos))
                    {
                        numMultilineComments--;
                        linePos += multiline_comment_end_sv.length();

                        if (numMultilineComments == 0) // comment has now ended
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

                tempLine = tempLine.Mid(linePos).Trim();
            }

            // now get rid of any future comments in the string, as well as strings
            TCHAR prevChar = 0;
            bool inSingleQuote = false;
            bool inDoubleQuote = false;
            int startCommentPos = 0;
            int startQuotePos = 0;

            for (int i = 0; i < tempLine.GetLength(); i++)
            {
                if (!inComment && !inSingleQuote && !inDoubleQuote && prevChar == '/' && tempLine[i] == '/')
                {
                    tempLine = tempLine.Left(i - 1);
                    break;
                }

                else if (!inSingleQuote && !inDoubleQuote && match_multiline_comment(multiline_comment_start_sv, tempLine, i))
                {
                    inComment = true;
                    numMultilineComments++;

                    if (numMultilineComments == 1)
                        startCommentPos = i;

                    i += ( multiline_comment_start_sv.length() - 1 );
                }

                else if (!inSingleQuote && !inDoubleQuote && numMultilineComments > 0 && match_multiline_comment(multiline_comment_end_sv, tempLine, i))
                {
                    numMultilineComments--;

                    i += ( multiline_comment_end_sv.length() - 1 );

                    if (numMultilineComments == 0) // comment has now ended
                    {
                        inComment = false;
                        tempLine = tempLine.Left(startCommentPos) + tempLine.Mid(i + 1);
                        i = -1;
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                else if (!inComment && tempLine[i] == '\'')
                {
                    if (!inSingleQuote && !inDoubleQuote)
                    {
                        inSingleQuote = true;
                        startQuotePos = i;
                    }

                    else if (inSingleQuote)
                    {
                        inSingleQuote = false;
                        tempLine = tempLine.Left(startQuotePos) + tempLine.Mid(i + 1);
                        i = -1;
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                else if (!inComment && tempLine[i] == '"')
                {
                    if (!inSingleQuote && !inDoubleQuote)
                    {
                        inDoubleQuote = true;
                        startQuotePos = i;
                    }

                    else if (inDoubleQuote)
                    {
                        inDoubleQuote = false;
                        tempLine = tempLine.Left(startQuotePos) + tempLine.Mid(i + 1);
                        i = -1;
                        continue; // start processing the string from the beginning (now that one group of comments has been removed)
                    }
                }

                prevChar = tempLine[i];
            }

            if (inComment)
                tempLine = tempLine.Left(startCommentPos);

            else if (inSingleQuote || inDoubleQuote) // (this case wouldn't compile correctly)
                tempLine = tempLine.Left(startQuotePos);

            // now tempLine contains the line of code without any comments or strings

            CString formattedLine;

            bool foundMatch = false;
            bool addTabAfterPrint = false;

            addedNewline = false;

            // process all potential keywords
            linePos = 0;

            TCHAR prevNonWhitespaceChar = 0; // 20101230

            while (linePos < tempLine.GetLength())
            {
                CString word;

                if (linePos < tempLine.GetLength() && _istalpha(tempLine[linePos]))
                {
                    // 20101216 modified because variable names like hello_for were getting read as for statements
                    while (linePos < tempLine.GetLength() && (_istalnum(tempLine[linePos]) || tempLine[linePos] == '_'))
                        // while( linePos < tempLine.GetLength() && isalpha(tempLine[linePos]) )
                    {
                        word.AppendChar(tempLine[linePos]);
                        linePos++;
                    }
                }

                else // read all non-word characters
                {
                    while (linePos < tempLine.GetLength() && !_istalpha(tempLine[linePos]))
                    {
                        TCHAR thisChar = tempLine[linePos];

                        if (thisChar != ' ' && thisChar != '\t' && thisChar != '\r' && thisChar != '\n') // 20101230
                            prevNonWhitespaceChar = thisChar;

                        linePos++;
                    }
                }

                if (!word.IsEmpty()) // see if this is a valid keyword
                {
                    for (int i = 0; i < numZeroTabWords; i++)
                    {
                        if (!word.CompareNoCase(zeroTabWords[i]))
                        {
                            curTabPos = 0;
                            foundMatch = true;
                            addTabAfterPrint = true;

                            if (i == 0 && tempLine.Find(_T("GLOBAL")) >= 0) // we're still in PROC GLOBAL (or the user unfortunately has a variable with GLOBAL in the name)
                                addTabAfterPrint = false;

                            if (firstKeyword)
                                firstKeyword = false;

                            // make sure that PROCs and functions (after the first one, PROC GLOBAL, have spacing above them)
                            else if (i == 0 || i == 1)
                            {
                                for (int j = prevBlankLines; j < 2; j++)
                                    formattedLine.AppendChar('\n');
                            }

                            // preproc, postproc, onfocus, and killfocus must also have spacing above them, though only 1 line will be required
                            else if (!prevBlankLines) {
                                formattedLine.AppendChar('\n');
                            }

                            if (i != 1) // for everything but a function, force there to be at least one empty line between the start of the code
                                addedNewline = true;
                        }
                    }

                    for (int i = 0; i < numAddTabWords; i++)
                    {
                        if (!word.CompareNoCase(addTabWords[i]))
                        {
                            if (i == 1 && prevNonWhitespaceChar == ',')
                                ;   // 20101230 format logic wasn't working with the do used in the userbar,
                                    // which will have a comma preceding the do word

                            else
                            {
                                foundMatch = true;
                                addTabAfterPrint = true;

                                if (i == 4 || i == 5) // for elseif and else we must remove the tab for this line but insert it on the next line
                                    curTabPos = std::max(0, curTabPos - 1);
                            }
                        }
                    }

                    for (int i = 0; i < numRemoveTabWords; i++)
                    {
                        if (!word.CompareNoCase(removeTabWords[i]))
                        {
                            if (addTabAfterPrint)
                                addTabAfterPrint = false; // this makes sense for lines where if and endif are on the same line

                            else
                            {
                                curTabPos--;

                                if (curTabPos < 0)
                                    curTabPos = 0;
                            }

                            foundMatch = true;
                        }
                    }

                    prevNonWhitespaceChar = word[word.GetLength() - 1];
                }
            }

            if (foundMatch)
            {
                prevShift = 0;
                prevSpaceCnt = 0;
            }

            if (!foundMatch) // then we see how the number of spaces compared to the previous line and we adjust accordingly
            {
                prevShift = prevShift + (numSpacesBeforeFirstWord - prevSpaceCnt);

                if (prevShift < 0)
                    prevShift = 0;

                prevSpaceCnt = numSpacesBeforeFirstWord;
            }

            if (lastWordWasAnIndentation)
                prevShift = 0;

            for (int i = 0; i < curTabPos; i++)
                formattedLine.AppendChar('\t');

            for (int i = 0; i < (prevShift / 4); i++)
                formattedLine.AppendChar('\t');

            for (int i = 0; i < (prevShift % 4); i++)
                formattedLine.AppendChar(' ');

            formattedLine.Append(line.Trim());

            formattedLogic.Append(formattedLine);
            //formattedLogic.AppendChar('\r');
            formattedLogic.AppendChar('\n');

            if (!foundMatch && lastWordWasAnIndentation)
                prevSpaceCnt = numSpacesBeforeFirstWord;

            lastWordWasAnIndentation = false;

            if (foundMatch)
            {
                if (addTabAfterPrint)
                {
                    curTabPos++;
                    lastWordWasAnIndentation = true;
                }

                prevSpaceCnt = 4 * curTabPos;
            }

            if (addedNewline)
            {
                //formattedLogic.AppendChar('\r');
                formattedLogic.AppendChar('\n');
                prevBlankLines = 1;
            }

            else
                prevBlankLines = 0;
        }


        else if (!formattedLogic.IsEmpty()) // don't allow newlines at the very beginning of the buffer
        {
            if (!addedNewline)
            {
                formattedLogic.AppendChar('\n');
                prevBlankLines++;
            }

            addedNewline = false;
        }
    }

    if (prevBlankLines < 2) {
        formattedLogic.AppendChar('\n');
    }

    formattedLogic.Replace(_T("\n"), _T("\r\n")); //For scintilla

    return formattedLogic;
}
