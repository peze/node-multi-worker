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
	
这样会阻塞一段时间的任务。现阶段分配任务的方式是（任务数%进程数）的方式分配，暂时不支持闲时抢占任务的形式。还有现阶段在子进程中传输数据时使用JSON.stringify的时候，如果需要stringify的数据过大，则无法解析。
