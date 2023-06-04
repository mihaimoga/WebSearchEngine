# This file is part of WebSearchEngine application developed by Stefan-Mihai MOGA.
#
# WebSearchEngine is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Open
# Source Initiative, either version 3 of the License, or any later version.
#
# WebSearchEngine is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# WebSearchEngine.  If not, see <http://www.opensource.org/licenses/gpl-3.0.html>

import mysql.connector
from bs4 import BeautifulSoup
from urllib.request import Request, urlopen
from urllib.parse import urljoin
import urllib
import time
from gensim.utils import tokenize


HOSTNAME = "localhost"
DATABASE = "r46882text_mining"
USERNAME = "r46882text_engine"
PASSWORD = "TextMining2021!@#$"

visited_urls = []
frontier_array = []
frontier_score = {}

webpage_count = 0
keyword_array = []


def create_database():
    try:
        connection = mysql.connector.connect(
            host=HOSTNAME,
            database=DATABASE,
            user=USERNAME,
            password=PASSWORD,
            autocommit=True,
        )
        server_info = connection.get_server_info()
        print("MySQL connection is open on", server_info)
        sql_drop_table = "DROP TABLE IF EXISTS `occurrence`"
        cursor = connection.cursor()
        cursor.execute(sql_drop_table)
        sql_drop_table = "DROP TABLE IF EXISTS `keyword`"
        cursor.execute(sql_drop_table)
        sql_drop_table = "DROP TABLE IF EXISTS `webpage`"
        cursor.execute(sql_drop_table)
        sql_create_table = (
            "CREATE TABLE `webpage` (`webpage_id` BIGINT NOT NULL AUTO_INCREMENT, "
            "`url` VARCHAR(256) NOT NULL, `title` VARCHAR(256) NOT NULL, "
            "`content` TEXT NOT NULL, PRIMARY KEY(`webpage_id`)) ENGINE=InnoDB"
        )
        cursor.execute(sql_create_table)
        sql_create_table = (
            "CREATE TABLE `keyword` (`keyword_id` BIGINT NOT NULL AUTO_INCREMENT, "
            "`name` VARCHAR(256) NOT NULL, PRIMARY KEY(`keyword_id`)) ENGINE=InnoDB"
        )
        cursor.execute(sql_create_table)
        sql_create_table = (
            "CREATE TABLE `occurrence` (`webpage_id` BIGINT NOT NULL, "
            "`keyword_id` BIGINT NOT NULL, `counter` BIGINT NOT NULL, "
            "`pagerank` REAL NOT NULL, PRIMARY KEY(`webpage_id`, `keyword_id`), "
            "FOREIGN KEY webpage_fk(webpage_id) REFERENCES webpage(webpage_id), "
            "FOREIGN KEY keyword_fk(keyword_id) REFERENCES keyword(keyword_id)) ENGINE=InnoDB"
        )
        cursor.execute(sql_create_table)
        sql_create_index = (
            "CREATE OR REPLACE UNIQUE INDEX index_name ON `keyword`(`name`)"
        )
        cursor.execute(sql_create_index)
        sql_no_of_words = (
            "CREATE OR REPLACE FUNCTION no_of_words(token VARCHAR(256)) RETURNS "
            "REAL READS SQL DATA RETURN (SELECT MAX(`counter`) FROM `occurrence` "
            "INNER JOIN `keyword` USING(`keyword_id`) WHERE `name` = token)"
        )
        cursor.execute(sql_no_of_words)
        sql_no_of_pages = (
            "CREATE OR REPLACE FUNCTION no_of_pages(token VARCHAR(256)) RETURNS "
            "REAL READS SQL DATA RETURN (SELECT COUNT(`webpage_id`) FROM `occurrence` "
            "INNER JOIN `keyword` USING(`keyword_id`) WHERE `name` = token)"
        )
        cursor.execute(sql_no_of_pages)
        sql_total_pages = (
            "CREATE OR REPLACE FUNCTION total_pages() RETURNS REAL READS SQL DATA "
            "RETURN (SELECT COUNT(`webpage_id`) FROM `webpage`)"
        )
        cursor.execute(sql_total_pages)
        sql_data_mining = (
            "CREATE OR REPLACE FUNCTION data_mining(webpage_no BIGINT, token VARCHAR(256)) "
            "RETURNS REAL READS SQL DATA RETURN (SELECT SUM(`counter`)/no_of_words(token)*"
            "LOG((1+total_pages())/no_of_pages(token)) FROM `occurrence` INNER JOIN `keyword` "
            "USING(`keyword_id`) WHERE `name` = token AND `webpage_id` = webpage_no)"
        )
        cursor.execute(sql_data_mining)
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
        return False
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()
            print("MySQL connection is now closed")
        return True


def add_url_to_frontier(url):
    global visited_urls
    global frontier_array
    global frontier_score
    found = False
    if url.find("#") > 0:
        url = url.split("#")[0]
    if len(url) > 256:
        return
    if url.endswith(".3g2"):
        return  # 3GPP2 multimedia file
    if url.endswith(".3gp"):
        return  # 3GPP multimedia file
    if url.endswith(".7z"):
        return  # 7-Zip compressed file
    if url.endswith(".ai"):
        return  # Adobe Illustrator file
    if url.endswith(".apk"):
        return  # Android package file
    if url.endswith(".arj"):
        return  # ARJ compressed file
    if url.endswith(".aif"):
        return  # AIF audio file
    if url.endswith(".avi"):
        return  # AVI file
    if url.endswith(".bat"):
        return  # Batch file
    if url.endswith(".bin"):
        return  # Binary disc image
    if url.endswith(".bmp"):
        return  # Bitmap image
    if url.endswith(".cda"):
        return  # CD audio track file
    if url.endswith(".com"):
        return  # MS-DOS command file
    if url.endswith(".csv"):
        return  # Comma separated value file
    if url.endswith(".dat"):
        return  # Binary Data file
    if url.endswith(".db") or url.endswith(".dbf"):
        return  # Database file
    if url.endswith(".deb"):
        return  # Debian software package file
    if url.endswith(".dmg"):
        return  # macOS X disk image
    if url.endswith(".doc") or url.endswith(".docx"):
        return  # Microsoft Word Open XML document file
    if url.endswith(".email") or url.endswith(".eml"):
        return  # E-mail message file from multiple e-mail clients
    if url.endswith(".emlx"):
        return  # Apple Mail e-mail file
    if url.endswith(".exe"):
        return  # MS-DOS executable file
    if url.endswith(".flv"):
        return  # Adobe Flash file
    if url.endswith(".fon"):
        return  # Generic font file
    if url.endswith(".fnt"):
        return  # Windows font file
    if url.endswith(".gadget"):
        return  # Windows gadget
    if url.endswith(".gif"):
        return  # GIF image
    if url.endswith(".h264"):
        return  # H.264 video file
    if url.endswith(".ico"):
        return  # Icon file
    if url.endswith(".iso"):
        return  # ISO disc image
    if url.endswith(".jar"):
        return  # Java archive file
    if url.endswith(".jpg") or url.endswith(".jpeg"):
        return  # JPEG image
    if url.endswith(".log"):
        return  # Log file
    if url.endswith(".m4v"):
        return  # Apple MP4 video file
    if url.endswith(".mdb"):
        return  # Microsoft Access database file
    if url.endswith(".mid") or url.endswith(".midi"):
        return  # MIDI audio file
    if url.endswith(".mov"):
        return  # Apple QuickTime movie file
    if url.endswith(".mp3") or url.endswith(".mpa"):
        return  # MP3 audio file
    if url.endswith(".mp4"):
        return  # MPEG4 video file
    if url.endswith(".mpa"):
        return  # MPEG-2 audio file
    if url.endswith(".mpg") or url.endswith(".mpeg"):
        return  # MPEG video file
    if url.endswith(".msg"):
        return  # Microsoft Outlook e-mail message file
    if url.endswith(".msi"):
        return  # Windows installer package
    if url.endswith(".odt"):
        return  # OpenOffice Writer document file
    if url.endswith(".ods"):
        return  # OpenOffice Calc spreadsheet file
    if url.endswith(".oft"):
        return  # Microsoft Outlook e-mail template file
    if url.endswith(".ogg"):
        return  # Ogg Vorbis audio file
    if url.endswith(".ost"):
        return  # Microsoft Outlook e-mail storage file
    if url.endswith(".otf"):
        return  # Open type font file
    if url.endswith(".pkg"):
        return  # Package file
    if url.endswith(".pdf"):
        return  # Adobe PDF file
    if url.endswith(".png"):
        return  # PNG image
    if url.endswith(".ppt") or url.endswith(".pptx"):
        return  # Microsoft PowerPoint Open XML presentation
    if url.endswith(".ps"):
        return  # PostScript file
    if url.endswith(".psd"):
        return  # PSD image
    if url.endswith(".pst"):
        return  # Microsoft Outlook e-mail storage file
    if url.endswith(".rar"):
        return  # RAR file
    if url.endswith(".rpm"):
        return  # Red Hat Package Manager
    if url.endswith(".rtf"):
        return  # Rich Text Format file
    if url.endswith(".sql"):
        return  # SQL database file
    if url.endswith(".svg"):
        return  # Scalable Vector Graphics file
    if url.endswith(".swf"):
        return  # Shockwave flash file
    if url.endswith(".xls") or url.endswith(".xlsx"):
        return  # Microsoft Excel Open XML spreadsheet file
    if url.endswith(".toast"):
        return  # Toast disc image
    if url.endswith(".tar"):
        return  # Linux tarball file archive
    if url.endswith(".tar.gz"):
        return  # Tarball compressed file
    if url.endswith(".tex"):
        return  # A LaTeX document file
    if url.endswith(".ttf"):
        return  # TrueType font file
    if url.endswith(".txt"):
        return  # Plain text file
    if url.endswith(".tif") or url.endswith(".tiff"):
        return  # TIFF image
    if url.endswith(".vcd"):
        return  # Virtual CD
    if url.endswith(".vcf"):
        return  # E-mail contact file
    if url.endswith(".vob"):
        return  # DVD Video Object
    if url.endswith(".xml"):
        return  # XML file
    if url.endswith(".wav") or url.endswith(".wma"):
        return  # WAV file
    if url.endswith(".wmv"):
        return  # Windows Media Video file
    if url.endswith(".wpd"):
        return  # WordPerfect document
    if url.endswith(".wpl"):
        return  # Windows Media Player playlist
    if url.endswith(".wsf"):
        return  # Windows script file
    if url.endswith(".z") or url.endswith(".zip"):
        return  # Z or Zip compressed file
    if url not in visited_urls:
        if url in frontier_array:
            found = True
            frontier_score[url] = frontier_score.get(url) + 1
        if not found:
            frontier_array.append(url)
            frontier_score[url] = 1


def extract_url_from_frontier():
    global frontier_array
    global frontier_score
    score = 0
    url = None
    for item in frontier_array:
        if score < frontier_score.get(item):
            url = item
            score = frontier_score.get(url)
    if url:
        frontier_array.remove(url)
        del frontier_score[url]
        visited_urls.append(url)
    return url


def download_page_from_url(url):
    html_title = None
    plain_text = None
    try:
        req = Request(url)
        html_page = urlopen(req)
        soup = BeautifulSoup(html_page, "html.parser")
        html_title = soup.title.get_text().strip()
        plain_text = soup.get_text().strip()
        plain_text = " ".join(plain_text.split())
        for hyperlink in soup.find_all("a"):
            hyperlink = urljoin(url, hyperlink.get("href"))
            add_url_to_frontier(hyperlink)
    except urllib.error.URLError as err:
        print(str(err))
    except urllib.error.HTTPError as err:
        print(str(err))
    except urllib.error.ContentTooShortError as err:
        print(str(err))
    finally:
        return html_title, plain_text


def web_search_engine():
    global webpage_count
    try:
        connection = mysql.connector.connect(
            host=HOSTNAME,
            database=DATABASE,
            user=USERNAME,
            password=PASSWORD,
            autocommit=True,
        )
        server_info = connection.get_server_info()
        print("MySQL connection is open on", server_info)
        while True:
            url = extract_url_from_frontier()
            if url:
                print("Crawling %s... [%d]" % (url, webpage_count + 1))
                html_title, plain_text = download_page_from_url(url)
                if html_title and plain_text:
                    if len(html_title) > 0:
                        connection = analyze_webpage(
                            connection, url, html_title, plain_text
                        )
                        if (webpage_count > 0) and ((webpage_count % 1000) == 0):
                            if connection.is_connected():
                                connection.close()
                                print("MySQL connection is now closed")
                            data_mining()
            else:
                break
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
    finally:
        if connection.is_connected():
            connection.close()
            print("MySQL connection is now closed")


def analyze_webpage(connection, url, html_title, plain_text):
    global webpage_count
    while not connection.is_connected():
        try:
            time.sleep(30)
            connection = mysql.connector.connect(
                host=HOSTNAME,
                database=DATABASE,
                user=USERNAME,
                password=PASSWORD,
                autocommit=True,
            )
            server_info = connection.get_server_info()
            print("MySQL connection is open on", server_info)
        except mysql.connector.Error as err:
            print("MySQL connector error:", str(err))
        finally:
            pass
    try:
        # html_title = html_title.encode(encoding='utf-8')
        # plain_text = plain_text.encode(encoding='utf-8')
        sql_statement = (
            "INSERT INTO `webpage` (`url`, `title`, `content`) VALUES ('%s', '%s', '%s')"
            % (url, html_title.replace("'", '"'), plain_text.replace("'", '"'))
        )
        cursor = connection.cursor()
        cursor.execute(sql_statement)
        if cursor.rowcount == 0:
            return connection
        sql_last_id = "SET @last_webpage_id = LAST_INSERT_ID()"
        cursor.execute(sql_last_id)
        cursor.close()
        webpage_count = webpage_count + 1
        return analyze_keyword(connection, plain_text)
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
    finally:
        pass
    return connection


def analyze_keyword(connection, plain_text):
    global webpage_count
    global keyword_array
    new_keyword = {}
    old_keyword = {}
    tokenize_list = tokenize(plain_text)
    for keyword in tokenize_list:
        if keyword.isascii() and keyword.isalnum():
            keyword = keyword.lower()
            if keyword not in keyword_array:
                keyword_array.append(keyword)
                new_keyword[keyword] = 1
            else:
                if new_keyword.get(keyword) is not None:
                    new_keyword[keyword] = new_keyword[keyword] + 1
                else:
                    if old_keyword.get(keyword) is None:
                        old_keyword[keyword] = 1
                    else:
                        old_keyword[keyword] = old_keyword[keyword] + 1
    for keyword in new_keyword.keys():
        done = False
        while not done:
            try:
                if not connection.is_connected():
                    time.sleep(30)
                    connection = mysql.connector.connect(
                        host=HOSTNAME,
                        database=DATABASE,
                        user=USERNAME,
                        password=PASSWORD,
                        autocommit=True,
                    )
                    server_info = connection.get_server_info()
                    print("MySQL connection is open on", server_info)
                    sql_last_id = "SET @last_webpage_id = %d" % webpage_count
                    cursor = connection.cursor()
                    cursor.execute(sql_last_id)
                # keyword = keyword.encode(encoding='utf-8')
                sql_statement = "INSERT INTO `keyword` (`name`) VALUES ('%s')" % keyword
                cursor = connection.cursor()
                cursor.execute(sql_statement)
                if cursor.rowcount == 0:
                    keyword_array.remove(keyword)
                    continue
                sql_last_id = "SET @last_keyword_id = LAST_INSERT_ID()"
                cursor.execute(sql_last_id)
                sql_statement = (
                    "INSERT INTO `occurrence` (`webpage_id`, `keyword_id`, `counter`, `pagerank`) "
                    "VALUES (@last_webpage_id, @last_keyword_id, %d, 0.0)"
                    % new_keyword[keyword]
                )
                cursor.execute(sql_statement)
                cursor.close()
                done = True
            except mysql.connector.Error as err:
                print("MySQL connector error:", str(err))
            finally:
                pass
    for keyword in old_keyword.keys():
        done = False
        while not done:
            try:
                if not connection.is_connected():
                    time.sleep(30)
                    connection = mysql.connector.connect(
                        host=HOSTNAME,
                        database=DATABASE,
                        user=USERNAME,
                        password=PASSWORD,
                        autocommit=True,
                    )
                    server_info = connection.get_server_info()
                    print("MySQL connection is open on", server_info)
                    sql_last_id = "SET @last_webpage_id = %d" % webpage_count
                    cursor = connection.cursor()
                    cursor.execute(sql_last_id)
                sql_last_id = (
                    "SET @last_keyword_id = (SELECT `keyword_id` FROM `keyword` WHERE `name` = '%s')"
                    % keyword
                )
                cursor = connection.cursor()
                cursor.execute(sql_last_id)
                sql_statement = (
                    "INSERT INTO `occurrence` (`webpage_id`, `keyword_id`, `counter`, `pagerank`) "
                    "VALUES (@last_webpage_id, @last_keyword_id, %d, 0.0)"
                    % old_keyword[keyword]
                )
                cursor.execute(sql_statement)
                cursor.close()
                done = True
            except mysql.connector.Error as err:
                print("MySQL connector error:", str(err))
            finally:
                pass
    return connection


def data_mining():
    records = None
    connection = None
    rowcount = 0
    try:
        connection = mysql.connector.connect(
            host=HOSTNAME,
            database=DATABASE,
            user=USERNAME,
            password=PASSWORD,
            autocommit=True,
        )
        server_info = connection.get_server_info()
        print("MySQL connection is open on", server_info)
        sql_select_query = "SELECT * FROM `keyword` ORDER BY `keyword_id`"
        cursor = connection.cursor()
        cursor.execute(sql_select_query)
        # get all records
        records = cursor.fetchall()
        print("Total number of rows in table:", cursor.rowcount)
        rowcount = cursor.rowcount
        cursor.close()
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
    finally:
        pass
    for row in records:
        done = False
        while not done:
            try:
                if not connection.is_connected():
                    time.sleep(30)
                    connection = mysql.connector.connect(
                        host=HOSTNAME,
                        database=DATABASE,
                        user=USERNAME,
                        password=PASSWORD,
                        autocommit=True,
                    )
                    server_info = connection.get_server_info()
                    print("MySQL connection is open on", server_info)
                data_update = connection.cursor()
                sql_update_query = (
                    "UPDATE `occurrence` INNER JOIN `keyword` USING(`keyword_id`)"
                    "SET `pagerank` = data_mining(`webpage_id`, `name`) WHERE `name` = '%s'"
                    % row[1]
                )
                print(
                    "Applying data mining for '%s'... [%d/%d]"
                    % (row[1], records.index(row) + 1, rowcount)
                )
                data_update.execute(sql_update_query)
                data_update.close()
                done = True
            except mysql.connector.Error as err:
                print("MySQL connector error:", str(err))
            finally:
                pass
    try:
        if connection.is_connected():
            connection.close()
            print("MySQL connection is now closed")
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
    finally:
        pass


add_url_to_frontier("https://en.wikipedia.org/")
if create_database():
    web_search_engine()
