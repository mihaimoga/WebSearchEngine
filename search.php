<?php
/* This file is part of Web Search Engine application developed by Mihai MOGA.

Web Search Engine is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Open
Source Initiative, either version 3 of the License, or any later version.

Web Search Engine is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Web Search Engine. If not, see <http://www.opensource.org/licenses/gpl-3.0.html>*/

$servername = "localhost";
$username = "r46882text_engine";
$password = "TextMining2021!@#$";
$dbname = "r46882text_mining";

echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "\t<head>\n";
echo "\t\t<title>" . $_GET['q'] . "</title>\n";
echo "\t\t<meta charset=\"utf-8\">\n";
echo "\t\t<link rel=\"icon\" type=\"image/png\" href=\"https://www.mihaimoga.com/images/romania-flag-square-icon-256.png\">\n";
echo "\t\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
echo "\t\t<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.0/css/bootstrap.min.css\">\n";
echo "\t\t<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.0/jquery.min.js\"></script>\n";
echo "\t\t<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.0/js/bootstrap.min.js\"></script>\n";
echo "\t</head>\n";
echo "\t<body>\n";
$search = strtolower($_GET['q']);
$counter = 0;
$mysql_clause = "";
$mysql_select = "";
$token = strtok($search, "\t\n\r\"\' !?#$%&|(){}[]*/+-:;<>=.,");
while ($token !== false) {
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
	        echo "<div class=\"container-fluid\">" . $row["webpage_id"] . ". <b>" . $row["title"] . "</b> Score: " . $row["score"] . "<br />";
	        echo "<a href=\"" . $row["url"] . "\">" . $row["url"] . "</a><br />";
	        echo "<i>" . utf8_encode(substr($row["content"], 0, 1024)) . "</i></div><br />\n";
	    }
	} else {
	    echo "0 results";
	}
	mysqli_close($conn);
}
echo "\t</body>\n";
echo "</html>\n";
?>
