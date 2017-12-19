<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" >
<?php
$ccid=$_GET['cc_id'];
?>

<script type="text/javascript"> 
   var ToH=550; 
   var ToW=272; 
function winResize(){ 
	if(window.navigator.appName=="Netscape"){ //firefox 
       window.innerHeight=ToH; 
       window.innerWidth=ToW; 
    }else{//ie 
       var cWinwidth=window.document.documentElement.clientWidth;
       var cWinheight = window.document.documentElement.clientHeight;

        window.resizeBy(ToW-cWinwidth,ToH-cWinheight);
       cWinwidth=window.document.documentElement.clientWidth; 
       cWinheight=window.document.documentElement.clientHeight; 
       window.resizeBy(ToW-cWinwidth,ToH-cWinheight); 
       self.moveTo(screen.availWidth/3 *2,0) 
   } 
		//yCtrlIE(1);
}
</script> 
<!-- saved from url=(0014)about:internet -->
<head>
    <title>JSC Start</title>
    <style type="text/css">
    html, body {
	    height: 100%;
	    overflow: auto;
    }
    body {
	    padding: 0;
	    margin: 0;
    }
    #silverlightControlHost {
	    height: 100%;
	    text-align:center;
    }
    </style>
    
  
</head>
<body>
	 
           <object  id="yycore1"  visible="false" CLASSID="CLSID:74E59815-8639-4E51-99A1-47F1A57160D9" 
            width="0%" height="0%" ID="Object1" VIEWASTEXT>
<?php

           echo "<param name=\"cc_id\" value=\"$ccid\" />"
?>             
            <param name="HtmlAccessEnabled" value="true" />
            <div class="background" style="margin: 0px; padding: 0px;">
                <div class="container">
                    <!-- header -->
                    <div class="yymeeting_logo">
                        <a href="http://www.yymeeting.com"><span>yymeeting.com</span></a>
                    </div>
                    <!--end header -->
                    <!-- info -->
                    <div class="info_box">
                        <p>
                            Welcome to the <strong>CCVIsion</strong>.<br />
                             you need to <a target="_blank" href="http://www.yymeeting.com/CC_installer.exe">
                                install CCVision Plugin</a>.
                        </p>
                        <p class="font_size_18">
                            <strong>Don¡¯t worry, it¡¯s easy and only takes a second</strong>. <span class="span_tx1">
                                or</span> <span class="span_tx2">Go to yymeeting.com <a href="http://www.yymeeting.com">
                                    Home Page</a></span> <span class="span_tx2">Go to <a href="http://www.yymeeting.com/silverlight">
                                        </a></span>
                        </p>
                    </div>
                    <!-- end info -->
                </div>
            </div>
        </object>

</body>
 
</html>
