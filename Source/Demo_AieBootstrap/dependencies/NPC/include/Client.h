#pragma once

// Standard libraries
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

// Raknet libraries
#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include "RakVoice.h"

// FMOD libraries
#include "fmod.hpp"
#include "fmod_errors.h"
#include "FMODVoiceAdapter.h"

// NPC libraries
#include "Enumeration.h"

// define sample type. Only short(16 bits sound) is supported at the moment.
typedef short SAMPLE;

// Reads and writes per second of the sound data
// Speex only supports these 3 values
#define SAMPLE_RATE  (8000)
///#define SAMPLE_RATE  (16000)
///#define SAMPLE_RATE  (32000)

// I think one buffer has to be full (of samples) before you hear the sound.
// So higher frames per buffer means that there will be a larger latency before you hear the sound
// However, it would lock and unlock the buffer more often, hindering performance.
#define FRAMES_PER_BUFFER  (2048 / (32000 / SAMPLE_RATE))

#define LATENCY_MS      (50) /* Some devices will require higher latency to avoid glitches */
#define DRIFT_MS        (1)
#define DEVICE_INDEX    (0)

class Client {

public:

	// Constructors
	Client(std::string IP, const unsigned short PORT);
	~Client();
	
	// Networking packets
	void Shutdown();
	void InitializeVoiceChat();
	void DisconnectFromServer();
	void onSetClientIDPacket(RakNet::Packet* packet);
	void onSetClientChannelPacket(RakNet::Packet * packet);
	void onSetClientProfileName(RakNet::Packet* packet);
	void onSetClientConnected(RakNet::Packet* packet);
	void onReceivedServerMessage(RakNet::Packet* packet);
	void onReceivedChatMessage(RakNet::Packet* packet);
	void onReceivedPreUpdateClientList(RakNet::Packet* packet);
	void onReceivedUpdatedClientList(RakNet::Packet* packet);
	void sendChatMessageToAll(int channel, std::string message, MessageChannelType messageChannelType);
	void sendChatMessageToGUID(RakNet::RakNetGUID guid, std::string message, MessageChannelType messageChannelType);
	void RequestProfileNameToServer(std::string name);
	void RequestChannelToServer(int channel);
	void HandleNetworkMessages();
	
	// Client properties
	ClientInfo getInfo()									{ return _Info; }
	std::string getConnectedIP()							{ return _ConnectedIP; }
	std::vector<ClientInfo*> getClientList()				{ return _ClientList; }
	bool isConnected()										{ return _IsConnected; }

	// Text message OUT
	std::string getOutMessage()								{ return _RakMsgOut.C_String(); }
	int getOutMessageChannelType()							{ return int(_MsgOutType); }

	// Text message IN
	void setHasReceivedServerMessage(bool value)			{ _HasReceivedServerMessage = value; }
	void setHasReceivedMessage(bool value)					{ _HasReceivedMessage = value; }
	bool getReceivedServerMessage()							{ return _HasReceivedServerMessage; }
	bool getReceivedMessage()								{ return _HasReceivedMessage; }
	int getInMessageClientID()								{ return _MsgInSenderID; }
	int getInMessageClientChannel()							{ return _MsgInSenderChannel; }
	int getInMessageChannel()								{ return _MsgInChannel; }
	std::string getInMessageProfileName()					{ return _MsgInProfileName.C_String(); }
	std::string getInMessageString()						{ return _RakMsgIn.C_String(); }
	int getInMessageChannelType()							{ return int(_MsgInType); }

	// Voice message
	void UpdateFMOD();
	void IncreaseVoiceEncoderComplexity(int amount = 1);	
	void DecreaseVoiceEncoderComplexity(int amount = 1);
	void RecordVoice(); 
	void DecodeIncomingVoice();
	void SendVoiceBuffer(RakNet::RakNetGUID targetGUID, FMOD::Sound* voiceSound);
	void setNoiseFilterActive(bool value)					{ _RakVoice.SetNoiseFilter(value); }
	void setVAD(bool value)									{ _RakVoice.SetVAD(value); }
	void setVBR(bool value)									{ _RakVoice.SetVBR(value); }
	void setTryingToBroadcastVoice(bool value)				{ _TryingToBroadCastingVoice = value; }
	void RequestVoiceChannel(RakNet::RakNetGUID targetGUID) { _RakVoice.RequestVoiceChannel(targetGUID); }
	void CloseVoiceChannel(RakNet::RakNetGUID targetGUID)	{ _RakVoice.CloseVoiceChannel(targetGUID); }
	FMOD::Sound* getVoiceBuffer()							{ return _SoundInput; }
	bool isTalking()										{ return _IsTalking; }
		
protected:

	RakNet::RakPeerInterface* _pPeerInterface = NULL;
	FMOD::System* _FMODsystem = NULL;
	bool _ShuttingDown = false;
	
	// Server info
	std::string _ConnectedIP;								// IP address of the server we are connected to
	std::vector<ClientInfo*> _ClientList;					// Array of all client infos connected to the server.
	
	// Client info
	ClientInfo _Info;										// Profile info related to this client.
	bool _IsConnected = false;								// Returns TRUE if the client is connected to a server or not.
	
	// Text message OUT
	RakNet::RakString _RakMsgOut;							// The text string of the most recent msg that the client has sent out
	MessageChannelType _MsgOutType;							// Enum identifier on whether the message is for ALL_CLIENTS, TEAM_ONLY or WHISPER

	// Text message IN
	bool _HasReceivedServerMessage = false;					// Returns TRUE when a server message packet has been successfully read in
	bool _HasReceivedMessage = false;						// Returns TRUE when a client chat message packet has been successfully read in
	int _MsgInSenderID = 0;									// The client ID of the sender, from the most recent msg received
	int _MsgInSenderChannel = 0;							// Channel identifier of the sender, for the most recent msg received
	int _MsgInChannel = 0;									// Channel identifier for the most recent msg received
	RakNet::RakString _MsgInProfileName;					// The profile name of the sender, for the most recent msg received
	RakNet::RakString _RakMsgIn;							// The text string of the most recent msg received
	MessageChannelType _MsgInType;							// Enum identifier on whether the message is for ALL_CLIENTS, TEAM_ONLY or WHISPER

	// Voice communication system
	std::thread _VoiceChatThread;							// The thread related to the voice chat recording process.
	std::mutex _VoiceChatMutex;								// Mutux related to the _VoiceChatThread.
	bool _VoiceChatMutexIsLocked = false;					// Returns TRUE if the voice chat mutex is currently locked.
	bool _TryingToBroadCastingVoice = false;				// Returns TRUE if the client is trying to broadcast. 
	bool _IsTalking = false;								// Returns TRUE if FMOD detects sound being recorded.
	RakNet::RakVoice _RakVoice;								// Reference to the RakVoice component.
	FMOD::Channel* _ChannelOutput = NULL;					// Reference to the channel that the sound output is being emitted from.
	FMOD::Sound* _SoundInput = NULL;						// Reference to sound from the client's input device.
	FMOD::Sound* _SoundOutput = NULL;						// Reference to sound being sent from the network.
	unsigned int _SamplesRecorded = 0;						// Counter for how many samples have been recorded.
	unsigned int _SamplesPlayed = 0;						// Counter for how many samples have been played.
	unsigned int _DriftThreshold;							// The point where we start compensating for drift.
	unsigned int _DesiredLatency;							// User specified latency.
	unsigned int _AdjustedLatency;							// User specified latency adjusted for driver update granularity.
	unsigned int _SoundLength;								// Length of the sound being recorded.
	int _ActualLatency;										// Latency measured once playback begins (smoothened for jitter).
	int _NativeRate = 0;									// 
	int _DriverCount = 0;									// Number of input devices detected.

};