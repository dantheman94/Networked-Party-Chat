/*
	Created by: DANIEL MARTON
	
	Created on: 03/04/2018
	Last edited on: 08/05/2018
*/

#include "Server.h"

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Overload constructor	- Creates & initializes server instance.

	@param:		MAXCLIENTS				- The maximum amount of connections allowed.
	@param:		PORT					- The internal pc port that the network will flow through.
*/
Server::Server(unsigned int MAXCLIENTS, const unsigned short PORT) {
	
	// Get reference to rak peer interface
	_pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	
	// Start server instance
	RakNet::SocketDescriptor sd(PORT, 0);
	_pPeerInterface->Startup(MAXCLIENTS, &sd, 1);
	_pPeerInterface->SetMaximumIncomingConnections(_MaxClients = MAXCLIENTS);
	
	// Setup ID array
	for (unsigned int i = 0; i < MAXCLIENTS; i++) {

		// This means that the first client who joins wont have an ID of 0, but 1 instead
		if (i == 0) { _IDArray.push_back(new bool(true)); }

		// All IDs are available from startup because no clients have joined yet
		else { _IDArray.push_back(new bool(false)); }
	}

	// Server properties
	std::cout << "\n------------------------------" << std::endl;
	std::cout << " Server Properties" << std::endl;
	std::cout << "------------------------------" << std::endl;
	std::cout << "\n - Internal Address:\t  " << _pPeerInterface->GetLocalIP(0)
			  << "\n - Local Address:\t  " << _pPeerInterface->GetLocalIP(1)
			  << "\n - Wifi Address:\t  " << _pPeerInterface->GetLocalIP(2)
			  << "\n - Port:\t\t  " << sd.port
			  << "\n - Max Clients:\t\t  " << MAXCLIENTS
			  << std::endl;

	// Run server commands on a separate thread
	_ServerCommandsThread = std::thread([=] { ServerCommands(); });

	// Start receiving packets
	UpdateReceivingPackets();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Default deconstructor
*/
Server::~Server() {

	// Free used resources
	for (auto iter : _IDArray)		{ delete iter; iter = nullptr; }
	for (auto iter : _ClientList)	{ delete iter; iter = nullptr; }
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Initializes a client & sets their ID so they can be identified in the server & by other clients.
	
	@param:		packet					- Packet containing the client's info
	@param:		clientsConnected		- Current amount of clients connected to the server (used to determine new ID).
	
	@return:	VOID
*/
void Server::SendClientID(RakNet::Packet* packet, int& clientsConnected) {

	// Add the client as new increment in counter
	clientsConnected++;

	// Find the lowest ID availiable
	int id;
	for (int i = 1; i < _MaxClients; i++) {

		if (_IDArray.at(i) != NULL) {

			// Found ID
			if (*_IDArray.at(i) == false) {

				id = i;
				*_IDArray.at(i) = true;
				break;
			}
		}
	}

	// Update client info list with new client data
	int i = _ClientList.size();
	_ClientList.push_back(new ClientInfo());
	_ClientList.at(i)->GUID = packet->guid;
	_ClientList.at(i)->ID = id;

	// Create packet
	RakNet::BitStream bitStream;
	bitStream.Write((RakNet::MessageID)GameMessages::ID_SERVER_SET_CLIENT_ID);
	bitStream.Write(id);

	// Send packet
	_pPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->guid, false);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Sets a client's team for association.
	
	@param:		packet					- Packet containing the client's info
	@param:		newTeamID				- The team ID to be set.
	
	@return:	VOID
*/
void Server::SendClientChannel(RakNet::Packet* packet, int newChannel) {

	// Update client info->channel in list
	int i = 0;
	for (auto iter : _ClientList) {

		if (iter->GUID == packet->guid) {

			_ClientList.at(i)->Channel = newChannel;
			break;
		}
		++i;
	}

	// Create bitstream
	RakNet::BitStream bitstream;

	// Write team ID to stream
	bitstream.Write((RakNet::MessageID)GameMessages::ID_SERVER_SET_CLIENT_CHANNEL);
	bitstream.Write(newChannel);

	// Send new Team ID to client
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->guid, false);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Sets a client's profile name.
	
	@param:		address					- The receiving client's address.
	@param:		name					- The new profile name.
	
	@return:	VOID
*/
void Server::SendClientProfileName(RakNet::SystemAddress address, std::string name) {

	// Setup rakstrings
	RakNet::RakString rakString;
	rakString.Set(name.c_str());

	// Create packet
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_SERVER_SET_CLIENT_NAME);
	bitstream.Write((unsigned short)strlen(rakString));
	bitstream.Write(rakString, strlen(rakString));

	// Send packet
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, address, false);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Sends a server message to a specific address.
	
	@param:		guid					- The receiving client's GUID.
	@param:		message					- The server message to be sent to the client.
	
	@return:	VOID
*/
void Server::SendClientServerMessage(RakNet::RakNetGUID guid, std::string message) {

	// Setup rakstring
	RakNet::RakString rakMsg;
	rakMsg.Set(message.c_str());

	// Create packet
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_SERVER_MESSAGE);
	bitstream.Write((unsigned short)strlen(rakMsg));
	bitstream.Write(rakMsg, strlen(rakMsg));

	// Send packet to all clients
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, guid, false);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Sends a server message to all connected clients.
	
	@param:		message					- The server message to be sent to the clients
	
	@return:	VOID
*/
void Server::BroadcastServerMessage(std::string message) {

	// Setup rakstring
	RakNet::RakString rakMsg;
	rakMsg.Set(message.c_str());

	// Create packet
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_SERVER_MESSAGE);
	bitstream.Write((unsigned short)strlen(rakMsg));
	bitstream.Write(rakMsg, strlen(rakMsg));

	// Send packet to all clients
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Updates all client versions of the client list
	
	@return:	VOID
*/
void Server::BroadcastUpdatedClientList() {

	// Get infos for packet
	std::vector<ClientInfo> info;
	for (unsigned int i = 0; i < _ClientList.size(); ++i) { 

		info.push_back(*_ClientList.at(i)); 
	}

	// Create packet to let the client know they are about to receive a vector array of a specified size
	RakNet::BitStream bitstream1;
	bitstream1.Write((RakNet::MessageID)GameMessages::ID_SERVER_SET_CLIENTLIST_SIZE);
	bitstream1.Write(info.size());

	// Send vector array size packet
	_pPeerInterface->Send(&bitstream1, IMMEDIATE_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);

	// Broadcast to clients
	int i = 0;
	for (auto iter : info) {

		// Break the struct into its members for writing
		RakNet::RakNetGUID guid = iter.GUID;
		int id = iter.ID;
		int channel = iter.Channel;
		RakNet::RakString rakString;
		rakString.Set(iter.ProfileName.c_str());

		// Create a client info packet
		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)GameMessages::ID_SERVER_UPDATE_CLIENTLIST);
		bs.Write(i);
		bs.Write(guid);
		bs.Write(id);
		bs.Write(channel);
		bs.Write((unsigned short)strlen(rakString));
		bs.Write(rakString, strlen(rakString));

		// Send a client info packet
		_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
		++i;
	}
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Mutual functionality when a client is banned from the server.
	
	@param:		guid					- The GUID of the client who is to be banned from the server
	
	@return:	VOID
*/
void Server::OnClientBanned(RakNet::RakNetGUID guid, int clientID) {

	// Kick
	OnClientKicked(guid, clientID);

	// Ban
	_pPeerInterface->AddToBanList(guid.ToString());
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Mutual functionality when a client leaves the server regardless of the reason.
	
	@param:		guid					- The GUID of the client who has just left the server
	
	@return:	VOID
*/
void Server::OnClientKicked(RakNet::RakNetGUID guid, int clientID) {
	
	// -1 from counter
	_ClientsConnected--;

	// Make ID available again for use
	if (clientID > 0 && clientID <= _MaxClients) {

		delete _IDArray.at(clientID -1);
		_IDArray.at(clientID -1) = new bool(false);

		// Notify other clients of the disconnection
		BroadcastServerMessage(_ClientList.at(clientID - 1)->ProfileName + " has left the server.");
	}

	// Remove from list by removing the last iterator in the list 
	// then re-queuing it to the front if its not meant to be removed
	for (unsigned int i = 0; i < _ClientList.size(); ++i) {
	
		// dequeue
		auto* value = _ClientList.back();
		_ClientList.pop_back();
	
		// re-queue if not the address to remove
		if (value->GUID != guid) { _ClientList.insert(_ClientList.begin(), value); }

		// Delete the resource if it was intended to be removed
		delete value; value = nullptr;
	}
	
	// Notify client that they have been disconnected
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_SERVER_SET_CLIENT_CONNECTED);

	// Create packet
	bitstream.Write(false);

	// Send packet
	_pPeerInterface->Send(&bitstream, IMMEDIATE_PRIORITY, RELIABLE, 0, guid, false);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Reads in the details related to the client who is quitting the server.
	
	@param:		packet					- Packet containing the client's info
	
	@return:	VOID
*/
void Server::OnClientRequestQuit(RakNet::Packet* packet, int& id, std::string& name) {
	
	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

	// Read in sender's ID
	bitstream.Read(id);

	// Read in sender's profile name
	RakNet::RakString rakName;
	bitstream.Read(rakName);
	name = rakName.C_String();

	// Kick the client from the server
	OnClientKicked(packet->guid, id);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Updates the server's list for a single client with a new channel
	
	@param:		packet					- Packet containing the client's info with the new channel
	
	@return:	VOID
*/
void Server::OnClientRequestChannel(RakNet::Packet * packet) {

	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

	// Read in client ID
	int id;
	bitstream.Read(id);

	int channel;
	bitstream.Read(channel);

	// Loop through client list until we find the matching client
	int i = 0;
	for (auto iter : _ClientList) {

		// Update the client list with the new channel
		if (iter->GUID == packet->guid) {

			iter->Channel = channel;
			break;
		}
		++i;
	}

	// Send a packet back to the client with their new profile name
	SendClientChannel(packet, _ClientList.at(i)->Channel);

	// Notify client
	SendClientServerMessage(packet->guid, "Team channel requested accepted.");

	// Update clientside client list
	BroadcastUpdatedClientList();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Updates the server's list of client profile names with a new name
	
	@param:		packet					- Packet containing the client's info with the new profile name
	
	@return:	VOID
*/
void Server::OnClientRequestNameChange(RakNet::Packet* packet) {

	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

	// Read in client ID
	int id;
	bitstream.Read(id);

	// Read in sender's profile name
	RakNet::RakString rakName;
	bitstream.Read(rakName);

	// Loop through client list until we find the matching client
	int i = 0;
	for (auto iter : _ClientList) {

		// Update the client list with the new name change
		if (iter->GUID == packet->guid) { 

			iter->ProfileName = rakName.C_String(); 
			break;
		}
		++i;
	}

	// Send a packet back to the client with their new profile name
	SendClientProfileName(packet->systemAddress, _ClientList.at(i)->ProfileName);

	// Notify client
	SendClientServerMessage(packet->guid, "Profile name change requested accepted.");

	// Update clientside client list
	BroadcastUpdatedClientList();	
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Receives packets & notifies client(s) of the event.
	
	@return:	VOID
*/
void Server::UpdateReceivingPackets() {

	RakNet::Packet* packet = nullptr;

	while (!_Shutdown) {
		
		for (packet = _pPeerInterface->Receive(); packet;
					  _pPeerInterface->DeallocatePacket(packet),
			 packet = _pPeerInterface->Receive()) {

			// On incoming packet
			switch (packet->data[0]) {

				// Client connecting
				case ID_NEW_INCOMING_CONNECTION: {
					
					// Setup client properties
					SendClientID(packet, _ClientsConnected);
					SendClientChannel(packet, 1);

					// Notify clients
					BroadcastServerMessage("A client has connected");
					BroadcastUpdatedClientList();
					break;
				}

				// Client disconnected
				case ID_DISCONNECTION_NOTIFICATION: {

					// Remove client address from list
					OnClientKicked(packet->guid, 0);
					break;
				}

				// Client lost connection
				case ID_CONNECTION_LOST: {

					// Remove client address from list
					OnClientKicked(packet->guid, 0);

					// Notify clients
					BroadcastServerMessage("A client has lost connection");
					break;
				}

				// Client attempting to quit
				case ID_CLIENT_REQUEST_DISCONNECTION: {

					// Read in packet
					int id;
					std::string clientProfileName;
					OnClientRequestQuit(packet, id, clientProfileName);
					break;
				}

				// Client chat message broadcast
				case ID_CLIENT_CHAT_MESSAGE_BROADCAST: {

					// Send the message to client(s)
					RakNet::BitStream bitStream(packet->data, packet->length, false);
					_pPeerInterface->Send(&bitStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
					break;
				}

				// Client name change
				case ID_CLIENT_REQUEST_NAME_CHANGE: {

					// Update list with new name
					OnClientRequestNameChange(packet);
					break;
				}

				// Client channel change
				case ID_CLIENT_REQUEST_CHANNEL_CHANGE: {

					// Read in packet & update
					OnClientRequestChannel(packet);
					break;
				}

				default: break;
			}
		}
	}

	// Shutdown the server
	Shutdown();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Initializes processes to run server commands (runs on a separate thread)
	
	@return:	VOID
*/
void Server::ServerCommands() {

	std::cout << "\n------------------------------" << std::endl;
	std::cout << " Server Commands" << std::endl;
	std::cout << "------------------------------" << std::endl;
	std::cout << " - Shutdown:\t\t< q > " << std::endl;
	std::cout << " - Kick client:\t\t< k >" << std::endl;
	std::cout << " - Ban client:\t\t< b >" << std::endl;
	std::cout << " - Broadcast Message:\t< s >" << std::endl;

	bool ValidInput = false;
	while (!ValidInput) {

		std::cout << "\n SERVER COMMAND: ";
		std::cin.clear();
		std::cin.ignore(INT_MAX, '\n');
		char cInput = '\0';
		std::cin >> cInput;

		// Switch on input
		switch (cInput) {
			
			 // Shutdown the server
			case 'q':
			case 'Q': {

				_Shutdown = true;
				ValidInput = true;
				break;
			}

			// Kick a client
			case 'k':
			case 'K': {

				if (_ClientList.size() > 0) {

					for (unsigned int i = 0; i < _ClientList.size(); ++i) {

						// Print the client's ID then profile name
						std::cout << " < " << _ClientList.at(i)->ID << " > " << _ClientList.at(i)->ProfileName.c_str() << std::endl;
					}

					// Selecting client to kick
					bool ValidInput2 = false;
					while (!ValidInput2) {

						std::cout << "\n Kick < # >: ";
						std::cin.clear();
						std::cin.ignore(INT_MAX, '\n');
						char cKick = '\0';
						int i = 0;

						std::cin >> cKick;
						i = cKick - '0';

						if (i > 0 && i < _MaxClients) {

							// Kick client
							OnClientKicked(_ClientList.at(i - 1)->GUID, _ClientList.at(i - 1)->ID);
							ValidInput2 = true;
						}
						else {

							// Invalid input
							std::cin.clear();
							std::cin.ignore(INT_MAX, '\n');
							std::cout << "\n Invalid input: ";
						}
					}
				}
				break;
			}

			// Ban a client
			case 'b':
			case 'B': {

				if (_ClientList.size() > 0) {

					for (unsigned int i = 0; i < _ClientList.size(); ++i) {

						// Print the client's ID then profile name
						std::cout << " < " << _ClientList.at(i)->ID << " > " << _ClientList.at(i)->ProfileName.c_str() << std::endl;
					}

					bool ValidInput2 = false;
					while (!ValidInput2) {

						std::cout << "\n Ban < # >: ";
						std::cin.clear();
						std::cin.ignore(INT_MAX, '\n');
						char cBan = '\0';
						int i = 0;

						std::cin >> cBan;
						i = cBan - '0';

						if (i > 0 && i < _MaxClients) {

							// Ban client
							OnClientBanned(_ClientList.at(i - 1)->GUID, _ClientList.at(i - 1)->ID);
							ValidInput2 = true;
						}
						else {

							// Invalid input
							std::cin.clear();
							std::cin.ignore(INT_MAX, '\n');
							std::cout << "\n Invalid input: ";
						}
					}
				}
				break;
			}

			// Send a message to all
			case 's':
			case 'S': {

				std::cout << "\n Compose message: ";
				std::cin.clear();
				std::cin.ignore(INT_MAX, '\n');

				// Create message
				std::string message;
				std::getline(std::cin, message);

				// Broadcast message
				BroadcastServerMessage(message);
				break;
			}

			// Invalid input
			default: {

				std::cin.clear();
				std::cin.ignore(INT_MAX, '\n');
				std::cout << "\n Invalid input: ";
				break;
			}
		}
	}
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Removes all clients still connected to the server
	
	@return:	VOID
*/
void Server::KickAllClients() {

	for (auto client : _ClientList) { OnClientKicked(client->GUID, client->ID); }
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Correctly shutdowns down the server and frees resources back to memory.
	
	@return:	VOID
*/
void Server::Shutdown() {

	// Kick everyone connected to the server
	KickAllClients();

	// Shutdown the peer
	_pPeerInterface->Shutdown(0);
	RakNet::RakPeerInterface::DestroyInstance(_pPeerInterface);

	// Join the server commands thread back to the main thread
	_ServerCommandsThread.join();
	system("pause");
}
