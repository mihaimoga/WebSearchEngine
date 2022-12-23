/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

#pragma once

class CHtmlToText
{
public:
	CHtmlToText();
	~CHtmlToText();

public:
	const std::string& Convert(const std::string& html);
	std::string ParseTag(bool& selfClosing);
	void EatInnerContent(const std::string& tag);

	bool EndOfText() { return (_pos >= _html.length()); };

	char Peek() { return (_pos < _html.length()) ? _html[_pos] : (char)0; }

	void MoveAhead() { _pos = ((_pos + 1 < _html.length()) ? (_pos + 1) : _html.length()); }

	bool IsWhiteSpace(char ch)
	{
		if ((ch == _T(' ')) || (ch == _T('\t')) || (ch == _T('\r')) || (ch == _T('\n')))
			return true;
		return false;
	}

	void EatWhitespace()
	{
		while (IsWhiteSpace(Peek()))
			MoveAhead();
	}

	void EatWhitespaceToNextLine()
	{
		while (IsWhiteSpace(Peek()))
		{
			char ch = Peek();
			MoveAhead();
			if (ch == _T('\n'))
				break;
		}
	}

	void EatQuotedValue()
	{
		char mark = Peek();
		if ((mark == _T('\"')) || (mark == _T('\'')))
		{
			// Opening quote
			MoveAhead();
			// Find end of value
			while (!EndOfText())
			{
				char ch = Peek();
				MoveAhead();
				if ((ch == mark) || (ch == _T('\r')) || (ch == _T('\n')))
					break;
			}
		}
	}

protected:
	std::string _text;
	std::string _html;
	size_t _pos;
	bool _preformatted;

	CMapStringToString _tags;
	CStringList _ignoreTags;
};
