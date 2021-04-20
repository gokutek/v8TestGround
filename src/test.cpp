// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>

#include "include/libplatform/libplatform.h"
#include "include/v8.h"

static std::string load_file(const char* path)
{
    std::string result;

    std::ifstream ifs(path);
    if (ifs.is_open())
    {
        ifs.seekg(0, std::ios_base::end);
        int64_t length = ifs.tellg();
        ifs.seekg(0, std::ios_base::beg);
        result.resize(length + 1);
        ifs.read(&result[0], length);
        ifs.close();
    }

    return result;
}

struct WorldContext
{
    std::string name;
    int         ver;
};

static void GetGameInstance(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);

    WorldContext* worldContext = reinterpret_cast<WorldContext*>((v8::Local<v8::External>::Cast(info.Data()))->Value());
    
    int32_t para1 = info[0]->Int32Value(context).ToChecked();
    
    v8::Local<v8::String> para2 = info[1]->ToString(context).ToLocalChecked();
    const char* strPara2 = *v8::String::Utf8Value(isolate, para2);

    info.GetReturnValue().Set(666);
}

int main(int argc, char* argv[]) 
{
    // 初始化V8
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    // 创建Isolate对象 and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate* isolate = v8::Isolate::New(create_params);
    {
        v8::Isolate::Scope isolate_scope(isolate);

        /**
         * 在栈上创建HandleScope.
         * 尽管后面的代码没有直接使用handle_scope变量，但它还是要必须创建的。
         * 如果注掉这行代码，后面在创建Context时就会报错：
         *  # Fatal error in v8::HandleScope::CreateHandle()
         *  # Cannot create a handle without a HandleScope
         */
        v8::HandleScope handle_scope(isolate);

        /**
         * 创建Context对象
         * 关于Local的说明可以去看下注释。
         * v8返回给我们的对象基本上都会用到它。
         */
        v8::Local<v8::Context> context = v8::Context::New(isolate);

        /**
         * Enter the context for compiling and running the hello world script.
         * 尽管后面的代码没有直接使用handle_scope变量，但它还是要必须创建的，否则也会引发异常。
         */
        v8::Context::Scope context_scope(context);

        {
            //注册一个C接口给JS调用
            WorldContext worldContext;
            worldContext.name = "TOM";
            worldContext.ver = 2021;
            v8::Local<v8::Object> global = context->Global();
            global->Set(context,
                v8::String::NewFromUtf8(isolate, "GetGameInstance", v8::NewStringType::kNormal).ToLocalChecked(),
                v8::FunctionTemplate::New(isolate, GetGameInstance, v8::External::New(isolate, &worldContext))->GetFunction(context).ToLocalChecked());

            // Create a string containing the JavaScript source code.
            std::string data = load_file("test.js");
            v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, data.c_str(), v8::NewStringType::kNormal).ToLocalChecked();

            // 编译
            v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();

            // 执行
            v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

            // Convert the result to an UTF8 string and print it.
            v8::String::Utf8Value utf8(isolate, result);
            printf("%s\n", *utf8);
        }
    }

    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}
