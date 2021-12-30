#pragma once

#include <string>

/**
 * @param debuginfoPath debuginfo-path for eu-addr2line
 * @param ip caller instruction pointer for the case when going through a signal handler
 * @param framesToSkip how many stack frames to skip
 * @param printThreadId print ID of the thread on every line
 */
void printFormatBacktrace(const std::string& debuginfoPath, void *ip = nullptr, size_t framesToSkip = 0, bool printThreadId = false);
