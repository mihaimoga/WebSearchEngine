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

#pragma once

#include <string>
#include <string_view>

/**
 * @brief Converts UTF-8 text to a wide string.
 *
 * The wide string is UTF-16 where wchar_t has 16 bits, as it has on Windows, so a character above
 * U+FFFF, an emoji for example, becomes a surrogate pair: U+1F600 is converted to U+D83D U+DE00.
 * Where wchar_t has 32 bits the wide string is UTF-32 and every character takes one wchar_t.
 *
 * The function does not fail on malformed text. A sequence of bytes that is not well-formed UTF-8
 * (a stray continuation byte, an overlong form such as C0 80, an encoded surrogate, a value above
 * U+10FFFF, a sequence that ends too soon) is replaced by U+FFFD, one for each maximal subpart:
 * the longest run of bytes that starts a well-formed sequence is replaced as a whole. This is the
 * practice recommended by the Unicode Standard (section 3.9), the one of the WHATWG Encoding
 * Standard and of the UTF transcoding views proposed for the C++ standard library (P2728), so the
 * result is always well-formed UTF-16 or UTF-32. The NUL character is converted like any other.
 *
 * The function replaces std::wstring_convert with std::codecvt_utf8<wchar_t>, which has been
 * deprecated since C++17 and is removed from C++26. That facet threw std::range_error at the first
 * malformed byte of a page and, because it produces UCS-2, it cut every character above U+FFFF to
 * its low 16 bits (U+1F600 became U+F600).
 *
 * The function takes linear time and keeps no state, so it can be called from several threads at
 * the same time.
 *
 * @param utf8 The UTF-8 encoded text. It is only read during the call.
 * @return The text as a wide string.
 */
[[nodiscard]] std::wstring utf8_to_wstring(std::string_view utf8);

/**
 * @brief Converts a wide string to UTF-8 text.
 *
 * The wide string is read as UTF-16 where wchar_t has 16 bits, as it has on Windows, so a surrogate
 * pair is converted to one character and then to a 4-byte sequence: U+D83D U+DE00 becomes
 * F0 9F 98 80. Where wchar_t has 32 bits the wide string is read as UTF-32.
 *
 * The function does not fail on malformed text. A surrogate that has no partner (a high surrogate
 * that is not followed by a low one, a low surrogate that does not follow a high one) and, in UTF-32,
 * a value above U+10FFFF is replaced by U+FFFD, so the result is always well-formed UTF-8. The NUL
 * character is converted like any other.
 *
 * The function replaces std::wstring_convert with std::codecvt_utf8<wchar_t>, which has been
 * deprecated since C++17 and is removed from C++26. That facet wrote each half of a surrogate pair
 * as a 3-byte sequence of its own (CESU-8, which is not well-formed UTF-8) and so it did for a
 * surrogate that has no partner.
 *
 * The function takes linear time and keeps no state, so it can be called from several threads at
 * the same time.
 *
 * @param wide The wide string. It is only read during the call.
 * @return The text encoded in UTF-8.
 */
[[nodiscard]] std::string wstring_to_utf8(std::wstring_view wide);

/**
 * @brief Replaces the characters that are above U+FFFF with U+FFFD, in place.
 *
 * Only the Basic Multilingual Plane is left in the string: each character above U+FFFF, which is a
 * surrogate pair where wchar_t has 16 bits, becomes one U+FFFD, and so does every surrogate that has
 * no partner. The other characters are not changed, so a text that has none of them is the same
 * afterwards and the string never gets longer.
 *
 * This is for the destinations that cannot store the other characters, such as a MySQL column with
 * the character set utf8 (utf8mb3), which refuses emoji and every other character that takes 4 bytes
 * in UTF-8.
 *
 * @param text The wide string to change.
 */
void replace_supplementary_characters(std::wstring& text) noexcept;
