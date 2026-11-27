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
 * @brief Replaces the character references of an HTML text with the characters they stand for.
 *
 * The function decodes the text the way a browser does it for the content of an element, following
 * the rules of the HTML Living Standard:
 * - named references cover the whole table of the standard, `&amp;`, `&eacute;`, `&hellip;`,
 *   `&rarr;`, ... Some of them are also recognized without the terminating semicolon, as in
 *   `&amp` or `&copy 2026`, and the longest name wins: `&notit;` is the not sign followed by `it;`.
 *   A name that is not in the table is left as it is;
 * - numeric references, decimal (`&#169;`) or hexadecimal (`&#xA9;`), are decoded with or without
 *   the terminating semicolon and for every code point, including the ones above U+FFFF such as
 *   `&#x1F600;`. The numbers that cannot be a character (zero, the surrogates, the values above
 *   U+10FFFF) become U+FFFD and `&#128;` to `&#159;` are the Windows-1252 characters, so `&#150;`
 *   is the en dash and not a control character;
 * - an ampersand that does not start a reference, as in `AT&T` or `Q&A`, is kept;
 * - a reference to a space, the no-break space `&nbsp;` for example, becomes an ordinary space,
 *   so that it separates the words as CHtmlToText does for the same characters when they are not
 *   written as references.
 *
 * The text is expected to be UTF-8. Every malformed byte is replaced by U+FFFD, so the result is
 * always well-formed UTF-8, whatever the page contained, and it can be converted to UTF-16 safely.
 * A reference never produces a NUL character.
 *
 * The function takes linear time, uses no recursion and keeps no state, so it can be called from
 * several threads at the same time.
 *
 * @param html The UTF-8 encoded text, for example the output of CHtmlToText::Convert or the title
 * of a page. It is only read during the call.
 * @return The decoded text, encoded in UTF-8.
 */
[[nodiscard]] std::string UnquoteHTML(std::string_view html);
