"use strict";

/*
  scheme timer module
*/

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

//var schemeGuardOn = false;
function getSchGuardState() {
    var dt = new Date();
    var dayId = dt.getDay();
    var ntime = dt.getHours()*60*60 + dt.getMinutes()*60 + dt.getSeconds();
    var schemeGuardOn = (weekSchemeTimes[dayId].begin<= ntime && ntime <= weekSchemeTimes[dayId].end);
    var checkoutOpen = (checkoutOpenTimer>0);
    if (schemeGuardOn)    {
        if (checkoutOpen)
            return(2);
        else
            return(1);
    }   else    {
        return (0);
    }
}

var tickCount = 0;

function schemeTimerStart(evTickCallback)   {
    var dt = new Date();
/*    var dayId = dt.getDay();
    var ntime = dt.getHours()*60*60 + dt.getMinutes()*60 + dt.getSeconds();
    var schemeGuardOn = (weekSchemeTimes[dayId].begin<= ntime && ntime <= weekSchemeTimes[dayId].end);
    var checkoutOpen = (checkoutOpenTimer>0);
*/
    var jo;
    var schguardst = getSchGuardState();
    if (evTickCallback) {
//        jo = {'time':dt, 'tickcount':tickCount,'schguard':schemeGuardOn, 'checkoutOpen': checkoutOpen};
        jo = {'time':dt, 'tickcount':tickCount,'schguardStatus': schguardst};
        tickCount++;
        evTickCallback(jo);
    }
    //
    var schemeTimer = setInterval(function()    {
        dt = new Date();
/*        dayId = dt.getDay();
        ntime = dt.getHours()*60*60 + dt.getMinutes()*60 + dt.getSeconds();
        schemeGuardOn = (weekSchemeTimes[dayId].begin<= ntime && ntime <= weekSchemeTimes[dayId].end);
        checkoutOpen = (checkoutOpenTimer>0); */
        schguardst = getSchGuardState();
        if (evTickCallback)     {
//            jo = {'time':dt, 'tickcount':tickCount,'schguard':schemeGuardOn, 'checkoutOpen': checkoutOpen};
            jo = {'time':dt, 'tickcount':tickCount,'schguardStatus': schguardst};
            tickCount++;
            evTickCallback(jo);
        }
        if (checkoutOpenTimer>0)    {
            checkoutOpenTimer -= 5;
        }
    }, 5000);
}

var checkoutOpenTimer = 0;
function startCheckOutOpen(minute) {
    checkoutOpenTimer = minute*60;
}

/*
function isCheckoutOpen()   {
    return (checkoutOpenTimer<=0);
}
*/

module.exports =    {
//    schemeGuardOn: schemeGuardOn,
    schemeTimerStart : schemeTimerStart,
    schemeCompile : schemeCompile,

    startCheckOutOpen: startCheckOutOpen,
    getSchGuardState : getSchGuardState
//    isCheckoutOpen: isCheckoutOpen
}
