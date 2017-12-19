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
     //前10个是按在线率 的房间，后10个是按消费排行的房间s
    //query the database
  $query = mysql_query("set names utf8");
  
  $query = mysql_query("SELECT * FROM mcc_session ORDER BY send_invitation DESC LIMIT 10");
  //   else
  //	$query = mysql_query("SELECT user_info.userid AS t1,user_info.alias_ext AS t2, ygifts.name  AS t3, ygifts.image AS t4, ygift_ranking_week.money AS t5  FROM user_info, ygifts, ygift_ranking_week WHERE user_info.userid= ygift_ranking_week.userid AND ygifts.id= ygift_ranking_week.gift_id ORDER BY money DESC LIMIT 6"  );
  	 
    
    //start json object
    $json = "["; 
     
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
        //if ($x < mysql_num_rows($query) -1)
            $json .= ",";
        //else
        //    $json .= "]";
  }//后10个是按消费排行的房间s
 
   $query =  mysql_query("SELECT  room_which_used, SUM(COUNT* ygifts.price ) AS t1,mcc_session.*  FROM ygift_consume_detail ,ygifts ,mcc_session WHERE mcc_session.id= room_which_used GROUP BY room_which_used ORDER BY t1 DESC LIMIT 10");
	 
   for ($x = 0; $x < mysql_num_rows($query); $x++) {
    $row = mysql_fetch_assoc($query);
         
        //continue json object
    $json .= "{\"session_id\":\""     . $row["session_id"] 
    . "\",\"session_name\":\""     . $row["session_name"]  
    . "\",\"id\":\""     . $row["id"] . "\"" 
    . ",\"online\":"     . $row["t1"] . "" 
    . ",\"avatar\":\""     . $row["session_key"] . "\"" 
    . ",\"password\":\""     . $row["password"] . "\"" 
    . ",\"description\":\""     . str_replace("\"","\\\"",$row["description"])  . "\"" 
    . ",\"maxofconf\":"  . $row["max_video_in_conf"] 
    . "}" ;
         
        //add comma if not last row, closing brackets if is
        if ($x < mysql_num_rows($query) -1)
            $json .= ",";
        //else
        //    $json .= "]";
  }
  
  ///end
  
     
      $json .= "]";
    //return JSON with GET for JSONP callback
    $response =  $json;
    echo $response;
 
    //close connection
    mysql_close($server);
 
?>
