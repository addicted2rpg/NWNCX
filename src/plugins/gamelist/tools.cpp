//
//	http://www.InsaneDevelopers.net/
//

// Include this file only once
#pragma once



#include <windows.h>
#include <tchar.h>
#include "tools.h"


//
//	Code
//

// Assembles a jump
INT iJmpTo(LPVOID lpvMemoryBuffer, LPVOID lpvJumpDestination)
{
	//
	//	Local variables
	//

	// Previous memory protection
		DWORD dwOldProtection;
	// Destination memory
		LPBYTE p_btDestinationMemory = (LPBYTE) lpvMemoryBuffer;




	//
	//	Code
	//

	// Assemble the JMP opcode
	if (!VirtualProtect(lpvMemoryBuffer, 5, PAGE_EXECUTE_READWRITE, &dwOldProtection))
		return 0;

	*p_btDestinationMemory = 0xE9;
	p_btDestinationMemory++;
	*((LPDWORD) p_btDestinationMemory) = (((LPBYTE) lpvJumpDestination) - p_btDestinationMemory - 4);

	if (!VirtualProtect(lpvMemoryBuffer, 5, dwOldProtection, &dwOldProtection))
		return 0;

	return 1;
}




// Assembles the code needed to create a stackframe
INT iCreateStackFrame(LPVOID lpvMemoryBuffer)
{
	//
	//	Local variables
	//

	// Previous memory protection
		DWORD dwOldProtection;
	// Destination memory
		LPBYTE p_btDestinationMemory = (LPBYTE) lpvMemoryBuffer;




	//
	//	Code
	//

	// Notes
	// The first opcode (mov edi, edi) is not part of the stackframe creation
	// code (it is used by Windows), but i added it here for simplicity.

	// Writes the opcodes
	if (!VirtualProtect(lpvMemoryBuffer, 5, PAGE_EXECUTE_READWRITE, &dwOldProtection))
		return 0;

	// mov edi, edi
	*((LPWORD) p_btDestinationMemory) = 0xFF89;
	p_btDestinationMemory += 2;

	// push ebp
	*((LPWORD) p_btDestinationMemory) = 0x55;
	p_btDestinationMemory++;

	// mov ebp, esp
	*((LPWORD) p_btDestinationMemory) = 0xEC8B;
	p_btDestinationMemory += 2;

	if (!VirtualProtect(lpvMemoryBuffer, 5, dwOldProtection, &dwOldProtection))
		return 0;

	return 1;
}



INT iCreateGeneralBridge(LPVOID *p_lpvBridgePointer, void *fn)
{
		HANDLE hdlProcessHeap;
		DWORD dwOldProtection;


	// Create the bridge
	hdlProcessHeap = GetProcessHeap();
	if (!hdlProcessHeap)
		return 0;

	*p_lpvBridgePointer = HeapAlloc(hdlProcessHeap, HEAP_ZERO_MEMORY, 10);
	if (!*p_lpvBridgePointer)
		return 0;

	// Make it executable
	if (!VirtualProtect(*p_lpvBridgePointer, 10, PAGE_EXECUTE_READWRITE, &dwOldProtection))
		return 0;

	// Restore the stackframe code
	if (!iCreateStackFrame(*p_lpvBridgePointer))
		return 0;

	// Add a jump to the rest of the function
	if (!iJmpTo(((LPBYTE) *p_lpvBridgePointer) + 5, ((LPBYTE) fn) + 5))
		return 0;

	return 1;
}





int HookFunction(LPVOID lpvFilterFunction, LPVOID *p_lpvBridgePointer, LPVOID target_fn) {


	// Write the jump at the first bytes of the function
	if (!iJmpTo( target_fn, lpvFilterFunction)) {
		MessageBox(NULL, _T("Failed to create hook portion of bridge."), _T("Error"), MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		return 0;
	}

	// Create the bridge
	if (!iCreateGeneralBridge(p_lpvBridgePointer, target_fn)) {
				MessageBox(NULL, _T("Failed to create callback portion of bridge."), _T("Error"), MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		return 0;
	}


	return 1;
}



