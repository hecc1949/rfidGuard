/**
  * UHF-RFID 图书标签门禁读卡
  *
  */


var wssocket = null;
var _clientId = -1;

var serverStartTime;

 //websocket-client功能
 $(function socketClient()    {
    wssocket = new window.WebSocket('ws://' + location.hostname + ':' + location.port + '/');
    wssocket.addEventListener('open', function() {
        setTimeout(function()   {
            wsSendCommand({'command':'getSysInfo','param':[]});
        }, 150);
    });
    wssocket.onerror = function(event) {
        console.log('websocket error: ' + event.message);
    };

    //websocket接收消息处理
    wssocket.onmessage = function(msg) {
        var jo = JSON.parse(msg.data);
        if (jo.event === "gotSysInfos") {       //对open时发出的getSysInfo命令的响应
            $("#configSettings").propertygrid("reload", {});
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
                    $('#netStatus').textbox({buttonText:'有线网络', buttonIcon:'icon-connect-on'});
                }   else if (obj.value === 2)    {
                    $('#netStatus').textbox({buttonText:'Wifi',buttonIcon:'icon-connect-wifi'});
                }   else    {
                    $('#netStatus').textbox({buttonText:'离线', buttonIcon:'icon-connect-off'});
                }
            }
            obj = $.grep(jo.param, function(obj) {return obj.name==='guardStatus';})[0];
            if (obj)    {
                stateView.setGuardStatusView(obj.value);
            }
            //
            obj = $.grep(jo.param, function(obj) {return obj.name==='clientCount';})[0];
            if (obj)    {
                var cid = parseInt(obj.value);
                if (_clientId <0)    {
                    _clientId = cid;
                    if (_clientId >2 || _clientId===1)    {
                        $("#sysClose").linkbutton("disable");
                    }
                }
            }
        }
        else if (jo.event === "tick")   {
            var dt = new Date(jo.param[0].time);
            if (!serverStartTime)  {
                serverStartTime = dt;
            }
            displayRuntime(dt);

            if (jo.param[0].guardStatus !== undefined)  {
                stateView.setGuardStatusView(jo.param[0].guardStatus);
            }
            stateView.tickforDevActionView();   //自动清除扫描状态。仅后备作用
        }
        else if (jo.event === "passcardUpd")    {
            $("#passcards").datalist("load");   //刷新显示
        }
        else if (jo.event === "tagCheckAcc")    {
            stateView.setDevActionView(2);
            if ($('#tabsWorksheet').tabs('getTabIndex', $('#tabsWorksheet').tabs('getSelected')) ===1)  {
                $("#tabCheckEvents").datagrid("reload");    //刷新当前页显示
            }
        }
        else if (jo.event === "alarmAct")    {
            if ($('#tabsWorksheet').tabs('getTabIndex', $('#tabsWorksheet').tabs('getSelected')) ===0)  {
                $("#tabAlarms").datagrid("reload");
            }
            if (jo.param[0].guardStatus ===3)
                $("#stopAlarm").linkbutton("enable");
            else
                $("#stopAlarm").linkbutton("disable");
            stateView.setGuardStatusView(jo.param[0].guardStatus);
        }
        else if (jo.event === "controlAct")    {
            stateView.setDevActionView(3);
            if ($('#tabsWorksheet').tabs('getTabIndex', $('#tabsWorksheet').tabs('getSelected')) ===1)  {
                $("#tabCheckEvents").datagrid("reload");    //刷新当前页显示
            }
        }
        else if (jo.event === "devMsg")  {
            if (jo.param[0].tagFinderRun !== undefined)   {
                if (jo.param[0].tagFinderRun === true) {
                    stateView.setDevActionView(1);
                } else  {
                    stateView.setDevActionView(0);
                }
            }   else if (jo.param[0].scanMode !== undefined)   {
                var v1 = parseInt(jo.param[0].scanMode) + 1;
                stateView.setDevScanModeView(v1);
            }   else if (jo.param[0].scanTick !== undefined)  {
                stateView.tickforDevActionView(parseInt(jo.param[0].scanTick));
            }
        }
    };
    wssocket.onclose = function(event) {
        wssocket = null;
    };
 });

/*
//---------------------------------
var st_devScaning = false, st_devScanMode = 0;
var n_scantickRefreshHold = 0;

function setGuardStatusView(val)   {
    if (val ===0)   {
        $("#guardStatus").textbox('setValue','撤防状态');
        $("#guardStatus").textbox('textbox').css({background:'#b8eecf', color:'#45872c'});
    }   else if (val === 1) {
        $("#guardStatus").textbox('setValue','布防中');
        $("#guardStatus").textbox('textbox').css({background:'#ffd7d7', color:'#c65353'});
    }   else if (val ===2)  {
        $("#guardStatus").textbox('setValue','允许通行');
        $("#guardStatus").textbox('textbox').css({background:'rgb(111,251,138)', color:'#c65353'});
    }   else if (val ===3) {
        $("#guardStatus").textbox('setValue','报警');
        $("#guardStatus").textbox('textbox').css({background:'rgb(243,64,12)', color:'rgb(247,228,64)'});
    }   else {
        $("#guardStatus").textbox('setValue','未知状态');
        $("#guardStatus").textbox('textbox').css({background:'black', color:'white'});
    }
}

function setDevActionView(val, param)  {
    if (val ===0)   {
        st_devScaning = false;
        $("#deviceAction").textbox('setValue','待机状态');
        $("#deviceAction").textbox('textbox').css({background:'#b8eecf', color:'#45872c'});
    }   else if (val ===1)  {
        st_devScaning = true;
        if (n_scantickRefreshHold >0)  {
            n_scantickRefreshHold--;
        }   else    {
            var st = '扫描中';
            if (st_devScanMode ===1)
                st = st + "S";
            else if (st_devScanMode ===2)
                st = st + "M";
            if (param !== undefined) {
                if (param % 2 ==0)
                    st = st + " # ";
                else
                    st = st + " + ";
            }

            $("#deviceAction").textbox('setValue',st);
            $("#deviceAction").textbox('textbox').css({background:'rgb(68,85,242)', color:'rgb(247,228,9)'});
        }
    }   else if (val ===2)  {
        n_scantickRefreshHold = 4;
        $("#deviceAction").textbox('setValue','发现标签');
        $("#deviceAction").textbox('textbox').css({background:'rgb(22,22,255)', color:'rgb(255,0,128)'});
    }   else if (val ===3)  {
        n_scantickRefreshHold = 4;
        $("#deviceAction").textbox('setValue','控制动作');
        $("#deviceAction").textbox('textbox').css({background:'rgb(122,122,255)', color:'rgb(247,228,9)'});
    }
}

function tickforDevActionView(tick)  {
    if (tick !== undefined)    {
//        st_devScaning = true;
        setDevActionView(1, tick);
    }   else    {
        if (st_devScaning === true) {       //自动清除扫描状态。仅后备作用
             if (++n_scantickRefreshHold >6)    {
                 st_devScaning = false;
                 setDevActionView(0);
             }
         }
    }
}

function setDevScanModeView(v1)  {
    if (st_devScanMode !== v1 || !st_devScaning)  {
        st_devScaning = true;
        st_devScanMode = v1;
        setDevActionView(1);
    }
}
//---------------------------------
*/
$(function viewInit()   {
    //runing clock
    $("#genClock").textbox('textbox').css({fontSize: "1.8em", fontWeight:"bold", backgroup: 'transparent',
                    color:"blue", textAlign:'center'});
    $("#runedTimes").textbox('setBorder', false);

    $("#guardStatus").textbox('textbox').css({fontSize: "1.2em", fontWeight:"bold", textAlign:'center'});
    $("#deviceAction").textbox('textbox').css({fontSize: "1.2em", fontWeight:"bold",textAlign:'center',
                                                  background:'#b8eecf', color:'#45872c'});

    //datagrid的分页布置
    $("#tabAlarms, #tabCheckEvents").each(function()    {
        $(this).datagrid('getPager').pagination({
            layout: ['sep','first','prev','links','next','last','sep','refresh','info']
        });
    });

})

$(function viewCtrl()  {
    $('#tabsWorksheet').tabs({
        onSelect: function(title, index) {
            if (index===1)  {
                $("#tabCheckEvents").datagrid("load");    //刷新全部页显示
            } else if (index===0)    {
                $("#tabAlarms").datagrid("load");
            }
        }
    });

    $("#ntpUpdate").linkbutton({
        onClick: function() {
            sendCommand({"command":"ntpUpdate"}, function(res) {
                $.messager.alert('执行命令', "命令已发送执行",'info');
            });
        }
    });
    $("#sysClose").linkbutton({
        onClick: function() {
            $.messager.prompt('关机', '请输入工作密码：', function(r){
                if (r){
                    var param = [];
                    param.push(r);
                    param.push(_clientId);
                    sendCommand({'command':'sysClose', 'param': JSON.stringify(param) }, function(dat) {
                        if (dat.result === true) {
//                            console.log("sysclosing..");
                            window.open('about:blank','_self').close();     //这个方法还是有warning的，勉强能用
                        }
                    });
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
            sendCommand({"command":"clearPasscard"});
        }
    });
    $("#btnEventsClear").linkbutton({
        onClick: function() {
            sendCommand({"command":"clearReports"}, function(dat) {
                    $("#tabCheckEvents").datagrid("load");
                    $.messager.alert('执行命令', "命令已发送执行, 删除 "+dat.count+" 个文件",'info');
                });
        }
    });
    $("#btnEventsReport").linkbutton({
        onClick: function() {
            sendCommand({"command":"reportEvents"}, function(dat) {
                    var a = document.createElement("a");
                    a.href = 'http://' + location.hostname + ':' + location.port + '/downloadreports?filename=' + dat.filename;
                    a.click();
                    $("#tabCheckEvents").datagrid("getPanel").focus();
                });
        }
    });
    $("#btnAlarmReport").linkbutton({
        onClick: function() {
            sendCommand({"command":"reportAlarms"}, function(dat) {
                    var a = document.createElement("a");
                    a.href = 'http://' + location.hostname + ':' + location.port + '/downloadreports?filename=' + dat.filename;
                    a.click();
                    $("#tabAlarms").datagrid("getPanel").focus();
                });
        }
    });
    $("#configApply").linkbutton({
        onClick: function() {
            var rows = $("#configSettings").propertygrid('getChanges');
            var comprows = [];
            for(var i=0; i<rows.length; i++)    {
                var sitem = {"prgId": rows[i].prgId, "value": rows[i].value };
                comprows.push(sitem);
            }
            if (comprows.length>0)  {
                $.messager.prompt('修改配置', '请输入工作密码：', function(r){
                    if (r){
                        comprows.push({"prgId":"usingpasswd", "value":r});
                        sendCommand({"command":"updateConfigs", "param": JSON.stringify(comprows)}, function(dat) {
                            if (dat.result === true)  {
                                $.messager.alert('执行命令', "命令已发送执行",'info');
                            }   else    {
                                $.messager.alert('错误', "命令执行出错:"+ dat.error,'error');
                            }
                            wsSendCommand({'command':'getSysInfo','param':[]});
                        });
                    }
                });
            }
        }
    });

    // ----------- 以下用websocket client控制
    $("#stopAlarm").linkbutton({
        disabled: true,
        onClick: function() {
            wsSendCommand({"command":"stopAlarm"});
        }
    });
    $("#checkoutOpen").linkbutton({
        onClick: function() {
            var times = $("#checkoutTimes").combobox('getValue');
            wsSendCommand({"command":"checkoutOpen", "param":[times]});
            $("#checkoutTimes").combobox('textbox').focus();
        }
    });
});

