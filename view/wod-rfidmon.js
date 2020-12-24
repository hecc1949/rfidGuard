/**
  * UHF-RFID 图书标签门禁读卡
  *
  */

 $.extend($.fn.textbox.methods, {
	setBorder: function(jq, border) {
		var style = $('#easyui-textbox-border');
		if (!style.length)  {
			$('head').append(
					'<style id="easyui-textbox-border">' +
					'.textbox-noborder{border-color:transparent;border-radius:0}' +
					'</style>'
			);
		}
		return jq.each(function(){
			var span = $(this).next();
			if (border) {
				span.removeClass('textbox-noborder');
			} else {
				span.addClass('textbox-noborder');
			}
		});
	}
});

var serverStartTime;

$(function generateClock()  {
    $("#genClock").textbox('textbox').css({fontSize: "1.8em", fontWeight:"bold", backgroup: 'transparent',
                    color:"blue", textAlign:'center'});
    $("#runedTimes").textbox('setBorder', false);
//	var begTime = new Date();
	setInterval(function()	{
		let dt = new Date();
		let hours = dt.getHours();
		let minutes = dt.getMinutes();
		let seconds = dt.getSeconds();
		if(hours<10)    {
			hours = '0'+hours;
		}
		if(minutes<10)  {
			minutes = '0'+minutes;
		}
		if(seconds<10)  {
			seconds = '0'+seconds;
		}
		$("#genClock").textbox('setValue', hours+':'+minutes+':'+seconds);

		if (serverStartTime)	{
//			let diff = (dt.getTime() - begTime.getTime())/1000;
			let diff = (dt.getTime() - serverStartTime.getTime())/1000;
			let days = parseInt(diff / (24*60*60));
			diff = diff - days*(24*60*60);
			hours = parseInt(diff / (60*60));
			diff = diff - hours*(60*60);
			minutes = parseInt(diff /60);
			seconds = parseInt(diff - minutes*60);
			if (seconds ===0)	{
				$("#runedTimes").textbox('setValue',"已运行"+ days +"天"+ hours + "小时" + minutes + "分钟");
			}	
		}
	}, 1000);		
});

 //websocket-client功能
 $(function()    {
    var socket = new window.WebSocket('ws://' + location.hostname + ':' + location.port + '/');
    socket.addEventListener('open', function() {
        console.log("websocket client opened");
        socket.send('Hello sever');
    });
    socket.onerror = function(event) {        
        console.log('websocket error: ' + event.message);
    };

    socket.onmessage = function(event) {
//        var msg = $("#prompt").textbox("getValue") + event.data + '\n';         //用$().val()取值会丢掉换行符
//        $("#prompt").textbox("setValue", msg);
    };

    socket.onclose = function(event) {
        console.log('websocket close: ' + event.code + ', ' + event.reason);
    };
 });

 