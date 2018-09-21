//
// Created by hehe on 18/9/17.
//

#ifndef NODE_WORKER_UTIL_INL_H
#define NODE_WORKER_UTIL_INL_H
#include <iostream>
#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <unistd.h>
#include <map>
#include <string>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#define BUFF_SIZE 4096
#define METHOD_LONG 64
#define WAIT_TIME 1000
#define TIMER_NO_REPEAT -1
#define FIRST_PARENT_CHECK_TIME 10 * 1000
#define REPEAT_PARENT_CHECK_TIME 5 * 1000
#define BACKLOG 128
#define SERVER_SOCK "node-worker.sock"

#define THROW_ERROR(bufname, x,isolate) { \
    char bufname[256] = {0}; \
    sprintf x; \
    isolate->ThrowException(Exception::Error (String::NewFromUtf8(isolate,bufname))); \
}

#define NODEJS_ERROR( strfmt, arg ,isolate) THROW_ERROR( _buf, (_buf, (const char*)strfmt, arg), isolate)

using namespace v8;
using namespace node;
using namespace std;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

inline void free_handle(uv_handle_t *handle){
    free(handle);
}

inline void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base =(char *) malloc(suggested_size);
    buf->len = suggested_size;
}

inline void free_write_req(uv_write_t *req,bool free_buf) {
    write_req_t *wr = (write_req_t*) req;
    if(free_buf){
        free(wr->buf.base);
    }
    free(wr);
}

inline int get_from_father(char* parameter ,char* task_id,char key){
    char* tmp_str = strchr(parameter,key);
    char dem[8];
    dem[0] = key;
    strncpy(dem + 1,"split$",6);
    dem[7] = '\0';
    while(tmp_str != NULL && strncmp(tmp_str,dem,7)){
        tmp_str = strchr(tmp_str + 1,key);
    }
    if(tmp_str == NULL){
        return 0;
    }
    strncpy(task_id,parameter,strlen(parameter) - strlen(tmp_str));
    task_id[strlen(parameter) - strlen(tmp_str)] = '\0';
    strncpy(parameter,tmp_str + 7,strlen(tmp_str + 7));
    parameter[strlen(tmp_str + 7)] = '\0';
    return 1;
}

inline int split_task_key(char* parameter ,char* task_id,char key){
    if(parameter == NULL){
        return 0;
    }
    char* func = strchr(parameter,key);
    if(func == NULL){
        strncpy(task_id,parameter,strlen(parameter));
        task_id[strlen(parameter)] = '\0';
        parameter = NULL;
        return 1;
    }
    strncpy(task_id,parameter,strlen(parameter) - strlen(func));
    task_id[strlen(parameter) - strlen(func)] = '\0';
    strncpy(parameter,func + 1,strlen(func + 1));
    parameter[strlen(func + 1)] = '\0';
    return 1;
}

inline std::string json_str(Isolate* isolate, Local<Value> value) {
    if (value.IsEmpty())
    {
        return std::string();
    }
//    if(value->IsString()){
//        String::Utf8Value const result(value);
//        return std::string(*result, result.length());
//    }
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Value> result = JSON::Stringify(context,Local<Object>::Cast(value)).ToLocalChecked();
    String::Utf8Value const str(result);
    return std::string(*str);
}

inline bool read_finish(char* str,char key){
    char dem[8];
    dem[0] = key;
    strncpy(dem + 1,"split$",6);
    dem[7] = '\0';
    char* tmp_str = strchr(str,key);
    while(tmp_str  != NULL){
        if(!strncmp(tmp_str,dem,7)){
            return true;
        }
        tmp_str = strchr(tmp_str,key);
    };
    return false;

}
#endif //NODE_WORKER_UTIL_INL_H
