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

webpage_count = 0


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
        sql_drop_table = "DROP TABLE IF EXISTS `frontier`"
        cursor.execute(sql_drop_table)
        sql_create_table = (
            "CREATE TABLE `frontier` (`url_id` BIGINT NOT NULL AUTO_INCREMENT, "
            "`address` VARCHAR(256) NOT NULL, `visited` BOOL NOT NULL, "
            "`score` BIGINT NOT NULL, PRIMARY KEY(`url_id`)) ENGINE=InnoDB "
            "CHARACTER SET utf8 COLLATE utf8_general_ci"
        )
        cursor.execute(sql_create_table)
        sql_create_index = (
            "CREATE UNIQUE INDEX frontier_address ON `frontier`(`address`)"
        )
        cursor.execute(sql_create_index)
        sql_create_table = (
            "CREATE TABLE `webpage` (`webpage_id` BIGINT NOT NULL AUTO_INCREMENT, "
            "`url` VARCHAR(256) NOT NULL, `title` VARCHAR(256) NOT NULL, "
            "`content` LONGTEXT NOT NULL, PRIMARY KEY(`webpage_id`)) ENGINE=InnoDB "
            "CHARACTER SET utf8 COLLATE utf8_general_ci"
        )
        cursor.execute(sql_create_table)
        sql_create_table = (
            "CREATE TABLE `keyword` (`keyword_id` BIGINT NOT NULL AUTO_INCREMENT, "
            "`name` VARCHAR(256) NOT NULL, PRIMARY KEY(`keyword_id`)) ENGINE=InnoDB "
            "CHARACTER SET utf8 COLLATE utf8_general_ci"
        )
        cursor.execute(sql_create_table)
        sql_create_table = (
            "CREATE TABLE `occurrence` (`webpage_id` BIGINT NOT NULL, "
            "`keyword_id` BIGINT NOT NULL, `counter` BIGINT NOT NULL, "
            "`pagerank` REAL NOT NULL, PRIMARY KEY(`webpage_id`, `keyword_id`), "
            "FOREIGN KEY webpage_fk(webpage_id) REFERENCES webpage(webpage_id), "
            "FOREIGN KEY keyword_fk(keyword_id) REFERENCES keyword(keyword_id)) "
            "ENGINE=InnoDB CHARACTER SET utf8 COLLATE utf8_general_ci"
        )
        cursor.execute(sql_create_table)
        sql_create_index = "CREATE UNIQUE INDEX index_name ON `keyword`(`name`)"
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


def add_url_to_frontier(connection, url):
    if url.find("#") > 0:
        url = url.split("#")[0]
    if len(url) > 256:
        return False
    if url.endswith(".3g2"):
        return False  # 3GPP2 multimedia file
    if url.endswith(".3gp"):
        return False  # 3GPP multimedia file
    if url.endswith(".7z"):
        return False  # 7-Zip compressed file
    if url.endswith(".ai"):
        return False  # Adobe Illustrator file
    if url.endswith(".apk"):
        return False  # Android package file
    if url.endswith(".arj"):
        return False  # ARJ compressed file
    if url.endswith(".aif"):
        return False  # AIF audio file
    if url.endswith(".avi"):
        return False  # AVI file
    if url.endswith(".bat"):
        return False  # Batch file
    if url.endswith(".bin"):
        return False  # Binary disc image
    if url.endswith(".bmp"):
        return False  # Bitmap image
    if url.endswith(".cda"):
        return False  # CD audio track file
    if url.endswith(".com"):
        return False  # MS-DOS command file
    if url.endswith(".csv"):
        return False  # Comma separated value file
    if url.endswith(".dat"):
        return False  # Binary Data file
    if url.endswith(".db") or url.endswith(".dbf"):
        return False  # Database file
    if url.endswith(".deb"):
        return False  # Debian software package file
    if url.endswith(".dmg"):
        return False  # macOS X disk image
    if url.endswith(".doc") or url.endswith(".docx"):
        return False  # Microsoft Word Open XML document file
    if url.endswith(".email") or url.endswith(".eml"):
        return False  # E-mail message file from multiple e-mail clients
    if url.endswith(".emlx"):
        return False  # Apple Mail e-mail file
    if url.endswith(".exe"):
        return False  # MS-DOS executable file
    if url.endswith(".flv"):
        return False  # Adobe Flash file
    if url.endswith(".fon"):
        return False  # Generic font file
    if url.endswith(".fnt"):
        return False  # Windows font file
    if url.endswith(".gadget"):
        return False  # Windows gadget
    if url.endswith(".gif"):
        return False  # GIF image
    if url.endswith(".h264"):
        return False  # H.264 video file
    if url.endswith(".ico"):
        return False  # Icon file
    if url.endswith(".iso"):
        return False  # ISO disc image
    if url.endswith(".jar"):
        return False  # Java archive file
    if url.endswith(".jpg") or url.endswith(".jpeg"):
        return False  # JPEG image
    if url.endswith(".log"):
        return False  # Log file
    if url.endswith(".m4v"):
        return False  # Apple MP4 video file
    if url.endswith(".mdb"):
        return False  # Microsoft Access database file
    if url.endswith(".mid") or url.endswith(".midi"):
        return False  # MIDI audio file
    if url.endswith(".mov"):
        return False  # Apple QuickTime movie file
    if url.endswith(".mp3") or url.endswith(".mpa"):
        return False  # MP3 audio file
    if url.endswith(".mp4"):
        return False  # MPEG4 video file
    if url.endswith(".mpa"):
        return False  # MPEG-2 audio file
    if url.endswith(".mpg") or url.endswith(".mpeg"):
        return False  # MPEG video file
    if url.endswith(".msg"):
        return False  # Microsoft Outlook e-mail message file
    if url.endswith(".msi"):
        return False  # Windows installer package
    if url.endswith(".odt"):
        return False  # OpenOffice Writer document file
    if url.endswith(".ods"):
        return False  # OpenOffice Calc spreadsheet file
    if url.endswith(".oft"):
        return False  # Microsoft Outlook e-mail template file
    if url.endswith(".ogg"):
        return False  # Ogg Vorbis audio file
    if url.endswith(".ost"):
        return False  # Microsoft Outlook e-mail storage file
    if url.endswith(".otf"):
        return False  # Open type font file
    if url.endswith(".pkg"):
        return False  # Package file
    if url.endswith(".pdf"):
        return False  # Adobe PDF file
    if url.endswith(".png"):
        return False  # PNG image
    if url.endswith(".ppt") or url.endswith(".pptx"):
        return False  # Microsoft PowerPoint Open XML presentation
    if url.endswith(".ps"):
        return False  # PostScript file
    if url.endswith(".psd"):
        return False  # PSD image
    if url.endswith(".pst"):
        return False  # Microsoft Outlook e-mail storage file
    if url.endswith(".rar"):
        return False  # RAR file
    if url.endswith(".rpm"):
        return False  # Red Hat Package Manager
    if url.endswith(".rtf"):
        return False  # Rich Text Format file
    if url.endswith(".sql"):
        return False  # SQL database file
    if url.endswith(".svg"):
        return False  # Scalable Vector Graphics file
    if url.endswith(".swf"):
        return False  # Shockwave flash file
    if url.endswith(".xls") or url.endswith(".xlsx"):
        return False  # Microsoft Excel Open XML spreadsheet file
    if url.endswith(".toast"):
        return False  # Toast disc image
    if url.endswith(".tar"):
        return False  # Linux tarball file archive
    if url.endswith(".tar.gz"):
        return False  # Tarball compressed file
    if url.endswith(".tex"):
        return False  # A LaTeX document file
    if url.endswith(".ttf"):
        return False  # TrueType font file
    if url.endswith(".txt"):
        return False  # Plain text file
    if url.endswith(".tif") or url.endswith(".tiff"):
        return False  # TIFF image
    if url.endswith(".vcd"):
        return False  # Virtual CD
    if url.endswith(".vcf"):
        return False  # E-mail contact file
    if url.endswith(".vob"):
        return False  # DVD Video Object
    if url.endswith(".xml"):
        return False  # XML file
    if url.endswith(".wav") or url.endswith(".wma"):
        return False  # WAV file
    if url.endswith(".wmv"):
        return False  # Windows Media Video file
    if url.endswith(".wpd"):
        return False  # WordPerfect document
    if url.endswith(".wpl"):
        return False  # Windows Media Player playlist
    if url.endswith(".wsf"):
        return False  # Windows script file
    if url.endswith(".z") or url.endswith(".zip"):
        return False  # Z or Zip compressed file
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
        sql_statement = (
            "UPDATE `frontier` SET `score` = `score` + 1 WHERE `address` = '%s'" % url
        )
        cursor = connection.cursor()
        cursor.execute(sql_statement)
        if cursor.rowcount == 0:
            sql_statement = (
                "INSERT INTO `frontier` (`address`,`visited`, `score`) VALUES ('%s', FALSE, 1)"
                % url
            )
            cursor.execute(sql_statement)
        cursor.close()
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
        return False
    finally:
        pass
    return True


def extract_url_from_frontier(connection):
    url = None
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
        sql_statement = "SELECT `address` FROM `frontier` WHERE `visited` = FALSE ORDER BY `score` DESC, `url_id` ASC LIMIT 1"
        cursor = connection.cursor()
        cursor.execute(sql_statement)
        records = cursor.fetchall()
        for row in records:
            url = row[0]
            sql_statement = (
                "UPDATE `frontier` SET `visited` = TRUE WHERE `address` = '%s'" % url
            )
            cursor.execute(sql_statement)
        cursor.close()
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
    finally:
        pass
    return connection, url


def download_page_from_url(connection, url):
    html_title = None
    plain_text = None
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
        req = Request(url)
        html_page = urlopen(req)
        soup = BeautifulSoup(html_page, "html.parser")
        html_title = soup.title.get_text().strip()
        plain_text = soup.get_text().strip()
        plain_text = " ".join(plain_text.split())
        for hyperlink in soup.find_all("a"):
            hyperlink = urljoin(url, hyperlink.get("href"))
            add_url_to_frontier(connection, hyperlink)
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
    except urllib.error.URLError as err:
        print(str(err))
    except urllib.error.HTTPError as err:
        print(str(err))
    except urllib.error.ContentTooShortError as err:
        print(str(err))
    finally:
        return connection, html_title, plain_text


def get_webpage_count(connection):
    counter = -1
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
        sql_last_id = "SELECT COUNT(`webpage_id`) FROM `webpage`"
        cursor = connection.cursor()
        cursor.execute(sql_last_id)
        records = cursor.fetchone()
        counter = records[0]
        cursor.close()
    except mysql.connector.Error as err:
        print("MySQL connector error:", str(err))
        return -1
    finally:
        pass
    return counter


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
        webpage_count = get_webpage_count(connection)
        print("get_webpage_count = %d" % webpage_count)
        add_url_to_frontier(connection, "https://en.wikipedia.org/")
        while True:
            connection, url = extract_url_from_frontier(connection)
            if url:
                print("Crawling %s... [%d]" % (url, webpage_count + 1))
                connection, html_title, plain_text = download_page_from_url(connection, url)
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
    keyword_count = {}
    tokenize_list = tokenize(plain_text)
    for keyword in tokenize_list:
        if keyword.isascii() and keyword.isalnum():
            keyword = keyword.lower()
            if keyword_count.get(keyword) is None:
                keyword_count[keyword] = 1
            else:
                keyword_count[keyword] = keyword_count[keyword] + 1
    for keyword in keyword_count.keys():
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
                sql_statement = (
                    "SELECT `keyword_id` FROM `keyword` WHERE `name` = '%s'" % keyword
                )
                cursor = connection.cursor()
                cursor.execute(sql_statement)
                records = cursor.fetchone()
                if cursor.rowcount == 0:
                    sql_statement = (
                        "INSERT INTO `keyword` (`name`) VALUES ('%s')" % keyword
                    )
                    cursor.execute(sql_statement)
                    sql_last_id = "SET @last_keyword_id = LAST_INSERT_ID()"
                    cursor.execute(sql_last_id)
                else:
                    sql_last_id = "SET @last_keyword_id = %d" % records[0]
                    cursor.execute(sql_last_id)
                sql_statement = (
                    "INSERT INTO `occurrence` (`webpage_id`, `keyword_id`, `counter`, `pagerank`) VALUES "
                    "(@last_webpage_id, @last_keyword_id, %d, 0.0)"
                    % keyword_count[keyword]
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


if create_database():
    web_search_engine()
