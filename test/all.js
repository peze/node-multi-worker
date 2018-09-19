var mgWorker = require('../index');
var Promise = require('bluebird');
mgWorker.tasks("task",{loop:function(data){
        var str = "";
        for(var i = data.index * data.length;i < data.length * (data.index + 1);i++){
            str += iq;
        }
        return str;
    }})
var worker = new mgWorker();
var arr = [worker.task.loop({
    length: 1000,
    index: 0
}),worker.task.loop({
    length: 1000,
    index: 1
}),worker.task.loop({
    length: 1000,
    index: 2
}),worker.task.loop({
    length: 1000,
    index: 3
}),worker.task.loop({
    length: 1000,
    index: 4
})]
Promise.all(arr).then(function(arg){
    console.log(arg.join(""));
})