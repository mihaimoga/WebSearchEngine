<?php
/* This file is part of WebSearchEngine application developed by Stefan-Mihai MOGA.

WebSearchEngine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

WebSearchEngine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
WebSearchEngine.  If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

$servername = "localhost";
$username = "r46882text_engine";
$password = "TextMining2021!@#$";
$dbname = "r46882text_mining";


function content_index($content, $keyword) {
	$pos = -1;
	foreach ($keyword as $key => $value) {
		$tmp = stripos($content, $value);
		$pos = ($pos < 0) ? $tmp : min($pos, $tmp);
	}
	return $pos;
}


echo "<!DOCTYPE html>\n";
echo "<html lang=\"en\">\n";
echo "\t<head>\n";
echo "\t\t<title>" . $_GET['q'] . "</title>\n";
echo "\t\t<meta charset=\"utf-8\">\n";
echo "\t\t<link rel=\"icon\" type=\"image/png\" href=\"romania-flag-square-icon-256.png\">\n";
echo "\t\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
echo "\t\t<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-T3c6CoIi6uLrA9TneNEoa7RxnatzjcDSCmG1MXxSR1GAsXEV/Dwwykc2MPK8M2HN\" crossorigin=\"anonymous\">\n";
echo "\t</head>\n";
echo "\t<body>\n";
echo "\t\t<div class=\"container\">\n";
$search = strtolower($_GET['q']);
$counter = 0;
$mysql_clause = "";
$mysql_select = "";
// $token_find = array();
// $token_replace = array();
$token = strtok($search, "\t\n\r\"\' !?#$%&|(){}[]*/+-:;<>=.,");
while ($token !== false) {
	// array_push($token_find, $token);
	// array_push($token_replace, "<em>" . $token . "</em>");
	if ($counter == 0) {
		$mysql_clause = "SELECT DISTINCT `webpage_id` FROM `occurrence` INNER JOIN `keyword` USING (`keyword_id`) WHERE `name` = '$token'";
		$mysql_select = "(`name` = '$token')";
	}
	else {
		$mysql_clause = "SELECT DISTINCT `webpage_id` FROM `occurrence` INNER JOIN `keyword` USING (`keyword_id`) WHERE `name` = '$token' AND `webpage_id` IN (" . $mysql_clause . ")";
		$mysql_select = $mysql_select . " OR (`name` = '$token')";
	}
	$counter++;
	$token = strtok("\t\n\r\"\' !?#$%&|(){}[]*/+-:;<>=.,");
};
if ($counter > 0)
{
	// Create connection
	$conn = mysqli_connect($servername, $username, $password, $dbname);
	// Check connection
	if (!$conn) {
	    die("Connection failed: " . mysqli_connect_error());
	}

	$statement = "SELECT DISTINCT `webpage_id`, `title`, `url`, `content`, AVG(`pagerank`) AS score FROM `occurrence` INNER JOIN `webpage` USING(`webpage_id`) INNER JOIN `keyword` USING(`keyword_id`) WHERE  `webpage_id` IN (" . $mysql_clause . ") AND (" . $mysql_select . ") GROUP BY `webpage_id` ORDER BY score DESC LIMIT 100;";
	$result = mysqli_query($conn, $statement);
	if (mysqli_num_rows($result) > 0) {
	    // output data of each row
	    while($row = mysqli_fetch_assoc($result)) {
	        echo "\t\t\t<div class=\"container-fluid\">" . $row["webpage_id"] . ". <strong>" . $row["title"] . "</strong> Score: " . $row["score"] . "<br>";
	        echo "<a href=\"" . $row["url"] . "\">" . $row["url"] . "</a><br>";
	        echo "<i>" . utf8_encode(substr($row["content"], 0, 1024)) . "</i></div><br>\n";
	        // echo str_ireplace($token_find, $token_replace, substr($row["content"], content_index($row["content"], $token_find), 1024)) . "</div><br>\n";
	    }
	} else {
	    echo "0 results";
	}
	mysqli_close($conn);
}
echo "\t\t</div>\n";
echo "\t\t<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-C6RzsynM9kWDrMNeT87bh95OGNyZPhcTNXj1NW7RuBCsyN/o0jlpcV8Qyq46cDfL\" crossorigin=\"anonymous\"></script>\n";
echo "\t</body>\n";
echo "</html>\n";
?>
