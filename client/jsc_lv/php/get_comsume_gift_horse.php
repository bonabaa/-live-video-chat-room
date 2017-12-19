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
   
  	$query = mysql_query("SELECT (SELECT alias_ext FROM user_info WHERE people_who_received= userid) AS people_who_received_alias, (SELECT alias_ext FROM user_info WHERE people_who_used= userid) AS people_who_used_alias,ygift_consume_detail.* FROM ygift_consume_horse,ygift_consume_detail WHERE ygift_consume_horse.gift_comsume_id=  ygift_consume_detail.id LIMIT 20");
  	
  	 
    
    //start json object
    $json = "["; 
     
    //loop through and return results
  for ($x = 0; $x < mysql_num_rows($query); $x++) {
    $row = mysql_fetch_assoc($query);
         //giftid, people_who_used, room_which_used,time_used
        //continue json object
    $json .= "{\"giftid\":\""     . $row["giftid"] 
    . "\",\"people_who_used\":\""     . $row["people_who_used"]   
    . "\",\"people_who_received\":\""     . $row["people_who_received"]   
    . "\",\"people_who_used_alias\":\""     . $row["people_who_used_alias"]   
    . "\",\"people_who_received_alias\":\""     . $row["people_who_received_alias"]   
    . "\",\"room_which_used\":\""     . $row["room_which_used"]  
    . "\",\"time_used\":\""     . $row["time_used"]  . "\"" 
     . ",\"count\":"      . $row["count"]    
    . "}" ;
         
        //add comma if not last row, closing brackets if is
        if ($x < mysql_num_rows($query) -1)
            $json .= ",";
        //else
        //    $json .= "]";
  }
      $json .= "]";
    //return JSON with GET for JSONP callback
    $response =  $json;
    echo $response;
 
    //close connection
    mysql_close($server);
 
?>
