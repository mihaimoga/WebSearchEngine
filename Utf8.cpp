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
 * @file Utf8.cpp
 * @brief Converts text between UTF-8 and wide strings.
 *
 * The standard library of MSVC has no replacement yet for the std::codecvt_utf8 facet and for
 * std::wstring_convert, which are deprecated since C++17 and removed from C++26 (P2871, P2872): the
 * UTF transcoding views that are proposed for the standard library (P2728) are not in it. So the
 * conversion is written here with the facilities of C++20 and C++23: std::string_view and
 * std::span for the input and the output, std::countl_one to measure a UTF-8 sequence and
 * std::basic_string::resize_and_overwrite to write the result into the string without filling it
 * with zeros first.
 *
 * The conversion is constexpr, and the tests at the end of this file are evaluated by the compiler,
 * so a change that breaks it does not build.
 */

#include "stdafx.h"
#include "Utf8.h"
#include <bit>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <version>

#ifndef __cpp_lib_string_resize_and_overwrite
#error Utf8.cpp needs std::basic_string::resize_and_overwrite of C++23: set the language standard to /std:c++latest
#endif

namespace
{
	/** @brief The character U+FFFD, which replaces every sequence that is not well-formed. */
	constexpr char32_t kReplacementCharacter = 0xFFFD;

	/** @brief The largest code point. */
	constexpr char32_t kMaxCodePoint = 0x10FFFF;

	/** @brief The first code point above the Basic Multilingual Plane. */
	constexpr char32_t kFirstSupplementary = 0x10000;

	/** @brief A wide string is UTF-16 when wchar_t has 16 bits, as on Windows, and UTF-32 otherwise. */
	constexpr bool kWideIsUtf16 = sizeof(wchar_t) == 2;

	static_assert(kWideIsUtf16 || (sizeof(wchar_t) == 4), "wchar_t must hold UTF-16 or UTF-32 code units");

	/** @brief A character read from a text, and the number of code units it takes there. */
	struct Character
	{
		char32_t value;	/**< The code point, or U+FFFD when the code units are not well-formed. */
		size_t length;	/**< The number of code units that were read; never 0. */
	};

	/**
	 * @brief Reads the character at the start of a UTF-8 text.
	 *
	 * The code units that are not well-formed are read as a whole and give one U+FFFD: they are the
	 * longest run that starts a well-formed sequence (the maximal subpart of the Unicode Standard).
	 * @param text The text; it must not be empty.
	 * @return The character and the number of bytes it takes.
	 */
	constexpr Character DecodeUtf8(std::string_view text) noexcept
	{
		const auto lead = static_cast<unsigned char>(text[0]);
		if (lead < 0x80)
			return { lead, 1 };
		if ((lead < 0xC2) || (lead > 0xF4))	// continuation byte, C0 and C1 (overlong forms), F5 to FF
			return { kReplacementCharacter, 1 };

		const auto length = static_cast<size_t>(std::countl_one(lead));	// the leading one bits count the bytes
		unsigned char lower = 0x80;	// range of the second byte, the others are always 0x80..0xBF
		unsigned char upper = 0xBF;
		if (lead == 0xE0)
			lower = 0xA0;	// no overlong forms
		else if (lead == 0xED)
			upper = 0x9F;	// no surrogates
		else if (lead == 0xF0)
			lower = 0x90;	// no overlong forms
		else if (lead == 0xF4)
			upper = 0x8F;	// nothing above U+10FFFF

		auto value = static_cast<char32_t>(lead & (0x7Fu >> length));
		for (size_t index = 1; index < length; ++index)
		{
			if (index == text.size())
				return { kReplacementCharacter, index };	// the text ends inside the sequence
			const auto next = static_cast<unsigned char>(text[index]);
			if ((next < lower) || (next > upper))
				return { kReplacementCharacter, index };	// the bytes before this one are the maximal subpart
			value = (value << 6) | (next & 0x3Fu);
			lower = 0x80;
			upper = 0xBF;
		}
		return { value, length };
	}

	/**
	 * @brief Reads the character at the start of a wide string.
	 *
	 * A surrogate that has no partner, and in UTF-32 a surrogate or a value above U+10FFFF, gives U+FFFD.
	 * @param text The wide string; it must not be empty.
	 * @return The character and the number of wchar_t it takes.
	 */
	constexpr Character DecodeWide(std::wstring_view text) noexcept
	{
		const auto unit = static_cast<char32_t>(text[0]);
		if ((unit < 0xD800) || ((unit > 0xDFFF) && (unit <= kMaxCodePoint)))
			return { unit, 1 };
		if constexpr (kWideIsUtf16)
		{
			if ((unit < 0xDC00) && (text.size() > 1))	// a high surrogate, and the low one follows it
			{
				const auto next = static_cast<char32_t>(text[1]);
				if ((next >= 0xDC00) && (next <= 0xDFFF))
					return { kFirstSupplementary + ((unit - 0xD800) << 10) + (next - 0xDC00), 2 };
			}
		}
		return { kReplacementCharacter, 1 };
	}

	/**
	 * @brief Measures the UTF-8 sequence of a character.
	 * @param value The code point, which must be a valid Unicode scalar value.
	 * @return The number of bytes, from 1 to 4.
	 */
	constexpr size_t Utf8Size(char32_t value) noexcept
	{
		if (value < 0x80)
			return 1;
		if (value < 0x800)
			return 2;
		return (value < kFirstSupplementary) ? 3 : 4;
	}

	/**
	 * @brief Writes a character in UTF-8.
	 * @param value The code point, which must be a valid Unicode scalar value.
	 * @param out The place to write the bytes; it must have room for Utf8Size(value) of them.
	 * @return The number of bytes written.
	 */
	constexpr size_t EncodeUtf8(char32_t value, std::span<char> out) noexcept
	{
		const size_t size = Utf8Size(value);
		if (size == 1)
			out[0] = static_cast<char>(value);
		else if (size == 2)
		{
			out[0] = static_cast<char>(0xC0 | (value >> 6));
			out[1] = static_cast<char>(0x80 | (value & 0x3F));
		}
		else if (size == 3)
		{
			out[0] = static_cast<char>(0xE0 | (value >> 12));
			out[1] = static_cast<char>(0x80 | ((value >> 6) & 0x3F));
			out[2] = static_cast<char>(0x80 | (value & 0x3F));
		}
		else
		{
			out[0] = static_cast<char>(0xF0 | (value >> 18));
			out[1] = static_cast<char>(0x80 | ((value >> 12) & 0x3F));
			out[2] = static_cast<char>(0x80 | ((value >> 6) & 0x3F));
			out[3] = static_cast<char>(0x80 | (value & 0x3F));
		}
		return size;
	}

	/**
	 * @brief Writes a character in the encoding of the wide strings.
	 * @param value The code point, which must be a valid Unicode scalar value.
	 * @param out The place to write the code units; it must have room for two of them.
	 * @return The number of wchar_t written, 1 or, for a character above U+FFFF in UTF-16, 2.
	 */
	constexpr size_t EncodeWide(char32_t value, std::span<wchar_t> out) noexcept
	{
		if constexpr (kWideIsUtf16)
		{
			if (value >= kFirstSupplementary)
			{
				value -= kFirstSupplementary;
				out[0] = static_cast<wchar_t>(0xD800 + (value >> 10));
				out[1] = static_cast<wchar_t>(0xDC00 + (value & 0x3FF));
				return 2;
			}
		}
		out[0] = static_cast<wchar_t>(value);
		return 1;
	}

	/**
	 * @brief Converts UTF-8 text to a wide string.
	 * @param utf8 The UTF-8 text.
	 * @return The wide string.
	 */
	[[nodiscard]] constexpr std::wstring ToWide(std::string_view utf8)
	{
		std::wstring wide;
		// A byte never gives more than one wchar_t (a 4-byte sequence gives a surrogate pair), so the
		// size of the text is enough, and the string is cut to the length that was written
		wide.resize_and_overwrite(utf8.size(), [utf8](wchar_t* buffer, size_t capacity) constexpr noexcept
			{
				const std::span<wchar_t> out(buffer, capacity);
				size_t written = 0;
				for (std::string_view rest = utf8; !rest.empty();)
				{
					const Character character = DecodeUtf8(rest);
					rest.remove_prefix(character.length);
					written += EncodeWide(character.value, out.subspan(written));
				}
				return written;
			});
		return wide;
	}

	/**
	 * @brief Converts a wide string to UTF-8 text.
	 * @param wide The wide string.
	 * @return The UTF-8 text.
	 */
	[[nodiscard]] constexpr std::string ToUtf8(std::wstring_view wide)
	{
		// A wchar_t gives from 1 to 4 bytes, so the size is measured first and the string is not larger than the text
		size_t size = 0;
		for (std::wstring_view rest = wide; !rest.empty();)
		{
			const Character character = DecodeWide(rest);
			rest.remove_prefix(character.length);
			size += Utf8Size(character.value);
		}

		std::string utf8;
		utf8.resize_and_overwrite(size, [wide](char* buffer, size_t capacity) constexpr noexcept
			{
				const std::span<char> out(buffer, capacity);
				size_t written = 0;
				for (std::wstring_view rest = wide; !rest.empty();)
				{
					const Character character = DecodeWide(rest);
					rest.remove_prefix(character.length);
					written += EncodeUtf8(character.value, out.subspan(written));
				}
				return written;
			});
		return utf8;
	}

	/**
	 * @brief Replaces the characters above U+FFFF of a wide string, and the surrogates without a partner, with U+FFFD.
	 * @param text The wide string to change in place.
	 */
	constexpr void ReplaceSupplementary(std::wstring& text) noexcept
	{
		// The characters are read ahead of the place where they are written, so the text is changed in place
		size_t written = 0;
		for (std::wstring_view rest = text; !rest.empty();)
		{
			const Character character = DecodeWide(rest);
			rest.remove_prefix(character.length);
			text[written++] = static_cast<wchar_t>((character.value < kFirstSupplementary) ? character.value : kReplacementCharacter);
		}
		text.resize(written);
	}

	/**
	 * @brief Applies ReplaceSupplementary to a copy of a wide string; this is for the tests.
	 * @param text The wide string.
	 * @return The changed copy.
	 */
	[[nodiscard]] constexpr std::wstring Replaced(std::wstring text)
	{
		ReplaceSupplementary(text);
		return text;
	}

	// The tests that follow are evaluated by the compiler, they take no time when the program runs.

	// UTF-8 to wide string: well-formed text
	static_assert(ToWide("") == L"");
	static_assert(ToWide("Hello, world") == L"Hello, world");
	static_assert(ToWide(std::string_view("a\0b", 3)) == std::wstring_view(L"a\0b", 3));	// NUL is a character like the others
	static_assert(ToWide("\xC2\x80") == L"\u0080");	// the shortest 2-byte sequence
	static_assert(ToWide("\xC3\xA9") == L"\u00E9");
	static_assert(ToWide("\xDF\xBF") == L"\u07FF");	// the longest 2-byte sequence
	static_assert(ToWide("\xE0\xA0\x80") == L"\u0800");	// the shortest 3-byte sequence
	static_assert(ToWide("\xE2\x82\xAC") == L"\u20AC");
	static_assert(ToWide("\xED\x9F\xBF") == L"\uD7FF");	// the last character before the surrogates
	static_assert(ToWide("\xEE\x80\x80") == L"\uE000");	// the first character after them
	static_assert(ToWide("\xEF\xBF\xBF") == L"\uFFFF");	// the longest 3-byte sequence
	static_assert(ToWide("\xF0\x90\x80\x80") == L"\U00010000");	// the shortest 4-byte sequence
	static_assert(ToWide("\xF0\x9F\x98\x80") == L"\U0001F600");
	static_assert(ToWide("\xF4\x8F\xBF\xBF") == L"\U0010FFFF");	// the last code point

	// UTF-8 to wide string: the text that is not well-formed gives one U+FFFD for each maximal subpart
	static_assert(ToWide("\x80") == L"\uFFFD");	// continuation byte
	static_assert(ToWide("\xFF") == L"\uFFFD");
	static_assert(ToWide("\xC0\x80") == L"\uFFFD\uFFFD");	// overlong NUL
	static_assert(ToWide("\xE0\x80\x80") == L"\uFFFD\uFFFD\uFFFD");	// overlong form
	static_assert(ToWide("\xF0\x8F\xBF\xBF") == L"\uFFFD\uFFFD\uFFFD\uFFFD");	// overlong 4-byte form
	static_assert(ToWide("\xED\xA0\x80") == L"\uFFFD\uFFFD\uFFFD");	// the surrogate U+D800
	static_assert(ToWide("\xF4\x90\x80\x80") == L"\uFFFD\uFFFD\uFFFD\uFFFD");	// U+110000
	static_assert(ToWide("\xF5\x80\x80\x80") == L"\uFFFD\uFFFD\uFFFD\uFFFD");
	static_assert(ToWide("\xE2\x82") == L"\uFFFD");	// the text ends inside a sequence
	static_assert(ToWide("\xF0\x9F\x98") == L"\uFFFD");
	static_assert(ToWide("\xE2\x82" "A") == L"\uFFFDA");	// the sequence is interrupted by a character
	static_assert(ToWide("\xF0\x9F" "\xC3\xA9") == L"\uFFFD\u00E9");
	static_assert(ToWide(std::string_view("\xE2\x82\xAC", 2)) == L"\uFFFD");	// a view that ends inside a sequence is not read beyond its end
	static_assert(ToWide(std::string_view("\xF0\x9F\x98\x80", 3)) == L"\uFFFD");

	// Wide string to UTF-8
	static_assert(ToUtf8(L"") == "");
	static_assert(ToUtf8(L"Hello, world") == "Hello, world");
	static_assert(ToUtf8(std::wstring_view(L"a\0b", 3)) == std::string_view("a\0b", 3));
	static_assert(ToUtf8(L"\u007F") == "\x7F");	// the longest 1-byte sequence
	static_assert(ToUtf8(L"\u0080") == "\xC2\x80");	// the shortest 2-byte sequence
	static_assert(ToUtf8(L"\u00E9") == "\xC3\xA9");
	static_assert(ToUtf8(L"\u07FF") == "\xDF\xBF");	// the longest 2-byte sequence
	static_assert(ToUtf8(L"\u0800") == "\xE0\xA0\x80");	// the shortest 3-byte sequence
	static_assert(ToUtf8(L"\u20AC") == "\xE2\x82\xAC");
	static_assert(ToUtf8(L"\uD7FF") == "\xED\x9F\xBF");	// the last character before the surrogates
	static_assert(ToUtf8(L"\uE000") == "\xEE\x80\x80");	// the first character after them
	static_assert(ToUtf8(L"\uFFFF") == "\xEF\xBF\xBF");	// the longest 3-byte sequence
	static_assert(ToUtf8(L"\U00010000") == "\xF0\x90\x80\x80");	// the shortest 4-byte sequence
	static_assert(ToUtf8(L"\U0001F600") == "\xF0\x9F\x98\x80");	// a surrogate pair in UTF-16
	static_assert(ToUtf8(L"\U0010FFFF") == "\xF4\x8F\xBF\xBF");	// the last code point
	static_assert(ToUtf8(L"\xD83D") == "\xEF\xBF\xBD");	// high surrogate without a partner
	static_assert(ToUtf8(L"\xDE00") == "\xEF\xBF\xBD");	// low surrogate without a partner
	static_assert(ToUtf8(L"\xD83D" L"A") == "\xEF\xBF\xBD" "A");
	static_assert(ToUtf8(L"\xDE00\xD83D") == "\xEF\xBF\xBD\xEF\xBF\xBD");
	static_assert(ToUtf8(L"\xDE00\xDC00") == "\xEF\xBF\xBD\xEF\xBF\xBD");	// a low surrogate cannot start a pair
	static_assert(ToUtf8(L"\xD83D\xD83D\xDE00") == "\xEF\xBF\xBD\xF0\x9F\x98\x80");	// the pair follows a high surrogate without a partner
	static_assert(ToUtf8(L"\xD83D\xDBFF") == "\xEF\xBF\xBD\xEF\xBF\xBD");	// the second unit is a high surrogate too
	static_assert(ToUtf8(L"\xD83D\xE000") == "\xEF\xBF\xBD\xEE\x80\x80");	// the second unit is above the surrogates
	static_assert(ToUtf8(std::wstring_view(L"\xD83D\xDE00", 1)) == "\xEF\xBF\xBD");	// a view that ends after a high surrogate is not read beyond its end

	// Both ways
	static_assert(ToUtf8(ToWide("A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80")) == "A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
	static_assert(ToWide(ToUtf8(L"A\u00E9\u20AC\U0001F600")) == L"A\u00E9\u20AC\U0001F600");
	static_assert(ToUtf8(std::wstring(100, L'\u20AC')).size() == 300);	// 3 bytes each: a text longer than the rounding of an allocation
	static_assert(ToWide(ToUtf8(std::wstring(100, L'\u20AC'))) == std::wstring(100, L'\u20AC'));

	// Replacing the characters above U+FFFF
	static_assert(Replaced(L"") == L"");
	static_assert(Replaced(L"abc \u20AC") == L"abc \u20AC");	// the Basic Multilingual Plane is left as it is
	static_assert(Replaced(L"a\U0001F600b") == L"a\uFFFDb");
	static_assert(Replaced(L"\uFFFF") == L"\uFFFF");	// the last character of the Basic Multilingual Plane
	static_assert(Replaced(L"\U00010000") == L"\uFFFD");	// the first character above it
	static_assert(Replaced(L"\U0001F600\U0010FFFF") == L"\uFFFD\uFFFD");
	static_assert(Replaced(L"\xD83D" L"b") == L"\uFFFDb");	// surrogate without a partner
	static_assert(Replaced(L"\xDE00\xDC00") == L"\uFFFD\uFFFD");	// a low surrogate cannot start a pair
}

std::wstring utf8_to_wstring(std::string_view utf8)
{
	return ToWide(utf8);
}

std::string wstring_to_utf8(std::wstring_view wide)
{
	return ToUtf8(wide);
}

void replace_supplementary_characters(std::wstring& text) noexcept
{
	ReplaceSupplementary(text);
}
