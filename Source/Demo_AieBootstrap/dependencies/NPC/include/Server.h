#pragma once

// Standard libraries
#include <iostream>
#include <vector>
#include <list>
#include <thread>

// Raknet libraries
#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

// NPC libraries
#include "Enumeration.h"

class Server {

public:

	// Constructors
	Server(unsigned int MAXCLIENTS, const unsigned short PORT);
	~Server();

	// Networking
	void SendClientID(RakNet::Packet* packet, int& clientID);
	void SendClientChannel(RakNet::Packet* packet, int newChannel);
	void SendClientProfileName(RakNet::SystemAddress address, std::string name);
	void SendClientServerMessage(RakNet::RakNetGUID guid, std::string message);
	void BroadcastServerMessage(std::string message);
	void BroadcastUpdatedClientList();
	void OnClientBanned(RakNet::RakNetGUID guid, int clientID);
	void OnClientKicked(RakNet::RakNetGUID guid, int clientID);
	void OnClientRequestQuit(RakNet::Packet* packet, int& id, std::string& name);
	void OnClientRequestChannel(RakNet::Packet* packet);
	void OnClientRequestNameChange(RakNet::Packet* packet);
	void UpdateReceivingPackets();
	void ServerCommands();
	void KickAllClients();
	void Shutdown();

protected:

	RakNet::RakPeerInterface* _pPeerInterface = NULL;

	// Server properties
	int _MaxClients;										// Maximum amount of client connections allowed.
	int _ClientsConnected = 0;								// Amount of client(s) that are connected to this server.
	bool _Shutdown = false;									// Returns TRUE when the server is starting the shutdown process.
	std::thread _ServerCommandsThread;						// The thread related to the server commands processes.

	// Client list
	std::vector<bool*> _IDArray;							// Array of all client IDs in use/not in use.
	std::vector<ClientInfo*> _ClientList;					// Array of all client infos connected to the server.

};