<?php
//include_once "auth.php";     
  //connection information
  $host = "localhost";
  $user = "chy";
  $password = "137807";
  $database = "cc";
  $gid= "100004";//"$_GET["gid"]) ;
  $gid=$_GET["gid"];
    //make connection
  $server = mysql_connect($host, $user, $password);
  $connection = mysql_select_db($database, $server);
     
    //query the database
  $query = mysql_query("set names utf8");
  $queryA =  mysql_query("SELECT  room_id, COUNT(room_id) AS t1 FROM ymcc_room_recommand   GROUP BY room_id ORDER BY t1 desc limit 6");
	$json = "[";  
  for ($y = 0; $y < mysql_num_rows($queryA); $y++) {
   	$rowA = mysql_fetch_assoc( $queryA );
  	$query = mysql_query(" SELECT * FROM mcc_session WHERE id =". $rowA["room_id"] );
    //start json object
    
     
    //loop through and return results
		  for ($x = 0; $x < mysql_num_rows($query); $x++) {
		    $row = mysql_fetch_assoc($query);
		         
		        //continue json object
		    $json .= "{\"session_id\":\""     . $row["session_id"] 
		    . "\",\"session_name\":\""     . $row["session_name"]  
		    . "\",\"id\":\""     . $row["id"] . "\"" 
		    . ",\"online\":"     . $row["send_invitation"] . "" 
		    . ",\"avatar\":\""     . $row["session_key"] . "\"" 
		    . ",\"password\":\""     . $row["password"] . "\"" 
		    . ",\"description\":\""     . str_replace("\"","\\\"",$row["description"])  . "\"" 
		    . ",\"maxofconf\":"  . $row["max_video_in_conf"] 
		    . "}" ;
		         
		        //add comma if not last row, closing brackets if is
		        if ($y < mysql_num_rows($queryA) -1)
		            $json .= ",";
		        //else
		        //    $json .= "]";
		  }
		}
      $json .= "]";
    //return JSON with GET for JSONP callback
    $response = $_GET["callback"] . $json;
    echo $response;
 
    //close connection
    mysql_close($server);
 
?>
