#pragma once

#include <Windows.h>

INT iJmpTo(LPVOID lpvMemoryBuffer, LPVOID lpvJumpDestination);

// Assembles the code needed to create a stackframe
INT iCreateStackFrame(LPVOID lpvMemoryBuffer);


INT HookFunction(LPVOID lpvFilterFunction, LPVOID *p_lpvBridgePointer, LPVOID target_fn);
INT iCreateGeneralBridge(LPVOID *p_lpvBridgePointer, void *fn);
