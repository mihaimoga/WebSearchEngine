CREATE UNIQUE INDEX index_name ON `keyword`(`name`);

DELIMITER //
CREATE OR REPLACE FUNCTION no_of_words(token VARCHAR(256)) RETURNS REAL READS SQL DATA
BEGIN
	DECLARE retVal REAL;
	SELECT MAX(`counter`) INTO retVal FROM `occurrence` INNER JOIN `keyword` USING(`keyword_id`) WHERE `name` = token;
 	RETURN retVal;
END//
DELIMITER ;

DELIMITER //
CREATE OR REPLACE FUNCTION no_of_pages(token VARCHAR(256)) RETURNS REAL READS SQL DATA
BEGIN
	DECLARE retVal REAL;
	SELECT COUNT(`webpage_id`) INTO retVal FROM `occurrence` INNER JOIN `keyword` USING(`keyword_id`) WHERE `name` = token;
 	RETURN retVal;
END//
DELIMITER ;

DELIMITER //
CREATE OR REPLACE FUNCTION total_pages() RETURNS REAL READS SQL DATA
BEGIN
	DECLARE retVal REAL;
	SELECT COUNT(`webpage_id`) INTO retVal FROM `webpage`;
 	RETURN retVal;
END//
DELIMITER ;

DELIMITER //
CREATE OR REPLACE FUNCTION data_mining(webpage_no BIGINT, token VARCHAR(256)) RETURNS REAL READS SQL DATA
BEGIN
	DECLARE retVal REAL;
	SELECT SUM(`counter`)/no_of_words(token)*LOG((1+total_pages())/no_of_pages(token)) INTO retVal FROM `occurrence` INNER JOIN `keyword` USING(`keyword_id`) WHERE `name` = token AND `webpage_id` = webpage_no;
 	RETURN retVal;
END//
DELIMITER ;
