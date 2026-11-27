# AGENTS.md — Developer & AI Agent Guide for WebSearchEngine

Welcome to the **WebSearchEngine** repository. This document provides comprehensive architectural context, build and test instructions, coding conventions, and guidelines for AI agents and developers working with this codebase.

---

## 1. Project Overview

**WebSearchEngine** is a multi-threaded C++/MFC web crawler and search engine designed around text-mining and information retrieval concepts (including TF-IDF relevance scoring).

The repository includes:
- **Core C++/MFC Desktop Application (`WebSearchEngine`)**: Multi-threaded web crawler, URL frontier scheduler, HTML parser, token indexer, and ODBC database client.
- **Database Layer (`search.sql`)**: MySQL/MariaDB schema definitions and stored functions (`data_mining`, `no_of_words`, `no_of_pages`, `total_pages`) for ranking and TF-IDF calculation.
- **Web Frontend (`search.php`, `index.html`)**: Web-based query processing interface communicating with the indexed database.
- **Python Alternative Crawler (`WebSearchEngine.py`)**: Script-based crawling and indexing utility using BeautifulSoup, Gensim, and MySQL Connector.

---

## 2. Tech Stack & Prerequisites

| Component | Technology / Tool | Notes |
| :--- | :--- | :--- |
| **Language** | C++ (MSVC toolset `v145` / Visual Studio 2022/2026) | Statically linked MFC (`UseOfMfc = Static`), Unicode character set, `/std:c++latest` (`Utf8.cpp` needs the C++23 `basic_string::resize_and_overwrite`) |
| **GUI Framework** | Microsoft Foundation Classes (MFC) | Dialog-based application (`CDialogEx`) |
| **Networking** | WinINet API & Windows Sockets | Used for downloading HTML pages and resolving URLs |
| **Database Connectivity** | ODBC API | Custom lightweight C++ wrappers in `ODBCWrappers.h` |
| **Database Engine** | MySQL / MariaDB | Tables: `frontier`, `webpage`, `keyword`, `occurrence` |
| **Web Frontend** | PHP 7/8, HTML5, CSS3, JavaScript | Queries MySQL database using TF-IDF ranking |
| **Scripting / Python** | Python 3.x (`mysql-connector`, `beautifulsoup4`, `gensim`) | Alternative standalone crawler |

---

## 3. Repository Structure

```plaintext
WebSearchEngine/
├── WebSearchEngine.sln           # Visual Studio Solution file
├── WebSearchEngine.vcxproj       # Main Visual C++ project file
├── WebSearchEngine.vcxproj.filters # Solution explorer filter mapping
├── WebSearchEngine.h/.cpp       # Application class (CWebSearchEngineApp derived from CWinApp)
├── WebSearchEngineDlg.h/.cpp    # Main GUI dialog (CWebSearchEngineDlg), handles UI & worker thread
├── WebSearchEngineExt.h/.cpp    # Crawler engine: URL frontier, page downloader, token extractor, DB updates
├── HtmlToText.h/.cpp            # HTML parser & plaintext extractor engine
├── UnquoteHTML.h/.cpp           # Decoder of the HTML character references (&amp;, &#8364;, ...)
├── HtmlEntities.h               # Table of the named character references (generated: do not edit by hand)
├── GenerateHtmlEntities.py      # Generates HtmlEntities.h from the WHATWG entities.json
├── Utf8.h/.cpp                  # UTF-8 <-> UTF-16 conversion: utf8_to_wstring, wstring_to_utf8, replace_supplementary_characters
├── ODBCWrappers.h               # Object-oriented C++ wrappers for ODBC environment, connection, and queries
├── ConnectionSettingsDlg.h/.cpp # Dialog for configuring ODBC connection strings / DSNs
├── HLinkCtrl.h/.cpp             # Custom hyperlink static control
├── VersionInfo.h/.cpp           # Helper to query PE version info
├── Resource.h                   # Resource symbol IDs (managed by Visual Studio Resource Editor)
├── WebSearchEngine.rc           # Win32 resource definitions (dialogs, icons, strings)
├── res/                         # Icons, manifests, and RC2 resources
├── search.sql                   # MySQL schema, indexes, and TF-IDF stored functions
├── search.php                   # PHP search backend querying the MySQL database
├── index.html                   # HTML search front-end page
├── WebSearchEngine.py           # Standalone Python crawler / indexer
├── ReadMe.txt                   # Visual Studio project overview
├── ReleaseNotes.html            # Version history and release notes
└── LICENSE                      # GNU General Public License v3.0
```

---

## 4. Key Architectural Components

### 4.1. Application Lifecycle & Background Thread
- **`CWebSearchEngineApp`** (`WebSearchEngine.h/.cpp`): Initializes OLE/COM, Windows Sockets (`AfxSocketInit`), common controls, and displays `CWebSearchEngineDlg`.
- **`CWebSearchEngineDlg`** (`WebSearchEngineDlg.h/.cpp`):
  - Manages ODBC connection setup on `OnInitDialog()`.
  - Spawns background worker thread (`WebSearchEngineWorkerThread`) to execute crawling without blocking the UI.
  - Updates counters (pages processed, keywords indexed, active URL) and progress bar.

### 4.2. Crawler Engine & URL Frontier
- **`WebSearchEngineExt.h/.cpp`**:
  - `AddURLToFrontier`: Enqueues newly discovered URLs into the priority queue / database table if not visited.
  - `ExtractURLFromFrontier`: Retrieves the next highest-scoring unvisited URL.
  - `DownloadURLToFile`: Fetches remote web pages using WinINet (`InternetOpen`, `InternetOpenUrl`, `InternetReadFile`) into temporary local files.
  - `ProcessHTML`: Coordinates parsing the downloaded document, harvesting hyperlinks, stripping markup, extracting tokens/keywords, and saving records to MySQL.

### 4.3. HTML Processing & Tokenization
- **`CHtmlToText`** (`HtmlToText.h/.cpp`):
  - Stateless, MFC-free, single-pass HTML tokenizer that follows the HTML Living Standard tokenization rules closely enough for real-world markup. It runs in linear time without recursion, and one instance can be shared by several threads.
  - Removes tags, comments, and the elements that are not displayed together with their content (`title`, `script`, `style`, `noscript`, `template`, `object`, `svg`, ...).
  - Starts a new line at block elements, separates table cells with a tab, never splits a word at inline tags, preserves white space inside `<pre>`, and replaces malformed UTF-8 with U+FFFD.
  - Does not decode character references: the caller does it afterwards with `UnquoteHTML`.
- **`UnquoteHTML`** (`UnquoteHTML.h/.cpp`, names in `HtmlEntities.h`):
  - Decodes the character references of a text the way the HTML Living Standard does for the text of an element: the 2,231 named references (the 106 legacy names such as `&amp`, `&copy` and `&lt` are valid without the semicolon, and the longest name wins, so `&notit;` is the not sign followed by `it;`), and the decimal and hexadecimal numeric references, with an optional semicolon and any number of digits. `&#128;` to `&#159;` give the Windows-1252 characters, and the numbers that are not a character (0, surrogates, above U+10FFFF) give U+FFFD.
  - A decoded no-break space or other Unicode space (the same set that `CHtmlToText` collapses) becomes an ordinary space, and a reference never produces a NUL.
  - Always returns well-formed UTF-8: every malformed byte of the input becomes U+FFFD.
  - Stateless, linear time, safe to call from several threads. `HtmlEntities.h` is generated: to change the table, edit `GenerateHtmlEntities.py` and run it (`python GenerateHtmlEntities.py`, which downloads `entities.json` from the WHATWG site).
- **`utf8_to_wstring` / `wstring_to_utf8`** (`Utf8.h/.cpp`):
  - Convert between UTF-8 (`std::string_view`) and the UTF-16 `std::wstring` that MFC and ODBC use on Windows. They replace `std::wstring_convert<std::codecvt_utf8<wchar_t>>`: `<codecvt>` is deprecated since C++17 and removed in C++26, and the MSVC standard library has no replacement yet (the UTF transcoding views proposed in P2728 are not in it), so the conversion is written by hand with C++20/23 (`std::string_view`, `std::span`, `std::countl_one`, `constexpr`, `basic_string::resize_and_overwrite`). Do not bring `<codecvt>` back.
  - Never throw. Every ill-formed sequence (a stray or truncated byte, an overlong form, an encoded surrogate, a value above U+10FFFF, an unpaired UTF-16 surrogate) becomes one U+FFFD per maximal subpart, as in the Unicode Standard (section 3.9) and the WHATWG Encoding Standard. The characters above U+FFFF are converted to and from UTF-16 surrogate pairs.
  - The `static_assert` tests at the end of `Utf8.cpp` check the conversion at compile time, on every build: keep them passing when you change the code.
  - The tables use the MySQL character set `utf8` (utf8mb3), which cannot store a character above U+FFFF (an emoji, for instance), and `ProcessHTML` stops the crawl when the server refuses a row. So `ProcessHTML` converts every text that goes to the database with `utf8_to_database_text`, which calls `utf8_to_wstring` and then `replace_supplementary_characters` (each character above U+FFFF, and each unpaired surrogate, becomes one U+FFFD). The replacement can be removed once the tables are converted to `utf8mb4`.
  - Stateless, linear time, safe to call from several threads.
- Tokenization produces normalized keyword arrays stored in the `keyword` and `occurrence` tables.

### 4.4. Database Schema & TF-IDF Stored Functions
- **`frontier`**: `url_id` (PK), `address`, `visited` (bool), `score`.
- **`webpage`**: `webpage_id` (PK), `address`, `title`, `description`.
- **`keyword`**: `keyword_id` (PK), `name` (unique).
- **`occurrence`**: `occurrence_id` (PK), `webpage_id`, `keyword_id`, `counter`.
- **TF-IDF Stored Functions (`search.sql`)**:
  - `no_of_words(token)`: Maximum frequency of a word in any page.
  - `no_of_pages(token)`: Total count of pages containing the word.
  - `total_pages()`: Total indexed webpage count.
  - `data_mining(webpage_no, token)`: Calculates TF-IDF relevance score for ranking search results.

---

## 5. Build & Verification Instructions

### 5.1. Building via Visual Studio / MSBuild
The project targets **Visual C++** with static MFC.

Supported Configurations:
- `Debug|x64`
- `Release|x64`
- `Debug|Win32`
- `Release|Win32`

**Command-line build:**
```powershell
# Build x64 Debug
msbuild WebSearchEngine.vcxproj /p:Configuration=Debug /p:Platform=x64

# Build x64 Release
msbuild WebSearchEngine.vcxproj /p:Configuration=Release /p:Platform=x64
```

### 5.2. Compilation Checks for Agents
When editing C++ source files, always verify build integrity using the workspace build tool or MSBuild in the terminal.

---

## 6. Guidelines for AI Agents & Contributors

1. **Precompiled Headers (`stdafx.h`)**:
   - Every `.cpp` file must include `"stdafx.h"` as its very first include.
   - Do not remove or reorder `#include "stdafx.h"`.

2. **Resource Management & MFC Conventions**:
   - When modifying UI elements or dialogs, update both `Resource.h` and `WebSearchEngine.rc` consistently.
   - Respect MFC naming conventions (`m_p...` for controls/pointers, `m_b...` for booleans, `m_s...` for strings, `lpsz...` for string pointers).

3. **ODBC & SQL Safety**:
   - Use `CODBC` wrapper classes (`CConnection`, `CStatement`, `CDataStore`) in `ODBCWrappers.h` for database operations.
   - Escape single quotes in SQL statements using `Replace(_T("'"), _T("''"))` or parameterized queries to prevent SQL syntax errors.

4. **License & File Headers**:
   - Preserve the standard GPL-3.0 header block on all modified and newly created C++/Python/PHP source files:
	 ```cpp
	 /* Copyright (C) 2022-2026 Stefan-Mihai MOGA
	 This file is part of WebSearchEngine application developed by Stefan-Mihai MOGA.
	 ... (GNU General Public License v3 or later) ... */
	 ```

5. **Doxygen Documentation**:
   - Maintain Doxygen comments (`/** ... */`, `@brief`, `@param`, `@return`) for all class definitions, member variables, and public/exported functions.

6. **Cross-Component Consistency**:
   - If database table structures or columns are modified, ensure updates are synchronized across:
	 - `WebSearchEngineExt.cpp` (C++ schema creation & queries)
	 - `search.sql` (SQL DDL and stored functions)
	 - `search.php` (PHP web interface queries)
	 - `WebSearchEngine.py` (Python crawler queries)

7. **Text Encoding**:
   - Convert between UTF-8 and `std::wstring` with `utf8_to_wstring` and `wstring_to_utf8` (`Utf8.h`). Do not use `<codecvt>` or `std::wstring_convert`: they are deprecated since C++17 and removed in C++26.
   - Pass any text that is stored in the database through `utf8_to_database_text` (`WebSearchEngineExt.cpp`), because the tables use the MySQL character set `utf8`, which cannot store a character above U+FFFF.
