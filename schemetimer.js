/*
  scheme timer module
*/

"use strict";

var weekSchemeTimes = [{"begin":0, "end":0 }, {"begin":0, "end":0}, {"begin":0, "end":0},
        {"begin":0, "end":0 }, {"begin":0, "end":0}, {"begin":0, "end":0 }, {"begin":0, "end":0 }];     //秒值时间

function schemeCompile(defobjs)   {
    var selIds = [0,0,0,0, 0,0,0];
    for(var i=0; i<7; i++)  {
        for(var j=0; j<defobjs.length; j++) {
            if (defobjs[j].prgId ===("schWeekd"+i)) {
                selIds[i] = defobjs[j].value;
            }
        }
    }
    var timeseg = [0,0,0,0];
    var obj = defobjs.find(obj=> obj.prgId ==='schNormalBeg');
    if (obj)    {
        timeseg[0] = parseInt((obj.value.split(':'))[0])*60*60 + parseInt((obj.value.split(':'))[1])*60;
    }
    obj = defobjs.find(obj=> obj.prgId ==='schNormalEnd');
    if (obj)    {
        timeseg[1] = parseInt((obj.value.split(':'))[0])*60*60 + parseInt((obj.value.split(':'))[1])*60;
    }
    obj = defobjs.find(obj=> obj.prgId ==='schHalfworkBeg');
    if (obj)    {
        timeseg[2] = parseInt((obj.value.split(':'))[0])*60*60 + parseInt((obj.value.split(':'))[1])*60;
    }
    obj = defobjs.find(obj=> obj.prgId ==='schHalfworkEnd');
    if (obj)    {
        timeseg[3] = parseInt((obj.value.split(':'))[0])*60*60 + parseInt((obj.value.split(':'))[1])*60;
    }

    for(i=0; i<7; i++)  {
        if (selIds[i] === '1')    {
            weekSchemeTimes[i].begin = 0;
            weekSchemeTimes[i].end = 24*60*60;
        }   else if (selIds[i] == 2)   {
            weekSchemeTimes[i].begin = timeseg[0];
            weekSchemeTimes[i].end = timeseg[1];
        }   else if (selIds[i] == 3)   {
            weekSchemeTimes[i].begin = timeseg[2];
            weekSchemeTimes[i].end = timeseg[3];
        }
    }
//    console.log(JSON.stringify(weekSchemeTimes));
}

var schemeGuardOn = false;

function schemeTimerStart(evCallback)   {
    var tickCount = 0;
    var dt = new Date();
    var dayId = dt.getDay();
    var ntime = dt.getHours()*60*60 + dt.getMinutes()*60 + dt.getSeconds();
    schemeGuardOn = (weekSchemeTimes[dayId].begin<= ntime && ntime <= weekSchemeTimes[dayId].end);

    var jo = {"event": "tick", "param":[] };
    if (evCallback) {
/*        jo.param.push(tickCount);
        jo.param.push(dt);
        jo.param.push(schemeGuardOn); */
        jo.param.push({'name':'tickcount', 'value':tickCount});
        jo.param.push({'name':'time', 'value':dt});
        jo.param.push({'name':'status','value':schemeGuardOn});

        evCallback(jo);
    }
    //
    var schemeTimer = setInterval(function()    {
        dt = new Date();
        dayId = dt.getDay();
        ntime = dt.getHours()*60*60 + dt.getMinutes()*60 + dt.getSeconds();
        schemeGuardOn = (weekSchemeTimes[dayId].begin<= ntime && ntime <= weekSchemeTimes[dayId].end);
        if (evCallback)     {
            jo = {"event": "tick", "param":[] };
/*            jo.param.push(++tickCount);
            jo.param.push(dt);
            jo.param.push(schemeGuardOn); */
            jo.param.push({'name':'tickcount', 'value':++tickCount});
            jo.param.push({'name':'time', 'value':dt});
            jo.param.push({'name':'status','value':schemeGuardOn});
            evCallback(jo);
        }
    }, 5000);
}

module.exports =    {
    schemeGuardOn: schemeGuardOn,
    schemeTimerStart: schemeTimerStart,
    schemeCompile: schemeCompile
}
