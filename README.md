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

不过目前仍然存在问题，在v8中没考虑过会被fork的情况，所以他的worker thread在主进程中被初始化了以后，不会调用pthread_atfork类似的方法来在子进程中重置初始化参数再重新启动worker thread（由此可见，v8其实就根本没想到你会fork他)。无法启用worker thread会造成垃圾回收任务变慢的情况存在，而在Mac OS的版本中，因为信号量mach/semaphore的缘故会造成主线程wait后，没有worker线程signal启用它 而无限期wait（我自己的mac系统上的mach/semaphore在使用mach_task_self()后再fork出子进程，子进程中的semaphore_t是无效的，应该是nvm安装的node打包所用的库或编译器支持不同），不过再Linux系统中可以正常使用。
