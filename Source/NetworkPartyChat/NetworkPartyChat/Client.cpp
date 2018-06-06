/*
	Created by: DANIEL MARTON
	
	Created on: 03/04/2018
	Last edited on: 08/05/2018
*/

#include "Client.h"

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Overload constructor	- Creates & initializes client instance.
	
	@param:		IP						- The ip address of the server to connect to.
	@param:		PORT					- The internal pc port that the network will flow through.
*/
Client::Client(std::string IP, const unsigned short PORT) {

	// Get reference to Rak peer interface
	_pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	
	// Initialize client connection to server
	RakNet::SocketDescriptor sd;
	_pPeerInterface->Startup(1, &sd, 1);

	_ConnectedIP = IP;
	std::cout << "\n - Connecting to server at: " << IP << std::endl;

	// Call connect to attempt connection to the server	
	RakNet::ConnectionAttemptResult result = _pPeerInterface->Connect(IP.c_str(), PORT, nullptr, 0);
	if (result != RakNet::CONNECTION_ATTEMPT_STARTED) {

		_IsConnected = false;
		std::cout << " *** ERROR *** Unable to start connection, Error number: " << result << std::endl;
	}

	// Successful connection to the swerver
	else { 
		
		_IsConnected = true;
		InitializeVoiceChat();
	}
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Default deconstructor
*/
Client::~Client() {

	// Free used resources
	for (auto iter : _ClientList) { delete iter; iter = nullptr; }
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Correctly shutdowns down the client and frees resources back to memory.
	
	@return:	VOID
*/
void Client::Shutdown() {

	// Start to shutdown
	_ShuttingDown = true;
		
	// Wait until the thread has been connected back to the main thread
	while (_VoiceChatMutexIsLocked) {} // This becomes false ONLY once the _VoiceChatThread has been closed
	if (_VoiceChatThread.joinable()) { _VoiceChatThread.join(); }
	
	// Release any FMOD resources used & shutdown FMOD itself
	if (_ChannelOutput) { _ChannelOutput->stop(); }
	_SoundInput->release();
	_SoundOutput->release();
	RakNet::FMODVoiceAdapter::Instance()->Release();
	_FMODsystem->close();
	_FMODsystem->release();	

	// Shutdown the client peer
	_pPeerInterface->Shutdown(300);
	_pPeerInterface->DetachPlugin(&_RakVoice);
	RakNet::RakPeerInterface::DestroyInstance(_pPeerInterface);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Startups the required components for the voice chat system.
	
	@return:	VOID
*/
void Client::InitializeVoiceChat() {

	// Initialize FMOD
	FMOD_RESULT result;
	unsigned int version;

	result = FMOD::System_Create(&_FMODsystem);
	RakAssert(result >= 0);
	result = _FMODsystem->getVersion(&version);
	RakAssert(result >= 0);

	if (version < FMOD_VERSION) {

		std::cout << "ERROR: You are using an old version of FMOD %080x. This program requires %09x\n." << version << FMOD_VERSION << std::endl;
	}

	result = _FMODsystem->init(100, FMOD_INIT_NORMAL, 0);
	RakAssert(result >= 0);

	// Initialize rakVoice & attach to peer
	_pPeerInterface->AttachPlugin(&_RakVoice);
	_RakVoice.Init(SAMPLE_RATE, FRAMES_PER_BUFFER * sizeof(SAMPLE));

	// Connect to FMOD
	RakNet::FMODVoiceAdapter::Instance()->SetupAdapter(_FMODsystem, &_RakVoice);

	/*
		Spawn a new dedicated thread for voice chat that will
		use a lambda which loops endlessly (until _Shutdown == true) to record voice
	*/
	_VoiceChatThread = std::thread([=] { RecordVoice(); });
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Sends a notification to the server that a client is disconnecting from the server
		
	@return:	VOID
*/
void Client::DisconnectFromServer() {

	// Get profile name as rakstring
	RakNet::RakString rakString;
	rakString.Set(_Info.ProfileName.c_str());

	// Create packet
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_CLIENT_REQUEST_DISCONNECTION);
	bitstream.Write(_Info.ID);								// Sender's ID
	bitstream.Write((unsigned short)strlen(rakString));		// Sender's profile name
	bitstream.Write(rakString, strlen(rakString));

	// Send packet
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Read in client ID packet. (& max server size).

	@param:		packet			- Reference to the packet received.
	
	@return:	VOID
*/
void Client::onSetClientIDPacket(RakNet::Packet* packet) {

	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
	bitstream.Read(_Info.ID);

	std::cout << "\t- My client ID is: " << _Info.ID << std::endl;
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Read in client team channel packet.
	
	@param:		packet			- Reference to the packet received.
	
	@return:	VOID
*/
void Client::onSetClientChannelPacket(RakNet::Packet* packet) {

	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
	bitstream.Read(_Info.Channel);

	std::cout << "\t- My team channel is: " << _Info.Channel << std::endl;
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Read in client profile name packet.
	
	@param:		packet			- Reference to the packet received.
	
	@return:	VOID
*/
void Client::onSetClientProfileName(RakNet::Packet* packet) {
	
	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

	// Read in new profile name
	RakNet::RakString rakString;
	bitstream.Read(rakString);
	_Info.ProfileName = rakString.C_String();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Read in client connection to the server.
	
	@param:		packet			- Reference to the packet received.
	
	@return:	VOID
*/
void Client::onSetClientConnected(RakNet::Packet * packet) {

	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
	
	// Read in packet
	bool connected = _IsConnected;
	bitstream.Read(connected);

	_IsConnected = connected;
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Read in server message packet.
	
	@param:		packet			- Reference to the packet received.
	
	@return:	VOID
*/
void Client::onReceivedServerMessage(RakNet::Packet* packet) {

	// Read in server message
	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

	// Output message
	RakNet::RakString msgString;
	bitstream.Read(msgString);

	// Notify client of newely read in message
	_RakMsgIn = msgString;
	_HasReceivedServerMessage = true;

	// Debug
	std::cout << " SERVER: " << _RakMsgIn.C_String() << std::endl;
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Reads in client chat message packet.
	
	@param:		packet			- Reference to the packet received.
	
	@return:	VOID
*/
void Client::onReceivedChatMessage(RakNet::Packet* packet) {

	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

	// Read in sender's ID
	int clientID;
	bitstream.Read(clientID);
	_MsgInSenderID = clientID;

	// Read in sender's channel ID
	int clientTeam;
	bitstream.Read(clientTeam);
	_MsgInSenderChannel = clientTeam;

	// Read in sender's profile name
	RakNet::RakString name;
	bitstream.Read(name);
	_MsgInProfileName = name;

	// Read in message's channel type
	int iChannelType;
	bitstream.Read(iChannelType);
	_MsgInType = MessageChannelType(iChannelType);

	// Read in message's team ID
	int channel;
	bitstream.Read(channel);
	_MsgInChannel = channel;

	// If this message is for us to read
	if (_MsgInChannel == 0 || _MsgInChannel == _Info.Channel) {

		// Read in message string
		RakNet::RakString msgString;
		bitstream.Read(msgString);

		// Notify client of newely read in message
		_RakMsgIn = msgString;
		_HasReceivedMessage = true;

		// Debug
		std::cout << " Client ' " << _MsgInSenderID << "': " << msgString << std::endl;
	}
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Reads in updated client list size
	
	@param:		packet			- Reference to the packet received.
	
	@return:	VOID
*/
void Client::onReceivedPreUpdateClientList(RakNet::Packet * packet) {

	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

	// Update local vector array size
	int size;
	bitstream.Read(size);

	// Empty the array
	for (auto iter : _ClientList) { delete iter; iter = nullptr; }
	_ClientList.clear();
	
	// Add empty pointers
	for (int i = 0; i < size; ++i) { _ClientList.push_back(new ClientInfo()); }
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Reads in updated client info
	
	@param:		packet			- Reference to the packet received.
	
	@return:	VOID
*/
void Client::onReceivedUpdatedClientList(RakNet::Packet * packet) {
	
	RakNet::BitStream bitstream(packet->data, packet->length, false);
	bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

	// The array should have already a size greater that 0 specified by the server
	if (_ClientList.size() > 0) {

		int i;
		RakNet::RakNetGUID guid;
		int id;
		int channel;
		RakNet::RakString rakString;

		bitstream.Read(i);
		bitstream.Read(guid);
		bitstream.Read(id);
		bitstream.Read(channel);
		bitstream.Read(rakString);

		// Update client list with new info
		_ClientList.at(i)->GUID = guid;
		_ClientList.at(i)->ID = id;
		_ClientList.at(i)->Channel = channel;
		_ClientList.at(i)->ProfileName = rakString.C_String();
	}
}

/** ---------------------------------------------------------------------------------------------------------------
	@Summary:	Sends a text message to client(s) on the matching channel identifier.
	
	@param:		channel				- channel identifier (0 for ALL_CLIENTS)
	@param:		message				- the message to send as a std::string
	@param:		messageChannelType	- enum identifier for the message type (All_CLIENTS, TEAM_ONLY, WHISPER)
	
	@return:	VOID
*/
void Client::sendChatMessageToAll(int channel, std::string message, MessageChannelType messageChannelType) {

	// Set rakstrings
	_RakMsgOut.Set(message.c_str());
	RakNet::RakString profileName;
	profileName.Set(_Info.ProfileName.c_str());

	_MsgOutType = messageChannelType;

	// Create packet
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_CLIENT_CHAT_MESSAGE_BROADCAST);
	bitstream.Write(_Info.ID);								// Sender's client ID
	bitstream.Write(_Info.Channel);							// Sender's channel ID
	bitstream.Write((unsigned short)strlen(profileName));	// Sender's current profile name
	bitstream.Write(profileName, strlen(profileName));

	bitstream.Write(_MsgOutType);							// Message's channel type
	bitstream.Write(channel);								// Message's channel ID
	bitstream.Write((unsigned short)strlen(_RakMsgOut));
	bitstream.Write(_RakMsgOut, strlen(_RakMsgOut));		// Message text string

	// Send packet
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
}

/** ---------------------------------------------------------------------------------------------------------------
	@Summary:	Sends a text message to a single client.
	
	@param:		channel				- GUID of the receipient.
	@param:		message				- the message to send as a std::string
	@param:		messageChannelType	- enum identifier for the message type (All_CLIENTS, TEAM_ONLY, WHISPER)
	
	@return:	VOID
*/
void Client::sendChatMessageToGUID(RakNet::RakNetGUID guid, std::string message, MessageChannelType messageChannelType) {

	// Set rakstrings
	_RakMsgOut.Set(message.c_str());
	RakNet::RakString profileName;
	profileName.Set(_Info.ProfileName.c_str());

	_MsgOutType = messageChannelType;

	// Create packet
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_CLIENT_CHAT_MESSAGE_WHISPER);
	bitstream.Write(_Info.ID);								// Sender's client ID
	bitstream.Write(_Info.Channel);							// Sender's channel ID
	bitstream.Write((unsigned short)strlen(profileName));	// Sender's current profile name
	bitstream.Write(profileName, strlen(profileName));

	bitstream.Write(_MsgOutType);							// Message's channel type
	bitstream.Write(0);										// Message's channel ID
	bitstream.Write((unsigned short)strlen(_RakMsgOut));
	bitstream.Write(_RakMsgOut, strlen(_RakMsgOut));		// Message text string

	// Send packet
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, guid, false);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Sends a request for the server to change this client's profile name 
				and broadcasts the updated client list.
	
	@param:		name			- the new profile name requested by the client
	
	@return:	VOID3
*/
void Client::RequestProfileNameToServer(std::string name) {

	// Set rakstrings
	RakNet::RakString profileName;
	profileName.Set(name.c_str());

	// Create packet
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_CLIENT_REQUEST_NAME_CHANGE);
	bitstream.Write(_Info.ID);								// Sender's client ID
	bitstream.Write((unsigned short)strlen(profileName));	// Sender's newely requested profile name
	bitstream.Write(profileName, strlen(profileName));

	// Send packet
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Sends a request for the server to change this client's team channel
				and broadcasts the updated client list.
	
	@param:		channel			- the new channel requested by the client
	
	@return:	VOID
*/
void Client::RequestChannelToServer(int channel) {


	// Create packet
	RakNet::BitStream bitstream;
	bitstream.Write((RakNet::MessageID)GameMessages::ID_CLIENT_REQUEST_CHANNEL_CHANGE);
	bitstream.Write(_Info.ID);								// Sender's client ID
	bitstream.Write(channel);								// New channel

	// Send packet
	_pPeerInterface->Send(&bitstream, HIGH_PRIORITY, RELIABLE, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Receives packets & performs actions based on the type.
	
	@return:	VOID
*/
void Client::HandleNetworkMessages() {

	RakNet::Packet* packet;
	for (packet = _pPeerInterface->Receive(); packet;
				  _pPeerInterface->DeallocatePacket(packet),
		 packet = _pPeerInterface->Receive()) {

		// On incoming packet
		switch (packet->data[0]) {
			
			// Another client disconnected
			case ID_REMOTE_DISCONNECTION_NOTIFICATION: {

				break;
			}

			// Client lost connection
			case ID_REMOTE_CONNECTION_LOST: {

				_IsConnected = false;
				break;
			}

			// Client connected
			case ID_REMOTE_NEW_INCOMING_CONNECTION: {

				break;
			}

			// Client accepted connection
			case ID_CONNECTION_REQUEST_ACCEPTED: {

				_Info.GUID = packet->guid;
				break;
			}

			// Server message
			case ID_SERVER_MESSAGE: {

				// Read in server message
				onReceivedServerMessage(packet);
				break;
			}

			// Set client connected
			case ID_SERVER_SET_CLIENT_CONNECTED: {

				onSetClientConnected(packet);
				break;
			}

			// Set client ID
			case ID_SERVER_SET_CLIENT_ID: {

				// Read in client ID packet
				onSetClientIDPacket(packet);
				break;
			}

			// Set client channel
			case ID_SERVER_SET_CLIENT_CHANNEL: {

				// Read in client channel packet
				onSetClientChannelPacket(packet);
				break;
			}

			// Set client profile name
			case ID_SERVER_SET_CLIENT_NAME: {

				// Read in client profile name packet
				onSetClientProfileName(packet);
				break;
			}

			// Set client list size
			case ID_SERVER_SET_CLIENTLIST_SIZE: {

				// Read in packet
				onReceivedPreUpdateClientList(packet);
				break;
			}

			// Client iterator in client list
			case ID_SERVER_UPDATE_CLIENTLIST: {

				// Read in client list packet
				onReceivedUpdatedClientList(packet);
				break;
			}

			// Received chat message via broadcast
			case ID_CLIENT_CHAT_MESSAGE_BROADCAST: {

				// Read in chat message
				onReceivedChatMessage(packet);
				break;
			}

			case ID_CLIENT_CHAT_MESSAGE_WHISPER: {

				// Read in chat message
				onReceivedChatMessage(packet);
				break;
			}

			case ID_RAKVOICE_OPEN_CHANNEL_REQUEST: {

				std::cout << "channel request from %s\n" << packet->systemAddress.ToString() << std::endl;
				break;
			}

			case ID_RAKVOICE_OPEN_CHANNEL_REPLY: {

				std::cout << "new channel from %s\n" << packet->systemAddress.ToString() << std::endl;
				DecodeIncomingVoice();
				break;
			}

			case ID_RAKVOICE_CLOSE_CHANNEL: {

				std::cout << "channel closed with %s\n" << packet->systemAddress.ToString() << std::endl;
				break;
			}

			default: break;			
		}
	}
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Updates the connection to FMOD.
	
	@return:	VOID
*/
void Client::UpdateFMOD() {

	_FMODsystem->update();
	_RakVoice.Update();
	RakNet::FMODVoiceAdapter::Instance()->Update();

	// Continue to update driver count
	FMOD_RESULT result;
	result = _FMODsystem->getRecordNumDrivers(NULL, &_DriverCount);
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Increases the encoder complexcity for the voice chat.
	
	@param:		amount			- increase by this amount
	
	@return:	VOID
*/
void Client::IncreaseVoiceEncoderComplexity(int amount) {

	int v = _RakVoice.GetEncoderComplexity();
	if (v < 10) { _RakVoice.SetEncoderComplexity(v + amount); }
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Decreases the encoder complexcity for the voice chat.
	
	@param:		amount			- decrease by this amount

	@return:	VOID
*/
void Client::DecreaseVoiceEncoderComplexity(int amount) {

	int v = _RakVoice.GetEncoderComplexity();
	if (v < 10) { _RakVoice.SetEncoderComplexity(v - amount); }
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Records to sound, from an input device if detected.
	
	@return:	VOID
*/
void Client::RecordVoice() {

	while (!_ShuttingDown) {
		
		// Lock the voice chat mutex if it isnt already
		if (!_VoiceChatMutexIsLocked) {

			_VoiceChatMutex.lock();
			_VoiceChatMutexIsLocked = true;
		}

		// Get input device
		FMOD_RESULT result;
		result = _FMODsystem->getRecordNumDrivers(NULL, &_DriverCount);

		/*
			Determine latency in sampels
		*/
		int nativeChannels = 0;
		result = _FMODsystem->getRecordDriverInfo(DEVICE_INDEX, NULL, 0, NULL, &_NativeRate, NULL, &nativeChannels, NULL);

		_DriftThreshold = (_NativeRate * DRIFT_MS) / 1000;		// The point where we start compensating for drift
		_DesiredLatency = (_NativeRate * LATENCY_MS) / 1000;	// User specified latency
		_AdjustedLatency = _DesiredLatency;						// User specified latency adjusted for driver update granularity
		_ActualLatency = _DesiredLatency;						// Latency measured once playback begins (smoothened for jitter)

		/*
			Create user sound to record into, then start playing
		*/
		FMOD_CREATESOUNDEXINFO exinfo = { 0 };
		exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		exinfo.numchannels = nativeChannels;
		exinfo.format = FMOD_SOUND_FORMAT_PCM16;
		exinfo.defaultfrequency = _NativeRate;
		exinfo.length = _NativeRate * sizeof(short) * nativeChannels; // 1 second buffer, size here doesnt change latency

		result = _FMODsystem->createSound(0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &_SoundInput);
		result = _FMODsystem->recordStart(DEVICE_INDEX, _SoundInput, true);
		result = _SoundInput->getLength(&_SoundLength, FMOD_TIMEUNIT_PCM);

		do {

			/*
				Determine how much has been recorded since we last checked
			*/
			unsigned int recordPos = 0;
			result = _FMODsystem->getRecordPosition(DEVICE_INDEX, &recordPos);

			static unsigned int lastRecordPos = 0;
			unsigned int recordDelta = (recordPos >= lastRecordPos) ? (recordPos - lastRecordPos) : (recordPos + _SoundLength - lastRecordPos);
			lastRecordPos = recordPos;
			_SamplesRecorded += recordDelta;

			static unsigned int minRecordDelta = (unsigned int)-1;
			if (recordDelta && (recordDelta < minRecordDelta)) {

				// Smallest driver granularity seen so far
				minRecordDelta = recordDelta;

				// Adjust our latency if driver granularity is high
				_AdjustedLatency = (recordDelta <= _DesiredLatency) ? _DesiredLatency : recordDelta;
			}

			/*
				Delay playback until our desired latency is reached
			*/
			if (!_ChannelOutput && _SamplesRecorded >= _AdjustedLatency) {

				result = _FMODsystem->playSound(_SoundInput, 0, false, &_ChannelOutput);
				_IsTalking = true;
			}

			if (_ChannelOutput) {

				/*
					Stop playback if recording stops
				*/
				bool isRecording = false;
				result = _FMODsystem->isRecording(DEVICE_INDEX, &isRecording);
				if (!isRecording) {
				
					result = _ChannelOutput->setPaused(true);
					_IsTalking = false;
				} 
				else { _IsTalking = true; }

				/*
					Determine how much has been played since we last checked
				*/
				unsigned int playPos = 0;
				result = _ChannelOutput->getPosition(&playPos, FMOD_TIMEUNIT_PCM);

				static unsigned int lastPlayPos = 0;
				unsigned int playDelta = (playPos >= lastPlayPos) ? (playPos - lastPlayPos) : (playPos + _SoundLength - lastPlayPos);
				lastPlayPos = playPos;
				_SamplesPlayed += playDelta;

				/*
					Compensate for any recording / playback drift
				*/
				int latency = _SamplesRecorded - _SamplesPlayed;
				_ActualLatency = int((0.97f * _ActualLatency) + (0.03f * latency));

				int playbackRate = _NativeRate;
				if (_ActualLatency < (int)(_AdjustedLatency - _DriftThreshold)) {

					// Play position is catching up to the record position, slow playback down by 2%
					playbackRate = _NativeRate - (_NativeRate / 50);
				} 
				else if (_ActualLatency > (int)(_AdjustedLatency + _DriftThreshold)) {

					// Play position is falling behind the record position, speed playback up by 2%
					playbackRate = _NativeRate + (_NativeRate / 50);
				}
				_ChannelOutput->setFrequency((float)playbackRate);
			}
		} while (_DriverCount > 0 && !_ShuttingDown);
	}
	_VoiceChatMutex.unlock();
	_VoiceChatMutexIsLocked = false;
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Decodes & plays back an incoming sound from the network.
	
	@return:	VOID
*/
void Client::DecodeIncomingVoice() {
	
	FMOD_RESULT result;

	// Create sound output
	int nativeRate = 0;
	int nativeChannels = 0;
	result = _FMODsystem->getRecordDriverInfo(DEVICE_INDEX, NULL, 0, NULL, &nativeRate, NULL, &nativeChannels, NULL);

	FMOD_CREATESOUNDEXINFO exinfo	= { 0 };
	exinfo.cbsize					= sizeof(FMOD_CREATESOUNDEXINFO);
	exinfo.numchannels				= nativeChannels;
	exinfo.format					= FMOD_SOUND_FORMAT_PCM16;
	exinfo.defaultfrequency			= nativeRate;
	exinfo.length					= nativeRate * sizeof(short) * nativeChannels; // 1 second buffer, size here doesnt change latency

	// Decode the sound to output
	result = _FMODsystem->createSound(0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &_SoundOutput);
	_RakVoice.ReceiveFrame(_SoundOutput);

	// Play output
	result = _FMODsystem->playSound(_SoundOutput, 0, false, &_ChannelOutput);
	_SoundOutput->release();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Encodes an sound and sents it to a recipient.
	
	@return:	VOID
*/
void Client::SendVoiceBuffer(RakNet::RakNetGUID targetGUID, FMOD::Sound* voiceSound) {
	
	// Encode the data & send it to the target recipient
	_RakVoice.SendFrame(targetGUID, voiceSound);
}