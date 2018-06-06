#include "DemoApplicationApp.h"
#include "Texture.h"
#include "Font.h"
#include "Input.h"
#include "definitions.h"
#include <Windows.h>

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Overload constructor.
	
	@param:		windowX			- the width of the bootstrap window
	@param:		windowY			- the height of the bootstrap window
	@param:		client			- reference to the client entity
*/
DemoApplicationApp::DemoApplicationApp(int windowX, int windowY, Client* client) {

	_WindowX = windowX;
	_WindowY = windowY;
	_Client = client;
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Called on application startup.
	
	@return:	VOID
*/
bool DemoApplicationApp::startup() {
	
	_Renderer = new aie::Renderer2D();

	// TODO: remember to change this when redistributing a build!
	// the following path would be used instead: "./font/consolas.ttf"
	_FontSml = new aie::Font("bin/font/consolas.ttf", 20);
	_FontMed = new aie::Font("bin/font/consolas.ttf", 28);

	// Set text colours
	_AllyColour.r = 0.0f; _AllyColour.g = 0.4f; _AllyColour.b = 1.0f;
	_EnemyColour.r = 1.0f; _EnemyColour.g = 0.1f; _EnemyColour.b = 0.0f;
	_WhisperColour.r = 0.5f; _WhisperColour.g = 0.0f; _WhisperColour.b = 1.0f;

	return true;
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Called when the application closes.
	
	@return:	VOID
*/
void DemoApplicationApp::shutdown() {
	
	// Delete used memory addresses
	delete _Renderer;	_Renderer = nullptr;
	delete _FontSml;	_FontSml = nullptr;
	delete _FontMed;	_FontMed = nullptr;

	// Delete all chat messages
	for each (auto iter in _ObjChatMessages) {

		// Precaution
		iter->setActive(false);
		
		// Delete memory address
		delete iter; iter = nullptr;
	}
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Function representing 1 update frame when the application is running.
	
	@return:	VOID
*/
void DemoApplicationApp::update(float deltaTime) {

	// Get input instance
	aie::Input* input = aie::Input::getInstance();

	// Update text chat outputs
	if (_ObjChatMessages.size() > 0) {

		for (auto msg : _ObjChatMessages) {

			if (msg != nullptr)
				msg->update(deltaTime, this);
		}
	}

	// Handle network packets
	if (_Client) { _Client->HandleNetworkMessages(); }

	// Update voice chat
	UpdateVoiceChat();
	
	// "A chat window" is open if any chat window type is currently open
	_AChatWindowIsActive = _MsgAllWindow || _MsgTeamWindow || _MsgWhisperWindow;

	// Display imgui panels
	if (_MsgAllWindow)				{ ShowMsgAllWindow(); }
	if (_MsgTeamWindow)				{ ShowMsgTeamWindow(); }
	if (_MsgWhisperWindow)			{ ShowMsgWhisperWindow(); }
	if (_ChangeProfileNameWindow)	{ ShowSetProfileName(); }

	// Flip chat all window states
	if (input->wasKeyPressed(aie::INPUT_KEY_F1)) {

		int x = _MsgAllWindow;
		x = 1 - x;

		_MsgAllWindow = x == 1;
		_MsgTeamWindow = false;
		_MsgWhisperWindow = false;
		_ChangeProfileNameWindow = false;
	}

	// Flip chat team only window states
	if (input->wasKeyPressed(aie::INPUT_KEY_F2)) {

		int x = _MsgTeamWindow;
		x = 1 - x;

		_MsgTeamWindow = x == 1;
		_MsgAllWindow = false;
		_MsgWhisperWindow = false;
		_ChangeProfileNameWindow = false;
	}

	// Flip chat whisper window states
	if (input->wasKeyPressed(aie::INPUT_KEY_F3)) {

		int x = _MsgWhisperWindow;
		x = 1 - x;

		_MsgWhisperWindow = x == 1;
		_MsgAllWindow = false;
		_MsgTeamWindow = false;
		_ChangeProfileNameWindow = false;
	}

	// Flip change profile name window states
	if (input->wasKeyPressed(aie::INPUT_KEY_TAB)) {

		int x = _ChangeProfileNameWindow;
		x = 1 - x;

		_ChangeProfileNameWindow = x == 1;
	}

	// Change team based on 1-9
	if (input->wasKeyPressed(aie::INPUT_KEY_1) && _Client) { _Client->RequestChannelToServer(1); }
	if (input->wasKeyPressed(aie::INPUT_KEY_2) && _Client) { _Client->RequestChannelToServer(2); }
	if (input->wasKeyPressed(aie::INPUT_KEY_3) && _Client) { _Client->RequestChannelToServer(3); }
	if (input->wasKeyPressed(aie::INPUT_KEY_4) && _Client) { _Client->RequestChannelToServer(4); }
	if (input->wasKeyPressed(aie::INPUT_KEY_5) && _Client) { _Client->RequestChannelToServer(5); }
	if (input->wasKeyPressed(aie::INPUT_KEY_6) && _Client) { _Client->RequestChannelToServer(6); }
	if (input->wasKeyPressed(aie::INPUT_KEY_7) && _Client) { _Client->RequestChannelToServer(7); }
	if (input->wasKeyPressed(aie::INPUT_KEY_8) && _Client) { _Client->RequestChannelToServer(8); }
	if (input->wasKeyPressed(aie::INPUT_KEY_9) && _Client) { _Client->RequestChannelToServer(9); }

	// Change voice encoder complexity
	///if (input->wasKeyPressed(aie::INPUT_KEY_PAGE_UP))	{ _Client->IncreaseVoiceEncoderComplexity(); }
	///if (input->wasKeyPressed(aie::INPUT_KEY_PAGE_DOWN))	{ _Client->DecreaseVoiceEncoderComplexity(); }
		
	// Exit the application (or close ImGui)
	if (input->wasKeyPressed(aie::INPUT_KEY_ESCAPE) && !_AChatWindowIsActive && _Client) {
		// Notify clients of quit
		_Client->DisconnectFromServer();

		// Close bootstrap
		quit();
	}
	else if (input->wasKeyPressed(aie::INPUT_KEY_ESCAPE)) {

		_MsgAllWindow = false;
		_MsgTeamWindow = false;
		_MsgWhisperWindow = false;
	}
	else if (!_Client->isConnected()) { quit(); }
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Function representing 1 rendering frame when the application is running.
	
	@return:	VOID
*/
void DemoApplicationApp::draw() {

	// Setup renderer
	clearScreen();
	_Renderer->setCameraPos(0, 0);
	_Renderer->begin();

	if (_Client) {

		// Draw client's profile card
		_Renderer->drawCircle(_WindowX - 440.0f, _WindowY - 95.0f, 15.0f);
		_Renderer->setRenderColour(0.0f, 0.0f, 0.0f);
		_Renderer->drawText(_FontSml, std::to_string(_Client->getInfo().Channel).c_str(), _WindowX - 445.0f, _WindowY - 100.0f);
		_Renderer->setRenderColour(1.0f, 1.0f, 1.0f);
		_Renderer->drawText(_FontSml, _Client->getInfo().ProfileName.c_str(), _WindowX - 400.0f, _WindowY - 100.0f);

		// On new client message
		if (_Client->getReceivedMessage()) {

			OnNewClientMessage();
			_Client->setHasReceivedMessage(false);
		}

		// On new server message
		if (_Client->getReceivedServerMessage()) {

			OnNewServerMessage();
			_Client->setHasReceivedServerMessage(false);
		}
	}

	// Draw text controls
	_Renderer->setRenderColour(1.0f, 1.0f, 1.0f);
	_Renderer->drawText(_FontMed, "Hold T to talk to teammates", 10, 300);
	_Renderer->drawLine(10, 280, 400, 280);
	_Renderer->drawText(_FontMed, "Press F1 to message all players", 10, 240);
	_Renderer->drawText(_FontMed, "Press F2 to message team only", 10, 200);
	_Renderer->drawText(_FontMed, "Press F3 to message a specific client", 10, 160);
	_Renderer->drawLine(10, 140, 400, 140);
	_Renderer->drawText(_FontMed, "Press 1-9 to set team", 10, 100);
	_Renderer->drawText(_FontMed, "Press TAB to set profile name", 10, 60);

	if (!_AChatWindowIsActive)	{ _Renderer->setRenderColour(1.0f, 1.0f, 1.0f, 1.0f); }
	else						{ _Renderer->setRenderColour(0.3f, 0.3f, 0.3f, 0.5f); }
	_Renderer->drawText(_FontMed, "Press ESC to exit application", 10, 10);

	// Draw lobby outlines
	_Renderer->setRenderColour(1.0f, 1.0f, 1.0f);
	_Renderer->drawLine(_WindowX - 500.0f, _WindowY - 120.0f, _WindowX - 100.0f, _WindowY - 120.0f);

	// Draw other connected client playercards
	float posY = _WindowY - 150.0f;
	for (auto iter : _Client->getClientList()) {

		if (iter->ID != _Client->getInfo().ID) {

			_Renderer->drawCircle(_WindowX - 440.0f, posY + 5, 15.0f);
			_Renderer->setRenderColour(0.0f, 0.0f, 0.0f);
			_Renderer->drawText(_FontSml, std::to_string(iter->Channel).c_str(), _WindowX - 445.0f, posY);
			_Renderer->setRenderColour(1.0f, 1.0f, 1.0f);
			_Renderer->drawText(_FontSml, iter->ProfileName.c_str(), _WindowX - 400.0f, posY);
			posY -= 40.0f;
		}
	}

	// Draw text chat outputs
	if (_ObjChatMessages.size() > 0) {

		for (auto msg : _ObjChatMessages) { 
			
			if (msg != nullptr) { msg->draw(_Renderer, _FontSml); }
		}
	}

	// Finished rendering for this frame
	_Renderer->end();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	ImGui window & process when a user is changing their profile name.
	
	@return:	VOID
*/
void DemoApplicationApp::ShowSetProfileName() {

	ImGui::Begin("Change Profile Name");
	ImGui::SetWindowPos(ImVec2(_WindowX - 600.0f, _WindowY - 300.0f));
	ImGui::SetWindowSize(ImVec2(500, 50)); 
	
	static char profileName[128];
	ImGui::InputText("##", profileName, sizeof(profileName));
	ImGui::SameLine();

	// Update on the server the client's profile name
	if (ImGui::Button(" Change ## Profile Name") && profileName[0] != '\0') {

		if (_Client) {
			
			// Set new profile name
			std::string name = profileName;
			_Client->RequestProfileNameToServer(name);
		}

		// Clear input buffer
		for (int i = 0; i < 128; ++i) { profileName[i] = '\0'; }

		// Close window
		_ChangeProfileNameWindow = false;
	}

	ImGui::End();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	ImGui window & process when a user wants to broadcast a text message to all clients.
	
	@return:	VOID
*/
void DemoApplicationApp::ShowMsgAllWindow() {
		
	ImGui::Begin("Text Message / Everyone");
	ImGui::SetWindowPos(ImVec2(_WindowX - 600.0f, _WindowY - 300.0f));
	ImGui::SetWindowSize(ImVec2(500, 50));

	static char allMsg[128];
	ImGui::InputText("##", allMsg, sizeof(allMsg));
	ImGui::SameLine();

	// Send message to ALL_CLIENTS when the "Send" button is pressed
	if (ImGui::Button(" SEND ## All Clients") && allMsg[0] != '\0') {

		if (_Client) { 

			// Send the message
			std::string str = allMsg;
			_Client->sendChatMessageToAll(0, str, MessageChannelType::ALL_CLIENTS); // '0' == ALL_CLIENTS

			// Store message locally
			ChatMessage* msg = new ChatMessage(_AllyColour, 10, _MostRecentMessagePosY);
			msg->setChatMessage(" [ You ]: " + _Client->getOutMessage());
			_ObjChatMessages.push_back(msg);

			UpdateNextPosition();
		}

		// Clear input buffer
		for (int i = 0; i < 128; ++i) { allMsg[i] = '\0'; }

		// Close window
		_MsgAllWindow = false;
	}

	ImGui::SameLine();
	if (ImGui::Button(" Clear ## All Clients")) {

		// Clear input buffer
		for (int i = 0; i < 128; ++i) { allMsg[i] = '\0'; }
	}

	ImGui::End();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	ImGui window & process when a user wants to broadcast a text message to clients with 
				the same communication channel.
	
	@return:	VOID
*/
void DemoApplicationApp::ShowMsgTeamWindow() {
	
	ImGui::Begin("Text Message / Teammates");
	ImGui::SetWindowPos(ImVec2(_WindowX - 600.0f, _WindowY - 300.0f));
	ImGui::SetWindowSize(ImVec2(500, 50));

	static char teamMsg[128];
	ImGui::InputText("##", teamMsg, sizeof(teamMsg));
	ImGui::SameLine();
	
	// Send message to TEAM_ONLY when the "Send" button is pressed
	if (ImGui::Button(" SEND ")) {

		if (_Client) {

			// Send the message
			_Client->sendChatMessageToAll(_Client->getInfo().Channel, teamMsg, MessageChannelType::TEAM_ONLY); // Channel represents team
			
			// Store message locally
			ChatMessage* msg = new ChatMessage(_AllyColour, 10, _MostRecentMessagePosY);
			msg->setChatMessage(" [ You ]: " + _Client->getOutMessage());
			_ObjChatMessages.push_back(msg);

			UpdateNextPosition();
		}

		// Clear input buffer
		for (int i = 0; i < 128; ++i) { teamMsg[i] = '\0'; }

		// Close window
		_MsgTeamWindow = false;
	}

	ImGui::SameLine();
	if (ImGui::Button(" Clear ## Team Only")) {

		// Clear input buffer
		for (int i = 0; i < 128; ++i) { teamMsg[i] = '\0'; }
	}

	ImGui::End();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	ImGui window & process when a user wants to send a text message to a specific client.
	
	@return:	VOID
*/
void DemoApplicationApp::ShowMsgWhisperWindow() {

	ImGui::Begin("Text Message / Whisper");
	ImGui::SetWindowPos(ImVec2(_WindowX - 600.0f, _WindowY - 300.0f));
	ImGui::SetWindowSize(ImVec2(500, 800));
	
	// Create ImGui listbox
	unsigned int i = 0;
	const char* listbox_items[MAX_CLIENTS];
	int remainder = MAX_CLIENTS - _Client->getClientList().size();

	for (i; i < _Client->getClientList().size(); ++i) {

		// Update listbox with the local client list
		listbox_items[i] = _Client->getClientList().at(i)->ProfileName.c_str();
	}
		
	for (unsigned int j = i; j < (remainder + i); ++j) { 

		listbox_items[j] = "MISSING CLIENT";
	}

	static int listbox_item_current = 1;
	ImGui::ListBox("##listbox\n(single select)", &listbox_item_current, listbox_items, 8, 8);
	ImGui::Spacing();
	
	static char whisperMsg[128];
	ImGui::InputText("## Message Whisper", whisperMsg, sizeof(whisperMsg));
	ImGui::SameLine();

	// Send message via WHISPER to client(s)
	if (ImGui::Button(" SEND ")) {

		if (_Client) {

			// Get GUID of the message recipient
			RakNet::RakNetGUID targetGUID;
			for (auto iter : _Client->getClientList()) {

				// Found a match based on profile name
				if (iter->ProfileName.compare(listbox_items[listbox_item_current]) == 0) {

					targetGUID = iter->GUID;
					break;
				}
			}

			// Send the message
			_Client->sendChatMessageToGUID(targetGUID, whisperMsg, MessageChannelType::WHISPER);
						
			// Store message locally
			ChatMessage* msg = new ChatMessage(_AllyColour, 10, _MostRecentMessagePosY);
			msg->setChatMessage(" [ You ]: " + _Client->getOutMessage());
			_ObjChatMessages.push_back(msg);

			UpdateNextPosition();
		}

		// Clear input buffer
		for (int i = 0; i < 128; ++i) { whisperMsg[i] = '\0'; }

		// Close window
		_MsgWhisperWindow = false;
	}

	ImGui::SameLine();
	if (ImGui::Button(" Clear ## Message Whisper")) {

		// Clear input buffer
		for (int i = 0; i < 128; ++i) { whisperMsg[i] = '\0'; }
	}

	ImGui::End();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Event when the client receives a message from another client.
	
	@return:	VOID
*/
void DemoApplicationApp::OnNewClientMessage() {

	ChatMessage::MessageColour colour;

	// Message is not a whisper
	if (_Client->getInMessageChannelType() != 2) {

		// If the msg channel is the same as the client's channel
		if (int teamID = _Client->getInMessageClientChannel() == _Client->getInfo().Channel) { colour = _AllyColour; }

		// msg channel does NOT match the client's channel
		else { colour = _EnemyColour; }
	}

	// Message is a whisper
	else { colour = _WhisperColour; }

	std::string s2 = " ]: ";
	std::string s = " [ " + _Client->getInMessageProfileName() + s2;

	// Display the most recent msg recieved
	ChatMessage* msg = new ChatMessage(colour, 10, _MostRecentMessagePosY);
	msg->setChatMessage(s + _Client->getInMessageString());
	_ObjChatMessages.push_back(msg);
	UpdateNextPosition();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Event when the client receives a message from the server.
	
	@return:	VOID
*/
void DemoApplicationApp::OnNewServerMessage() {

	// Display message in white
	ChatMessage::MessageColour colour;
	colour.r = 1.0f; colour.g = 1.0f; colour.b = 1.0f;
	std::string s = " [ SERVER ]: ";
	
	// Display the most recent server msg recieved
	ChatMessage* msg = new ChatMessage(colour, 10, _MostRecentMessagePosY);
	msg->setChatMessage(s + _Client->getInMessageString());
	_ObjChatMessages.push_back(msg);
	UpdateNextPosition();
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Used to "push up" the older text chat messages in the rendering window.
	
	@return:	VOID
*/
void DemoApplicationApp::UpdateNextPosition() {

	// Push all known messages upwards
	for (ChatMessage* iter : _ObjChatMessages) { iter->addPosY(25.0f); }
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Updates the voice chat processes in the client object.
	
	@return:	VOID
*/
void DemoApplicationApp::UpdateVoiceChat() {

	// Get input instance
	aie::Input* input = aie::Input::getInstance();
	
	// Update FMOD voice compoent
	if (_Client) { _Client->UpdateFMOD(); }

	// Push to talk
	if (input->isKeyDown(aie::INPUT_KEY_T) && !_AChatWindowIsActive) {

		// Client isnt currently talking
		if (!_Client->isTalking()) {

			_Client->setTryingToBroadcastVoice(true);

			// Request talking channel with teammates only
			for (auto iter : _Client->getClientList()) {

				// Skip ourself
				if (iter->GUID != _Client->getInfo().GUID) {

					// iterator is a team mate
					if (iter->Channel == _Client->getInfo().Channel) { _Client->RequestVoiceChannel(iter->GUID); }
				}
			}
		}
	}

	// Only broadcast voice if client is talking
	if (_Client->isTalking()) {

		// Only send voice to clients on the same channel (team mates)
		for (auto iter : _Client->getClientList()) {

			// Skip ourself
			if (iter->ID != _Client->getInfo().ID) {

				// Send voice
				if (iter->Channel == _Client->getInfo().Channel) { _Client->SendVoiceBuffer(iter->GUID, _Client->getVoiceBuffer()); }
			}
		}
	}

	// We are no longer trying to talk so close all open voice channels
	else {

		// Only send voice to clients on the same channel (team mates)
		for (auto iter : _Client->getClientList()) {

			// Skip ourself
			if (iter->ID != _Client->getInfo().ID) {

				// Close channel
				if (iter->Channel == _Client->getInfo().Channel) { _Client->CloseVoiceChannel(iter->GUID); }
			}
		}
	}
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Function representing 1 update frame on a chat message object when the application is running.
	
	@return:	VOID
*/
void DemoApplicationApp::ChatMessage::update(float deltaTime, DemoApplicationApp* app) {

	if (_Active) {

		// Add to timer
		if (_TimeOnScreen < _MaxTimeOnScreen) { _TimeOnScreen += deltaTime; }

		// Timer threshold reached
		else { setFade(true); }

		// Fade out to full transparency
		if (_Fade) { _Alpha -= deltaTime; }	
	}
}

/** --------------------------------------------------------------------------------------------------------------
	@Summary:	Function representing 1 rendering frame on a chat message object when the application is running.
	
	@return:	VOID
*/
void DemoApplicationApp::ChatMessage::draw(aie::Renderer2D* renderer, aie::Font* font) {

	// Draw message text
	renderer->setRenderColour(_MessageColour.r, _MessageColour.g, _MessageColour.b, _Alpha);
	renderer->drawText(font, _MessageString.c_str(), _PosX, _PosY);
}