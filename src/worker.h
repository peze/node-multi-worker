#ifndef MAGICARE_WORKER_H
#define MAGICARE_WORKER_H

#include "util-inl.h"

static uv_loop_t *loop;
static sigset_t setmask;


static struct child_worker {
    uv_loop_t* loop;
    int pid;
    uv_process_options_t options;
    uv_pipe_t pipe;
    uv_pipe_t queue;
    v8::Isolate* isolate;
} *workers;

static int round_robin_counter;
static int child_worker_count;

class PromiseDeliver{
public:
    PromiseDeliver(v8::Isolate* globalIsolate,uv_handle_t* handle,uv_stream_t *client,uv_write_t *req,Local<Object> promise)
            :client(client),
             req(req),
             isolate(globalIsolate),
             promise(globalIsolate,promise),_handle(handle){
        _handle->data = this;
    };
    uv_stream_t* getClient(void){
        return this->client;
    };
    uv_write_t* getReq(void){
        return this->req;
    };
    v8::Local<v8::Object> getPromise(void){
        return Local<Object>::New(isolate, promise);
    };

private:
    uv_handle_t* _handle;
    uv_stream_t *client;
    uv_write_t *req;
    Isolate* isolate;
    v8::Persistent<v8::Object> promise;
};

void init_parent_pipe(const FunctionCallbackInfo<Value>& args);

void register_task(const FunctionCallbackInfo<Value>& args);

#endif //MAGICARE_WORKER_H