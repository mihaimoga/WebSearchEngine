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
 * @brief Converts an HTML document into plain text that is ready to be indexed.
 *
 * The class is a small, forgiving, single-pass HTML tokenizer; it does not build a DOM tree.
 * It follows the tokenization rules of the HTML Living Standard closely enough to cope with the
 * markup found on the real Web (unquoted attributes, `>` inside quoted values, unterminated
 * comments, stray `<` characters, upper case tags, ...). It runs in linear time and without
 * recursion, so hostile or truncated pages cannot exhaust the stack or the CPU.
 *
 * The text that comes out is what a person reads on the page:
 * - tags, comments, doctype declarations and processing instructions are removed;
 * - elements that are not displayed are removed together with their content: `title`, `script`,
 *   `style`, `noscript`, `template`, `iframe`, `textarea`, `object`, `svg`, `math`, `canvas`,
 *   `audio`, `video`, ...;
 * - block elements, `br`, list items and table rows start a new line, paragraphs and headings are
 *   separated by a blank line and table cells are separated by a tab, while inline elements never
 *   split a word: `<b>H</b>ello` becomes `Hello`;
 * - white space, including the Unicode spaces such as the no-break space, is collapsed into a
 *   single separator, except inside `<pre>` where it is preserved;
 * - the result never starts or ends with white space.
 *
 * Character references such as `&amp;` or `&#169;` are not decoded, the caller is expected to do
 * it afterwards (see UnquoteHTML). The input is expected to be UTF-8; every malformed byte is
 * replaced by U+FFFD, so the result is always well-formed UTF-8 and never contains a NUL.
 *
 * The class keeps no state between calls: one instance can be reused and shared by several threads.
 */
class CHtmlToText
{
public:
	/** @brief Constructs a converter. */
	CHtmlToText() = default;

	/** @brief Destroys the converter. */
	~CHtmlToText() = default;

	/**
	 * @brief Extracts the text of an HTML document.
	 * @param html The UTF-8 encoded HTML document. It is only read during the call.
	 * @return The plain text of the document, encoded in UTF-8.
	 */
	[[nodiscard]] std::string Convert(std::string_view html) const;
};
