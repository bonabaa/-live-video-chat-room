﻿ function onSourceDownloadProgressChanged(sender, eventArgs) {
            sender.findName("uxStatus").Text = "Loading: " + Math.round((eventArgs.progress * 1000)) / 10 + "%";
            sender.findName("uxProgressBar").ScaleY = eventArgs.progress * 356;
        }
 
function onsourcedownloadcomplete(sender, eventArgs) {
     sender.findName("uxStatus").Text= "已经完成！";
     alert("已经完成！");
 } 