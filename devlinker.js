"use strict";

/*
  websocket & webchannel updown
*/

var WebSocket = require('faye-websocket');
var QWebChannel = require('./qwebchannel.js').QWebChannel;
var devwrapper = null, wcClient;

var wsServers = [];
var clientCount = 0;

//为前端提供websocket server
var wsServerCreate = function(server, onWsServerMessage, onStart)   {
    server.on('upgrade', function(request, socket, body)   {
        if (WebSocket.isWebSocket(request)) {
            var id = clientCount;
            wsServers[id] = new WebSocket(request, socket, body);
            wsServers[id].on('message', function(event) {
                var cmd = JSON.parse(event.data);
                onWsServerMessage(cmd);
            });
            console.log("websocket server created."+clientCount);

            wsServers[id].on('close', function(event) {
                console.log('websocket client close', event.code, event.reason);
                if (clientCount>0)
                    clientCount--;
                if (clientCount===0)
                    wsServers[id] = null;
            });
            clientCount++;
            if (onStart !== undefined && clientCount===1)  {
                onStart();
            }
        }
    });
}

//
function linkWebchannel(onDeviceEvents)   {
    if (devwrapper !== null)
        return;
    wcClient = new WebSocket.Client('ws://127.0.0.1:2285/');
    wcClient.on('open', function(event) {
        var transport = {
            send: function(data) {
                wcClient.send(data)
            }
        };

        var channel_1 = new QWebChannel(transport, function(channel) {
            devwrapper = channel.objects.devwrapper;
            if (devwrapper === undefined)    {
                console.log("Device Wrapper undefined");    //会出现!
                return;
            }
            console.log("webchannel linked.");
            //挂接devwrapper对象的属性/事件/方法，方法可全局执行
            devwrapper.onDeviceEvent.connect(onDeviceEvents);
        });

        wcClient.on('message', function(event) {
            transport.onmessage(event);
        });
    });

    wcClient.on('error', function(error) {
        console.log('Webchannel error: ' + error.message);
    });
    wcClient.on('close', function() {
        console.log('Webchannel close.');
    });
}

var doDevCommand = function(cmd, param, callback) {
    if (devwrapper !== null)    {
        if (callback !== undefined)
            devwrapper.doCommand(cmd, param, callback);
        else
            devwrapper.doCommand(cmd, param);
    }
}

var wsSeverNotify = function(jo)    {
    for(var id=0; id<wsServers.length; id++)    {
        if (wsServers[id] !== null)   {
            wsServers[id].send(JSON.stringify(jo));
        }
    }
}

function getClientCount()   {
    return(clientCount);
}

module.exports =    {
    wsServerCreate: wsServerCreate,
    wsSeverNotify: wsSeverNotify,
    getClientCount: getClientCount,

    linkWebchannel: linkWebchannel,
    doDevCommand: doDevCommand
}
