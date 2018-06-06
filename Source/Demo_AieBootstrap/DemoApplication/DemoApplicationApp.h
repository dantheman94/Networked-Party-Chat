#pragma once

// Visual studio libraries
#include <iostream>
#include <vector>
#include <list>

// Bootstrap libraries
#include "Application.h"
#include "Renderer2D.h"
#include <imgui.h>

// NPC libraries
#include <Client.h>

class DemoApplicationApp : public aie::Application {

	class ChatMessage {
	public:

		struct MessageColour {
			
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
		};

		ChatMessage(MessageColour col, float x, float y) {

			_MessageColour = col; 
			_PosX = x; 
			_PosY = y; 		
		}
		~ChatMessage() {}

		void update(float deltaTime, DemoApplicationApp* app);
		void draw(aie::Renderer2D* renderer, aie::Font* font);

		void setChatMessage(std::string newMessage) { _MessageString = newMessage; }
		void setActive(bool value) { _Active = value; }
		void setFade(bool value) { _Fade = value; }
		void addPosY(float y) { _PosY += y; }

		MessageColour _MessageColour;

	protected:

		std::string			_MessageString;
		float				_MaxTimeOnScreen = 10.0f;
		float				_TimeOnScreen = 0.0f;
		float				_Alpha = 1.0f;
		bool				_Fade = false;
		bool				_Active = true;
		float				_PosX;
		float				_PosY;

	};

public:
	
	DemoApplicationApp(int windowX, int windowY, Client* client = nullptr);
	virtual ~DemoApplicationApp() {}

	virtual bool startup();
	virtual void shutdown();
	virtual void update(float deltaTime);
	virtual void draw();

	void ShowSetProfileName();
	void ShowMsgAllWindow();
	void ShowMsgTeamWindow();
	void ShowMsgWhisperWindow();
	void OnNewClientMessage();
	void OnNewServerMessage();
	void UpdateNextPosition();
	void UpdateVoiceChat();

protected:

	int								_WindowX, _WindowY;
	aie::Renderer2D*				_Renderer;
	aie::Font*						_FontMed;
	aie::Font*						_FontSml;

	Client*							_Client;

	bool							_ChangeProfileNameWindow = false;
	bool							_MsgAllWindow = false;
	bool							_MsgTeamWindow = false;
	bool							_MsgWhisperWindow = false;
	bool							_AChatWindowIsActive = false;

	float							_MostRecentMessagePosY = 500.0f;
	std::list<ChatMessage*>			_ObjChatMessages;

	ChatMessage::MessageColour		_AllyColour;
	ChatMessage::MessageColour		_EnemyColour;
	ChatMessage::MessageColour		_WhisperColour;
	
};