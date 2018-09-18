//
// Created by hehe on 18/9/17.
//
#include "client.h"
extern Isolate* globalIsolate;
extern int have_init;
extern int main_pid;
void exec_send_task(uv_timer_t* timer){
    uv_pipe_t* send_pipe = (uv_pipe_t*) timer->data;
    if(have_init == 1){
        uv_connect_t* connect_req = (uv_connect_t*) malloc(sizeof(uv_connect_t));
        uv_pipe_connect(connect_req, send_pipe, SERVER_SOCK, Client::on_connect);
    }else{
        uv_timer_start(timer, exec_send_task, WAIT_TIME, TIMER_NO_REPEAT);
    }
}

void empty_function(const v8::FunctionCallbackInfo<Value> &args){
    String::Utf8Value str(args[0]);
    printf("%s",*str);
    return;
}

void Client::solve_data(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        HandleScope scope(globalIsolate);
        Local<Context> context = globalIsolate->GetCurrentContext();
        dataSend* data_send = reinterpret_cast<dataSend*>(client->data);
        data_send->addPackage(buf->base);
        char* package = data_send->getPackage();
        if(!read_finish(buf->base,';')){
            return;
        }
        char* result = (char*) malloc(data_send->getPackageLength());
        get_from_father(package,result,0x3a);
        get_from_father(package,result,';');
        MaybeLocal<Value> json_ob = JSON::Parse(globalIsolate,String::NewFromUtf8(globalIsolate, result));
        Local<Value> NullObj = Local<Value>::Cast(Null(globalIsolate));
        Local<Value> parameters[2] = {NullObj,NullObj};
        if(json_ob.IsEmpty()){
            parameters[1] = String::NewFromUtf8(globalIsolate, result ,NewStringType::kNormal).ToLocalChecked();;
        }else{
            parameters[1] = json_ob.ToLocalChecked();
        }
        data_send->getCallback()->Call(context->Global(),2,parameters);
//        delete data_send;
        free(result);
    }
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, free_handle);
    }

    free(buf->base);

}

void Client::on_read(uv_write_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "write failed error %s\n", uv_err_name(status));
        free(req);
        return;
    }
    uv_read_start((uv_stream_t*) req->handle, alloc_buffer, Client::solve_data);
    free_write_req(req,true);
}


void Client::on_connect(uv_connect_t *req, int status){
    if (status < 0) {
        fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
        free(req);
        return;
    }
    write_req_t *write_req = (write_req_t*) malloc(sizeof(write_req_t));
    uv_pipe_t *socket = (uv_pipe_t*) req->handle;
    dataSend* send_data = static_cast<dataSend*>(req->handle->data);
    unsigned int length = BUFF_SIZE + METHOD_LONG + 1;
    char* buf = new char[length];
    sprintf(buf,"%s:split$%s;split$",send_data->getMethod(),send_data->getParameter());
    write_req->buf = uv_buf_init(buf, length);
    uv_write((uv_write_t*) write_req, (uv_stream_t*) socket, &write_req->buf, 1, Client::on_read);
    free(req);
    return;
};

void Client::send(char* method,const char* val,Local<Value> func){

    HandleScope scope(globalIsolate);
    Local<Function> cb = Local<Function>::Cast(func);
    uv_loop_t* default_loop = uv_default_loop();
    uv_pipe_t* send_pipe = (uv_pipe_t*) malloc(sizeof(uv_pipe_t));
    uv_pipe_init(default_loop,send_pipe,0);
    Client::dataSend* send_data = new Client::dataSend(globalIsolate,(uv_handle_t*) send_pipe,cb,val,method);
    if(have_init == 1){
        uv_connect_t* connect_req = (uv_connect_t*) malloc(sizeof(uv_connect_t));
        uv_pipe_connect(connect_req, send_pipe, SERVER_SOCK, Client::on_connect);
    }else{
        uv_timer_t* send_task = (uv_timer_t*)malloc(sizeof(uv_timer_t));
        send_task->data = (void*)send_pipe;
        uv_timer_init(default_loop, send_task);
        uv_timer_start(send_task, exec_send_task, WAIT_TIME, TIMER_NO_REPEAT);
    }


}

void  Client::New(const v8::FunctionCallbackInfo<v8::Value>& args){
    if(!args.IsConstructCall()){
        NODEJS_ERROR("Client must be called by a Constructor: %d", 1,globalIsolate);
        return;
    }
    if(getpid() != main_pid){
        return;
    }
    Local<Object> self = args.This();
    Client *client = new Client();
    self->SetInternalField(0, External::New(globalIsolate, client));
    args.GetReturnValue().Set(self);
}



void client_send(const v8::FunctionCallbackInfo<Value> &args) {
    Local<Object> self = args.Holder();
    Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
    void *ptr = wrap->Value();
    int length = args.Length();
    if(length < 2){
        NODEJS_ERROR("expected at least %d arguments", 2,globalIsolate);
        return;
    }
    String::Utf8Value methodUtf8(args[0]);
    Local<Value> params = args[1];
    std::string params_json = json_str(globalIsolate,params);
    const char* parameters = params_json.c_str();
    Local<Value> callback;
    if(length == 2){
        Local<FunctionTemplate> def_func_tem = FunctionTemplate::New(globalIsolate, empty_function);
        callback = Local<Value>::Cast(def_func_tem->GetFunction());
    }
    if(length == 3){
        callback = args[2];
    }
    static_cast<Client*>(ptr)->send(*methodUtf8,parameters,callback);
}