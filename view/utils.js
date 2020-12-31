/**
  */

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

