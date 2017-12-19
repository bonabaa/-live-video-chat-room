<?php
     
  //connection information
  $host = "localhost";
  $user = "chy";
  $password = "137807";
  $database = "cc";
  //string data = string.Format("userid={0}&room_id={1}&gift_id{2}&gift_count={3}&gift_peer={4}"
  $userid= "0";//"$_GET["gid"]) ;
  $userid=$_POST["userid"];
  $room_id="";
  $room_id=$_POST["room_id"];
  $gift_id="";
  $gift_id=$_POST["gift_id"];
  $gift_count=1;
  $gift_count=$_POST["gift_count"];
  $gift_peer="";
  $gift_peer=$_POST["gift_peer"];
 
  $url="http://60.190.113.55:5301/callback?x=sync&sync=self&uid=". $userid ;
  $server = mysql_connect($host, $user, $password);
  $connection = mysql_select_db($database, $server);
     
    //query the database
  $query = mysql_query("set names utf8");
  
   //save to ygift_consume_detail
   mysql_query("INSERT INTO ygift_consume_detail   VALUES ('" . $gift_id ."','" . $userid ."','" . $room_id ."', now(),'" . $gift_peer ."'," . $gift_count ." )");
   //cut money for comsumer
   mysql_query("UPDATE user_info SET occupation =  occupation- ( SELECT (SELECT price FROM ygifts WHERE id=".  $gift_id . ") AS t1)* ". $gift_count ." where userid=" . $userid);
	 
 
	 // add money for peer
   mysql_query("UPDATE user_info SET occupation =  occupation+ ( SELECT (SELECT receiver_monney FROM ygifts WHERE id=".  $gift_id . ") AS t1)  where userid=" . $userid);
	
    file_get_contents($url);
     $response =  "<?xml version='1.0'?><r to_id=\"" . $gift_peer . "\" gift_id=\"" .  $gift_id ."\" count=\"" . $gift_count ."\"  ></r>";
    //$response= file_get_contents($url);
     echo $response;
 
    //close connection
    mysql_close($server);
 
?>
