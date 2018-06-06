/*
	Created by: DANIEL MARTON
	
	Created on: 04/04/2018
	Last edited on: 05/05/2018
*/

// Standard libraries
#include <iostream>
#include <string>

// Bootstrap libraries
#include "DemoApplicationApp.h"

// Raknet libraries
#include <stdio.h>
#include <RakPeerInterface.h>

// NPC libraries
#include <Server.h>
#include <Client.h>

// Definitions
#include "definitions.h"

#ifdef _DEBUG
	//#include <vld.h>
#endif // DEBUG

//*********************************************************
// VARIABLES

Server* _Server = nullptr;
Client* _Client = nullptr;

//*********************************************************
// FUNCTIONS

void GetDesktopResolution(int& a_Horizontal, int& a_Vertical) {

	// Get a handle to the desktop window
	RECT desktop;

	// Get the size of screen to the variable desktop
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);

	// Changes the variable in memory instead of using a copy
	a_Horizontal = desktop.right;
	a_Vertical = desktop.bottom;
}

void EnterAddress(std::string& address) {

	if (std::cin.bad())
		std::cin.clear();

	// Enter IP address
	std::cout << "\n - Enter IP address to connect to: " << address;
	std::cin >> address;

	// Press enter
	std::cin.get();
	return;
}

int main() {

	// Debugs memory leaks
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	RakNet::RakPeerInterface* rakp = RakNet::RakPeerInterface::GetInstance();

	// Determines if peer client or server based on user input
	std::cout << " Client or Server? < C > / < S >: ";
	bool IsServer;
	bool ValidInput = false;
	while (!ValidInput) {

		char cInput = '\0';
		std::cin >> cInput;
		switch (cInput) {

			// Starting as SERVER
			case 's':
			case 'S':
			{
				IsServer = true;
				ValidInput = true;
				_Server = new Server(MAX_CLIENTS, PORT);
				break;
			}

			// Starting as CLIENT
			case 'c':
			case 'C':
			{
				IsServer = false;
				ValidInput = true;
				std::string address;
				///address = "127.0.0.1";
				EnterAddress(address);
				_Client = new Client(address, PORT);
				break;
			}

			// Invalid input
			default:
			{
				std::cin.clear();
				std::cin.ignore(INT_MAX, '\n');
				std::cout << "\n Invalid input - Please try again: ";
				break;
			}
		}
	}

	// Run client window through bootstrap application
	if (!IsServer && _Client) {

		if (_Client->isConnected()) {

			// Get desktop resolution
			int windowX;
			int windowY;
			GetDesktopResolution(windowX, windowY);

			// Run bootstrap application
			auto app = new DemoApplicationApp(windowX - 300, windowY - 300, _Client);
			app->run("AIE", windowX - 300, windowY - 300, false);
			delete app;
		}
	}

	// Destroy network objects
	RakNet::RakPeerInterface::DestroyInstance(rakp);
	if (_Server) { delete _Server; _Server = nullptr; }

	if (_Client) {

		_Client->Shutdown();
		delete _Client; _Client = nullptr;
	}
	return 0;
}