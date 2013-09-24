/* Utils.em - a small collection of useful editing macros */



/*-------------------------------------------------------------------------
	I N S E R T   H E A D E R

	Inserts a comment header block at the top of the current function. 
	This actually works on any type of symbol, not just functions.

	To use this, define an environment variable "MYNAME" and set it
	to your email name.  eg. set MYNAME=raygr
-------------------------------------------------------------------------*/
macro InsertHeader()
{
	// Get the owner's name from the environment variable: MYNAME.
	// If the variable doesn't exist, then the owner field is skipped.
	szMyName = getenv(MYNAME)
	
	// Get a handle to the current file buffer and the name
	// and location of the current symbol where the cursor is.
	hbuf = GetCurrentBuf()
	szFunc = GetCurSymbol()
	ln = GetSymbolLine(szFunc)

	// begin assembling the title string
	sz = "/*   "
	
	/* convert symbol name to T E X T   L I K E   T H I S */
	cch = strlen(szFunc)
	ich = 0
	while (ich < cch)
		{
		ch = szFunc[ich]
		if (ich > 0)
			if (isupper(ch))
				sz = cat(sz, "   ")
			else
				sz = cat(sz, " ")
		sz = Cat(sz, toupper(ch))
		ich = ich + 1
		}
	
	sz = Cat(sz, "   */")
	InsBufLine(hbuf, ln, sz)
	InsBufLine(hbuf, ln+1, "/*-------------------------------------------------------------------------")
	
	/* if owner variable exists, insert Owner: name */
	if (strlen(szMyName) > 0)
		{
		InsBufLine(hbuf, ln+2, "    Owner: @szMyName@")
		InsBufLine(hbuf, ln+3, " ")
		ln = ln + 4
		}
	else
		ln = ln + 2
	
	InsBufLine(hbuf, ln,   "    ") // provide an indent already
	InsBufLine(hbuf, ln+1, "-------------------------------------------------------------------------*/")
	
	// put the insertion point inside the header comment
	SetBufIns(hbuf, ln, 4)
}


/* InsertFileHeader:

   Inserts a comment header block at the top of the current function. 
   This actually works on any type of symbol, not just functions.

   To use this, define an environment variable "MYNAME" and set it
   to your email name.  eg. set MYNAME=raygr
*/

macro InsertFileHeader()
{
	szMyName = getenv(MYNAME)
	
	hbuf = GetCurrentBuf()

	InsBufLine(hbuf, 0, "/*-------------------------------------------------------------------------")
	
	/* if owner variable exists, insert Owner: name */
	InsBufLine(hbuf, 1, "    ")
	if (strlen(szMyName) > 0)
		{
		sz = "    Owner: @szMyName@"
		InsBufLine(hbuf, 2, " ")
		InsBufLine(hbuf, 3, sz)
		ln = 4
		}
	else
		ln = 2
	
	InsBufLine(hbuf, ln, "-------------------------------------------------------------------------*/")
}



// Inserts "Returns True .. or False..." at the current line
macro ReturnTrueOrFalse()
{
	hbuf = GetCurrentBuf()
	ln = GetBufLineCur(hbuf)

	InsBufLine(hbuf, ln, "    Returns True if successful or False if errors.")
}



/* Inserts ifdef REVIEW around the selection */
macro IfdefReview()
{
	IfdefSz("REVIEW");
}


/* Inserts ifdef BOGUS around the selection */
macro IfdefBogus()
{
	IfdefSz("BOGUS");
}


/* Inserts ifdef NEVER around the selection */
macro IfdefNever()
{
	IfdefSz("NEVER");
}


// Ask user for ifdef condition and wrap it around current
// selection.
macro InsertIfdef()
{
	sz = Ask("Enter ifdef condition:")
	if (sz != "")
		IfdefSz(sz);
}

macro InsertCPlusPlus()
{
	IfdefSz("__cplusplus");
}


// Wrap ifdef <sz> .. endif around the current selection
macro IfdefSz(sz)
{
	hwnd = GetCurrentWnd()
	lnFirst = GetWndSelLnFirst(hwnd)
	lnLast = GetWndSelLnLast(hwnd)
	 
	hbuf = GetCurrentBuf()
	InsBufLine(hbuf, lnFirst, "#ifdef @sz@")
	InsBufLine(hbuf, lnLast+2, "#endif /* @sz@ */")
}



// Delete the current line and appends it to the clipboard buffer
macro KillLine()
{
	hbufCur = GetCurrentBuf();
	lnCur = GetBufLnCur(hbufCur)
	hbufClip = GetBufHandle("Clipboard")
	AppendBufLine(hbufClip, GetBufLine(hbufCur, lnCur))
	DelBufLine(hbufCur, lnCur)
}


// Paste lines killed with KillLine (clipboard is emptied)
macro PasteKillLine()
{
	Paste
	EmptyBuf(GetBufHandle("Clipboard"))
}



// delete all lines in the buffer
macro EmptyBuf(hbuf)
{
	lnMax = GetBufLineCount(hbuf)
	while (lnMax > 0)
		{
		DelBufLine(hbuf, 0)
		lnMax = lnMax - 1
		}
}


// Ask the user for a symbol name, then jump to its declaration
macro JumpAnywhere()
{
	symbol = Ask("What declaration would you like to see?")
	JumpToSymbolDef(symbol)
}

	
// list all siblings of a user specified symbol
// A sibling is any other symbol declared in the same file.
macro OutputSiblingSymbols()
{
	symbol = Ask("What symbol would you like to list siblings for?")
	hbuf = ListAllSiblings(symbol)
	SetCurrentBuf(hbuf)
}


// Given a symbol name, open the file its declared in and 
// create a new output buffer listing all of the symbols declared
// in that file.  Returns the new buffer handle.
macro ListAllSiblings(symbol)
{
	loc = GetSymbolLocation(symbol)
	if (loc == "")
		{
		msg ("@symbol@ not found.")
		stop
		}
	
	hbufOutput = NewBuf("Results")
	
	hbuf = OpenBuf(loc.file)
	if (hbuf == 0)
		{
		msg ("Can't open file.")
		stop
		}
		
	isymMax = GetBufSymCount(hbuf)
	isym = 0;
	while (isym < isymMax)
		{
		AppendBufLine(hbufOutput, GetBufSymName(hbuf, isym))
		isym = isym + 1
		}

	CloseBuf(hbuf)
	
	return hbufOutput

}

macro AddMacroComment()
{
    hwnd=GetCurrentWnd()
    sel=GetWndSel(hwnd)
    lnFirst=GetWndSelLnFirst(hwnd)
    lnLast=GetWndSelLnLast(hwnd)
    hbuf=GetCurrentBuf()
 
    if(LnFirst == 0) {
            szIfStart = ""
    }else{
            szIfStart = GetBufLine(hbuf, LnFirst-1)
    }
    szIfEnd = GetBufLine(hbuf, lnLast+1)
    if(szIfStart == "#if 0" && szIfEnd == "#endif") {
            DelBufLine(hbuf, lnLast+1)
            DelBufLine(hbuf, lnFirst-1)
            sel.lnFirst = sel.lnFirst 每 1
            sel.lnLast = sel.lnLast 每 1
    }else{
            InsBufLine(hbuf, lnFirst, "#if 0")
            InsBufLine(hbuf, lnLast+2, "#endif")
            sel.lnFirst = sel.lnFirst + 1
            sel.lnLast = sel.lnLast + 1
    }
 
    SetWndSel( hwnd, sel )
}


//
// Undo the CommentBlock for the selected text.
//
macro UnCommentBlock()
{
	hbuf = GetCurrentBuf();
	hwnd = GetCurrentWnd();

	sel = GetWndSel(hwnd);

	iLine = sel.lnFirst;


	tabSize = 0;
	while (iLine <= sel.lnLast)
	{
		szLine = GetBufLine(hbuf, iLine);
		len = strlen(szLine);
		szNewLine = "";
		if (len > 1)
		{
			if (szLine[0] == "/" && szLine[1] == "/")
			{
				if (len > 2)
				{
					if (AsciiFromChar(szLine[2]) == 9)
					{
						tabSize = _tsGetTabSize() - 1;
						szNewLine = strmid(szLine, 3, strlen(szLine));
					}
				}

				if (szNewLine == "")
				{
					szNewLine = strmid(szLine, 2, strlen(szLine));
					tabSize = 2;
				}
				
				PutBufLine(hbuf, iLine, szNewLine);
			}
		}
		iLine = iLine + 1;
	}

	if (sel.lnFirst == sel.lnLast)
	{
		sel.ichFirst = sel.ichFirst - tabSize;
		sel.ichLim = sel.ichLim - tabSize;
	}

	SetWndSel(hwnd, sel);
}


macro CommentSelStr()  
{  
    hbuf = GetCurrentBuf()  
    ln = GetBufLnCur(hbuf)  
    str = GetBufSelText(hbuf)  
    str = cat("/*",str)  
    str = cat(str,"*/")  
    SetBufSelText (hbuf, str)  
}  





//
// Comment the selected block of text using single line comments and indent it
//
macro CommentBlock()
{
 	hbuf = GetCurrentBuf();
 	hwnd = GetCurrentWnd();
 
 	sel = GetWndSel(hwnd);
 
 	iLine = sel.lnFirst;
	
	while (iLine <= sel.lnLast)
	{
		szLine = GetBufLine(hbuf, iLine);
		szLine = cat("//	", szLine);
		PutBufLine(hbuf, iLine, szLine);
		iLine = iLine + 1;
	}

	if (sel.lnFirst == sel.lnLast)
	{
		tabSize = _tsGetTabSize() - 1;
		sel.ichFirst = sel.ichFirst + tabSize;
		sel.ichLim = sel.ichLim + tabSize;
	}
	SetWndSel(hwnd, sel);
}


//
// Undo the CommentBlock for the selected text.
//
macro UnCommentBlock()
{
	hbuf = GetCurrentBuf();
	hwnd = GetCurrentWnd();

	sel = GetWndSel(hwnd);

	iLine = sel.lnFirst;


	tabSize = 0;
	while (iLine <= sel.lnLast)
	{
		szLine = GetBufLine(hbuf, iLine);
		len = strlen(szLine);
		szNewLine = "";
		if (len > 1)
		{
			if (szLine[0] == "/" && szLine[1] == "/")
			{
				if (len > 2)
				{
					if (AsciiFromChar(szLine[2]) == 9)
					{
						tabSize = _tsGetTabSize() - 1;
						szNewLine = strmid(szLine, 3, strlen(szLine));
					}
				}

				if (szNewLine == "")
				{
					szNewLine = strmid(szLine, 2, strlen(szLine));
					tabSize = 2;
				}
				
				PutBufLine(hbuf, iLine, szNewLine);
			}
		}
		iLine = iLine + 1;
	}

	if (sel.lnFirst == sel.lnLast)
	{
		sel.ichFirst = sel.ichFirst - tabSize;
		sel.ichLim = sel.ichLim - tabSize;
	}

	SetWndSel(hwnd, sel);
}



macro _tsGetTabSize()
{
	szTabSize = GetReg("TabSize");

	if (szTabSize != "")
	{
		tabSize = AsciiFromChar(szTabSize[0]) - AsciiFromChar("0");
	}
	else
	{
		tabSize = 4;
	}

	return tabSize;
}




//
// Reformat a selected comment block to wrap text at 80 columns.
// The start of the selection (upper left most character of the selection) is
// handled specially, in that it specifies the left most column at which all
// lines will begin.  For example, if the following block was selected starting
// at the @ symbol, through the last line of the block...
//------------------------------------------------------------------------------
// preamble:	@ This is a line that will be wrapped keeping the "at" symbol in its current column.
// All lines following it that are selected will use that as their starting column.  See below to see how the wrapping
// works for this block of text.
//------------------------------------------------------------------------------
// preamble:	@ This is a line that will be wrapped keeping the "at" symbol in
//				its current column. All lines following it that are selected
//				will use that as their starting column.  See below to see how
//				the wrapping works for this block of text.
//
macro tsReformatCommentBlock()
{
	hbuf = GetCurrentBuf();
	hwnd = GetCurrentWnd();

	sel = GetWndSel(hwnd);

	tabSize = _tsGetTabSize();
	leftTextCol = 0 - 1;
	colWrap = 80;

	// Find the starting column, and create a Margin string
	ichFirst = sel.ichFirst;
	
	// Single line comment reformat?
	if (sel.ichFirst == sel.ichLim && sel.lnFirst == sel.lnLast)
	{
		ichFirst = 0;
	}

	rec = _tsGetStartColumn(hbuf, ichFirst, sel.lnFirst);

	if (rec == "")
		stop;

	colLeftMargin = rec.colMargin;
	szMargin = "";

	colComment = 0;
	if (rec.colComment >= 0)
	{
		colComment = rec.colComment + 2
		szMargin = _tsAddWhiteToColumn(szMargin, 0, rec.colComment, tabSize);
		szMargin = cat(szMargin, "//");
	}

	szMargin = _tsAddWhiteToColumn(szMargin, colComment, rec.colMargin, tabSize);

	rec = "";

	szCurMargin = "";
	if (ichFirst != 0)
	{
		szLine = GetBufLine(hbuf, sel.lnFirst);
		szCurMargin = strmid(szLine, 0, ichFirst);
	}
	else
	{
		szCurMargin = szMargin;
		szMargin = "";
	}

	insertLine = sel.lnFirst;
	iLine = sel.lnFirst;
	szRemainder = "";
	while (1)
	{
//		msg("$0-" # iLine # ":" # szRemainder);
		rec = _tsGetNextCommentString(hbuf, ichFirst, szRemainder, iLine, sel.lnLast, colWrap);
		ichFirst = 0;

		if (rec == "")
			break;

//		msg("$1-" # rec.ln # ":" # rec.szComment);
		szLine = rec.szComment;
	
		ich = 0;
		col = colLeftMargin;
		len = strlen(szLine);
		
		ichPrevCharToWhite = 0-1;
		ichPrevWhiteToChar = 0-1;
//		msg("Leftovers @szRemainder@");

		while (ich < len)
		{
			if (AsciiFromChar(szLine[ich]) == 9)
			{
				col = (((col + tabSize) / tabSize) * tabSize);
			}
			else
			{
				col = col + 1;
			}

			if (col > colWrap)
				break;

			fIsWhitespace = _tsIsWhitespaceChar(szLine[ich]);
			fIsWhitespace1 = 1;

			if (ich + 1 < len)
			{
				fIsWhitespace1 = _tsIsWhitespaceChar(szLine[ich + 1]);
			}

			if (!fIsWhitespace && fIsWhitespace1)
				ichPrevCharToWhite = ich;
			
			ich = ich + 1;
		}

		if (ichPrevCharToWhite > 0)
		{
//			msg("$2:" # strmid(szLine, 0, ichPrevCharToWhite + 1));
			ich = ichPrevCharToWhite + 1;

			while (ich < len)
			{
				if (!_tsIsWhitespaceChar(szLine[ich]))
				{
					ichPrevWhiteToChar = ich - 1;
//					msg("$3:" # strmid(szLine, ichPrevWhiteToChar + 1, len));
					break;
				}
				ich = ich + 1;
			}
		}

		if (ichPrevCharToWhite > 0 && col > colWrap)
		{
			szNewLine = cat(szCurMargin, strmid(szLine, 0, ichPrevCharToWhite + 1));
			szRemainder = "";
			if (ichPrevWhiteToChar > 0)
				szRemainder = strmid(szLine, ichPrevWhiteToChar + 1, len);
				
			if (ichPrevCharToWhite > ichPrevWhiteToChar)
				msg("!!!Wrap, duplicating word " # ichPrevWhiteToChar # " " # ichPrevCharToWhite # " " # szNewLine # " >>> " # szRemainder);
//				msg(szLine);
//				msg(col # " " # ichPrevWhiteToChar # " " # ichPrevCharToWhite # " " # szNewLine # " >>> " # szRemainder);
		}
		else if (szLine != "")
		{
			szNewLine = cat(szCurMargin, szLine );
			szRemainder = "";
//			sel.lnLast = sel.lnLast + 1;
		}

		iLine = rec.ln;
		if (insertLine == iLine)
		{
			iLine = iLine + 1;
			sel.lnLast = sel.lnLast + 1;
			
//			msg("$5-" # insertLine # ":" # szNewLine);
			InsBufLine(hbuf, insertLine, szNewLine);
		}
		else
		{
			szLine = GetBufLine(hbuf, insertLine);
			if (szLine != szNewLine)
			{
//				msg("$6-" # insertLine # ":" # szNewLine);
				PutBufLine(hbuf, insertLine, szNewLine);
			}
		}
		insertLine = insertLine + 1;

		if (szMargin != "")
		{
			szCurMargin = szMargin;
			szMargin = "";
		}
	}

	while (insertLine <= sel.lnLast)
	{
		DelBufLine(hbuf, insertLine);
		sel.lnLast = sel.lnLast - 1;
	}

	len = GetBufLineLength(hbuf, insertLine-1);

	sel.ichFirst = len;
	sel.ichLim = len;
	sel.lnFirst = sel.lnLast;
	SetWndSel(hwnd, sel);
}


macro _tsAddWhiteToColumn(sz, col0, col, tabSize)
{
	szTabs = "																														";
	szSpaces = "                ";

	tabs0 = col0 / tabSize;
	tabs = (col / tabSize) - tabs0;

	if (tabs == 0)
		foo = col0;
	else
		foo = (tabs + tabs0) * tabSize;
	spaces = col - foo;
//	msg(col0 # " " # col # " " # tabs # " " # spaces # " " # tabs0);

	if (tabs)
		sz = cat(sz, strmid(szTabs, 0, tabs));

	if (spaces)
		sz = cat(sz, strmid(szSpaces, 0, spaces));

	return sz;
}

macro _tsGetStartColumn(hbuf, ichBegin, ln)
{
	szLine = GetBufLine(hbuf, ln);
	len = strlen(szLine);
	tabSize = _tsGetTabSize();
	ich = 0;

	colMargin = 0;
	colComment = 0-1;

	rec = "";
	rec.colMargin = colMargin;
	rec.colComment = colComment;
	
	while (ich < len)
	{
		if (AsciiFromChar(szLine[ich]) == 9)
		{
			colMargin = (((colMargin + tabSize) / tabSize) * tabSize);
		}
		else
		{
			colMargin = colMargin + 1;
		}

		if (colComment < 0)
		{
			if (ich + 1 < len)
			{
				if (szLine[ich] == "/" && szLine[ich+1] == "/")
				{
					colComment = colMargin - 1;
					ich = ich + 2;
					colMargin = colMargin + 1;
					continue;
				}
			}
		}

		if (ich >= ichBegin)
		{
			if (!_tsIsWhitespaceChar(szLine[ich]))
			{
				rec.colMargin = colMargin - 1;
				rec.colComment = colComment;
//				msg(szLine[ich]);
				return rec;
			}
		}

		ich = ich + 1;
	}
	
	return rec;
}

macro _tsGetNextCommentString(hbuf, ichSkip, szRemainder, ln, lnLast, colWrap)
{
	rec = "";

	// Go until we get a string that is at least long enough to fill a line
	// or, we run out of lines.
	if (szRemainder == "" && ln > lnLast)
		return "";
		
	ichFirst = ichSkip;
//	msg(ichSkip);
	while (1)
	{
		if (ln > lnLast)
		{
			rec.szComment = szRemainder;
			rec.ln = ln;
			return rec;
		}

		cchRemainder = strlen(szRemainder);

		if (cchRemainder > colWrap)
		{
			rec.szComment = szRemainder;
			rec.ln = ln;
			return rec;
		}

		szLine = GetBufLine(hbuf, ln);
		len = strlen(szLine);

		if (ichSkip == 0)
			ichFirst = _tsSkipPastCommentAndWhitespace(szLine, len);
		ichSkip = 0;
			
		ichLast = len - 1;

		// Now, strip out all whitespace at the end of the line
		while (ichLast >= ichFirst)
		{
			if (!_tsIsWhitespaceChar(szLine[ichLast]))
			{
				break;
			}
			ichLast = ichLast - 1;
		}

		// Entire line is whitespace?
		if (ichLast < ichFirst)
		{
			if (szRemainder == "")
				ln = ln + 1;
			rec.szComment = szRemainder;
			rec.ln = ln;
			return rec;

		}
		// length of the non whitespaced comment + 1 space + cchRemainder
		if ((ichLast + 1) - ichFirst + cchRemainder + 1 > 255)
		{
			// It may not format the current line quite right, but
			// but at least we won't throw away some of the comment.
			rec.szComment = szRemainder;
			rec.ln = ln;
			return rec;
		}

		if (szRemainder != "")
			szRemainder = cat(szRemainder, " ");
		szRemainder = cat(szRemainder, strmid(szLine, ichFirst, ichLast + 1));
		ln = ln + 1;
	}
}



macro _tsSkipPastCommentAndWhitespace(szLine, len)
{
	ichFirst = 0;
	// Skip past the comment initiator "//" if there is one.
	while (ichFirst < len)
	{
		if (ichFirst + 1 < len)
		{
			if (szLine[ichFirst] == "/" && szLine[ichFirst+1] == "/")
			{
				ichFirst = ichFirst + 2;
				break;
			}
		}
		ichFirst = ichFirst + 1;
	}

	// If no comment found in line, then start from the beginning
	if (ichFirst >= len)
		ichFirst = 0;

	ichFirst = ichFirst;
	// Now, strip out all whitespace after the comment start.
	while (ichFirst < len)
	{
		if (!_tsIsWhitespaceChar(szLine[ichFirst]))
		{
			break;
		}
		ichFirst = ichFirst + 1;
	}

	return ichFirst;
}

macro AddMacroComment()
{
    hwnd=GetCurrentWnd()
    sel=GetWndSel(hwnd)
    lnFirst=GetWndSelLnFirst(hwnd)
    lnLast=GetWndSelLnLast(hwnd)
    hbuf=GetCurrentBuf()
 
    if(LnFirst == 0) {
            szIfStart = ""
    }else{
            szIfStart = GetBufLine(hbuf, LnFirst-1)
    }
    szIfEnd = GetBufLine(hbuf, lnLast+1)
    if(szIfStart == "#if 0" && szIfEnd == "#endif") {
            DelBufLine(hbuf, lnLast+1)
            DelBufLine(hbuf, lnFirst-1)
            sel.lnFirst = sel.lnFirst 每 1
            sel.lnLast = sel.lnLast 每 1
    }else{
            InsBufLine(hbuf, lnFirst, "#if 0")
            InsBufLine(hbuf, lnLast+2, "#endif")
            sel.lnFirst = sel.lnFirst + 1
            sel.lnLast = sel.lnLast + 1
    }
 
    SetWndSel( hwnd, sel )
}

/* --------------------------------------------------------------
# Module CompleteWord version 0.0.1
# Released to the public domain 14-Aug-1999,
# by Tim Peters (tim_one@email.msn.com).

# Provided as-is; use at your own risk; no warranty; no promises; enjoy!

Word completion.
Macro CompleteWord moves forward.
Macro CompleteWordBack moves backward.
Assign to e.g. F12 and Shift+F12.

The word characters ([a-zA-Z0-9_]) immediately preceding the
cursor are called the "stem".  A "completion" is any full word
that begins with the stem.  Conceptually, all possible
completions are built into a list as follows:

	look backward in the buffer from the stem location,
	and append each completion not seen before

	similarly, look forward in the buffer from the stem
	location for other new completions

	for all other open windows, look forward from the start
	of their buffers for other new completions

CompleteWord then moves forward through this list, and
CompleteWordBack moves backward through it.  If you invoke
CompleteWord when you're already at the end of the list,
a msg pops up telling you so; likewise if you're at the start
of the list and invoke CompleteWordBack.

Each time you move to a new list entry, the stem is completed
and the insertion point is moved to the end of the completion.

Example:

   	Py_BEGIN_ALLOW_THREADS
	errno = 0;
	res = fflush(f->f_fp);
	Py_END_ALLOW_THREADS
	if (res != 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		clearerr(f->f_fp);
		return NULL;
	}
	Py_INCREF(Py_None);
	Py_
    Py_SomethingElse();

Suppose the cursor follows the "Py_" on the penultimate line.
Then CompleteWord will first suggest Py_None, then Py_INCREF,
then Py_END_ALLOW_THREADS, then Py_BEGIN_ALLOW_THREADS, then
Py_SomethingElse, and if you're still unhappy <wink> will go on
to search other windows' buffers.

Notes:

+ This has nothing to do with Source Insight's notion of "symbols".
  To the contrary, the long hairy words I need to type most often
  over & over are local words that SI's Complete_Symbol doesn't
  know about.  It's also the case that the word you most often
  need to type is one you most recently typed, so the macros
  work hard to suggest the closest preceding matches first.
  This flavor of completion also works fine in file types SI knows
  nothing about.

+ The list isn't actually built up at once -- as far as possible,
  it's built incrementally as you continue to invoke CompleteWord.

+ It would help if SI's SearchInBuf could search backwards.  As
  is, finding the first suggestion is done by searching the entire
  buffer forward up until the stem location, and fiddling the
  results to act "as if" things were found in the other order.
  This is clumsy, but worse if you're near the end of a long file
  with many stem matches it can take appreciable time to find the
  "first" match (since it's actually found last ...)..

+ Would be nice to be able to display msgs on the status line;
  e.g., the macros keep track of the file names and line numbers
  at which completions were found, and that's sometimes useful
  info to know (the completion process sometimes turns up
  surprises! then you'd like to know where they came from).
  The list is built into a buffer named "*Completion*", and you
  may want to peek at that.
-------------------------------------------------------------- */

macro CompleteWord()
{
	CW_guts(1)
}

macro CompleteWordBack()
{
	CW_guts(0)
}

/* BUG ALERT:  there's apparently an undocumented limit on
   string & record vrbl size (about 2**8 chars).  Makes the
   following more convoluted than I'd like, and it will still
   fail in bizarre ways if e.g the stem is "too big".
 */

/* --------------------------------------------------------------
Structure of *Completion* buffer:
	First record summarizes our state:
		.orighbuf	original buffer
		.orighwnd	original window
		.origlno	original line number
		.origi		slice indices of start ...
		.origj		... and end of stem
		.stem		original stem word
		.newj		where we left insertion point
		.index		index into *Completion* of current completion
		.searchwnd	window we're searching now
		.searchlno	next line number to search at
		.searchich  next char index to search at
	Remaining records detail unique completions:
		.file		name of file match found in
		.line		line number within file of match
		.match		the completion
-------------------------------------------------------------- */

/* Selection format
lnFirst		the first line number
ichFirst	the index of the first character on the line lnFirst
lnLast		the last line number
ichLim		the limit index (one past the last) of the last character
			on the line given in lnLast
fExtended	TRUE if the selection is extended to include more than one
            character
        .   FALSE if the selection is a simple insertion point.
Note: this is the same as the following expression:
(sel.fRect || sel.lnFirst != sel.lnLast || sel.ichFirst != sel.ichLim)

fRect		TRUE if selection is rectangular (block style),
FALSE 		if the selection is a linear range of characters.
The following fields only apply if fRect is TRUE:
xLeft		the left pixel position of the rectangle in window coordinates.
xRight		the pixel position of the right edge of the rectangle in
			window coordinates.
*/

/* Completion "word" was found in file "fname" at line "lno".
 * Search hBuf for an exact previous match to "word".  If
 * none found, append a match record to hBuf, and return
 * the match record.  If found and bReplace is false, leave
 * hBuf alone and return "".  Else replace the file & line
 * fields of the matching record, move it to end of the
 * buffer, & return it.
 */
macro CW_addword(word, fname, lno, hBuf, bReplace)
{
    /* SearchInBuf (hbuf, pattern, lnStart, ichStart,
                    fMatchCase, fRegExpr, fWholeWordsOnly)
    */
	foundit = SearchInBuf(hBuf, ";match=\"@word@\"", 1, 0, 1, 0, 0)
	record = ""
	if (foundit == "") {
		record = "file=\"@fname@\";line=\"@lno@\";match=\"@word@\""
	}
	else if (bReplace) {
		record = GetBufLine(hBuf, foundit.lnFirst)
		record.file = fname
		record.line = lno
		DelBufLine(hBuf, foundit.lnFirst)
	}
	if (record != "") {
		AppendBufLine(hBuf, record)
	}
	return record
}

/* Search in hSourceBuf for unique full-word matches to regexp,
 * up through line lastlno, adding match records to hResultBuf.
 * In the end, the match recrods look "as if" we had really
 * searched backward from lastlno, which the closest preceding
 * matches earliest in the list.
 */
macro CW_addallbackwards(regexp, hSourceBuf, hResultBuf, lastlno)
{
	lno = 0
	ich = 0	
	fname = GetBufName(hSourceBuf)
	while (1) {
	    /* SearchInBuf(hbuf, pattern, lnStart, ichStart,
	                   fMatchCase, fRegExpr, fWholeWordsOnly)
	    */
		foundit = SearchInBuf(hSourceBuf, regexp, lno, ich, 1, 1, 1)
		if (foundit == "") {
			break
		}
		lno = foundit.lnFirst
		if (lno > lastlno) {
			break
		}
		ich = foundit.ichLim
		matchline = GetBufLine(hSourceBuf, lno)
		match = strmid(matchline, foundit.ichFirst, ich)
		/* We're forced to search forward, but want the last match
		 * (closest preceding the target), so tell CW_addword to
		 * replace any previous match.
		 */
		CW_addword(match, fname, lno, hResultBuf, 1)
	}
	/* reverse the match order */
	n = GetBufLineCount(hResultBuf) - 1
	i = 1
	while (i < n) {
		r1 = GetBufLine(hResultBuf, i)
		r2 = GetBufLine(hResultBuf, n)
		PutBufLine(hResultBuf, i, r2)
		PutBufLine(hResultBuf, n, r1)
		i = i + 1
		n = n - 1
	}
}

/* The major complication here is that this is essentially an asynch
 * event-driven process:  we don't know what the user has done
 * between invocations, so have to squirrel away and check a lot
 * of state in order to guess whether they're invoking the
 * CompleteWord macros repeatedly.
 */
macro CW_guts(bForward)
{
	hwnd = GetCurrentWnd()
	selection = GetWndSel(hwnd)
	if (selection.fExtended) {
		Msg("Cannot word-complete with active selection")
		stop
	}
	hbuf = GetCurrentBuf()
	hResultBuf = GetOrCreateBuf("*Completion*")

	/* Guess whether we're continuing an old one. */
	newone = 0
	if (GetBufLineCount(hResultBuf) == 0) {
		newone = 1
	}
	else {
		stat = GetBufLine(hResultBuf, 0)
		newone = stat.orighbuf != hbuf ||
				 stat.orighwnd != hwnd ||
				 stat.origlno != selection.lnFirst ||
		         stat.newj != selection.ichFirst
	}

	/* suck up stem word */
	if (newone) {
		j = selection.ichFirst	/* index of char to right of cursor */
	}
	else {
		j = stat.origj
	}
	line = GetBufLine(hbuf, selection.lnFirst)
	i = j - 1				/* index of char to left of cursor */
	while (i >= 0) {
		ch = line[i]
		if (isupper(ch) || islower(ch) || IsNumber(ch) || ch == "_") {
			i = i - 1
		}
		else {
			break
		}
	}
	i = i + 1
	if (i >= j) {
		Msg("Cursor must follow [a-zA-Z0-9_]")
		stop
	}
	/* BUG contra docs, line[j] is not included in the following */
	word = strmid(line, i, j)
	regexp = "@word@[a-zA-Z0-9_]+"


	/* BUG "||" apparently doesn't short-circuit, so
        	if (newone || word != stat.stem)
       doesn't work (if newone, stat isn't defined)
    */
    if (!newone) {
    	/* despite that everything looks the same, they
    	   may have changed the stem! */
    	newone = word != stat.stem
    }
    if (newone) {
		stat = ""
		stat.orighbuf = hbuf
		stat.orighwnd = hwnd
		stat.origlno  = selection.lnFirst
		stat.origi    = i
		stat.origj    = j
		stat.stem     = word
		stat.newj     = j
		stat.index    = 0
		stat.searchwnd = hwnd
		stat.searchlno = selection.lnFirst
		stat.searchich = j
		ClearBuf(hResultBuf)
		AppendBufLine(hResultBuf, stat)
		CW_addallbackwards(regexp, hbuf, hResultBuf, stat.origlno)
		if (GetBufLineCount(hResultBuf) >= 2) {
			/* found at least one completion in this buffer,
			   so display the first */
			CW_completeindex(hResultBuf, 1)
			return
		}
	}

	/* continuing an old one, or a new one w/o backward match */
	n = GetBufLineCount(hResultBuf)
	i = stat.index
	if (!bForward) {
		if (i > 1) {
			CW_completeindex(hResultBuf, i - 1)
		}
		else {
			CW_completeword(hResultBuf, word, 0)
			Msg("move forward for completions")
		}
		return
	}

	/* moving forward */
	if (i < n-1) {
		CW_completeindex(hResultBuf, i + 1)
		return
	}

	if (i == n) {
		Msg("move back for completions")
		return
	}

	/* i == n-1: we're at the last one; look for another completion */
	while (1) {
		stat = GetBufLine(hResultBuf, 0)
		hwnd = stat.searchwnd
		lno	= stat.searchlno
		ich = stat.searchich
		hbuf = GetWndBuf(hwnd)
	    /* SearchInBuf(hbuf, pattern, lnStart, ichStart,
	                   fMatchCase, fRegExpr, fWholeWordsOnly)
	    */
	    if (hBuf == hResultBuf) {
	    	/* no point searching our own result list! */
	    	foundit = ""
	    }
	    else {
			foundit = SearchInBuf(hbuf, regexp, lno, ich, 1, 1, 1)
		}
		if (foundit == "") {
			hwnd = GetNextWnd(hwnd)
			if (hwnd == 0 || hwnd == stat.orighwnd) {
				n = GetBufLineCount(hResultBuf)
				if (n == 1) {
					Msg("No completions for @word@")
				}
				else {
					CW_completeword(hResultBuf, word, n)
					Msg("No more completions for @word@")
				}
				break
			}
			stat.searchwnd = hwnd
			stat.searchlno = 0
			stat.searchich = 0
			PutBufLine(hResultBuf, 0, stat)
			continue
		}
		lno = foundit.lnFirst
		ich = foundit.ichLim
		stat.searchlno = lno
		stat.searchich = ich
		PutBufLine(hResultBuf, 0, stat)
		matchline = GetBufLine(hbuf, lno)
		match = strmid(matchline, foundit.ichFirst, ich)
		result = CW_addword(match, GetBufName(hbuf), lno, hResultBuf, 0)
		if (result != "") {
			CW_completeindex(hResultBuf, GetBufLineCount(hResultBuf) - 1)
			break
		}
	}
}

/* Replace the stem with the completion at index i */
macro CW_completeindex(hBuf, i)
{
	record = GetBufLine(hBuf, i)
	CW_completeword(hBuf, record.match, i)
}

/* Replace the stem with the given completion */
macro CW_completeword(hBuf, completion, i)
{
	stat = GetBufLine(hBuf, 0)
	targetBuf = stat.orighbuf
	oldline = GetBufLine(targetBuf, stat.origlno)
	newline = cat(strmid(oldline, 0, stat.origi), completion)
	newj = strlen(newline)
	newline = cat(newline, strmid(oldline, stat.newj, strlen(oldline)))
	PutBufLine(targetBuf, stat.origlno, newline)
	SetBufIns(targetBuf, stat.origlno, newj)
	stat.newj = newj
	stat.index = i
	PutBufLine(hBuf, 0, stat)
}

/* Get handle of buffer with name "name", or create a new one
 * if no such buffer exists.
 */
macro GetOrCreateBuf(name)
{
	hBuf = GetBufHandle(name)
	if (hBuf == 0) {
		hBuf = NewBuf(name)
	}
	return hBuf
}







/*   A U T O   E X P A N D   */
/*-------------------------------------------------------------------------
    Automatically expands C statements like if, for, while, switch, etc..

    To use this macro, 
    	1. Add this file to your project or your Base project.
		
		2. Run the Options->Key Assignments command and assign a 
		convenient keystroke to the "AutoExpand" command.
		
		3. After typing a keyword, press the AutoExpand keystroke to have the
		statement expanded.  The expanded statement will contain a ### string
		which represents a field where you are supposed to type more.
		
		The ### string is also loaded in to the search pattern so you can 
		use "Search Forward" to select the next ### field.

	For example:
		1. you type "for" + AutoExpand key
		2. this is inserted:
			for (###; ###; ###)
				{
				###
				}
		3. and the first ### field is selected.
-------------------------------------------------------------------------*/
macro AutoExpand()
{
	// get window, sel, and buffer handles
	hwnd = GetCurrentWnd()
	if (hwnd == 0)
		stop
	sel = GetWndSel(hwnd)
	if (sel.ichFirst == 0)
		stop
	hbuf = GetWndBuf(hwnd)
	
	// get line the selection (insertion point) is on
	szLine = GetBufLine(hbuf, sel.lnFirst);
	
	// parse word just to the left of the insertion point
	wordinfo = GetWordLeftOfIch(sel.ichFirst, szLine)
	ln = sel.lnFirst;
	
	chTab = CharFromAscii(9)
	
	// prepare a new indented blank line to be inserted.
	// keep white space on left and add a tab to indent.
	// this preserves the indentation level.
	ich = 0
	while (szLine[ich] == ' ' || szLine[ich] == chTab)
		{
		ich = ich + 1
		}
	
	szLine = strmid(szLine, 0, ich) # chTab
	sel.lnFirst = sel.lnLast
	sel.ichFirst = wordinfo.ich
	sel.ichLim = wordinfo.ich
	
	// expand szWord keyword...

	
	if (wordinfo.szWord == "if" || 
		wordinfo.szWord == "while" ||
		wordinfo.szWord == "elseif")
		{
		SetBufSelText(hbuf, " (###)")
		InsBufLine(hbuf, ln + 1, "@szLine@" # "{");
		InsBufLine(hbuf, ln + 2, "@szLine@" # "###");
		InsBufLine(hbuf, ln + 3, "@szLine@" # "}");
		}
	else if (wordinfo.szWord == "for")
		{
		SetBufSelText(hbuf, " (###; ###; ###)")
		InsBufLine(hbuf, ln + 1, "@szLine@" # "{");
		InsBufLine(hbuf, ln + 2, "@szLine@" # "###");
		InsBufLine(hbuf, ln + 3, "@szLine@" # "}");
		}
	else if (wordinfo.szWord == "switch")
		{
		SetBufSelText(hbuf, " (###)")
		InsBufLine(hbuf, ln + 1, "@szLine@" # "{")
		InsBufLine(hbuf, ln + 2, "@szLine@" # "case ###:")
		InsBufLine(hbuf, ln + 3, "@szLine@" # chTab # "###")
		InsBufLine(hbuf, ln + 4, "@szLine@" # chTab # "break;")
		InsBufLine(hbuf, ln + 5, "@szLine@" # "}")
		}
	else if (wordinfo.szWord == "do")
		{
		InsBufLine(hbuf, ln + 1, "@szLine@" # "{")
		InsBufLine(hbuf, ln + 2, "@szLine@" # "###");
		InsBufLine(hbuf, ln + 3, "@szLine@" # "} while (###);")
		}
	else if (wordinfo.szWord == "case")
		{
		SetBufSelText(hbuf, " ###:")
		InsBufLine(hbuf, ln + 1, "@szLine@" # "###")
		InsBufLine(hbuf, ln + 2, "@szLine@" # "break;")
		}
	else
		stop

	SetWndSel(hwnd, sel)
	LoadSearchPattern("###", true, false, false);
	Search_Forward
}


/*   G E T   W O R D   L E F T   O F   I C H   */
/*-------------------------------------------------------------------------
    Given an index to a character (ich) and a string (sz),
    return a "wordinfo" record variable that describes the 
    text word just to the left of the ich.

    Output:
    	wordinfo.szWord = the word string
    	wordinfo.ich = the first ich of the word
    	wordinfo.ichLim = the limit ich of the word
-------------------------------------------------------------------------*/
macro GetWordLeftOfIch(ich, sz)
{
	wordinfo = "" // create a "wordinfo" structure
	
	chTab = CharFromAscii(9)
	
	// scan backwords over white space, if any
	ich = ich - 1;
	if (ich >= 0)
		while (sz[ich] == " " || sz[ich] == chTab)
			{
			ich = ich - 1;
			if (ich < 0)
				break;
			}
	
	// scan backwords to start of word	
	ichLim = ich + 1;
	asciiA = AsciiFromChar("A")
	asciiZ = AsciiFromChar("Z")
	while (ich >= 0)
		{
		ch = toupper(sz[ich])
		asciiCh = AsciiFromChar(ch)
		if ((asciiCh < asciiA || asciiCh > asciiZ) && !IsNumber(ch))
			break // stop at first non-identifier character
		ich = ich - 1;
		}
	
	ich = ich + 1
	wordinfo.szWord = strmid(sz, ich, ichLim)
	wordinfo.ich = ich
	wordinfo.ichLim = ichLim;
	
	return wordinfo
}



macro PrintSelection()
{
	hbufCur = GetCurrentBuf()
	filename = GetBufName(hbufCur)
	Copy
	hbufTemp = NewBuf("Output: @filename@")
	SetCurrentBuf(hbufTemp)
	Paste
	Print()
	CloseBuf(hbufTemp)
}



/***************************************************************************************
****************************************************************************************
* FILE		: SourceInsight_Comment.em
* Description	: utility to insert comment in Source Insight project
*			  
* Copyright (c) 2007 by Liu Ying. All Rights Reserved.
* 
* History:
* Version		Name       	Date			Description
   0.1		Liu Ying		2006/04/07	Initial Version
   0.2		Liu Ying		2006/04/21	add Ly_InsertHFileBanner
****************************************************************************************
****************************************************************************************/


/*==================================================================
* Function	: InsertFileHeader
* Description	: insert file header
*			  
* Input Para	: none
			  
* Output Para	: none
			  
* Return Value: none
==================================================================*/
macro Ly_InsertFileHeader()
{
	// get aurthor name
	szMyName = getenv(MYNAME)
	if (strlen(szMyName) <= 0)
	{
		szMyName = "www.yiview.com"
	}

	// get company name
	szCompanyName = getenv(MYCOMPANY)
	if (strlen(szCompanyName) <= 0)
	{
		szCompanyName = szMyName
	}

	// get time
	szTime = GetSysTime(True)
	Day = szTime.Day
	Month = szTime.Month
	Year = szTime.Year
	if (Day < 10)
	{
		szDay = "0@Day@"
	}
	else
	{
		szDay = Day
	}
	if (Month < 10)
	{
		szMonth = "0@Month@"
	}
	else
	{
		szMonth = Month
	}

	// get file name
	hbuf = GetCurrentBuf()
	szpathName = GetBufName(hbuf)
	szfileName = GetFileName(szpathName)
	nlength = StrLen(szfileName)

	// assemble the string
	hbuf = GetCurrentBuf()
	InsBufLine(hbuf, 0, "")
	InsBufLine(hbuf, 1, "/***************************************************************************************")
	InsBufLine(hbuf, 2, "****************************************************************************************")
	InsBufLine(hbuf, 3, "* FILE		: @szfileName@")
	InsBufLine(hbuf, 4, "* Description	: ")
	InsBufLine(hbuf, 5, "*			  ")
	InsBufLine(hbuf, 6, "* Copyright (c) @Year@ by @szCompanyName@. All Rights Reserved.")
	InsBufLine(hbuf, 7, "* ")
	InsBufLine(hbuf, 8, "* History:")
	InsBufLine(hbuf, 9, "* Version				Name       					Date						Description")
	InsBufLine(hbuf, 10, "   0.1				@szMyName@					@Year@/@szMonth@/@szDay@	Initial Version")
	InsBufLine(hbuf, 11, "   ")
	InsBufLine(hbuf, 12, "****************************************************************************************")
	InsBufLine(hbuf, 13, "****************************************************************************************/")
	InsBufLine(hbuf, 14, "")
	InsBufLine(hbuf, 15, "")

	// put the insertion point
	SetBufIns(hbuf, 16, 0)
}


/*==================================================================
* Function	: InsertFileHeader
* Description	: insert file header
*			  
* Input Para	: none
			  
* Output Para	: none
			  
* Return Value: none
==================================================================*/
macro Ly_InsertFunctionHeader()
{
	// get function name
	hbuf = GetCurrentBuf()
	szFunc = GetCurSymbol()
	ln = GetSymbolLine(szFunc)

	// assemble the string
	hbuf = GetCurrentBuf()
	//InsBufLine(hbuf, ln, "")
	InsBufLine(hbuf, ln+1, "/*==================================================================")
	InsBufLine(hbuf, ln+2, "* Function	: @szFunc@")
	InsBufLine(hbuf, ln+3, "* Description	: ")
	InsBufLine(hbuf, ln+4, "* Input Para	: ")
	InsBufLine(hbuf, ln+5, "* Output Para	: ")
	InsBufLine(hbuf, ln+6, "* Return Value: ")
	InsBufLine(hbuf, ln+7, "==================================================================*/")

	// put the insertion point
	SetBufIns(hbuf, ln+8, 0)
}


/*==================================================================
* Function	: InsertFileHeader
* Description	: insert file header
*			  
* Input Para	: none
			  
* Output Para	: none
			  
* Return Value: none
==================================================================*/
macro Ly_InsertHFileBanner()
{
	// get file name
	hbuf = GetCurrentBuf()
	szpathName = GetBufName(hbuf)
	szfileName = GetFileName(szpathName)
	szfileName = toupper(szfileName)

	// create banner
	banner = "_"
	nlength = strlen(szfileName)
	
	i=0
	while (i < nlength)
	{
		if (szfileName[i] == ".")
		{
			banner = cat(banner, "_")
		}
		else
		{
			banner = cat(banner, szfileName[i])
		}

		i = i+1
	}

	banner = cat(banner, "_")

	// print banner
	hwnd = GetCurrentWnd()
	lnFirst = GetWndSelLnFirst(hwnd)
	lnLast = GetWndSelLnLast(hwnd)
	 
	hbuf = GetCurrentBuf()
	InsBufLine(hbuf, lnFirst, "#ifndef @banner@")
	InsBufLine(hbuf, lnFirst+1, "#define @banner@")
	InsBufLine(hbuf, lnFirst+2, "")
	InsBufLine(hbuf, lnFirst+3, "")
	InsBufLine(hbuf, lnFirst+4, "")
	InsBufLine(hbuf, lnFirst+5, "")
	InsBufLine(hbuf, lnFirst+6, "")
	InsBufLine(hbuf, lnLast+7, "#endif /*@banner@*/")

	SetBufIns(hbuf, lnFirst+4, 0)
}

/*==================================================================
* Function	: GetFileName
* Description	: get file name from path
*			  
* Input Para	: pathName	: path string
			  
* Output Para	: None
			  
* Return Value: name		: file name
==================================================================*/
macro GetFileName(pathName)
{
	nlength = strlen(pathName)
	i = nlength - 1
	name = ""
	while (i + 1)
	{
		ch = pathName[i]
		if ("\\" == "@ch@")
			break
		i = i - 1
	}
	i = i + 1
	while (i < nlength)
	{
		name = cat(name, pathName[i])
		i = i + 1
	}

	return name
}


macro _tsGetTabSize()  
{  
    szTabSize = GetReg("TabSize");  
  
    if (szTabSize != "")  
    {  
        tabSize = AsciiFromChar(szTabSize[0]) - AsciiFromChar("0");  
    }  
    else  
    {  
        tabSize = 4;  
    }  
  
    return tabSize;  
}  
  
  
macro CommentBlock_Joyce()  
{  
    hbuf = GetCurrentBuf();  
    hwnd = GetCurrentWnd();  
  
    sel = GetWndSel(hwnd);  
  
    iLine = sel.lnFirst;  
      
    // indicate the comment char according to the file type  
    // for example, using "#" for perl file(.pl) and "/* */" for C/C++.  
    filename = tolower(GetBufName(hbuf));  
    suffix = "";  
    len = strlen(filename);  
    i = len - 1;  
    while (i >= 0)  
    {  
        if (filename[i-1] == ".")  
        {  
            suffix = strmid(filename, i, len)  
            break;  
        }  
        i = i -1;  
    }  
    if  ( suffix == "pl" )  
    {  
        filetype = 2; // PERL  
    }  
    else  
    {  
        filetype = 1; // C  
    }  
  
    szLine = GetBufLine(hbuf, iLine);  
    if (filetype == 1)  // C  
    {  
        szLine = cat("/*    ", szLine);  
    }  
    else                // PERL  
    {  
        szLine = cat("# ", szLine);  
    }  
    PutBufLine(hbuf, iLine, szLine);  
    iLine = sel.lnLast;  
    szLine = GetBufLine(hbuf, iLine);  
    if (filetype == 1)  // C  
    {  
        szLine = cat(szLine, "*/    ");  
    }  
    else                // PERL  
    {  
        szLine = cat("# ", szLine);  
    }  
    PutBufLine(hbuf, iLine, szLine);  
  
  
  
    if (sel.lnFirst == sel.lnLast)  
    {  
        tabSize = _tsGetTabSize() - 1;  
        sel.ichFirst = sel.ichFirst + tabSize;  
        sel.ichLim = sel.ichLim + tabSize;  
    }  
    SetWndSel(hwnd, sel);  
}  
  
  
  
  
//  
// Undo the CommentBlock for the selected text.  
//  
macro UnCommentBlock_Joyce()  
{  
    hbuf = GetCurrentBuf();  
    hwnd = GetCurrentWnd();  
      
    sel = GetWndSel(hwnd);  
  
    iLine = sel.lnFirst;  
  
  
    // indicate the comment char according to the file type  
    // for example, using "#" for perl file(.pl) and "/* */" for C/C++.  
    filename = tolower(GetBufName(hbuf));  
    suffix = "";  
    len = strlen(filename);  
    i = len - 1;  
    while (i >= 0)  
    {  
        if (filename[i-1] == ".")  
        {  
            suffix = strmid(filename, i, len)  
            break;  
        }  
        i = i -1;  
    }  
    if  ( suffix == "pl" )  
    {  
        filetype = 2; // PERL  
    }  
    else  
    {  
        filetype = 1; // C  
    }  
  
    tabSize = 0;  
  
    endLine = GetBufLine(hbuf, sel.lnLast);  
    endLineLen = strlen(endLine);  
    szLine = GetBufLine(hbuf, iLine);  
    len = strlen(szLine);  
    szNewLine = "";  
    commentState = 1;  
  
    if (szLine[0] == "/" && szLine[1] == "*")  
    {  
        if(endLine[endLineLen-2] == "/" && endLine[endLineLen-3] == "*")  
        {  
            if (filetype == 1)  // C  
            {  
                if (len > 1)  
                {  
                    if (szLine[0] == "/" && szLine[1] == "*")  
                    {  
                        if (len > 2)  
                        {  
                            if (AsciiFromChar(szLine[2]) == 9)  
                            {  
                                tabSize = _tsGetTabSize() - 1;  
                                szNewLine = strmid(szLine, 3, strlen(szLine));  
                            }  
                        }  
  
                        if (szNewLine == "")  
                        {  
                            szNewLine = strmid(szLine, 2, strlen(szLine));  
                            tabSize = 2;  
                        }  
                          
                        PutBufLine(hbuf, iLine, szNewLine);  
                    }  
                }  
            }  
            if (filetype == 2)  // PERL  
            {  
                if (len > 0)  
                {  
                    if (szLine[0] == "#")     
                    {  
                        if (len > 1)  
                        {  
                            if (AsciiFromChar(szLine[1]) == 9)  
                            {  
                                tabSize = _tsGetTabSize() - 1;  
                                szNewLine = strmid(szLine, 2, strlen(szLine));  
                            }  
                        }  
  
                        if (szNewLine == "")  
                        {  
                            szNewLine = strmid(szLine, 1, strlen(szLine));  
                            tabSize = 2;  
                        }  
                          
                        PutBufLine(hbuf, iLine, szNewLine);  
                    }  
                }  
            }  
  
            iLine = sel.lnLast;  
            szLine = GetBufLine(hbuf, iLine);  
            len = strlen(szLine);  
            szNewLine = "";  
            if (filetype == 1)  // C  
            {  
                if (len > 1)  
                {  
                    if (szLine[strlen(szLine)-2] == "/" && szLine[strlen(szLine)-3] == "*")  
                    {  
                        if (len > 2)  
                        {  
                            if (AsciiFromChar(szLine[2]) == 9)  
                            {  
                                tabSize = _tsGetTabSize() - 1;  
                                szNewLine = strmid(szLine, 0, strlen(szLine)-2);  
                            }  
                        }  
  
                        if (szNewLine == "")  
                        {  
                            szNewLine = strmid(szLine, 0, strlen(szLine)-3);  
                            tabSize = 2;  
                        }  
                          
                        PutBufLine(hbuf, iLine, szNewLine);  
                    }  
                }  
            }  
            if (filetype == 2)  // PERL  
            {  
                if (len > 0)  
                {  
                    if (szLine[0] == "#")     
                    {  
                        if (len > 1)  
                        {  
                            if (AsciiFromChar(szLine[1]) == 9)  
                            {  
                                tabSize = _tsGetTabSize() - 1;  
                                szNewLine = strmid(szLine, 2, strlen(szLine));  
                            }  
                        }  
  
                        if (szNewLine == "")  
                        {  
                            szNewLine = strmid(szLine, 1, strlen(szLine));  
                            tabSize = 2;  
                        }  
                          
                        PutBufLine(hbuf, iLine, szNewLine);  
                    }  
                }  
            }  
        }  
  
    }  
      
  
    if (sel.lnFirst == sel.lnLast)  
    {  
        sel.ichFirst = sel.ichFirst - tabSize;  
        sel.ichLim = sel.ichLim - tabSize;  
    }  
  
    SetWndSel(hwnd, sel);  
}  

