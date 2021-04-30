#pragma once

#include "include/v8.h"

inline v8::Local<v8::String> InternalString(v8::Isolate* isolate, const char* str)
{
    return v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal).ToLocalChecked();
}
