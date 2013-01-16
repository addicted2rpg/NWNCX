// (c) Addicted2rpg (2013) - see core.cpp for License (GNU).


#ifndef _WINSOCK2API_
#include <winsock2.h>
#endif
#pragma comment(lib, "Ws2_32.lib")


#include <stdio.h>
//#include <tchar.h>
#include <string.h>
#include "gamelist.h"

// my own set of hooking tools because I have no idea how that detour lib works :)
// its actually some very short codes...
#include "tools.h"



#define NEW_MASTER "nwn1.mst.valhallalegends.com"





//#include "soapWSHttpBinding_USCOREINWNMasterServerAPIService.h"
#include "soapWSHttpBinding_USCOREINWNMasterServerAPIProxy.h"
#include "WSHttpBinding_USCOREINWNMasterServerAPI.nsmap"





//typedef WSHttpBinding_USCOREINWNMasterServerAPIService MasterService;
typedef WSHttpBinding_USCOREINWNMasterServerAPIProxy MasterProxy;
typedef _ns1__GetOnlineServerList ServerList;
typedef _ns1__GetOnlineServerListResponse ListResponse;
typedef ns4__ArrayOfNWGameServer GameServerArray;
typedef ns4__NWGameServer GameServer;


// Globals and function decs.
struct gamelist_struct **s_gamelist = NULL; 
unsigned int s_gamelist_length;

LPVOID gethostbynameBridge = NULL;
struct hostent* WINAPI gethostbynameFilter(_In_ char *name);

const char *endpoint = "http://api.mst.valhallalegends.com/NWNMasterServerAPI/NWNMasterServerAPI.svc/ASMX";


void WINAPI InitHooks();
void WINAPI GameList();
int SOAPErrorCheck(int res);
const char *InterpretPVPType(unsigned int x);
void cleanup_gamelist_structure();


extern void LogMessage(char *s);

const char *pvpfull = "Full";
const char *pvpparty = "Party";
const char *pvpnone = "None";

void WINAPI InitHooks() {
		if(!HookFunction((LPVOID) gethostbynameFilter, &gethostbynameBridge, (LPVOID) gethostbyname)) {
		MessageBoxA(NULL, "Could not initialize gethostbyname() hook", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		ExitProcess(1);
	}
}

void WINAPI GameList() {
	int numServers, i;
//	char debuggyA[512];
	int res;

	MasterProxy   *listserver = NULL;
	ServerList *srv_list = NULL;
	ListResponse *srv_response = NULL;
	GameServerArray *game_list = NULL;
	GameServer *game = NULL;


	
	listserver = new MasterProxy();
	listserver->soap_endpoint = endpoint;

	srv_list = new ServerList();
	srv_list->Product = (char *)malloc(sizeof(char) * 5);  // NWN1 plus a null character.
	strcpy_s(srv_list->Product, sizeof(char) * 5,  "NWN1");
	
	srv_response = new ListResponse();

	res = listserver->GetOnlineServerList(srv_list, srv_response);
	if(SOAPErrorCheck(res)) {
		// You didn't think I would forget about those 5 bytes, did you?
		free(srv_list->Product);
		return;
	}

	
	game_list = srv_response->GetOnlineServerListResult;
	if(game_list == NULL) {
		MessageBoxA(NULL, "Server Listing Failed.  Better luck next time!", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);		
		free(srv_list->Product);
		return;
	}
	
	numServers = game_list->__sizeNWGameServer;

	// Is the array allocated already?  If so, we need to de-allocate and setup for a new listing.
	if(s_gamelist != NULL) {
		cleanup_gamelist_structure();
	}

	// Malloc() the 2D array...
	s_gamelist = (struct gamelist_struct **)malloc(sizeof(struct gamelist_struct *) * numServers);
	s_gamelist_length = numServers;

	for(i=0;i < numServers;i++) {
		s_gamelist[i] = (struct gamelist_struct *)malloc(sizeof(struct gamelist_struct));
		
		game = game_list->NWGameServer[i];		
		
		s_gamelist[i]->is_online = (int) *(game->Online);
		
		// A NULL value here should never happen, or I should say, Skywing shouldn't let it happen :)
		// But be that as it may, let's check for good practice.
		if(game->ServerName == NULL) {
			s_gamelist[i]->server_name = (char *)malloc(1);
			s_gamelist[i]->server_name[0] = '\0';
		}
		else {
			s_gamelist[i]->server_name = (char *)malloc(sizeof(char) * strlen(game->ServerName));
			strncpy(s_gamelist[i]->server_name, game->ServerName, strlen(game->ServerName));
		}

		// As before.
		if(game->ModuleName == NULL) {
			s_gamelist[i]->module_name = (char *)malloc(1);
			s_gamelist[i]->module_name[0] = '\0';
		}
		else {
			s_gamelist[i]->module_name = (char *)malloc(sizeof(char) * strlen(game->ModuleName));
			strncpy(s_gamelist[i]->module_name, game->ModuleName, strlen(game->ModuleName));
		}

		s_gamelist[i]->active_players = (unsigned int) *(game->ActivePlayerCount);
		s_gamelist[i]->maximum_players = (unsigned int) *(game->MaximumPlayerCount);
		s_gamelist[i]->minimum_level = (unsigned int) *(game->MinimumLevel);
		s_gamelist[i]->maximum_level = (unsigned int) *(game->MaximumLevel);
		s_gamelist[i]->pvp_level = (unsigned int) *(game->PVPLevel);
		// source string is static, so vanilla strcpy is safe.
		strcpy(s_gamelist[i]->pvp_text, InterpretPVPType(*(game->PVPLevel)));

		s_gamelist[i]->allow_localvault = (int) *(game->LocalVault);
		s_gamelist[i]->is_private_server = (int) *(game->PrivateServer);
		s_gamelist[i]->buildnumber = (unsigned int) *(game->BuildNumber);
		s_gamelist[i]->expansion = (unsigned int) *(game->ExpansionsMask);
		s_gamelist[i]->last_heartbeat = (time_t) *(game->LastHeartbeat);

		s_gamelist[i]->ELC_enabled = (int) *(game->ELCEnforced);
		s_gamelist[i]->ILR_enabled = (int) *(game->ILREnforced);
		s_gamelist[i]->one_party = (int) *(game->OnePartyOnly);
		s_gamelist[i]->player_pause = (int) *(game->PlayerPause);
		
		// All these are as before - ideally wouldn't be null anyway
		if(game->ServerDescription == NULL) { 
			s_gamelist[i]->server_desc = (char *)malloc(1);
			s_gamelist[i]->server_desc[0] = '\0';
		}
		else {
			s_gamelist[i]->server_desc = (char *)malloc(sizeof(char) * strlen(game->ServerDescription));
			strncpy(s_gamelist[i]->server_desc, game->ServerDescription, strlen(game->ServerDescription));
		}

		if(game->ServerAddress == NULL) {
			s_gamelist[i]->server_address = (char *)malloc(1);
			s_gamelist[i]->server_address[0] = '\0';
		}
		else {
			s_gamelist[i]->server_address = (char *)malloc(sizeof(char) * strlen(game->ServerAddress));
			strncpy(s_gamelist[i]->server_address, game->ServerAddress, strlen(game->ServerAddress));
		}
	
		if(game->ModuleDescription == NULL) {
			s_gamelist[i]->module_desc = (char *)malloc(1);
			s_gamelist[i]->module_desc[0] = '\0';
		}
		else {
			s_gamelist[i]->module_desc = (char *)malloc(sizeof(char) * strlen(game->ModuleDescription));
			strncpy(s_gamelist[i]->module_desc, game->ModuleDescription, strlen(game->ModuleDescription));
		}

		if(game->ModuleUrl == NULL) {
			s_gamelist[i]->module_url = (char *)malloc(1);
			s_gamelist[i]->module_url[0] = '\0';
		}
		else {
			s_gamelist[i]->module_url = (char *)malloc(sizeof(char) * strlen(game->ModuleUrl));
			strncpy(s_gamelist[i]->module_url, game->ModuleUrl, strlen(game->ModuleUrl));
		}

	}


	free(srv_list->Product);
	
}


struct hostent* WINAPI gethostbynameFilter(_In_ char *name) {
	struct hostent* original;



	if (strcmp(name, "nwn.master.gamespy.com") == 0 
		|| strcmp(name, "nwmaster.bioware.com") == 0 
		) {

		original = (((struct hostent* (WINAPI *)(_In_ char *))gethostbynameBridge))(NEW_MASTER);
	}	
	else {
		original = (((struct hostent* (WINAPI *)(_In_ char *))gethostbynameBridge))(name);
	}

	return original;


}





// returns 0 if everything is OK.
// returns 1 if there was a problem, and it gives a proper textbox and explanation of WTF it was.
int SOAPErrorCheck(int res) {
	if(res != SOAP_OK) {
		if(soap_xml_error_check(res)) {
			MessageBoxA(NULL, "Server Listing Failed.  It was an XML error!", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		}
		else if(soap_soap_error_check(res)) {
			MessageBoxA(NULL, "Server Listing Failed.  SOAP itself had some kind of problem.", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		}
		else if(soap_tcp_error_check(res)) {
			MessageBoxA(NULL, "Server Listing Failed.  There was a TCP problem!", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		}
		else if(soap_ssl_error_check(res)) { 
			MessageBoxA(NULL, "Server Listing Failed.  There is an SSL issue", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		}
		else if(soap_zlib_error_check(res)) { 
			MessageBoxA(NULL, "Server Listing Failed.  There was a ZLIB error!", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		}
		else if(soap_http_error_check(res)) {
			if(res == 415) {
				MessageBoxA(NULL, "Server Listing Failed.  Got back HTTP/Unsupported Media!", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
			} else {
				MessageBoxA(NULL, "Server Listing Failed.  It was an HTTP error!", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
			}
		}
		else {
			MessageBoxA(NULL, "Server Listing Failed.  We have no idea why.  SOAP hates you.", "Error", MB_TASKMODAL | MB_TOPMOST | MB_ICONERROR | MB_OK);
		}
	}
	else {
		return 0;  
	}

	return 1;

}

const char *InterpretPVPType(unsigned int x) {
	if(x == 2) {
		return pvpfull;
	}
	else if(x == 1) {
		return pvpparty;
	}
	else {
		return pvpnone;
	}

}

void cleanup_gamelist_structure() {
	unsigned int i;

	for(i=0; i < s_gamelist_length;i++) {
		free(s_gamelist[i]->server_name);
		free(s_gamelist[i]->server_address);
		free(s_gamelist[i]->module_desc);
		free(s_gamelist[i]->module_name);
		free(s_gamelist[i]->module_url);
		free(s_gamelist[i]->server_desc);
		free(s_gamelist[i]);
	}
	free(s_gamelist);
	s_gamelist = NULL;


}

