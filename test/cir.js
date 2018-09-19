var mgWorker = require('../index');
// mgWorker.tasks("task",{loop:function(data){
//         var str = [];
//         for(var i = 1;i < data.length;i++){
//             str.push({a:1,b:2});
//         }
//         // console.log(str);
//         return str;
//     }})
// var worker = new mgWorker();
// worker.task.loop({
//     length: 10000
// }).then(function (data) {
//     // console.log(data);
// }).catch(function(err){
//     console.log(err);
// });
//
// worker.task.loop({
//     length: 1000
// }).then(function (data) {
//     // console.log(data);
// }).catch(function(err){
//     console.log(err);
// });

mgWorker.tasks("loop",{a:function(data){
        var str = "";
        for(var i = 1;i < data.length;i++){
            str += i;
        }
        return str;
    },b:function(data){
        var str = [];
        for(var i = 1;i < data.length;i++){
            str.push({a:3,b:4});
        }

        return str;
    },c:function(data){
        var str = [];
        for(var i = 1;i < data.length;i++){
            str.push({a:5,b:6});
        }
        return JSON.stringify(str);
    }})
var worker = new mgWorker();


worker.loop.a({
    length:8000
}).then(function (data) {
    console.log(data);
}).catch(function(err){
    console.log(err);
});
// worker.loop.b({
//     length: 1000
// }).then(function (data) {
//     console.log(data);
// }).catch(function(err){
//     console.log(err);
// });
// worker.loop.c({
//     length: 1000
// }).then(function (data) {
//     console.log(data);
// }).catch(function(err){
//     console.log(err);
// });