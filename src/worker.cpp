#include "worker.h"

//
// Created by page on 17/1/11.
//

extern v8::Isolate* globalIsolate;
Persistent<Function> async_promise_callback;
uv_buf_t dummy_buf;
extern Persistent<Object> funcs;

extern int have_init;
extern int reactor_pid;
extern int main_pid;



void handle_signal(int signo)
{
    if (signo == SIGINT || signo == SIGSTOP)
    {
        uv_fs_t req;
        uv_fs_unlink(loop, &req, SERVER_SOCK, NULL);
        sem_unlink("mgw_sem");
        sigemptyset(&setmask);
        sigaddset(&setmask,SIGCHLD);
        sigset_t old_set;
        sigprocmask(SIG_BLOCK,&setmask,&old_set);
        fprintf(stderr, "Stop workers \n");
        kill(0,SIGKILL);

//        sigprocmask(SIG_SETMASK,&old_set,NULL); //不解除信号屏蔽退出
//        _exit(127);
    }
}


void finish_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_err_name(status));
    }
//    if(req->data != NULL){
//        std::string* return_params_json = reinterpret_cast<std::string*>(req->data);
//        if(return_params_json->length() < BUFF_SIZE){
//            char return_params_json_string[BUFF_SIZE + 7];
//            sprintf(return_params_json_string,"%s;split$",return_params_json->c_str());
//            req->data = NULL;
//            ((write_req_t*)req)->buf = uv_buf_init(return_params_json_string,BUFF_SIZE + 7);
//            uv_write(req, req->handle, &(((write_req_t*)req)->buf), 1, finish_write);
//        }else{
//            char return_params_json_string[BUFF_SIZE];
//            snprintf(return_params_json_string,BUFF_SIZE,"%s",return_params_json->c_str());
//            req->data = (void*)new std::string(return_params_json->substr(BUFF_SIZE));
//            ((write_req_t*)req)->buf = uv_buf_init(return_params_json_string,BUFF_SIZE);
//            uv_write(req, req->handle, &(((write_req_t*)req)->buf), 1, finish_write);
//        }
//        return;
//    }
    free_write_req(req,false);
}

void promise_callback(uv_timer_t *handle) {
    v8::HandleScope scope(globalIsolate);
    PromiseDeliver* promise_deliver = static_cast<PromiseDeliver*>(handle->data);
    uv_stream_t* client = promise_deliver->getClient();
    write_req_t* req = (write_req_t*)promise_deliver->getReq();
    Local<Object> promise = promise_deliver->getPromise();
    Local<Value> have_done = promise->ToObject()->Get(String::NewFromUtf8(globalIsolate, "have_done"));
    if(have_done->IsBoolean()){
        bool async_have_done = have_done->BooleanValue();
        if(async_have_done){
            Local<Value> returnValue = promise->ToObject()->Get(String::NewFromUtf8(globalIsolate, "result_arguments"));
            std::string return_params_json = json_str(globalIsolate,returnValue);
            char* return_params_json_string =  (char*) malloc(return_params_json.length() + 15);
            sprintf(return_params_json_string,"%csplit$%s;split$",0x3a,return_params_json.c_str());
            req->buf = uv_buf_init(return_params_json_string,return_params_json.length() + 15);
            uv_write((uv_write_t*) req, client, &req->buf, 1, finish_write);
            delete promise_deliver;
            uv_close((uv_handle_t*)handle,free_handle);
        }
    }
    return;
}

void begin_task(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
        char* parameter;
        char* left = (char *) malloc(nread+1);
        char* val = (char *) malloc(nread+1);
        char* task_key = new char[METHOD_LONG];
        char* task_key_long = new char[METHOD_LONG];
        HandleScope scope(globalIsolate);
        Local<Object> target = Local<Object>::New(globalIsolate, funcs);
        memcpy(left,buf->base,nread+1);
        while(get_from_father(left,val,';')){
            get_from_father(val,task_key_long,':');
            while(split_task_key(task_key_long,task_key,':')){
                Local<Value> func_val = target->ToObject()->Get(String::NewFromUtf8(globalIsolate, task_key));
                if(func_val->IsFunction()){
                    Local<Function> func_exec = Local<Function>::Cast(func_val);
                    Local<Value> parameter = JSON::Parse(globalIsolate,String::NewFromUtf8(globalIsolate, val)).ToLocalChecked();
                    Local<Value> parameters[1] = {parameter};
                    Local<Value> returnValue = func_exec->Call(target,1,parameters);
                    if(func_exec->IsAsyncFunction() || returnValue->IsPromise()){

                        Local<Function> async_callback = Local<Function>::New(globalIsolate,async_promise_callback);
                        Local<Value> promise_parameters[1] = {returnValue};
                        returnValue = async_callback->Call(target,1,promise_parameters);
                    }
                    if(returnValue->IsUndefined()){

                        req->buf = uv_buf_init("a",10);
                        uv_write((uv_write_t*) req, client, &req->buf, 1, finish_write);
                        break;
                    }
                    if(returnValue->IsObject()){
                        Local<Object> returnObj = Local<Object>::Cast(returnValue);
                        Local<Value> isAsyncFunc = returnObj->ToObject()->Get(String::NewFromUtf8(globalIsolate, "is_aysnc_promise"));
                        if(isAsyncFunc->IsBoolean()){
                            bool is_async_func = isAsyncFunc->BooleanValue();
                            if(is_async_func){

                                uv_timer_t *promiser = (uv_timer_t *)malloc(sizeof(uv_timer_t));
                                uv_timer_init(client->loop, promiser);
                                PromiseDeliver *promise_deliver = new PromiseDeliver(globalIsolate,(uv_handle_t*)promiser,client,(uv_write_t*) req,returnObj);
                                uv_timer_start(promiser, promise_callback, 0, 50);
                                break;
                            }
                        }
                    }
                    std::string return_params_json = json_str(globalIsolate,returnValue);
//                    if(return_params_json.length() > BUFF_SIZE){
//                        char return_params_json_string[BUFF_SIZE];
//                        snprintf(return_params_json_string,BUFF_SIZE,"%csplit$%s",0x3a,return_params_json.c_str());
//                        ((uv_write_t*)req)->data = (void*)new std::string(return_params_json.substr(BUFF_SIZE - 7));
//                        req->buf = uv_buf_init(return_params_json_string,BUFF_SIZE);
//                        uv_write((uv_write_t*) req, client, &req->buf, 1, finish_write);
//                    }else{
                        char* return_params_json_string =  (char*) malloc(return_params_json.length() + 15);
                        sprintf(return_params_json_string,"%csplit$%s;split$",0x3a,return_params_json.c_str());
                        req->buf = uv_buf_init(return_params_json_string,return_params_json.length() + 15);
                        uv_write((uv_write_t*) req, client, &req->buf, 1, finish_write);
//                    }
//                    if(return_params_json.length() > BUFF_SIZE){
//                        int nbuf = return_params_json.length() % BUFF_SIZE  ? return_params_json.length() / BUFF_SIZE + 1 : return_params_json.length() / BUFF_SIZE;
//                        uv_buf_t* bufs = (uv_buf_t*)calloc(sizeof(uv_buf_t),nbuf);
//                        int i = 0;
//                        while ( i < nbuf ){
//                            char* return_params_json_string =  (char*) malloc(BUFF_SIZE);
//                            if(i == 0){
//                                snprintf(return_params_json_string,BUFF_SIZE,"%csplit$%s",0x3a,return_params_json.c_str());
//                                return_params_json = return_params_json.substr(BUFF_SIZE - 7);
//                            }else if(i == nbuf -1){
//                                snprintf(return_params_json_string,BUFF_SIZE,"%s;split$",return_params_json.c_str());
//                            }else{
//                                snprintf(return_params_json_string,BUFF_SIZE,"%s",return_params_json.c_str());
//                                return_params_json = return_params_json.substr(BUFF_SIZE);
//                            }
//                            *(bufs + i) = uv_buf_init(return_params_json_string,BUFF_SIZE);
//                            i++;
//                        }
//                        req->buf.base = (char*)bufs;
//                        uv_write((uv_write_t*) req, client, bufs, nbuf, finish_write);
//                    }else{
//                        char* return_params_json_string =  (char*) malloc(return_params_json.length() + 15);
//                        sprintf(return_params_json_string,"%csplit$%s;split$",0x3a,return_params_json.c_str());
//                        req->buf = uv_buf_init(return_params_json_string,return_params_json.length() + 15);
//                        uv_write((uv_write_t*) req, client, &req->buf, 1, finish_write);
//                    }
                    break;
                }
                if(func_val->IsObject()){
                    target = Local<Object>::Cast(func_val);
                }
                if(func_val->IsUndefined()){
                    break;
                }
            }
        }
        delete task_key;
        delete task_key_long;
    }

    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, free_handle);
    }

    free(buf->base);
}



void on_child_work(uv_stream_t *q, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) q, NULL);
        return;
    }
    uv_pipe_t *pipe = (uv_pipe_t*) q;
    uv_pipe_t *client = (uv_pipe_t*) malloc(sizeof(uv_pipe_t));
    uv_pipe_init(pipe->loop, client, 0);
    if (uv_accept(q, (uv_stream_t*) client) == 0) {
        uv_os_fd_t fd;
        uv_fileno((const uv_handle_t*) client, &fd);
        uv_read_start((uv_stream_t*) client, alloc_buffer, begin_task);
    }
    else {
        uv_close((uv_handle_t*) client, free_handle);
    }
}

void check_parent_exist(uv_timer_t *handle) {
    int parent_id = getppid();
    if(parent_id != reactor_pid){
        exit(128);
    }
}

void check_master_parent_exist(uv_timer_t *handle) {
    int parent_id = getppid();
    if(parent_id != main_pid){
        kill(getpid(),SIGINT);
    }
}

void check_immediate(uv_check_t* handle){
    v8::HandleScope scope(globalIsolate);
    Local<Object> global =  globalIsolate->GetCurrentContext()->Global();
    Local<Object> process = Local<Object>::Cast(global->ToObject()->Get(String::NewFromUtf8(globalIsolate, "process")));
    Local<Function> immediateFunc = Local<Function>::Cast(process->ToObject()->Get(String::NewFromUtf8(globalIsolate, "_immediateCallback")));
    if(immediateFunc->IsFunction()){
        immediateFunc->Call(process,0,NULL);
    }

}

void doNodeJob(){
    v8::HandleScope scope(globalIsolate);
    Local<Object> global =  globalIsolate->GetCurrentContext()->Global();
    Local<Object> process = Local<Object>::Cast(global->ToObject()->Get(String::NewFromUtf8(globalIsolate, "process")));
    Local<Function> tickFunc = Local<Function>::Cast(process->ToObject()->Get(String::NewFromUtf8(globalIsolate, "_tickCallback")));
    tickFunc->Call(process,0,NULL);
    return;
}

void begin_uv_run(uv_loop_t* loop){
    bool more;
    do {
        more = uv_run(loop, UV_RUN_ONCE);
        if (more == false) {
            more = uv_loop_alive(loop);
            if (uv_run(loop, UV_RUN_NOWAIT) != 0)
                more = true;
        }
        doNodeJob();//这里在asyncCallbackScope中，所以makeback以后不会调用tick队列，这里手动调用tick队列并执行microtask
    } while (more == true);
    _exit(0);
}

void children_fork_worker(uv_loop_t* loop2,uv_pipe_t* pipe_t,int use_fd){

    uv_loop_init(loop2);
    int std_in = 0;
    int set = 0;
    dup2(use_fd, std_in);
    ioctl(std_in, FIONBIO, &set);
    uv_check_t *immediate_check_handle = new uv_check_t;
    uv_check_init(loop2, immediate_check_handle);
    uv_check_start(immediate_check_handle, check_immediate); //执行immediately列表
    uv_pipe_init(loop2, pipe_t, 1 /* ipc */);
    uv_pipe_open(pipe_t, std_in);
    uv_read_start((uv_stream_t*) pipe_t, alloc_buffer, on_child_work);
    begin_uv_run(loop2);
}

void sig_child( int signo ) {
    pid_t pid;
    int stat;
    pid = wait(&stat);
    printf( "child %d is dead\n", pid );
    int cpu_count;
    // launch same number of workers as number of CPUs
    uv_cpu_info_t *info;
    uv_cpu_info(&info, &cpu_count);
    cpu_count = cpu_count * 2;
    while (cpu_count--) {
        if(workers[cpu_count].pid == pid){
            printf( "child %d\n", workers[cpu_count].pid  );
            struct child_worker *worker = &workers[cpu_count];
            uv_pipe_init(loop, &worker->pipe, 1);
            pid_t pid;
            int fds[2];
            socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
            ioctl(fds[0], FIOCLEX);
            ioctl(fds[1], FIOCLEX);
            uv_pipe_open(&worker->pipe, fds[0]);
            if((pid = fork()) == 0){
                worker->loop = uv_default_loop();
                children_fork_worker(worker->loop,&worker->queue,fds[1]);
            }
            worker->pid = pid;
            fprintf(stderr, "Started worker %d\n", pid);
        }
    }
    return;
}

void on_new_task(uv_stream_t *server, int status) {
    if (status == -1) {
        // error!
        return;
    }
    uv_pipe_t *client = (uv_pipe_t*) malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, client, 0);

    if (uv_accept(server, (uv_stream_t*) client) == 0) {
        uv_write_t *write_req = (uv_write_t*) malloc(sizeof(uv_write_t));
        dummy_buf = uv_buf_init("a",1);
        struct child_worker *worker = &workers[round_robin_counter];
        uv_write2(write_req, (uv_stream_t*) &worker->pipe, &dummy_buf, 1, (uv_stream_t*) client, finish_write);
        round_robin_counter = (round_robin_counter + 1) % child_worker_count;
    }
    else {
        uv_close((uv_handle_t*) client, free_handle);
    }
}

void setup_workers() {
    round_robin_counter = 0;

    // ...
    int cpu_count;
    // launch same number of workers as number of CPUs

    uv_cpu_info_t *info;
    uv_cpu_info(&info, &cpu_count);
    uv_free_cpu_info(info, cpu_count);
    cpu_count = cpu_count * 2;
    child_worker_count = cpu_count;

    workers = (child_worker*) calloc(sizeof(struct child_worker), cpu_count);

    while (cpu_count--) {
        struct child_worker *worker = &workers[cpu_count];
        uv_pipe_init(loop, &worker->pipe, 1);
        int pid;
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        ioctl(fds[0], FIOCLEX);
        ioctl(fds[1], FIOCLEX);
        uv_pipe_open(&worker->pipe, fds[0]);
        if((pid = fork()) == 0){
            worker->loop = uv_default_loop();
            children_fork_worker(worker->loop,&worker->queue,fds[1]);
            break;
        }else{
            worker->pid = pid;
            fprintf(stderr, "Started worker %d\n", pid);
        }
    }
}

void register_task(const FunctionCallbackInfo<Value>& args){
    v8::HandleScope scope(globalIsolate);
    Local<Object> obj = args.Holder();
    Local<String> func_key = args[0]->ToString();
    Local<Value> func = args[1];
    Local<Object> target = Local<Object>::New(globalIsolate, funcs);
    if(funcs.IsEmpty()){
        printf(("empty object"));
        return;
    }
    target->Set(func_key,func);
}

void start_tasks(sem_t *pwsem){
//    loop = (uv_loop_t*) malloc(sizeof(uv_loop_t));
    loop = uv_default_loop();
    uv_loop_init(loop);
    reactor_pid = getpid();
    uv_timer_t *checker = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, checker);
    uv_timer_start(checker, check_master_parent_exist, FIRST_PARENT_CHECK_TIME, REPEAT_PARENT_CHECK_TIME);
    int r;
    fprintf(stderr, "reactor is running %d\n",getpid());

    uv_pipe_t *server = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, server, 0);
    if ((r = uv_pipe_bind(server, SERVER_SOCK))) {
        fprintf(stderr, "Bind error %s\n", uv_err_name(r));
        return;
    }
    if ((r = uv_listen((uv_stream_t*) server, BACKLOG, on_new_task))) {
        fprintf(stderr, "Listen error %s\n", uv_err_name(r));
        return;
    }
    if(sem_post(pwsem) < 0)
    {
        fprintf(stderr,"Fail to pwsem");
        return;
    }
    setup_workers();
    signal(SIGCHLD,  &sig_child);
//    struct sigaction sa;
//    struct sigaction old_signal_handler;
//    sa.sa_sigaction = handle_signal;
//    sigemptyset(&sa.sa_mask);
//    sa.sa_flags = SA_RESTART | SA_SIGINFO;
//    sigaction(SIGINT, &sa, &old_signal_handler);
    signal(SIGINT, handle_signal);
    signal(SIGSTOP, handle_signal);
    begin_uv_run(loop);
}

void prepare_start_tasks(){
    main_pid = getpid();
    int pid;
    sem_t *prsem;
    if((prsem = sem_open("mgw_sem",O_CREAT,0666,0)) == SEM_FAILED)
    {
        fprintf(stderr,"Fail to sem open");
        return;
    }
    if((pid = fork()) == 0){
        sem_t *pwsem;
        if((pwsem = sem_open("mgw_sem",O_CREAT,0666,1)) == SEM_FAILED)
        {
            fprintf(stderr,"Fail to sem open");
            return;
        }
        start_tasks(pwsem);
    }else{
        if(sem_wait(prsem) < 0)
        {
            fprintf(stderr,"Fail to pwsem");
            return;
        }
        have_init = 1;
    }
}

void init_parent_pipe(const FunctionCallbackInfo<Value>& args){
    Local<Value> function = args[0];
    Local<Object> self = args.Holder();
    if(function->IsFunction()){
        Local<Function> callback = Local<Function>::Cast(function);
        async_promise_callback.Reset(globalIsolate,callback);
    }
    prepare_start_tasks();
    args.GetReturnValue().Set(self);
}



