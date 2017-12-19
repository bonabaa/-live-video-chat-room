<?php
     
  //connection information
  $host = "localhost";
  $user = "chy";
  $password = "137807";
  $database = "cc";
  $uid= 0;//"$_GET["gid"]) ;
  $uid=$_GET["uid"];
    //make connection
  $server = mysql_connect($host, $user, $password);
  $connection = mysql_select_db($database, $server);
     
    //query the database
  $query = mysql_query("set names utf8");
  //if (strlen($gid)>0 )
  	$query = mysql_query("SELECT * FROM user_info WHERE userid =". $uid .")"   );
  //else
  	//$query = mysql_query("SELECT * FROM mcc_session"  );
    
    //start json object
    $json = "["; 
     
    //loop through and return results
  for ($x = 0; $x < mysql_num_rows($query); $x++) {
    $row = mysql_fetch_assoc($query);
         
        //continue json object
    $json .= "{\"usercomment\":\""     . $row["usercomment"] 
    . "\",\"username\":\""     . $row["alias"]  
    . "\",\"userid\":\""     . $row["userid"] . "\"" 
    . "}" ;
         
        //add comma if not last row, closing brackets if is
        if ($x < mysql_num_rows($query) -1)
            $json .= ",";
        //else
        //    $json .= "]";
  }
      $json .= "]";
    //return JSON with GET for JSONP callback
    $response = $_GET["callback"] . $json;
    //$sql ="SELECT * FROM user_info WHERE userid IN ( SELECT user_id FROM mcc_participant WHERE SESSION_id= ". uid .")" ;
    echo $response  ;
 
    //close connection
    mysql_close($server);
 
?>
