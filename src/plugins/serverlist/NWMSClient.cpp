#ifndef _WINSOCK2API_
#include <winsock2.h>
#endif
#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <string.h>

#include "NWNMSClient.h"
#include "soap/soapWSHttpBinding_USCOREINWNMasterServerAPIProxy.h"
#include "soap/WSHttpBinding_USCOREINWNMasterServerAPI.nsmap"

NWNMSClient::NWNMSClient(FILE *logFile, ServerListCallback_t serverListCallback)
{
	this->logFile = logFile;
	this->serverListCallback = serverListCallback;

	InitializeCriticalSection(&cs);
}

NWNMSClient::~NWNMSClient() {

}

void NWNMSClient::RequestServerList(int roomId)
{
	fprintf(logFile, "Creating thread...\n");
	fflush(logFile);

	this->currentRoom = roomId;

	// params is virusman's struct for tracking client object and roomid :)
	// imo I would of thought malloc is better but I guess MS is smart enough enough not to delete 
	// at end of function when using their QueueUserWorkItem (a ref to the allocated memory)
	RequestThreadParams *params = new RequestThreadParams;
	params->client = this;
	params->roomId = roomId;
	// that last NULL parameter is the same as WT_EXECUTEDEFAULT... good grief..
	QueueUserWorkItem((LPTHREAD_START_ROUTINE) NWNMSClient::RequestThread, params, NULL);
}

void NWNMSClient::Update()
{
	if(HasResults())
	{
		ServerListResult result = PopResult();
		if (result.roomId == this->currentRoom)
			this->serverListCallback(result);
		//delete result.servers;
		delete result.api;
	}
}

void NWNMSClient::PushResult(ServerListResult result)
{
	EnterCriticalSection(&cs);
	this->resultQueue.push(result);
	LeaveCriticalSection(&cs);
}

ServerListResult NWNMSClient::PopResult()
{
	ServerListResult result;

	EnterCriticalSection(&cs);
	result = this->resultQueue.front();
	this->resultQueue.pop();
	LeaveCriticalSection(&cs);

	return result;
}

bool NWNMSClient::HasResults()
{
	bool result;
	EnterCriticalSection(&cs);
	result = !this->resultQueue.empty();
	LeaveCriticalSection(&cs);
	return result;
}

// This is the start function for the queued thread.  It doesn't actually request a thread :)
DWORD WINAPI NWNMSClient::RequestThread(void *param)
{
	NWNMSClient *client;
	int roomId;
	FILE *logFile;

	//Get the params from struct and delete it
	RequestThreadParams *params = (RequestThreadParams *)param;
	if (!params)
		return NULL;

	client  = params->client;
	roomId  = params->roomId;
	logFile = client->logFile;
	delete params;

	//Query the master server
	NWNMasterServerAPIProxy *api = new NWNMasterServerAPIProxy();
	GameLookup srv_request;
	GameLookupResponse srv_response;

	api->soap_endpoint = API_ENDPOINT;
	//TODO: cleanup
	srv_request.Product = (char *)soap_malloc(api, sizeof(char) * 5);  // NWN1 plus a null character.
	strcpy_s(srv_request.Product, sizeof(char) * 5,  "NWN1");	
	srv_request.GameType = (unsigned int *)soap_malloc(api, sizeof(int));
	*(srv_request.GameType) = RoomToSkywing(roomId);
	
	int res = api->LookupServerByGameType(&srv_request, &srv_response);
	if(res != SOAP_OK) {
		//MessageBoxA(NULL, GetErrorMessage(res), "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		//fprintf(logFile, GetErrorMessage(res));
		delete api;
		return NULL;
	}
	
	ArrayOfNWGameServer *servers = srv_response.LookupServerByGameTypeResult; // srv_response->GetOnlineServerListResult;
	if(servers == NULL) {
		//MessageBoxA(NULL, "This should never happen; The Gamelist is NULL - (no results, maybe?)", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);		
		//fprintf(logFile, "This should never happen; The Gamelist is NULL - (no results, maybe?)");
		delete api;
		return NULL;
	}

	//Hooray!!
	ServerListResult result;
	result.roomId = roomId;
	result.api = api;
	result.servers = servers;
	client->PushResult(result);

	return NULL;
}



int NWNMSClient::RoomToSkywing(int room) {

	switch(room) {
	case 274:
		return 0; break;
	case 275:
		return 3; break;
	case 276:
		return 4; break;
	case 277:
		return 7; break;
	case 278:
		return 9; break;
	case 279:
		return 8; break;
	case 363:
		return 1; break;
	case 364:
		return 2; break;
	case 365:
		return 5; break;
	case 366:
		return 6; break;
	case 367:
		return 10; break;
	case 368:
		return 11; break;
	case 370:
		return 12; break;
	default:
		return 3; break;  // just put it in Roleplay :P
	}

}

const char * NWNMSClient::GetErrorMessage(int res)
{

	if(soap_xml_error_check(res)) {
		return "Server Listing Failed.  It was an XML issue.\n";
	}
	else if(soap_soap_error_check(res)) {
		"Server Listing Failed.  SOAP itself had some kind of problem.\n";
	}
	else if(soap_tcp_error_check(res)) {
		"Server Listing Failed.  There was a TCP problem.\n";
	}
	else if(soap_ssl_error_check(res)) { 
		"Server Listing Failed.  There was a SSL problem.\n";
	}
	else if(soap_zlib_error_check(res)) { 
		"Server Listing Failed.  There was a ZLIB error!\n";
	}
	else if(soap_http_error_check(res)) {
		if(res == 415) {
			"Server Listing Failed.  Got back HTTP/Unsupported Media.  SOAP XML is probably for old bindings.\n";
		} else {
			"Server Listing Failed.  It was an HTTP error!\n";
		}
	}
	else {
		return "Server Listing Failed.  We have no idea why.  SOAP hates you.\n";
		
	}

	return "Undiagnosed error.\n";
}