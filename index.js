var nodeWorkers = require('./build/Release/node-worker');
var Promise = require('bluebird');

var client;

var worker = {};

var init = false;

function taskCommonFunc (key){
    return function(data,callback){
        if(typeof data == "function"){
            callback = data;
            data = {};
        }
        client.send(key,data,function(err,data){
            callback(err,data);
            process._tickCallback();//因为没在node的makeCallback中，所以只能手动触发tickCallback
        })
    }
}

function registerKey(dest,src,keys){
    if(!src){
        return;
    }
    for(var key in src){
        if(typeof src[key] == "function"){
            dest[key] = Promise.promisify(taskCommonFunc(keys.join(":") + ":" + key));
            continue;
        }
        if(src[key] instanceof  Object){
            dest[key] = {};
            registerKey(dest[key],src[key],keys.concat([key]))
        }
    }
}

function MgWorkers(){
    if(init){
        return worker;
    }
    nodeWorkers.init(function(promise){
        var pro =  promise.then(function(data){
            pro.result_arguments = arguments;
            pro.have_done = true;
        })
        pro.is_aysnc_promise = true;
        return pro;
    });
    init = true;
    client = new nodeWorkers.Client;
    client =Promise.promisifyAll(client);
    return worker;
}

MgWorkers.tasks = function(key,obj){
    if(typeof obj == "function"){
        worker[key] = Promise.promisify(taskCommonFunc(key));
    }else if(obj instanceof  Object){
        worker[key] = {};
        registerKey(worker[key],obj,[key])
    }
    nodeWorkers.register(key,obj);
}

MgWorkers.aysnc = function(callback){
    return function(data){
        var pro =  callback(data).then(function(){
            pro.result_arguments = arguments;
            pro.have_done = true;
        })
        pro.is_aysnc_promise = true;
        return pro;
    }
}

module.exports = MgWorkers;