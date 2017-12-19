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
  if ($gid=='1')
  	$query = mysql_query("SELECT user_info.userid AS t1,user_info.alias_ext AS t2, ygifts.name  AS t3, ygifts.image AS t4, ygift_ranking_today.money AS t5  FROM user_info, ygifts, ygift_ranking_today WHERE user_info.userid= ygift_ranking_today.userid AND ygifts.id= ygift_ranking_today.gift_id ORDER BY money DESC LIMIT 6"  );
   else
  	$query = mysql_query("SELECT user_info.userid AS t1,user_info.alias_ext AS t2, ygifts.name  AS t3, ygifts.image AS t4, ygift_ranking_week.money AS t5  FROM user_info, ygifts, ygift_ranking_week WHERE user_info.userid= ygift_ranking_week.userid AND ygifts.id= ygift_ranking_week.gift_id ORDER BY money DESC LIMIT 6"  );
  	 
    
    //start json object
    $json = "["; 
     
    //loop through and return results
  for ($x = 0; $x < mysql_num_rows($query); $x++) {
    $row = mysql_fetch_assoc($query);
         
        //continue json object
    $json .= "{\"userid\":\""     . $row["t1"]   . "\"" 
    . ",\"alias\":\""     . $row["t2"]  . "\"" 
    . ",\"gift_name\":\""     . $row["t3"] . "\"" 
    . ",\"gift_img\":\""     . $row["t4"]  . "\"" 
    . ",\"money\":"    . $row["t5"]   
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
