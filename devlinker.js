"use strict";

/*
  websocket & webchannel updown
*/

var WebSocket = require('faye-websocket');
var QWebChannel = require('./qwebchannel.js').QWebChannel;
var devwrapper = null, wcClient;

var wsServer  =  null;
var clientCount = 0;

//为前端提供websocket server
var wsServerCreate = function(server, onWsServerMessage, onStart)   {
    server.on('upgrade', function(request, socket, body)   {
        if (WebSocket.isWebSocket(request)) {
            wsServer = new WebSocket(request, socket, body);
            wsServer.on('message', function(event) {
                var cmd = JSON.parse(event.data);
                onWsServerMessage(cmd);
            });
//            console.log("websocket server created.");
            wsServer.on('close', function(event) {
                console.log('close', event.code, event.reason);
                if (clientCount>0)
                    clientCount--;
                if (clientCount===0)
                    wsServer = null;
            });
            clientCount++;
            if (onStart !== undefined && clientCount===1)  {
                onStart();
            }
        }
    });
}

//
//var reLinkTimer;
//var reLinkCount = 5;
//var autoReLink = true;
//var tmpEventWorker;
function linkWebchannel(onDeviceEvents)   {
    if (devwrapper !== null)
        return;
    wcClient = new WebSocket.Client('ws://127.0.0.1:2285/');
//    tmpEventWorker = onDeviceEvents;
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

/*        if (reLinkCount>0)  {
            reLinkCount = 0;
            clearTimeout(reLinkTimer);
        } */
    });

    wcClient.on('error', function(error) {
        console.log('Webchannel error: ' + error.message);
    });
    wcClient.on('close', function() {
        console.log('Webchannel close.');
/*        if (reLinkCount >0) {
            reLinkTimer = setTimeout(linkWebchannel(tmpEventWorker), 2000);   //重新连接
            console.log("Webchannel re-link:" + reLinkCount);
            reLinkCount--;
        }   else    {
            console.log('Webchannel closed, process exit');
            clearTimeout(reLinkTimer);
            process.exit(1);
        }
*/

/*
        setTimeout(function()   {
            if (autoReLink)     linkWebchannel();
        }, 2000);	//重新连接
*/
    });
}
//linkWebchannel();

var doDevCommand = function(cmd, param, callback) {
    if (devwrapper !== null)    {
        if (callback !== undefined)
            devwrapper.doCommand(cmd, param, callback);
        else
            devwrapper.doCommand(cmd, param);
    }
}

var wsSeverNotify = function(jo)    {
    if (wsServer !== null)   {
        wsServer.send(JSON.stringify(jo));
    }
}

function getClientCount()   {
    return(clientCount);
}

/*
function stopAutoRelink()   {
//    clearTimeout(reLinkTimer);
    autoReLink = false;
}
*/

module.exports =    {
    wsServerCreate: wsServerCreate,
    wsSeverNotify: wsSeverNotify,
    getClientCount: getClientCount,

    linkWebchannel: linkWebchannel,
//    stopAutoRelink: stopAutoRelink,
    doDevCommand: doDevCommand
}
