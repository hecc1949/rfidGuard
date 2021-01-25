//#!/usr/bin/env node
var express = require('express');
var app = express();
var fs = require("fs");
var path = require("path");
//var bodyparser = require('body-parser');

var sysInfo = [{"name":"startTime", "value":new Date()}];
var _upassword = "12345";

var schtimer = require('./schemetimer.js');
var devcon = require("./devlinker.js");
var reports = require("./reports.js");

var guardStatus = 0;        //0-未布防, 1-布防中， 2-人工放行(限时))， 3-报警
var _guardState = {"idle":0, "watch":1, "checkout":2, "alert":3  };
var sch_securityAval = 0;       //产生报警的(安全位)值

var lastFoundTagTime = new Date();

var passCards = [];     //obj:{"tagId":"xx", "ucount":0,"utime":dt }
var runingEvents = [], alarmRecords = [];

schtimer.schemeTimerStart(function(jo) {
    //scheme timer tick callback
    var n_guardStatus = guardStatus;
    if (guardStatus !== jo.schguardStatus && guardStatus !==_guardState.alert) {
        if (guardStatus===_guardState.idle)    {    //空闲状态，在client已连接时可启动监控
            if (devcon.getClientCount() ===1)   {
                setTimeout(function()   {
                    runTagFinder(true);
                    devcon.doDevCommand("setGpioOut",[0, 1]);
                }, 100);   //启动监控
                n_guardStatus = jo.schguardStatus;
            }   //else 将保持状态不变
        }
        else if (guardStatus===_guardState.watch)   {   //监控状态，无条件停止监控, 或临时通行
            if (jo.schguardStatus ===0) {
                runTagFinder(false);        //停监控
                devcon.doDevCommand("setGpioOut",[0, 0]);
            }
            n_guardStatus = jo.schguardStatus;      //
        }
        else if (guardStatus===_guardState.checkout)   {    //临时通行, 无条件停止
            n_guardStatus = jo.schguardStatus;
        }
        else    {           //alarm状态，只允许转到临时通行状态
            if (jo.schguardStatus ===2) {
                n_guardStatus = jo.schguardStatus;
            }
        }
    }
    var ev = {"event": "tick", "param":[] };
    if (guardStatus === n_guardStatus)
        ev.param.push({'time':jo.time, 'tickcount': jo.tickcount });
    else    {
        guardStatus = n_guardStatus;
        ev.param.push({'time':jo.time, 'tickcount': jo.tickcount,'guardStatus':guardStatus});
    }    
    devcon.wsSeverNotify(ev);
    //
    var dt = new Date();
    clearPasscards(dt);
    if ((dt.getTime() - lastFoundTagTime.getTime())/(1000*60) >30)   {
        devcon.doDevCommand("clearInventBuf", []);
        lastFoundTagTime = dt;
    }
});

//提供http服务,静态路由
app.use(express.static(path.join(__dirname,'/view')));
//app.use(bodyparser.json());     //接受json数据类型
//app.use(bodyparser.urlencoded({extended: false}));      //解析post请求数据

//提供configs显示列表数据（ajax)
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
    //屏蔽密码显示
    var elem = jodat.rows.find(function(elem){ if (elem.prgId==="upassword") return (elem); });
    if (elem)   {
        _upassword = elem.value;
        elem.value = "";
    }    
    schtimer.schemeCompile(jodat.rows);
    jodat.total = jodat.rows.length;    //
    res.send(JSON.stringify(jodat));
});

//提供passCards显示列表数据（ajax)
app.get("/passcards", function(req, res) {
    var len = passCards.length;
    var jodat = [];
    for(var i=0; i<len; i++)    {
        jodat.push({'text':passCards[i].tagId});
    }
    res.send(JSON.stringify(jodat));
});

//接收一般性ajax命令，处理
app.get("/commands", function(req, res) {
    var param = [];    
    if (req.query.param !== undefined)    {
        param = JSON.parse(req.query.param);
    }
    if (req.query.command ==="ntpUpdate") {
        devcon.doDevCommand(req.query.command, param, function(jores)   {
            res.send(JSON.stringify({"result": jores.result}));
        });
    }
    else if (req.query.command==="sysClose") {
        if (param.length>0 && param[0] === _upassword && devcon.getClientCount() <=2) {
            devcon.doDevCommand(req.query.command, param, function(jores)   {
                res.send(JSON.stringify({"result": jores.result}));
            });
        }   else    {
            console.log("sysClose at"+devcon.getClientCount());
            res.send(JSON.stringify({"result":false}));
        }
    }
    else if (req.query.command === "addPasscard") {
        if (param.length>0 && param[0].length>3)    {
            for(var i=0; i<param.length; i++)   {
                var obj = {"tagId":param[i], "ucount":0, "utime":new Date() };
                passCards.push(obj);
                clearAlarms("授权通过", param[i]);
            }
            res.send(JSON.stringify({"result":true}));
            devcon.wsSeverNotify({"event": "passcardUpd", "param":[]});
        }   else    {
            res.send(JSON.stringify({"result":false}));
        }
    }
    else if (req.query.command === "clearPasscard") {
        passCards.splice(0, passCards.length);
        res.send(JSON.stringify({"result":true}));
        devcon.wsSeverNotify({"event": "passcardUpd", "param":[]});
    }
    else if (req.query.command === "clearReports") {
        runingEvents.splice(0, runingEvents.length);
        var filepath = __dirname + "/reports/";
        var files =fs.readdirSync(filepath);
        var cnt = files.length;
        files.forEach(function(file, index)    {
            fs.unlinkSync(filepath + file);     //不能用rmSync(); file是纯文件名，要加上路径
        });
        res.send(JSON.stringify({"result":true, "count":cnt}));
    }
    else if (req.query.command === "reportEvents") {
        var filename = reports.fileNameFromDate("EV");
        reports.saveToCsvFile(runingEvents, filename);
        res.send(JSON.stringify({"result":true, "filename": path.basename(filename) }));
    }
    else if (req.query.command === "reportAlarms") {
        filename = reports.fileNameFromDate("ALM");
        reports.saveToCsvFile(runingEvents, filename);
        res.send(JSON.stringify({"result":true, "filename": path.basename(filename) }));
    }
    else if (req.query.command === "updateConfigs") {
        var valid = false;
        var xid = param.length -1;
        if (xid >0)    {
            if (param[xid].prgId === "usingpasswd"  && param[xid].value===_upassword)
                valid = true;
        }
        if (valid)  {
            param.splice(xid, 1);
            updateConfigFile(param);
            res.send(JSON.stringify({"result":true }));
        }   else    {
            res.send(JSON.stringify({"result":false, "error":"password error"}));
        }
    }
    else    {
        res.send(JSON.stringify({"result":false}));
    }
});

//下载reports文件
app.get("/downloadreports", function(req, res) {
    if (req.query.filename !== undefined)  {
        var fpath = __dirname + "/reports/" + path.basename(req.query.filename);
        if (fs.existsSync(fpath))    {
            res.download(fpath, function(err)  {
                if (err)    {
                    console.log("download file err");
                }
            });
        }   else {
            res.end(JSON.stringify({"result":false, "error":"not files"}));
        }
    }   else    {
        res.send(JSON.stringify({"result":false, "error": "下载失败！" }));
    }
});

//
app.get("/guardEvents", function(req, res) {
    //datagird请求数据时传来2个req.query参数： rows=每页行数, page=当前页号,1开始
    var pagesize = parseInt(req.query.rows);
    var pagebeg = (parseInt(req.query.page) -1)* pagesize;
    var ar = runingEvents.slice(pagebeg, pagebeg+pagesize);
    var jodat = {"total": runingEvents.length, "rows": ar };
    res.send(JSON.stringify(jodat));
});

app.get("/alarmRecords", function(req, res) {
    var pagesize = parseInt(req.query.rows);
    var pagebeg = (parseInt(req.query.page) -1)* pagesize;
    var ar = alarmRecords.slice(pagebeg, pagebeg+pagesize);
    var jodat = {"total": alarmRecords.length, "rows": ar };
    res.send(JSON.stringify(jodat));
});

var server = app.listen(2280, function(req, res)  {
    console.log("nodejs server running @2280.-"+__dirname);
});

app.get('*', function(req, res) {
    res.status(404)
    res.send("找不到资源");
});

devcon.wsServerCreate(server, onWsServerMessage, function() {
    devcon.linkWebchannel(onDeviceEvents);
});
devcon.linkWebchannel(onDeviceEvents);

//---------------------------------------------------------------------------------------------------

function getSysInfos()  {
    devcon.doDevCommand("getSysInfo", [], function(res)  {
        if (sysInfo.length >1)
            sysInfo.splice(1, sysInfo.length-1);    //清除旧的，除了startTime外
        if (res.result !==true  || res.data.length<1)
            return;
        res.data.forEach(function(item)  {
            sysInfo.push(item);
        });
        sysInfo.push({"name":"guardStatus","value":guardStatus});
        sysInfo.push({"name":"clientCount","value":devcon.getClientCount() });
        devcon.wsSeverNotify({"event": "gotSysInfos", "param":sysInfo });
    });
}

var m_tagFinderRuning = false;
function runTagFinder(start) {
    var param = [start, 2];
//#    var param = [start, 4];
    devcon.doDevCommand("runTagFinder", param, function(jores)   {
        if (jores.result === true)  {
            m_tagFinderRuning = jores.data[0].active;
            var p = [];
            p.push({"tagFinderRun": m_tagFinderRuning });
            devcon.wsSeverNotify({"event": "devMsg", "param":p});
        }
    });
}

function GuardEvent(ev)   {
    this.id = -1;
    this.event = ev;
    this.firstTime = new Date();
    this.lastTime = this.firstTime;
    this.epc = "";
    this.identifier = "";
    this.prompt = "";
    this.linkid = -1;
    //
    this.reason = "";
    this.state = 0;
}

function addRuningEvent(checkres, joTag)
{
    var ev = new GuardEvent("发现标签");
    ev.id = runingEvents.length +1;
    ev.epc = joTag.epc;
    ev.identifier = joTag.context;
    ev.linkid = joTag.id;
    if (checkres=== -1)
        ev.reason = "未布防";
    else if (checkres=== -2)
        ev.reason = "临时放行";
    else if (checkres=== -3)
        ev.reason = "授权通过";
    else
        ev.reason = "符合安全";
    runingEvents.push(ev);
//    devcon.wsSeverNotify({"event": "tagCheckAcc", "param":[]});
}

var oneshot_AlarmStop = false, oneshot_CheckOut = false;
function onDeviceEvents(joRes)    {
    var checkres;
    var dt;
    if (joRes.event ==='devMsg')   {
        if (joRes.param[0].AlarmStopReq !== undefined)  {
            if (parseInt(joRes.param[0].AlarmStopReq) !==0)  {
                if (!oneshot_AlarmStop) {
                    clearAlarms("人工关停");
                    devcon.wsSeverNotify({"event": "controlAct", "param":[]});
                    oneshot_AlarmStop = true;
                    setTimeout(function()   { oneshot_AlarmStop = false; }, 500);
                }
            }
        }
        else if (joRes.param[0].CheckoutReq !== undefined)  {
            if (parseInt(joRes.param[0].CheckoutReq) !==0)  {
                if (!oneshot_CheckOut)   {
                    doCheckoutOpen();
                    oneshot_CheckOut = true;
                    setTimeout(function()   { oneshot_CheckOut = false; }, 500);
                }
            }
        }
        else if (joRes.param[0].serverClose !== undefined)  {
            console.log("nodejs server close..");
            process.exit(parseInt(joRes.param[0].serverClose));
        }
        else    {
            devcon.wsSeverNotify(joRes);       //转发消息
        }
    }
    else if (joRes.event==='tagCapture')  {
        dt = new Date();
        lastFoundTagTime = dt;
        checkres = tagAlarmChecker(joRes.param[0]);
        if (checkres <=0)  {
            addRuningEvent(checkres, joRes.param[0]);
            devcon.wsSeverNotify({"event": "tagCheckAcc", "param":[]});
        }
    }
    else if (joRes.event === 'tagRepeat')  {
        dt = new Date();
        lastFoundTagTime = dt;
        //重复的标签，先再允许通行表中找，不行再到Alarm中赵，减少tagAlarmChecker调用次数
        var item = runingEvents.find(function(elem){
            if (elem.linkid===joRes.param[0].id && elem.identifier===joRes.param[0].context)   return(elem);
        });
        if (item)   {
            item.prompt = joRes.param[0].hitCount + " 次发现";
            var diff = dt.getTime() - item.lastTime.getTime();
            if (diff >=2000)    {       //降低数据刷新率
                item.lastTime = dt;
                devcon.wsSeverNotify({"event": "tagCheckAcc", "param":[]});
            }
        }
        else {
            checkres = tagAlarmChecker(joRes.param[0]);
            if (checkres <=0)   {
                addRuningEvent(checkres, joRes.param[0]);   //非报警
            }
            if (parseInt(joRes.param[0].hitCount) % 4 ===0) {       //降低数据刷新率
                devcon.wsSeverNotify({"event": "tagCheckAcc", "param":[]}); //?
            }
        }
    }
}

function onWsServerMessage(cmd)   {
    if (cmd.command === 'getSysInfo') {
        getSysInfos();
    }
    else if (cmd.command === "stopAlarm")    {
        clearAlarms("人工关停");
        devcon.wsSeverNotify({"event": "controlAct", "param":[]});
    }
    else if (cmd.command === "checkoutOpen") {
        doCheckoutOpen(cmd.param[0]);
    }
}

function doCheckoutOpen(minutes)   {
    if (guardStatus !== _guardState.idle)  {
        var times1;
        if (minutes === undefined)
            times1 = 3;
        else
            times1 = minutes;
        schtimer.startCheckOutOpen(times1);
        clearAlarms("临时放行", "", "限时"+ times1 + "分钟");
        guardStatus = schtimer.getSchGuardState();

        devcon.wsSeverNotify({"event": "alarmAct", "param":[{'guardStatus':guardStatus }]});
        devcon.wsSeverNotify({"event": "controlAct", "param":[]});
    }
}

//返回： >0 更新了报警, ==0无改变， <=不报警但安全位激活(条件放行))
function tagAlarmChecker(joTag)    {        
    if (parseInt(joTag.formatId) <0 || joTag.context.length <3 || joTag.security===undefined)
        return (0);
    var i, rec;
    if (parseInt(joTag.security) ===sch_securityAval)  {
        if (guardStatus===_guardState.idle)
            return(-1);         //未布放      
        if (guardStatus===_guardState.checkout)
            return (-2);        //临时放行
        for(i=0; i<passCards.length; i++)   {            
            if (joTag.context === passCards[i].tagId) {
                passCards[i].ucount++;
                passCards[i].utime = new Date();
                console.log("fix passcard.");
                return (-3);    //授权通过
            }
        }
        //
        rec = alarmRecords.find(function(rec)   {
            var dt_ms = new Date().getTime();
            if (rec.identifier===joTag.context) {
                if (rec.state===0)
                    return(rec);
                else    {
                    if (((dt_ms - rec.lastTime.getTime())/(1000*60)) < 5)   {
                        return(rec);
                    }
                }
            }
        });
        if (rec)    {
            var dt2 = new Date();
            var delta = dt2.getTime() - rec.lastTime.getTime();
            if (rec.state ===0)
                rec.lastTime = dt2;
            //重复出现的标签，刷新alarmTab的显示，但限制刷新速率
            if (delta/1000 >3)  {
                rec.prompt = "安全位报警+" + joTag.hitCount;
                rec.lastTime = dt2;     //这个产生歧义，但没办法
                devcon.wsSeverNotify({"event": "alarmAct", "param":[{'guardStatus':guardStatus }]});
            }
            return(2);
        }
        else    {
            var ev = new GuardEvent("Alarm");
            ev.epc = joTag.epc;
            ev.identifier = joTag.context;
            ev.linkid = joTag.id;
            ev.id = alarmRecords.length + 1;
            ev.prompt = "安全位报警";
            alarmRecords.push(ev);

            guardStatus = _guardState.alert;
//            devcon.doDevCommand("setGpioOut",[1]);
            devcon.doDevCommand("setGpioOut",[1, 1]);
            setTimeout(function()   {       //延时开启第2级警报
                if (guardStatus === _guardState.alert)  {
//                    devcon.doDevCommand("setGpioOut",[2]);
                    devcon.doDevCommand("setGpioOut",[2, 1]);
                }
            }, 11000);

            devcon.wsSeverNotify({"event": "alarmAct", "param":[{'guardStatus':guardStatus }]});
            return(1);
        }
    }
    else {
        if (guardStatus === _guardState.alert) {
            rec = alarmRecords.find(function(rec)   {
                if (rec.identifier===joTag.context && rec.state===0)    return(rec);
            });
            if (rec)    {
                clearAlarms("安全位解除", joTag.context);
                return(3);
            }
            else    {
                return(0);
            }
        }
        else return (0);
    }
}

function clearAlarms(reason, tagNo, evPrompt) {
    var selall = !(tagNo !== undefined && tagNo.length>3);
    var i=0, cnt1 = 0, cnt2 = 0;
    if (guardStatus === _guardState.alert || tagNo !== undefined)  {
        alarmRecords.forEach(function(rec)   {
            if (rec.state ===0)  {
                cnt1++;
                if (selall || rec.identifier===tagNo)   {
                    rec.state = 1;      //closed
                    rec.lastTime = new Date();
                    rec.reason = reason;
                    cnt2++;
                }
            }
            if (++i === alarmRecords.length)    {       //遍历结束
                if (cnt1>0 && cnt1 === cnt2)  {
                    guardStatus = schtimer.getSchGuardState();
//                    devcon.doDevCommand("setGpioOut",[0]);
                    devcon.doDevCommand("setGpioOut",[1, 0]);
                    devcon.doDevCommand("setGpioOut",[2, 0]);
                    devcon.wsSeverNotify({"event": "alarmAct", "param":[{'guardStatus':guardStatus }]});
                }
            }
        })
    }
    //
    var ev = new GuardEvent(reason);
    ev.id = runingEvents.length +1;
    if (tagNo !== undefined)    {
        ev.identifier = tagNo;
    }
    if (evPrompt !== undefined) {
        ev.prompt = evPrompt;
    }
    runingEvents.push(ev);
//    devcon.wsSeverNotify({"event": "tagCheckAcc", "param":[]});
}

function clearPasscards(dt)   {
    if (passCards === undefined || passCards.length===0)
        return;
    var dt_ms = dt.getTime();
    var cnt1 = 0;
    for(var item of passCards)  {
        if ((dt_ms - item.utime.getTime())/(1000*60) >2) {
            passCards.splice(passCards.indexOf(item), 1);
            cnt1++;
        }
    }
    if (cnt1>0)
        devcon.wsSeverNotify({"event": "passcardUpd", "param":[]});
}

function updateConfigFile(items)   {
    fs.readFile("./view/configs.json", function(err, dat)   {
        if (err)    {
            console.error(err);
        }
        var jodat = JSON.parse(dat);
        for(var item of items)  {
            var jo = jodat.rows.find(function(elem)   {
                if (elem.prgId === item.prgId && elem.value !== item.value) return(elem)
            });
            if (jo) {
                jo.value = item.value;
            }
        }
        jodat.total = jodat.rows.length;
        fs.writeFile("./view/configs.json", JSON.stringify(jodat, null, 2), function(err) {
            if (err)    {
                console.error(err);
            }
        });
    });
}
