//#!/usr/bin/env node
var express = require('express');
var app = express();
var fs = require("fs");
var path = require("path");
//var bodyparser = require('body-parser');
var WebSocket = require('faye-websocket');

var QWebChannel = require('./qwebchannel.js').QWebChannel;
var devwrapper, wsClient;

var sysInfo = [{"name":"startTime", "value":new Date()}];

var schtimer = require('./schemetimer.js');

schtimer.schemeTimerStart(function(jo) {
    if (wsServer)   {
        wsServer.send(JSON.stringify(jo));
    }
});

//提供http服务,静态路由
app.use(express.static(path.join(__dirname,'/view')));

//app.use(bodyparser.json());     //接受json数据类型
//app.use(bodyparser.urlencoded({extended: false}));      //解析post请求数据

app.get("/configs_json", function(request, res) {
    var jodat = JSON.parse(fs.readFileSync("./view/configs.json"));
    //用从webchannel读取的"主机名"/"版本号"替换文件中的值
    for(var i=0; i<sysInfo.length; i++ )    {
        for(var j=0; j<jodat.rows.length; j++)  {
            if (jodat.rows[j].name === sysInfo[i].name)    {
                jodat.rows[j].value = sysInfo[i].value;
                break;
            }
        }
    }
    schtimer.schemeCompile(jodat.rows);
    jodat.total = jodat.rows.length;    //
    res.send(JSON.stringify(jodat));
});

var passCards = [];
app.get("/passcards", function(req, res) {
    var len = passCards.length;
    var jodat = [];
    for(var i=0; i<len; i++)    {
        jodat.push({'text':passCards[i]});
    }
    res.send(JSON.stringify(jodat));
});

app.get("/commands", function(req, res) {
    var param = [];
    if (req.query.param)    {
        param = JSON.parse(req.query.param);
    }
    if (req.query.command ==="ntpUpdate" && devwrapper) {
        devwrapper.doCommand(req.query.command, param, function(jores)   {
            res.send(JSON.stringify({"result": jores.result}));
        });
    }   else if (req.query.command==="sysClose" && devwrapper) {
        if (param.length>0 && param[0] ==="12345") {
            devwrapper.doCommand(req.query.command, param, function(jores)   {
                res.send(JSON.stringify({"result": jores.result}));
            });
        }   else    {
            res.send(JSON.stringify({"result":"false"}));
        }
    }   else    {
        res.send(JSON.stringify({"result":"false"}));
    }
});

var server = app.listen(2280, function(req, res)  {
    console.log("nodejs server running @2280.");
});

app.get('*', function(req, res) {
//    console.log("nodejs not found resources:"+req.url);
    res.status(404)
    res.send("找不到资源");
});

//为前端提供websocket server
var wsServer  =  null;
server.on('upgrade', function(request, socket, body)   {
    if (WebSocket.isWebSocket(request)) {
        wsServer = new WebSocket(request, socket, body);      
        wsServer.on('message', function(event) {
            var cmd = JSON.parse(event.data);
            if (cmd.command === 'getSysInfo') {
                getSysInfos();
            }
            else if (cmd.command === "addPasscard") {
                passCards.push(cmd.param);
                wsServer.send(JSON.stringify({"event": "passcardUpd", "param":[]}));
            }
            else if (cmd.command === "clearPasscard") {
                passCards.splice(0, passCards.length);
                wsServer.send(JSON.stringify({"event": "passcardUpd", "param":[]}));
            }
            else if (cmd.command === "stopAlarm")    {
                console.log("cmd:" + cmd.command);
            }
            else if (cmd.command === "checkoutOpen") {
                console.log("cmd:" + cmd.command+" -parm:"+cmd.param[0]);

            }
        });
        wsServer.on('close', function(event) {
            console.log('close', event.code, event.reason);
            wsServer = null;
        });
    }
});

//
function linkWebchannel()   {
    wsClient = new WebSocket.Client('ws://127.0.0.1:2285/');
    wsClient.on('open', function(event) {
        var transport = {
            send: function(data) {
                wsClient.send(data)
            }
        };
    
        var channel_1 = new QWebChannel(transport, function(channel) {
            devwrapper = channel.objects.devwrapper;
            console.log("linked to webchannel devwrapper.");
            if (devwrapper === undefined)    {
                console.log("Device Wrapper undefined");
                return;
            }

            //挂接devwrapper对象的属性/事件/方法，方法可全局执行
            devwrapper.onDeviceEvent.connect(function(jo)   {       //event
                if (wsServer)   {
                    wsServer.send(JSON.stringify(jo));
                }
            });
            devwrapper.doCommand("startDev", [], function(jores)   {   //method
                console.log("StartDev Command result:"+ jores.result);
            });       
        });
    
        wsClient.on('message', function(event) {
            transport.onmessage(event);
        });
    });
    
    wsClient.on('error', function(error) {
//        console.log('Webchannel Connection error: ' + error.message);
    });
    wsClient.on('close', function() {
//        console.log('Webchannel Connection closed.');
        setTimeout(linkWebchannel, 2000);	//重新连接
    });    
}
linkWebchannel();

function getSysInfos()  {
    if (!devwrapper)
        return;
//    devwrapper.getSysInfo(function(res)  {
    devwrapper.doCommand("getSysInfo", [], function(res)  {
        if (sysInfo.length >1)
            sysInfo.splice(1, sysInfo.length-1);    //清除旧的，除了startTime外
        if (res.result !==true  || res.data.length<1)
            return;
//        res.forEach(function(item)  {
        res.data.forEach(function(item)  {
            sysInfo.push(item);
        });
        if (wsServer)   {
            var jo = {"event": "getSysInfos", "param":sysInfo };
            wsServer.send(JSON.stringify(jo));
/*            var jo = {"event": "getSysInfos", "param":[] };
            jo.param.push(sysInfo[0]);      //starttime

            var obj = res.find(obj=> obj.name ==='netStatus');
            if (obj)
                jo.param.push(obj);
            obj = res.find(obj=> obj.name ==='IP');
            if (obj)
                jo.param.push(obj);
            wsServer.send(JSON.stringify(jo));
*/
        }
    });
}

