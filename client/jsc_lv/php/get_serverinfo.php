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
   
  	$query = mysql_query("SELECT * FROM yserverinfo"  );
   
  	 
    
    //start json object
    $json = "["; 
     
    //loop through and return results
  for ($x = 0; $x < mysql_num_rows($query); $x++) {
    $row = mysql_fetch_assoc($query);
         
        //continue json object
    $json .= "{\"type\":\""     . $row["type"] 
    . "\",\"display_text\":\""     . $row["display_text"]  . "\"" 
    . ",\"web_link\":\""     . $row["web_link"] . "\"" 
    . ",\"click_open\":\""  . $row["click_open"]  . "\"" 
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
