<?php
/**
 * Copyright(C) 2018 Smx
 */
function readHeraClass($fp){
	$limit = 32; //unverified

	$fileClass = "";
	
	for($read=0, $pcnt=0; $read < $limit && !feof($fp) && $pcnt < 4; $read++){
		$ch = fgetc($fp);
		if($ch == '%'){ //%%%% marks end of fileClass name
			$pcnt++;
			continue;
		}

		$fileClass .= $ch;
	}

	if($read == $limit){
		fseek($fp, -$limit, SEEK_CUR);
		return NULL;
	}

	while(!feof($fp)){
		$ch = fgetc($fp);
		if($ch == "\t")
			break;
	}
	if(feof($fp)){
		fprintf(STDERR, "[ERROR] couldn't find the header end marker\n");
		return NULL;
	}

	// Class includes newlines
	return rtrim($fileClass);
}

if($argc < 2){
	fprintf(STDERR, "Usage: %s [hera file]\n");
	return 1;
}

$fp = fopen($argv[1], "rb") or die("fopen() failed for {$argv[1]}\n");

$hclass = readHeraClass($fp);
if(is_null($hclass)){
	fprintf(STDERR, "[SKIP] %s\n", $argv[1]);
	fclose($fp);
	return 1;
}

printf("%s\n\t=> %s\n\n", $argv[1], $hclass);

fclose($fp);
return 0;
?>