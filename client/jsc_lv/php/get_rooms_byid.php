<?php
     
  //connection information
  $host = "localhost";
  $user = "chy";
  $password = "137807";
  $database = "cc";
  $gid= "100004";//"$_GET["gid"]) ;
  $gid=$_GET["gid"];
  $gname="";
  $gname=$_GET["gname"];
    //make connection
  $server = mysql_connect($host, $user, $password);
  $connection = mysql_select_db($database, $server);
     
    //query the database
  $query = mysql_query("set names utf8");
  if (strlen($gid)>0 )
  	$query = mysql_query("SELECT * FROM mcc_session where id=" . $gid);
  else if (strlen($gname)>0)
  	$query = mysql_query("SELECT * FROM mcc_session  WHERE session_name LIKE '%" . $gname . "%' or id like '%" . $gname ."%'");
  else
  	$query = mysql_query("SELECT * FROM mcc_session"  );
    
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
    . ",\"opened\":\""     . $row["no_video"] . "\"" 
    . ",\"description\":\""     . str_replace("\"","\\\"",$row["description"])  . "\"" 
    . ",\"maxofconf\":"  . $row["max_video_in_conf"] 
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
    echo $response;
 
    //close connection
    mysql_close($server);
 
?>
