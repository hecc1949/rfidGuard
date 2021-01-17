"use strict";

var fs = require("fs");
var path = require("path");

function fileNameFromDate(prefix) {
    var dt = new Date();
    var month = dt.getMonth()+1;
    if (month <10)
        month = '0' + month;
    var filename = "/reports/"+ prefix + dt.getFullYear() + month + dt.getDate();

    if (fs.existsSync(path.join(__dirname + filename + ".csv")))    {
        var repeatCnt = 1;
        while (fs.existsSync(path.join(__dirname + filename +"-"+ repeatCnt + ".csv")) && repeatCnt<100) {
            repeatCnt++;
        }
        filename = filename + "-"+repeatCnt;
    }
    return(path.join(__dirname + filename + ".csv"));
}

function formatDateTime(dt) {
    var year = dt.getFullYear();
    var month = dt.getMonth()+1;
    var date = dt.getDate();
    var hours = dt.getHours();
    var minutes = dt.getMinutes();
    var seconds = dt.getSeconds();
    return(year+"-"+month +"-"+ date +" "+hours+":"+minutes+":"+seconds);
}

function saveToCsvFile(joAr, filename, titles)   {
    if (joAr.length <1)
        return;
    var wrstream = fs.createWriteStream(filename, {flags:'a+', encoding:'utf-8'});
    wrstream.cork();        //使用内存缓冲
    //
    var line = "", i, key;
    if (titles !== undefined)   {
        for(i=0; i<titles.length; i++)  {
            if (line.length ===0)
                line += titles[i];
            else
                line += (", " + titles[i]);
        }
    }   else    {
        for(key in joAr[0]) {
            if (line.length ===0)
                line += key;
            else
                line += (", " + key);
        }
    }
    //
    wrstream.write(line + "\r\n");
    for(var jo of joAr) {
        line = "";
        var itemstr;
        for(key in jo)  {
            if (jo[key] instanceof Date)
                itemstr = formatDateTime(jo[key]);
            else
                itemstr = jo[key];
            if (itemstr !== undefined && (itemstr instanceof String))  {
                itemstr.replace(/\"/,'\"\"');
                if (itemstr.indexOf(',')>=0)    {
                    itemstr = '\"' + itemstr + '\"';
                }
            }
            if (line.length ===0)
                line += itemstr;
            else
                line += (", " +itemstr);
        }
        wrstream.write(line + "\r\n");
    }
    //
    wrstream.end();
    wrstream.uncork();
}

module.exports =    {
    fileNameFromDate: fileNameFromDate,
    saveToCsvFile: saveToCsvFile
}
