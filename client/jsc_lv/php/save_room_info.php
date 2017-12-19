<?php
     
  //connection information
  $host = "localhost";
  $user = "chy";
  $password = "137807";
  $database = "cc";
  $gid= "0";//"$_GET["gid"]) ;
  $gid=$_POST["id"];
  $groom_pass="";
  $groom_pass=$_POST["room_pass"];
  $groom_name="";
  $groom_name=$_POST["room_name"];  
  $groom_des="";
  $groom_des=$_POST["room_des"];    //make connection
  $groom_open="";
  $groom_open=$_POST["opened"];    //make connection
  
  $server = mysql_connect($host, $user, $password);
  $connection = mysql_select_db($database, $server);
     
    //query the database
  $query = mysql_query("set names utf8");
  
    
  mysql_query("UPDATE mcc_session SET description = '". addslashes($groom_des) ."',session_name='" . $groom_name ."', password='" . $groom_pass. "',no_video=" . $groom_open. " WHERE id =" . $gid);

    //return JSON with GET for JSONP callback
    $response =  "Name=" . $groom_name . "Pass=" . $groom_pass. "groom_des=" . $groom_des ;
    echo $response;
 
    //close connection
    mysql_close($server);
 
?>
