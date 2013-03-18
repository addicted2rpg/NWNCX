// (c) Addicted2rpg (2013) - see core.cpp for License (GNU).


struct gamelist_struct { 
	// These show up on the main display list to the human player listing the servers

	int is_online;    // probably don't show the server if this is 0
	char *server_name;
	char *module_name;
	unsigned int active_players;
	unsigned int maximum_players;
	unsigned int minimum_level;
	unsigned int maximum_level;

	unsigned int pvp_level; // 2, 1, or 0
	char pvp_text[6];   // "Full", "Party", or "None"
	/////////////////////////////////////////////////////////
	// These are for the connection, server info button, or are of interest to the Filter (i.e., no localvault), 
	// server software version, and so forth.
	int allow_localvault;
	int is_private_server; 
	unsigned int buildnumber; 
	unsigned int expansion;

	time_t last_heartbeat;

	int ELC_enabled;  // enforce legal characters
	int ILR_enabled; // item level restrictions
	int one_party;
	int player_pause;

	char *server_desc;
	char *server_address;
	char *module_desc;
	char *module_url;

	//char * PWCUrl; // I think that is only used in NWN2, so not supporting it here.

};

