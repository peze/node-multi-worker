# node-multi-worker
the node multi-worker moudle

一个node多进程处理任务的模块，分为一个reactor接收主进程发来的任务指令，和CPU数*2的worker进程处理任务并且返回。主要用来处理一些阻塞性任务例如：

	var mgWorker = require('node-worker');
	mgWorker.tasks("task",{loop:function(data){
		var str = "";
		for(var i = 1;i < data.length;i++){
			str += i;
		}
		return str;
	}})
	var worker = new mgWorker();
	worker.task.loop({
    	length: 5000
	}).then(function (data) {
	    console.log(data);
	}).catch(function(err){
	    console.log(err);
	});
	
这样会阻塞一段时间的任务。现阶段分配任务的方式是（任务数%进程数）的方式分配，暂时不支持闲时抢占任务的形式。虽然可以一次性在子进程中做大量的循环，但是建议拆分循环到不同的子进程中然后汇总结果例如：

	var mgWorker = require('node-worker');
	var Promise = require('bluebird');
	mgWorker.tasks("task",{loop:function(data){
		var str = "";
		for(var i = data.index * data.length;i < (data.index + 1);i++){
			str += i;
		}
		return str;
	}})
	var worker = new mgWorker();
	var arr = [worker.task.loop({
    	length: 1000,
    	index:0
	}),worker.task.loop({
    	length: 1000,
    	index:1
	}),worker.task.loop({
    	length: 1000,
    	index:2
	}),worker.task.loop({
    	length: 1000,
    	index:3
	}),worker.task.loop({
    	length: 1000,
    	index:4
	})]
	Promise.all(arr).then(function(str){
    	console.log(str.join(""));
	})
这样可以充分的利用多worker的性能。
