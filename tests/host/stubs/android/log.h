#pragma once
// Minimal stand-in for the Android log API; test_stubs.cpp records the lines.
enum { ANDROID_LOG_DEBUG = 3, ANDROID_LOG_INFO = 4, ANDROID_LOG_WARN = 5, ANDROID_LOG_ERROR = 6 };
extern "C" int __android_log_print(int prio, const char *tag, const char *fmt, ...);
