/**
 * Created by jay on 17/2/20.
 */
var mgWorker = require('../index');
var fs = require("fs");
const Promise = require("bluebird");
fs.readdir = Promise.promisify(fs.readdir);
fs.readFile = Promise.promisify(fs.readFile);
fs.writeFile = Promise.promisify(fs.writeFile);

mgWorker.tasks("task",{a: mgWorker.aysnc(function(data){
        // console.log(data);
        return fs.writeFile("./text","promise test content:" + data.bar + "\n", {flag: "a"}).then(function(){
            return fs.readFile("./text");
        }).then(function(data){
            return data.toString("utf8");
        });
    }),
    b: async function(data){
        // // abc();

        await fs.writeFile("./text","async/await test content:" + data.foo + "\n", {flag: "a"});
        var data = await fs.readFile("./text",{
            encoding: "utf8"
        });
        return data;
    },
    c:function(data){
        return data.foo + data.bar + "bcd2";
    },
    d:function(data){
        setImmediate(function(){
            console.log("setImmediate test");
        })
        setTimeout(function(){
            console.log("setTimeout test");
        })
        process.nextTick(function(){
            console.log("nextTick test");
        })
        return {
            a: data.foo,
            b: data.bar
        };
    }});



var worker = new mgWorker();
worker.task.a({
    bar: "this is bar"
}).then(function (data) {

    console.log(data);
}).catch(function(err){
    console.log(err);
})

worker.task.b({
    foo: "this is foo"
}).then(function (data) {

    console.log(data);
}).catch(function(err){
    console.log(err);
})

worker.task.c({
    foo: 1,
    bar: "a"
}).then(function (data) {

    console.log(data);
}).catch(function(err){
    console.log(err);
})

worker.task.d({
    foo: 2,
    bar: "d"
}).then(function (data) {
    console.log(data);
}).catch(function(err){
    console.log(err);
})