/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

#include "stdafx.h"
#include "HtmlToText.h"
#include <algorithm>

CHtmlToText::CHtmlToText()
{
	_tags.SetAt(_T("address"), _T("\n"));
	_tags.SetAt(_T("blockquote"), _T("\n"));
	_tags.SetAt(_T("div"), _T("\n"));
	_tags.SetAt(_T("dl"), _T("\n"));
	_tags.SetAt(_T("fieldset"), _T("\n"));
	_tags.SetAt(_T("form"), _T("\n"));
	_tags.SetAt(_T("h1"), _T("\n"));
	_tags.SetAt(_T("/h1"), _T("\n"));
	_tags.SetAt(_T("h2"), _T("\n"));
	_tags.SetAt(_T("/h2"), _T("\n"));
	_tags.SetAt(_T("h3"), _T("\n"));
	_tags.SetAt(_T("/h3"), _T("\n"));
	_tags.SetAt(_T("h4"), _T("\n"));
	_tags.SetAt(_T("/h4"), _T("\n"));
	_tags.SetAt(_T("h5"), _T("\n"));
	_tags.SetAt(_T("/h5"), _T("\n"));
	_tags.SetAt(_T("h6"), _T("\n"));
	_tags.SetAt(_T("/h6"), _T("\n"));
	_tags.SetAt(_T("p"), _T("\n"));
	_tags.SetAt(_T("/p"), _T("\n"));
	_tags.SetAt(_T("table"), _T("\n"));
	_tags.SetAt(_T("/table"), _T("\n"));
	_tags.SetAt(_T("ul"), _T("\n"));
	_tags.SetAt(_T("/ul"), _T("\n"));
	_tags.SetAt(_T("ol"), _T("\n"));
	_tags.SetAt(_T("/ol"), _T("\n"));
	_tags.SetAt(_T("/li"), _T("\n"));
	_tags.SetAt(_T("br"), _T("\n"));
	_tags.SetAt(_T("/td"), _T("\t"));
	_tags.SetAt(_T("/tr"), _T("\n"));
	_tags.SetAt(_T("/pre"), _T("\n"));

	_ignoreTags.AddTail(_T("script"));
	_ignoreTags.AddTail(_T("noscript"));
	_ignoreTags.AddTail(_T("style"));
	_ignoreTags.AddTail(_T("object"));
}

CHtmlToText::~CHtmlToText()
{
}

const std::string& CHtmlToText::Convert(const std::string& html)
{
	// Initialize state variables
	bool selfClosing = false;
	_html = html;
	_pos = 0;

	// Process input
	while (!EndOfText())
	{
		if (Peek() == '<')
		{
			// HTML tag
			std::string tag = ParseTag(selfClosing);

			// Handle special tag cases
			if (tag.compare("body") == 0)
			{
				// Discard content before <body>
				_text.empty();
			}
			else if (tag.compare("/body") == 0)
			{
				// Discard content after </body>
				_pos = _html.length();
			}
			else if (tag.compare("pre") == 0)
			{
				// Enter preformatted mode
				_preformatted = true;
				EatWhitespaceToNextLine();
			}
			else if (tag.compare("/pre") == 0)
			{
				// Exit preformatted mode
				_preformatted = false;
			}

			_text.append(" ");

			if (_ignoreTags.Find(CString(tag.c_str())) != NULL)
				EatInnerContent(tag);
		}
		else if (IsWhiteSpace(Peek()))
		{
			// Whitespace (treat all as space)
			_text += (_preformatted ? Peek() : ' ');
			MoveAhead();
		}
		else
		{
			// Other text
			_text += Peek();
			MoveAhead();
		}
	}

	return _text;
}

std::string CHtmlToText::ParseTag(bool& selfClosing)
{
	std::string tag;
	selfClosing = false;

	// Eat comments
	if (((_pos + 4) < _html.length()) &&
		(_html[_pos] == '<') &&
		(_html[_pos + 1] == '!') &&
		(_html[_pos + 2] == '-') &&
		(_html[_pos + 3] == '-'))
	{
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();

		while (!EndOfText())
		{
			if (((_pos + 3) < _html.length()) &&
				(_html[_pos] == '-') &&
				(_html[_pos + 1] == '-') &&
				(_html[_pos + 2] == '>'))
				break;

			MoveAhead();
		}

		MoveAhead();
		MoveAhead();
		MoveAhead();
		EatWhitespace();
	}

	// Eat scripts
	if (((_pos + 7) < _html.length()) &&
		(_html[_pos] == '<') &&
		(_html[_pos + 1] == 's') &&
		(_html[_pos + 2] == 'c') &&
		(_html[_pos + 3] == 'r') &&
		(_html[_pos + 4] == 'i') &&
		(_html[_pos + 5] == 'p') &&
		(_html[_pos + 6] == 't'))
	{
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();


		while (!EndOfText())
		{
			if (((_pos + 7) < _html.length()) &&
				(_html[_pos] == '/') &&
				(_html[_pos + 1] == 's') &&
				(_html[_pos + 2] == 'c') &&
				(_html[_pos + 3] == 'r') &&
				(_html[_pos + 4] == 'i') &&
				(_html[_pos + 5] == 'p') &&
				(_html[_pos + 6] == 't'))
				break;

			MoveAhead();
		}

		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();
		MoveAhead();
		EatWhitespace();
	}

	if (Peek() == _T('<'))
	{
		MoveAhead();

		// Parse tag name
		EatWhitespace();
		size_t start = _pos;
		if (Peek() == '/')
			MoveAhead();
		while (!EndOfText() && !IsWhiteSpace(Peek()) &&
			(Peek() != '/') && (Peek() != '>'))
			MoveAhead();
		tag = _html.substr(start, _pos - start);
		std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

		// Parse rest of tag
		while (!EndOfText() && (Peek() != '>'))
		{
			if ((Peek() == '\"') || (Peek() == '\''))
				EatQuotedValue();
			else
			{
				if (Peek() == '/')
					selfClosing = true;
				MoveAhead();
			}
		}

		MoveAhead();
	}
	return tag;
}

void CHtmlToText::EatInnerContent(const std::string& tag)
{
	bool selfClosing = false;
	const std::string endTag = "/" + tag;

	while (!EndOfText())
	{
		if (Peek() == '<')
		{
			// Consume a tag
			if (ParseTag(selfClosing).compare(endTag) == 0)
				return;
			// Use recursion to consume nested tags
			if (!selfClosing && (tag[0] != '/'))
				EatInnerContent(tag);
		}
		else
			MoveAhead();
	}
}
