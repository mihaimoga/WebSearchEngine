/* Copyright (C) 2022-2026 Stefan-Mihai MOGA
This file is part of WebSearchEngine application developed by Stefan-Mihai MOGA.

WebSearchEngine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

WebSearchEngine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
WebSearchEngine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

#include "stdafx.h"
#include "HtmlToText.h"
#include <algorithm>
#include <iterator>

namespace
{
	/**
	 * @brief White space that is written before the next piece of visible text.
	 *
	 * A stronger separator replaces a weaker one, so a pile of block tags never produces more than
	 * one blank line, and nothing is written when no text follows.
	 */
	enum class Separator : unsigned char
	{
		None,		/**< Nothing. */
		Space,		/**< A space. */
		Tab,		/**< A tab, between table cells. */
		Line,		/**< A line break. */
		Paragraph	/**< A blank line. */
	};

	/** @brief How the content of an element is treated. */
	enum class Content : unsigned char
	{
		Flow,			/**< Regular content. */
		Preformatted,	/**< White space is significant. */
		RawText,		/**< Text that is never displayed, up to the matching end tag. */
		Subtree,		/**< Markup that is never displayed; nested elements of the same name are balanced and the raw text elements inside are skipped as a whole. */
		Foreign			/**< Like Subtree, but a self-closing start tag has no content, and the content is regular markup that may have CDATA sections. */
	};

	/** @brief What is known about an element. Elements that are not listed are inline. */
	struct TagInfo
	{
		std::string_view name;	/**< Lower case name of the element. */
		Separator separator;	/**< Separator written for the start tag and for the end tag. */
		Content content;		/**< Treatment of the content. */
	};

	/** @brief Longest element name that can be in the table. */
	constexpr size_t kMaxNameLength = 12;

	/** @brief The elements that influence the text, sorted by name. */
	constexpr TagInfo kTags[] =
	{
		{ "address", Separator::Paragraph, Content::Flow },
		{ "applet", Separator::Space, Content::Subtree },
		{ "article", Separator::Line, Content::Flow },
		{ "aside", Separator::Line, Content::Flow },
		{ "audio", Separator::Space, Content::Subtree },
		{ "blockquote", Separator::Paragraph, Content::Flow },
		{ "body", Separator::Line, Content::Flow },
		{ "br", Separator::Line, Content::Flow },
		{ "button", Separator::Space, Content::Flow },
		{ "canvas", Separator::Space, Content::Subtree },
		{ "caption", Separator::Line, Content::Flow },
		{ "center", Separator::Line, Content::Flow },
		{ "datalist", Separator::None, Content::Subtree },
		{ "dd", Separator::Line, Content::Flow },
		{ "details", Separator::Line, Content::Flow },
		{ "dialog", Separator::Line, Content::Flow },
		{ "dir", Separator::Line, Content::Flow },
		{ "div", Separator::Line, Content::Flow },
		{ "dl", Separator::Line, Content::Flow },
		{ "dt", Separator::Line, Content::Flow },
		{ "embed", Separator::Space, Content::Flow },
		{ "fieldset", Separator::Line, Content::Flow },
		{ "figcaption", Separator::Line, Content::Flow },
		{ "figure", Separator::Paragraph, Content::Flow },
		{ "footer", Separator::Line, Content::Flow },
		{ "form", Separator::Line, Content::Flow },
		{ "h1", Separator::Paragraph, Content::Flow },
		{ "h2", Separator::Paragraph, Content::Flow },
		{ "h3", Separator::Paragraph, Content::Flow },
		{ "h4", Separator::Paragraph, Content::Flow },
		{ "h5", Separator::Paragraph, Content::Flow },
		{ "h6", Separator::Paragraph, Content::Flow },
		{ "head", Separator::Line, Content::Flow },
		{ "header", Separator::Line, Content::Flow },
		{ "hgroup", Separator::Line, Content::Flow },
		{ "hr", Separator::Paragraph, Content::Flow },
		{ "html", Separator::Line, Content::Flow },
		{ "iframe", Separator::Space, Content::RawText },
		{ "img", Separator::Space, Content::Flow },
		{ "input", Separator::Space, Content::Flow },
		{ "legend", Separator::Line, Content::Flow },
		{ "li", Separator::Line, Content::Flow },
		{ "main", Separator::Line, Content::Flow },
		{ "math", Separator::Space, Content::Foreign },
		{ "menu", Separator::Line, Content::Flow },
		{ "nav", Separator::Line, Content::Flow },
		{ "noembed", Separator::None, Content::RawText },
		{ "noframes", Separator::None, Content::RawText },
		{ "noscript", Separator::None, Content::RawText },
		{ "object", Separator::Space, Content::Subtree },
		{ "ol", Separator::Line, Content::Flow },
		{ "optgroup", Separator::Line, Content::Flow },
		{ "option", Separator::Line, Content::Flow },
		{ "p", Separator::Paragraph, Content::Flow },
		{ "pre", Separator::Paragraph, Content::Preformatted },
		{ "script", Separator::None, Content::RawText },
		{ "search", Separator::Line, Content::Flow },
		{ "section", Separator::Line, Content::Flow },
		{ "select", Separator::Space, Content::Flow },
		{ "style", Separator::None, Content::RawText },
		{ "summary", Separator::Line, Content::Flow },
		{ "svg", Separator::Space, Content::Foreign },
		{ "table", Separator::Paragraph, Content::Flow },
		{ "tbody", Separator::Line, Content::Flow },
		{ "td", Separator::Tab, Content::Flow },
		{ "template", Separator::None, Content::Subtree },
		{ "textarea", Separator::Space, Content::RawText },
		{ "tfoot", Separator::Line, Content::Flow },
		{ "th", Separator::Tab, Content::Flow },
		{ "thead", Separator::Line, Content::Flow },
		{ "title", Separator::None, Content::RawText },
		{ "tr", Separator::Line, Content::Flow },
		{ "ul", Separator::Line, Content::Flow },
		{ "video", Separator::Space, Content::Subtree }
	};

	constexpr bool IsTableValid() noexcept
	{
		for (size_t index = 0; index < std::size(kTags); ++index)
		{
			if (kTags[index].name.empty() || kTags[index].name.size() > kMaxNameLength)
				return false;
			if (index > 0 && !(kTags[index - 1].name < kTags[index].name))
				return false;
		}
		return true;
	}

	static_assert(IsTableValid(), "kTags must be sorted by name and its names must fit in kMaxNameLength");

	/** @brief The UTF-8 encoding of U+FFFD, which replaces every malformed byte. */
	constexpr std::string_view kReplacement("\xEF\xBF\xBD", 3);

	constexpr bool IsHtmlSpace(char ch) noexcept
	{
		return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\f') || (ch == '\r');
	}

	constexpr bool IsAsciiAlpha(char ch) noexcept
	{
		return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z'));
	}

	constexpr char ToAsciiLower(char ch) noexcept
	{
		return ((ch >= 'A') && (ch <= 'Z')) ? static_cast<char>(ch - 'A' + 'a') : ch;
	}

	/** @brief Non-ASCII characters that separate words. */
	constexpr bool IsUnicodeSpace(char32_t cp) noexcept
	{
		return (cp == 0x0085) || (cp == 0x00A0) || (cp == 0x1680) || ((cp >= 0x2000) && (cp <= 0x200B)) ||
			(cp == 0x2028) || (cp == 0x2029) || (cp == 0x202F) || (cp == 0x205F) || (cp == 0x3000);
	}

	/**
	 * @brief Finds an element in the table.
	 * @param name The lower case name of the element.
	 * @return The description of the element, or nullptr when the element is inline.
	 */
	const TagInfo* FindTag(std::string_view name) noexcept
	{
		const TagInfo* const last = std::end(kTags);
		const TagInfo* const tag = std::lower_bound(std::begin(kTags), last, name,
			[](const TagInfo& info, std::string_view key) noexcept { return info.name < key; });
		return ((tag != last) && (tag->name == name)) ? tag : nullptr;
	}

	/**
	 * @brief Decodes the UTF-8 sequence that starts at the given position.
	 * @param text The text; the position must be inside it.
	 * @param at The position of the first byte.
	 * @param cp Receives the code point.
	 * @return The length of the sequence, or 0 when the bytes are not well-formed UTF-8: stray
	 * continuation byte, overlong form, surrogate, value above U+10FFFF or truncated sequence.
	 */
	size_t DecodeUtf8(std::string_view text, size_t at, char32_t& cp) noexcept
	{
		const auto lead = static_cast<unsigned char>(text[at]);
		size_t length = 0;
		unsigned char lower = 0x80;	// range of the second byte, the others are always 0x80..0xBF
		unsigned char upper = 0xBF;
		if ((lead >= 0xC2) && (lead <= 0xDF))
			length = 2;
		else if ((lead >= 0xE0) && (lead <= 0xEF))
		{
			length = 3;
			if (lead == 0xE0)
				lower = 0xA0;	// no overlong forms
			else if (lead == 0xED)
				upper = 0x9F;	// no surrogates
		}
		else if ((lead >= 0xF0) && (lead <= 0xF4))
		{
			length = 4;
			if (lead == 0xF0)
				lower = 0x90;	// no overlong forms
			else if (lead == 0xF4)
				upper = 0x8F;	// nothing above U+10FFFF
		}
		else
			return 0;

		if (text.size() - at < length)
			return 0;

		const auto second = static_cast<unsigned char>(text[at + 1]);
		if ((second < lower) || (second > upper))
			return 0;

		char32_t value = ((static_cast<char32_t>(lead) & (0xFFu >> (length + 1))) << 6) | (second & 0x3Fu);
		for (size_t index = 2; index < length; ++index)
		{
			const auto next = static_cast<unsigned char>(text[at + index]);
			if ((next & 0xC0) != 0x80)
				return 0;
			value = (value << 6) | (next & 0x3Fu);
		}
		cp = value;
		return length;
	}

	/**
	 * @brief A single conversion: tokenizes the document and writes the text.
	 *
	 * Every step consumes input and nothing is ever parsed twice, so the conversion takes linear time.
	 * There is no recursion either: the nesting of the elements that are skipped is only counted.
	 */
	class Extractor
	{
	public:
		/**
		 * @brief Prepares the conversion.
		 * @param html The document; it must outlive the object.
		 */
		explicit Extractor(std::string_view html) noexcept
			: _html(html)
		{
		}

		/**
		 * @brief Converts the whole document.
		 * @return The text of the document.
		 */
		std::string Run()
		{
			_text.reserve(_html.size() / 4);
			while (_pos < _html.size())
			{
				if (_html[_pos] == '<')
					ParseMarkup();
				else if (_hidden != nullptr)
					SkipToMarkup();
				else
					ParseText();
			}

			// Only preformatted text can end with white space
			while (!_text.empty() && ((_text.back() == ' ') || (_text.back() == '\t') || (_text.back() == '\n')))
				_text.pop_back();
			return std::move(_text);
		}

	private:
		/** @brief Writes the pending separator, except at the very beginning of the text. */
		void FlushSeparator()
		{
			const Separator separator = _pending;
			_pending = Separator::None;
			if (_text.empty())
				return;

			switch (separator)
			{
				case Separator::Space:
					_text.push_back(' ');
					break;
				case Separator::Tab:
					_text.push_back('\t');
					break;
				case Separator::Line:
					PadNewLines(1);
					break;
				case Separator::Paragraph:
					PadNewLines(2);
					break;
				default:
					break;
				}
		}

		/** @brief Makes sure that the text ends with at least the given number of line breaks. */
		void PadNewLines(size_t count)
		{
			size_t present = 0;
			while ((present < count) && (present < _text.size()) && (_text[_text.size() - 1 - present] == '\n'))
				++present;
			_text.append(count - present, '\n');
		}

		/** @brief Asks for a separator before the next visible text. */
		void Merge(Separator separator) noexcept
		{
			if (separator > _pending)
				_pending = separator;
		}

		/** @brief Writes visible text. */
		void Append(std::string_view text)
		{
			if (text.empty())
				return;
			FlushSeparator();
			_text.append(text);
		}

		/** @brief Writes a white space character of preformatted text. */
		void AppendPreformatted(char ch)
		{
			if (_text.empty())
				return;
			FlushSeparator();
			_text.push_back(ch);
		}

		/** @brief Handles a '<' that does not start any markup: it is plain text. */
		void LessThan()
		{
			if (_hidden == nullptr)
				Append("<");
			++_pos;
		}

		/**
		 * @brief Handles a character that is not copied as it is: white space, control character or
		 * Unicode space, byte order mark. The position is already behind the character.
		 */
		void Blank(char32_t cp)
		{
			if (cp == 0xFEFF)
				return;

			if (_preDepth == 0)
			{
				Merge(Separator::Space);
				return;
			}

			if (cp == '\r')
			{
				if ((_pos < _html.size()) && (_html[_pos] == '\n'))
					++_pos;
				cp = '\n';
			}
			AppendPreformatted(((cp == '\n') || (cp == '\t')) ? static_cast<char>(cp) : ' ');
		}

		/** @brief Copies the text up to the next tag. */
		void ParseText()
		{
			size_t run = _pos;	// start of the characters that are copied as they are
			while ((_pos < _html.size()) && (_html[_pos] != '<'))
			{
				const auto ch = static_cast<unsigned char>(_html[_pos]);
				if ((ch > 0x20) && (ch < 0x7F))
				{
					++_pos;
					continue;
				}

				char32_t cp = ch;
				size_t length = 1;
				if (ch >= 0x80)
					length = DecodeUtf8(_html, _pos, cp);	// 0 when malformed
				if ((length != 0) && (cp > 0x20) && (cp != 0x7F) && (cp != 0xFEFF) && !IsUnicodeSpace(cp))
				{
					_pos += length;
					continue;
				}

				Append(_html.substr(run, _pos - run));
				if (length == 0)
				{
					Append(kReplacement);
					++_pos;
				}
				else
				{
					_pos += length;
					Blank(cp);
				}
				run = _pos;
			}
			Append(_html.substr(run, _pos - run));
		}

		/** @brief Skips the text up to the next tag. */
		void SkipToMarkup() noexcept
		{
			const size_t next = _html.find('<', _pos);
			_pos = (next == std::string_view::npos) ? _html.size() : next;
		}

		/** @brief Handles whatever starts with a '<'. */
		void ParseMarkup()
		{
			const size_t next = _pos + 1;
			if (next >= _html.size())
			{
				LessThan();
				return;
			}

			const char ch = _html[next];
			if (IsAsciiAlpha(ch))
				ParseStartTag();
			else if (ch == '/')
				ParseEndTag();
			else if (ch == '!')
			{
				if (StartsWith(next + 1, "--"))
					SkipComment();
				else if (InForeignContent() && StartsWith(next + 1, "[CDATA["))
					SkipCData();
				else
					SkipUntilGreaterThan(next + 1);	// doctype or bogus comment, which includes a CDATA section in HTML content
			}
			else if (ch == '?')
				SkipUntilGreaterThan(next + 1);	// processing instruction or bogus comment
			else
				LessThan();
		}

		/** @brief Parses a start tag; the position is at its '<'. */
		void ParseStartTag()
		{
			const TagInfo* tag = nullptr;
			_pos = ReadTagName(_pos + 1, tag);
			bool selfClosing = false;
			if (SkipTagTail(selfClosing))
				StartTag(tag, selfClosing);
		}

		/** @brief Parses an end tag; the position is at its '<'. */
		void ParseEndTag()
		{
			const size_t next = _pos + 2;
			if (next >= _html.size())
			{
				LessThan();
				return;
			}

			const char ch = _html[next];
			if (ch == '>')
				_pos = next + 1;	// "</>" is ignored
			else if (!IsAsciiAlpha(ch))
				SkipUntilGreaterThan(next);	// bogus comment
			else
			{
				const TagInfo* tag = nullptr;
				_pos = ReadTagName(next, tag);
				bool selfClosing = false;
				if (SkipTagTail(selfClosing))
					EndTag(tag);
			}
		}

		/**
		 * @brief Reads the name of a tag.
		 * @param from The position of the first character of the name.
		 * @param tag Receives the description of the element, nullptr when the element is inline.
		 * @return The position behind the name.
		 */
		size_t ReadTagName(size_t from, const TagInfo*& tag) const noexcept
		{
			char name[kMaxNameLength] = {};
			size_t length = 0;
			size_t end = from;
			while ((end < _html.size()) && !IsHtmlSpace(_html[end]) && (_html[end] != '/') && (_html[end] != '>'))
			{
				if (length < kMaxNameLength)
					name[length] = ToAsciiLower(_html[end]);
				++length;
				++end;
			}
			tag = (length <= kMaxNameLength) ? FindTag(std::string_view(name, length)) : nullptr;
			return end;
		}

		/**
		 * @brief Skips the attributes of a tag and its closing '>'; the position is behind the tag name.
		 * @param selfClosing Receives whether the tag ends with "/>".
		 * @return false when the document ends inside the tag, which is then dropped with the rest of the document.
		 */
		bool SkipTagTail(bool& selfClosing) noexcept
		{
			selfClosing = false;
			const size_t size = _html.size();
			size_t at = _pos;
			while (at < size)
			{
				const char ch = _html[at];
				if (IsHtmlSpace(ch))
					++at;
				else if (ch == '>')
				{
					_pos = at + 1;
					return true;
				}
				else if (ch == '/')
				{
					++at;
					if ((at < size) && (_html[at] == '>'))
					{
						selfClosing = true;
						_pos = at + 1;
						return true;
					}
				}
				else
				{
					// An attribute; a leading '=' is part of its name
					++at;
					while ((at < size) && !IsHtmlSpace(_html[at]) && (_html[at] != '/') && (_html[at] != '>') && (_html[at] != '='))
						++at;
					while ((at < size) && IsHtmlSpace(_html[at]))
						++at;
					if ((at < size) && (_html[at] == '='))
					{
						++at;
						while ((at < size) && IsHtmlSpace(_html[at]))
							++at;
						if ((at < size) && ((_html[at] == '"') || (_html[at] == '\'')))
						{
							// A quoted value may contain anything, including '>' and line breaks
							const size_t close = _html.find(_html[at], at + 1);
							if (close == std::string_view::npos)
								break;
							at = close + 1;
						}
						else
						{
							while ((at < size) && !IsHtmlSpace(_html[at]) && (_html[at] != '>'))
								++at;
						}
					}
				}
			}
			_pos = size;
			return false;
		}

		/** @brief Skips a comment; the position is at its "<!--". An unterminated comment lasts until the end. */
		void SkipComment() noexcept
		{
			const size_t size = _html.size();
			size_t at = _pos + 4;
			if ((at < size) && (_html[at] == '>'))
			{
				_pos = at + 1;	// "<!-->"
				return;
			}
			if (StartsWith(at, "->"))
			{
				_pos = at + 2;	// "<!--->"
				return;
			}

			size_t dashes = _html.find("--", at);
			while (dashes != std::string_view::npos)
			{
				at = dashes + 2;
				while ((at < size) && (_html[at] == '-'))
					++at;
				if ((at < size) && (_html[at] == '>'))
				{
					_pos = at + 1;	// "-->"
					return;
				}
				if (StartsWith(at, "!>"))
				{
					_pos = at + 2;	// "--!>"
					return;
				}
				dashes = _html.find("--", at);
			}
			_pos = size;
		}

		/** @brief Skips a CDATA section; the position is at its "<![CDATA[". An unterminated section lasts until the end. */
		void SkipCData() noexcept
		{
			const size_t end = _html.find("]]>", _pos + 9);
			_pos = (end == std::string_view::npos) ? _html.size() : end + 3;
		}

		/** @brief Skips up to and including the next '>', or to the end of the document. */
		void SkipUntilGreaterThan(size_t from) noexcept
		{
			const size_t end = _html.find('>', from);
			_pos = (end == std::string_view::npos) ? _html.size() : end + 1;
		}

		/** @brief Skips the raw text of an element, up to and including its end tag. */
		void SkipRawText(std::string_view name) noexcept
		{
			size_t end = _html.find("</", _pos);
			while (end != std::string_view::npos)
			{
				if (IsEndTagOf(end, name))
				{
					bool selfClosing = false;
					_pos = end + 2 + name.size();
					SkipTagTail(selfClosing);
					return;
				}
				end = _html.find("</", end + 2);
			}
			_pos = _html.size();
		}

		/** @brief Tells whether the "</" at the given position starts the end tag of the element. */
		bool IsEndTagOf(size_t at, std::string_view name) const noexcept
		{
			const size_t first = at + 2;
			if (_html.size() - first < name.size())
				return false;
			for (size_t index = 0; index < name.size(); ++index)
			{
				if (ToAsciiLower(_html[first + index]) != name[index])
					return false;
			}

			const size_t after = first + name.size();
			return (after >= _html.size()) || IsHtmlSpace(_html[after]) || (_html[after] == '/') || (_html[after] == '>');
		}

		/** @brief Tells whether the document contains the prefix at the given position. */
		bool StartsWith(size_t at, std::string_view prefix) const noexcept
		{
			return (at <= _html.size()) && (_html.size() - at >= prefix.size()) && (_html.compare(at, prefix.size(), prefix) == 0);
		}

		/** @brief Drops the line break that directly follows a <pre> tag. */
		void SkipLeadingNewLine() noexcept
		{
			if ((_pos < _html.size()) && (_html[_pos] == '\r'))
			{
				++_pos;
				if ((_pos < _html.size()) && (_html[_pos] == '\n'))
					++_pos;
			}
			else if ((_pos < _html.size()) && (_html[_pos] == '\n'))
				++_pos;
		}

		/** @brief Tells whether the markup is inside an <svg> or <math> element, where CDATA sections exist and nothing is raw text. */
		bool InForeignContent() const noexcept
		{
			return (_hidden != nullptr) && (_hidden->content == Content::Foreign);
		}

		/** @brief Starts to skip the content of an element that is not displayed. */
		void Hide(const TagInfo* tag) noexcept
		{
			_hidden = tag;
			_hiddenDepth = 1;
		}

		/**
		 * @brief Handles a start tag.
		 * @param tag The description of the element, nullptr when the element is inline.
		 * @param selfClosing Whether the tag ends with "/>".
		 */
		void StartTag(const TagInfo* tag, bool selfClosing) noexcept
		{
			if (tag == nullptr)
				return;

			if (_hidden != nullptr)
			{
				// Only the tags that decide where the hidden content ends matter
				if (tag == _hidden)
				{
					if (!selfClosing || (tag->content != Content::Foreign))
						++_hiddenDepth;
				}
				else if ((tag->content == Content::RawText) && !InForeignContent())
					SkipRawText(tag->name);
				return;
			}

			Merge(tag->separator);
			switch (tag->content)
			{
			case Content::Preformatted:
				++_preDepth;
				SkipLeadingNewLine();
				break;
			case Content::RawText:
				SkipRawText(tag->name);
				break;
			case Content::Subtree:
				Hide(tag);
				break;
			case Content::Foreign:
				if (!selfClosing)
					Hide(tag);
				break;
			default:
				break;
			}
		}

		/**
		 * @brief Handles an end tag.
		 * @param tag The description of the element, nullptr when the element is inline.
		 */
		void EndTag(const TagInfo* tag) noexcept
		{
			if (tag == nullptr)
				return;

			if (_hidden != nullptr)
			{
				if ((tag == _hidden) && (--_hiddenDepth == 0))
				{
					_hidden = nullptr;
					Merge(tag->separator);
				}
				return;
			}

			Merge(tag->separator);
			if ((tag->content == Content::Preformatted) && (_preDepth > 0))
				--_preDepth;
		}

		std::string_view _html;	/**< The document. */
		size_t _pos = 0;	/**< Position of the next character to read. */
		std::string _text;	/**< The text written so far. */
		Separator _pending = Separator::None;	/**< Separator to write before the next visible text. */
		size_t _preDepth = 0;	/**< Number of open <pre> elements. */
		const TagInfo* _hidden = nullptr;	/**< The element whose content is skipped, if any. */
		size_t _hiddenDepth = 0;	/**< Nesting of that element, to find its matching end tag. */
	};
}

std::string CHtmlToText::Convert(std::string_view html) const
{
	return Extractor(html).Run();
}
