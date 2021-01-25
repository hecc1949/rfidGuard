/**
  */

var stateView = (function($) {
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

    return  {
        setGuardStatusView : setGuardStatusView,
        setDevActionView : setDevActionView,
        setDevScanModeView : setDevScanModeView,
        tickforDevActionView : tickforDevActionView
    };
}(jQuery));

