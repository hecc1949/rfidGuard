/**
  */

//textbox无边框扩展
$.extend($.fn.textbox.methods, {
   setBorder: function(jq, border) {
       var style = $('#easyui-textbox-border');
       if (!style.length)  {
           $('head').append(
                   '<style id="easyui-textbox-border">' +
                   '.textbox-noborder{border-color:transparent;border-radius:0}' +
                   '</style>'
           );
       }
       return jq.each(function(){
           var span = $(this).next();
           if (border) {
               span.removeClass('textbox-noborder');
           } else {
               span.addClass('textbox-noborder');
           }
       });
   }
});

//状态条时钟
function displayRuntime(dt) {
   var hours = dt.getHours();
   var minutes = dt.getMinutes();
   var seconds = dt.getSeconds();
   if(hours<10)    {
       hours = '0'+hours;
   }
   if(minutes<10)  {
       minutes = '0'+minutes;
   }
   if(seconds<10)  {
       seconds = '0'+seconds;
   }
   $("#genClock").textbox('setValue', hours+':'+minutes+':'+seconds);

   if (serverStartTime)	{
       var diff = (dt.getTime() - serverStartTime.getTime())/1000;
       var days = parseInt(diff / (24*60*60));
       diff = diff - days*(24*60*60);
       hours = parseInt(diff / (60*60));
       diff = diff - hours*(60*60);
       minutes = parseInt(diff /60);
//        seconds = parseInt(diff - minutes*60);
       if (minutes >0)	{
           $("#runedTimes").textbox('setValue',"已运行"+ days +"天"+ hours + "小时" + minutes + "分钟");
       }   else {
           $("#runedTimes").textbox('setValue',serverStartTime.toLocaleString());
       }
   }
}

//表格列的显示格式，combobox列
function formatComboboxItem(value, rowData, rowIndex)   {
    if (rowData.editor.type==='combobox')    {
        var itemtitle = '--';
        for(var i=0; i<rowData.editor.options.data.length; i++)    {
            if (value===rowData.editor.options.data[i].value) {
                itemtitle = rowData.editor.options.data[i].text;
                break;
            }
        }
        return(itemtitle);
    }   else    {
        return(value);
    }
}

//表格列日期时间格式化显示
function formatDateTimeItem(val,rowData, rowIndex)  {
    var dt;
    if (val instanceof Date)
        dt = val;
    else
        dt = new Date(val);
    var month = dt.getMonth()+1;
    var dates = dt.getDate();
    var hours = dt.getHours();
    var minutes = dt.getMinutes();
    return(month+"月"+dates + "日 "+ hours+":"+minutes);
}

//表格列时间格式化显示(短)
function formatTimeItem(val,rowData, rowIndex)  {
    var dt;
    if (val instanceof Date)
        dt = val;
    else
        dt = new Date(val);
    var hours = dt.getHours();
    var minutes = dt.getMinutes();
    return(hours+":"+minutes);
}

//发送websocket命令
function wsSendCommand(jo)  {
    if (wssocket && wssocket !== null)  {
        wssocket.send(JSON.stringify(jo));
    }
}

//发送ajax命令
function sendCommand(jo, callback)    {
    $.ajax({
        url: "/commands",
        type: "get",
        dataType: 'json',
        data: jo,
        success:function(dat)   {
            if (dat.result===true) {
                if (callback)
                    callback(dat);
            }   else    {
                $.messager.alert("错误", "命令执行失败: 权限或参数错误",'error');
            }
        },
        error: function(err)    {
            $.messager.alert("错误", "命令发送失败:" + err);
        }
    });
}

//单项功能： 发送passCard
function sendPasscard()   {
    var card = $("#addPasscard").val().replace("\n", "");     //不带回车
    if (card.length >3) {
        var param = [];
        param.push(card);
        sendCommand({"command":"addPasscard", "param":JSON.stringify(param)});
    }
    $("#addPasscard").textbox('setValue', '');
}
