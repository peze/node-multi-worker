//
// Created by page on 17/1/11.
//

#include "client.h"
#include "worker.h"

Isolate* globalIsolate;
Persistent<Object> funcs;
int have_init = 0;
int reactor_pid;
int main_pid;

void init(Local<Object> exports) {
    /*注册api函数*/
    globalIsolate = exports->GetIsolate();
    Local<Object> emptyObject = Local<Object>::Cast(JSON::Parse(globalIsolate,String::NewFromUtf8(globalIsolate, "{}")).ToLocalChecked());
    funcs.Reset(globalIsolate,emptyObject);
    exports->Set(String::NewFromUtf8(globalIsolate, "register"), FunctionTemplate::New(globalIsolate,register_task)->GetFunction());
    exports->Set(String::NewFromUtf8(globalIsolate, "init"), FunctionTemplate::New(globalIsolate,init_parent_pipe)->GetFunction());
    Local<FunctionTemplate> clientClass = FunctionTemplate::New(globalIsolate, Client::New);
    clientClass->SetClassName(v8::String::NewFromUtf8(globalIsolate, "Client"));
    v8::Local<ObjectTemplate> clientClassPrototype = clientClass->PrototypeTemplate();
    clientClassPrototype->Set(String::NewFromUtf8(globalIsolate, "send"), FunctionTemplate::New(globalIsolate, client_send));
    Local<ObjectTemplate> clientInstance = clientClass->InstanceTemplate();
    clientInstance->SetInternalFieldCount(1);
    exports->Set(String::NewFromUtf8(globalIsolate, "Client"), clientClass->GetFunction());
}

NODE_MODULE(binding, init)