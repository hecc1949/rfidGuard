#!/usr/bin/env node

var express = require('express');
var app = express();
//var fs = require("fs");
var path = require("path");
var WebSocket = require('faye-websocket');

var QWebChannel = require('./qwebchannel.js').QWebChannel;
var devwrapper, wsClient;

//提供http服务
app.use(express.static(path.join(__dirname,'/view')));

app.get('*', function(req, res) {
    res.status(404)
    res.send("找不到资源");
});
var server = app.listen(2280, function(req, res)  {
    console.log("nodejs server running @2280.");
});

//为前端提供websocket server
var wsServer  =  null;
server.on('upgrade', function(request, socket, body)   {
    if (WebSocket.isWebSocket(request)) {
        wsServer = new WebSocket(request, socket, body);      
        wsServer.on('message', function(event) {
            console.log("Receive WS message:"+ event.data);
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
        console.log("wsClient: Client connected");
        var transport = {
            send: function(data) {
                wsClient.send(data)
            }
        };
    
        new QWebChannel(transport, function(channel) {
            devwrapper = channel.objects.devwrapper;
            if (devwrapper === undefined)    {
                console.log("Device Wrapper undefined");
                return;
            }
            
            //挂接devwrapper对象的属性/事件/方法，方法可全局执行
            devwrapper.onDeviceEvent.connect(function(jo)   {       //event
                console.log("Device Event:" + JSON.stringify(jo));
                if (wsServer)   {
                    wsServer.send("DevEvent:" + jo.data);
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
        console.log('Webchannel Connection error: ' + error.message);
    });
    wsClient.on('close', function() {
        console.log('Webchannel Connection closed.');
        setTimeout(linkWebchannel, 2000);	//重新连接
    });    
}

linkWebchannel();

