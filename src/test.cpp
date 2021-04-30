// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include "include/libplatform/libplatform.h"
#include "include/v8.h"
#include "tinyxml.h"

// 读取文件，返回其中的字符串
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

// JS接口：打印日志
static void log(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);

    v8::Local<v8::String> str = info[0]->ToString(context).ToLocalChecked();

    // 这种写法是错误的，会有野指针的情况
    //const char* strPara2 = *v8::String::Utf8Value(isolate, str);
    //std::cout << strPara2 << std::endl;

    v8::String::Utf8Value strPara2(isolate, str);
    std::cout << *strPara2 << std::endl;
}

// JS接口：
static void GetGameInstance(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);

    WorldContext* worldContext = reinterpret_cast<WorldContext*>((v8::Local<v8::External>::Cast(info.Data()))->Value());
    
    int32_t num = info[0]->Int32Value(context).ToChecked();
    
    v8::Local<v8::String> v8Str = info[1]->ToString(context).ToLocalChecked();
    v8::String::Utf8Value str(isolate, v8Str);

    char buffer[1024];
    sprintf_s(buffer, "%s:%d:%d,%s", worldContext->name.c_str(), worldContext->ver, num , *str);

    info.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, buffer, v8::NewStringType::kNormal).ToLocalChecked());
}

// 工具函数
static void ParseNode(v8::Isolate* isolate, v8::Local<v8::Object> object, TiXmlNode* pNode)
{
    if (!pNode) { return; }

    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::HandleScope handle_scope(isolate);
    v8::Context::Scope context_scope(context);

    TiXmlElement* pElem = pNode->ToElement();
    if (pElem)
    {
        for (TiXmlAttribute* pAttr = pElem->FirstAttribute(); pAttr; pAttr = pAttr->Next())
        {
            v8::Local<v8::String> v8Key = v8::String::NewFromUtf8(isolate, pAttr->Name(), v8::NewStringType::kNormal).ToLocalChecked();
            v8::Local<v8::String> v8Value = v8::String::NewFromUtf8(isolate, pAttr->Value(), v8::NewStringType::kNormal).ToLocalChecked();
            object->Set(context, v8Key, v8Value);
        }
    }

    for (TiXmlNode* pChild = pNode->FirstChild(); pChild; pChild = pChild->NextSibling())
    {
        switch (pChild->Type())
        {
        case TiXmlNode::TINYXML_ELEMENT:
        {
            v8::Local<v8::String> v8Key = v8::String::NewFromUtf8(object->GetIsolate(), pChild->Value(), v8::NewStringType::kNormal).ToLocalChecked();
            if (object->Get(context, v8Key).ToLocalChecked()->IsUndefined())
            {
                v8::Local<v8::Array> v8Array = v8::Array::New(context->GetIsolate());
                object->Set(context, v8Key, v8Array);
            }

            v8::Local<v8::Array> v8Array = object->Get(context, v8Key).ToLocalChecked().As<v8::Array>();
            assert(!v8Array->IsUndefined());

            v8::Local<v8::Object> v8ArrayElement = v8::Object::New(context->GetIsolate());
            ParseNode(isolate, v8ArrayElement, pChild);

            uint32_t arrayLen = v8Array->Length();
            v8::Local<v8::Number> v8ArrayIndex = v8::Number::New(isolate, arrayLen);

            v8Array->Set(context, v8ArrayIndex, v8ArrayElement);
        }
        break;
        }
    }
}

// JS接口：读取XML文件，返回JS对象
static void LoadXml(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    v8::Isolate* isolate = info.GetIsolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Context::Scope context_scope(context);

    v8::Local<v8::String> v8Path = info[0]->ToString(context).ToLocalChecked();
    v8::String::Utf8Value path(isolate, v8Path);

    TiXmlDocument doc(*path);
    doc.LoadFile();

    v8::Local<v8::Object> rootObject = v8::Object::New(isolate);
    ParseNode(isolate, rootObject, doc.RootElement());
    info.GetReturnValue().Set(rootObject);
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
            global->Set(context,
                v8::String::NewFromUtf8(isolate, "LoadXml", v8::NewStringType::kNormal).ToLocalChecked(),
                v8::FunctionTemplate::New(isolate, LoadXml, v8::External::New(isolate, &worldContext))->GetFunction(context).ToLocalChecked());
            global->Set(context,
                v8::String::NewFromUtf8(isolate, "log", v8::NewStringType::kNormal).ToLocalChecked(),
                v8::FunctionTemplate::New(isolate, log, v8::External::New(isolate, &worldContext))->GetFunction(context).ToLocalChecked());

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
