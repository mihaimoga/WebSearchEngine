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

/**
 * @file UnquoteHTML.cpp
 * @brief Decodes the character references of an HTML text.
 *
 * The decoding follows the character reference state of the tokenizer of the HTML Living Standard
 * (https://html.spec.whatwg.org/multipage/parsing.html#character-reference-state), which is the
 * one that applies to the text of an element, and the table of the named character references of
 * the standard (see HtmlEntities.h).
 */

#include "stdafx.h"
#include "UnquoteHTML.h"
#include "HtmlEntities.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

namespace
{
	/** @brief The UTF-8 encoding of U+FFFD, which replaces every malformed byte. */
	constexpr std::string_view kReplacementBytes("\xEF\xBF\xBD", 3);

	/** @brief The character U+FFFD, which stands for the numbers that are not a character. */
	constexpr char32_t kReplacementCharacter = 0xFFFD;

	/** @brief The largest code point. */
	constexpr char32_t kMaxCodePoint = 0x10FFFF;

	/** @brief The lengths of the names in the table of the references. */
	struct NameLengths
	{
		size_t longest;			/**< The longest name; a name that is followed by a semicolon cannot be longer. */
		size_t longestLegacy;	/**< The longest name that is valid without a semicolon. */
		size_t shortestLegacy;	/**< The shortest name that is valid without a semicolon. */
	};

	/**
	 * @brief Measures the names in the table of the references.
	 * @return The lengths of the names.
	 */
	constexpr NameLengths MeasureNames() noexcept
	{
		NameLengths lengths = { 0, 0, 0 };
		for (const HtmlEntity& entity : kHtmlEntities)
		{
			const size_t size = entity.name.size();
			if (size > lengths.longest)
				lengths.longest = size;
			if (entity.legacy)
			{
				if (size > lengths.longestLegacy)
					lengths.longestLegacy = size;
				if ((lengths.shortestLegacy == 0) || (size < lengths.shortestLegacy))
					lengths.shortestLegacy = size;
			}
		}
		return lengths;
	}

	/** @brief The lengths of the names in kHtmlEntities. */
	constexpr NameLengths kNameLengths = MeasureNames();

	/**
	 * @brief Checks the table of the references: the search needs the names sorted, without duplicates.
	 * @return true when the table can be searched.
	 */
	constexpr bool IsTableValid() noexcept
	{
		for (size_t index = 0; index < std::size(kHtmlEntities); ++index)
		{
			const HtmlEntity& entity = kHtmlEntities[index];
			if (entity.name.empty() || (entity.first == 0))
				return false;
			if ((index > 0) && !(kHtmlEntities[index - 1].name < entity.name))
				return false;
		}
		return kNameLengths.shortestLegacy != 0;
	}

	static_assert(IsTableValid(), "kHtmlEntities must be sorted by name, without duplicates, and must have legacy names");

	/**
	 * @brief The characters the numeric references 128 to 159 stand for.
	 *
	 * Browsers do not decode them as the C1 control characters, which nobody means, but as the
	 * characters of Windows-1252, the code page the pages that use them were written in. The five
	 * numbers that Windows-1252 does not assign stay what they are.
	 */
	constexpr char16_t kWindows1252[32] =
	{
		0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
		0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
		0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178
	};

	/**
	 * @brief Tells whether a character is an ASCII letter or digit, which is what the names are made of.
	 * @param ch The character.
	 * @return true for an ASCII letter or digit.
	 */
	constexpr bool IsAsciiAlphanumeric(char ch) noexcept
	{
		return ((ch >= '0') && (ch <= '9')) || ((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z'));
	}

	/**
	 * @brief Tells whether a character is a non-ASCII space; these are the characters that
	 * CHtmlToText treats as white space (see HtmlToText.cpp).
	 * @param cp The code point.
	 * @return true when the character separates words.
	 */
	constexpr bool IsUnicodeSpace(char32_t cp) noexcept
	{
		return (cp == 0x0085) || (cp == 0x00A0) || (cp == 0x1680) || ((cp >= 0x2000) && (cp <= 0x200B)) ||
			(cp == 0x2028) || (cp == 0x2029) || (cp == 0x202F) || (cp == 0x205F) || (cp == 0x3000);
	}

	/**
	 * @brief Converts a hexadecimal digit.
	 * @param ch The character.
	 * @return The value of the digit, or 16 when the character is not a hexadecimal digit.
	 */
	constexpr unsigned DigitValue(char ch) noexcept
	{
		if ((ch >= '0') && (ch <= '9'))
			return static_cast<unsigned>(ch - '0');
		if ((ch >= 'A') && (ch <= 'F'))
			return static_cast<unsigned>(ch - 'A') + 10;
		if ((ch >= 'a') && (ch <= 'f'))
			return static_cast<unsigned>(ch - 'a') + 10;
		return 16;
	}

	/**
	 * @brief Measures the UTF-8 sequence that starts at the given position.
	 * @param text The text; the position must be inside it.
	 * @param at The position of the first byte.
	 * @return The length of the sequence, or 0 when the bytes are not well-formed UTF-8: stray
	 * continuation byte, overlong form, surrogate, value above U+10FFFF or truncated sequence.
	 */
	size_t Utf8Length(std::string_view text, size_t at) noexcept
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

		for (size_t index = 2; index < length; ++index)
		{
			if ((static_cast<unsigned char>(text[at + index]) & 0xC0) != 0x80)
				return 0;
		}
		return length;
	}

	/**
	 * @brief Appends a character, encoded in UTF-8; a space of any kind is appended as an ordinary space.
	 * @param text The text to append to.
	 * @param cp The code point, which must be a valid Unicode scalar value.
	 */
	void AppendCharacter(std::string& text, char32_t cp)
	{
		if (IsUnicodeSpace(cp))
			text += ' ';
		else if (cp < 0x80)
			text += static_cast<char>(cp);
		else if (cp < 0x800)
		{
			text += static_cast<char>(0xC0 | (cp >> 6));
			text += static_cast<char>(0x80 | (cp & 0x3F));
		}
		else if (cp < 0x10000)
		{
			text += static_cast<char>(0xE0 | (cp >> 12));
			text += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			text += static_cast<char>(0x80 | (cp & 0x3F));
		}
		else
		{
			text += static_cast<char>(0xF0 | (cp >> 18));
			text += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
			text += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
			text += static_cast<char>(0x80 | (cp & 0x3F));
		}
	}

	/**
	 * @brief Finds a reference in the table.
	 * @param name The name, without the semicolon.
	 * @return The reference, or nullptr when there is no such name.
	 */
	const HtmlEntity* FindEntity(std::string_view name) noexcept
	{
		const HtmlEntity* const last = std::end(kHtmlEntities);
		const HtmlEntity* const entity = std::lower_bound(std::begin(kHtmlEntities), last, name,
			[](const HtmlEntity& entry, std::string_view key) noexcept { return entry.name < key; });
		return ((entity != last) && (entity->name == name)) ? entity : nullptr;
	}

	/**
	 * @brief Turns the number of a numeric reference into the character it stands for.
	 * @param value The number; any value above kMaxCodePoint stands for all the larger ones.
	 * @return The character.
	 */
	constexpr char32_t ResolveNumeric(char32_t value) noexcept
	{
		if ((value == 0) || (value > kMaxCodePoint) || ((value >= 0xD800) && (value <= 0xDFFF)))
			return kReplacementCharacter;
		if ((value >= 0x80) && (value <= 0x9F))
			return kWindows1252[value - 0x80];
		return value;
	}

	/**
	 * @brief Decodes a numeric reference, decimal as in `&#169;` or hexadecimal as in `&#xA9;`.
	 *
	 * The semicolon is optional, as it is in the browsers, and the number can have any number of digits.
	 * @param text The text after the ampersand; it starts with the '#'.
	 * @param cp Receives the character the reference stands for.
	 * @return The number of characters of the reference after the ampersand, or 0 when the '#' is not
	 * followed by a digit and there is no reference.
	 */
	size_t ParseNumeric(std::string_view text, char32_t& cp) noexcept
	{
		size_t index = 1;
		unsigned base = 10;
		if ((index < text.size()) && ((text[index] == 'x') || (text[index] == 'X')))
		{
			base = 16;
			++index;
		}

		const size_t digits = index;
		char32_t value = 0;
		for (; index < text.size(); ++index)
		{
			const unsigned digit = DigitValue(text[index]);
			if (digit >= base)
				break;
			if (value <= kMaxCodePoint)	// the value stays out of the range, however many digits follow
				value = value * base + digit;
		}

		if (index == digits)
			return 0;

		if ((index < text.size()) && (text[index] == ';'))
			++index;

		cp = ResolveNumeric(value);
		return index;
	}

	/**
	 * @brief Finds a named reference, as in `&amp;`, `&copy` or `&hellip;`.
	 *
	 * The longest name wins. A name is valid when it is followed by a semicolon, or when it is one of
	 * the legacy names, which are valid without it too: `&amp;` and `&amp` are the same but `&hellip`
	 * is not a reference. Whatever follows a legacy name is just text, so `&notit;` is the not sign
	 * followed by `it;`.
	 * @param text The text after the ampersand; it starts with an ASCII letter or digit.
	 * @param consumed Receives the number of characters of the reference after the ampersand.
	 * @return The reference, or nullptr when there is none.
	 */
	const HtmlEntity* ParseNamed(std::string_view text, size_t& consumed) noexcept
	{
		size_t length = 0;
		const size_t limit = (std::min)(text.size(), kNameLengths.longest + 1);
		while ((length < limit) && IsAsciiAlphanumeric(text[length]))
			++length;

		if ((length <= kNameLengths.longest) && (length < text.size()) && (text[length] == ';'))
		{
			const HtmlEntity* const entity = FindEntity(text.substr(0, length));
			if (entity != nullptr)
			{
				consumed = length + 1;
				return entity;
			}
		}

		for (size_t size = (std::min)(length, kNameLengths.longestLegacy); size >= kNameLengths.shortestLegacy; --size)
		{
			const HtmlEntity* const entity = FindEntity(text.substr(0, size));
			if ((entity != nullptr) && entity->legacy)
			{
				consumed = size;
				return entity;
			}
		}
		return nullptr;
	}

	/**
	 * @brief Decodes the reference that follows an ampersand and appends what it stands for.
	 * @param text The text after the ampersand.
	 * @param output The decoded text.
	 * @return The number of characters of the reference after the ampersand, or 0 when there is no
	 * reference, in which case nothing is appended.
	 */
	size_t ExpandReference(std::string_view text, std::string& output)
	{
		if (text.empty())
			return 0;

		if (text[0] == '#')
		{
			char32_t cp = 0;
			const size_t consumed = ParseNumeric(text, cp);
			if (consumed != 0)
				AppendCharacter(output, cp);
			return consumed;
		}

		if (!IsAsciiAlphanumeric(text[0]))
			return 0;

		size_t consumed = 0;
		const HtmlEntity* const entity = ParseNamed(text, consumed);
		if (entity == nullptr)
			return 0;

		AppendCharacter(output, entity->first);
		if (entity->second != 0)
			AppendCharacter(output, entity->second);
		return consumed;
	}
}

std::string UnquoteHTML(std::string_view html)
{
	std::string text;
	text.reserve(html.size());

	size_t run = 0;	// start of the bytes that are copied as they are
	size_t pos = 0;
	while (pos < html.size())
	{
		const auto ch = static_cast<unsigned char>(html[pos]);
		if (ch == '&')
		{
			text.append(html.substr(run, pos - run));
			const size_t consumed = ExpandReference(html.substr(pos + 1), text);
			if (consumed == 0)
				text += '&';	// not a reference
			pos += consumed + 1;
			run = pos;
		}
		else if (ch < 0x80)
			++pos;
		else
		{
			const size_t length = Utf8Length(html, pos);
			if (length != 0)
				pos += length;
			else
			{
				text.append(html.substr(run, pos - run));
				text.append(kReplacementBytes);
				run = ++pos;
			}
		}
	}
	text.append(html.substr(run));
	return text;
}
