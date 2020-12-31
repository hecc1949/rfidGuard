/**
  * UHF-RFID 图书标签门禁读卡
  *
  */


var serverStartTime;
var wssocket = null;

 //websocket-client功能
 $(function()    {
//    var socket = new window.WebSocket('ws://' + location.hostname + ':' + location.port + '/');
    wssocket = new window.WebSocket('ws://' + location.hostname + ':' + location.port + '/');
    wssocket.addEventListener('open', function() {
//        wssocket.send('clientStart');
        wsSendCommand({'command':'getSysInfo','param':[]});
    });
    wssocket.onerror = function(event) {
        console.log('websocket error: ' + event.message);
    };

    //websocket接收消息处理
    wssocket.onmessage = function(msg) {
        var jo = JSON.parse(msg.data);
        if (jo.event === "getSysInfos") {
            $("#configSettings").propertygrid("reload", {});
//#            var obj = jo.param.find(obj=> obj.name ==='startTime');
            var obj = $.grep(jo.param, function(obj) {return obj.name==='startTime';})[0];
            if (obj && !serverStartTime)    {
                serverStartTime = new Date(obj.value);
            }
            obj = $.grep(jo.param, function(obj) {return obj.name==='IP';})[0];
            if (obj)    {
                $("#netStatus").textbox('setValue', obj.value);
            }
            obj = $.grep(jo.param, function(obj) {return obj.name==='netStatus';})[0];
            if (obj)    {
                if (obj.value === 1) {
                    $('#netStatus').textbox({buttonText:'有线网络', buttonIcon:'icon-man'});
                }   else if (obj.value === 2)    {
                    $('#netStatus').textbox({buttonText:'Wifi',buttonIcon:'icon-man'});
                }   else    {
                    $('#netStatus').textbox({buttonText:'离线', buttonIcon:'icon-man'});
                }
            }
        }
        else if (jo.event === "tick")   {
            var dt = new Date(jo.param[1].value);
            if (!serverStartTime)  {
                serverStartTime = dt;
            }
            displayRuntime(dt);
        }
        else if (jo.event === "passcardUpd")    {
            $("#passcards").datalist("load");   //刷新显示
        }
    };

    wssocket.onclose = function(event) {
//#        console.log('websocket close: ' + event.code + ', ' + event.reason);
        wssocket = null;
    };
 });


function wsSendCommand(jo)  {
    if (wssocket && wssocket !== null)  {
        wssocket.send(JSON.stringify(jo));
    }
}

function sendCommand(jo, callback)    {
    $.ajax({
        url: "/commands",
        type: "get",
        dataType: 'json',
        data: jo,
        success:function(dat)   {
            if (dat.result === true) {
                if (callback)
                    callback(dat.result);
            }   else    {
                $.messager.alert("错误", "命令执行失败: 权限或参数错误",'error');
            }
        },
        error: function(err)    {
            $.messager.alert("错误", "命令发送失败:" + err);
        }
    });
}

function sendPasscard()   {
    var card = $("#addPasscard").val();     //不带回车
    if (card.length >3) {
        var jo = {"command":"addPasscard", "param":[]};
        jo.param.push(card);
        wsSendCommand(jo);
    }
    $("#addPasscard").textbox('setValue', '');
//    $("#passcards").datalist("load");   //刷新显示
}

$(function viewCtrl()  {
    //runing clock
    $("#genClock").textbox('textbox').css({fontSize: "1.8em", fontWeight:"bold", backgroup: 'transparent',
                    color:"blue", textAlign:'center'});
    $("#runedTimes").textbox('setBorder', false);

    //
    $("#ntpUpdate").linkbutton({
        onClick: function() {
            sendCommand({"command":"ntpUpdate"}, function(res) {
                $.messager.alert('执行命令', "命令已发送执行",'info');
            });
        }
    });
    $("#sysClose").linkbutton({
        onClick: function() {
            $.messager.prompt('关机', '请输入关机密码：', function(r){
                if (r){
                    var param = [];
                    param.push(r);
                    sendCommand({'command':'sysClose', 'param': JSON.stringify(param) });
                }
            });
        }
    });

    $("#addPasscard").textbox({ onClickButton:sendPasscard    });
    $("#addPasscard").textbox('textbox').bind('keydown', function(e) {
        if (e.keyCode === 13)   {	// 当按下回车键时接受输入的值。
            sendPasscard();
        }
    });
    $("#clearPasscard").linkbutton({
        onClick: function() {
            wsSendCommand({"command":"clearPasscard"});
//            $("#passcards").datalist("load");
        }
    });

    $("#stopAlarm").linkbutton({
        onClick: function() {
            wsSendCommand({"command":"stopAlarm"});
        }
    });
    $("#checkoutOpen").linkbutton({
        onClick: function() {
            var times = $("#checkoutTimes").combobox('getValue');
            wsSendCommand({"command":"checkoutOpen", "param":[times]});
        }
    });

});
