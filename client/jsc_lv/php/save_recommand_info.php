<?php
     
  //connection information
  $host = "localhost";
  $user = "chy";
  $password = "137807";
  $database = "cc";
  $gid= "0";//"$_GET["gid"]) ;
  $gid=$_POST["id"];
  $groom_id="";
  $groom_id=$_POST["groom_id"];
 
  
  $server = mysql_connect($host, $user, $password);
  $connection = mysql_select_db($database, $server);
     
    //query the database
  $query = mysql_query("set names utf8");
  
    
  mysql_query("INSERT INTO ymcc_room_recommand (room_id, recommandpeople) VALUES (" . $groom_id ."," . $gid . ")");

    //return JSON with GET for JSONP callback
   // $response =  "Name=" . $groom_name . "Pass=" . $groom_pass. "groom_des=" . $groom_des ;
   // echo $response;
 
    //close connection
    mysql_close($server);
 
?>
