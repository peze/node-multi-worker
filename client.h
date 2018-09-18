//
// Created by hehe on 18/9/17.
//

#ifndef NODE_WORKER_CLIENT_H
#define NODE_WORKER_CLIENT_H
#include "util-inl.h"
class Client{
private:
    class dataSend{
    private:
        v8::Persistent<v8::Function> cb;
        char* prameter_json;
        char* method_func;
        Isolate* isolate;
        uv_handle_t* _handle;
        string package;
        char* package_str;
    public:
        dataSend(v8::Isolate* globalIsolate,
                 uv_handle_t* handle,
                 Local<Function> cb_f,
                 const char* prameter_json_pa,
                 char* method)
                : cb(globalIsolate,cb_f),
                  isolate(globalIsolate),
                  _handle(handle){
            _handle->data = this;
            prameter_json = new char[BUFF_SIZE];
            method_func = new char[METHOD_LONG];
            strcpy(prameter_json,prameter_json_pa);
            strcpy(method_func,method), package = "";
        };
        ~dataSend(){
//            uv_close((uv_handle_t*) this->_handle, NULL);
//            free(this->_handle);
//            delete [] prameter_json;
            free(package_str);
        }
        const char* getParameter(){
            return prameter_json;
        }
        const char* getMethod(){
            return method_func;
        }
        const Local<Function> getCallback(){
            return Local<Function>::New(isolate, cb);

        }
        void addPackage(const char* str){
            package += str;
        }
        char* getPackage(){
            int length = this->getPackageLength();
            package_str = (char*)malloc(length);
            snprintf(package_str,length,"%s",package.c_str());
            return package_str;
        }
        int getPackageLength(){
            return package.length() + 1;
        }
    };
public:
    Client(){

    }

    ~Client(){
//        delete this->server_addr;

    }
    static void solve_data(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);
    static void on_read(uv_write_t *req, int status);
    static void on_connect(uv_connect_t *req, int status);
    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    void send(char* method,const char* val,Local<Value> func);
};

void client_send(const v8::FunctionCallbackInfo<Value> &args);

void empty_function(const v8::FunctionCallbackInfo<Value> &args);
#endif //NODE_WORKER_CLIENT_H
