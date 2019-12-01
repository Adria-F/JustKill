#include "Networks.h"

GameObject *background = nullptr;

void ScreenGame::enable()
{
	if (isServer)
	{
		App->modNetServer->setListenPort(serverPort);
		App->modNetServer->setEnabled(true);

	}
	else
	{
		App->modNetClient->setServerAddress(serverAddress, serverPort);
		App->modNetClient->setPlayerInfo(playerName);
		App->modNetClient->setEnabled(true);
	}

	background = Instantiate();
	background->texture = App->modResources->parking_lot;
	background->order = -1;
}

void ScreenGame::update()
{
	if (!(App->modNetServer->isConnected() || App->modNetClient->isConnected()))
	{
		App->modScreen->swapScreensWithTransition(this, App->modScreen->screenMainMenu);
	}
	//else
	//{
	//	if (!isServer)
	//	{
	//		vec2 camPos = App->modRender->cameraPosition;
	//		vec2 bgSize = background->texture->size;
	//	}
	//}
}

void ScreenGame::gui()
{
}

void ScreenGame::disable()
{
	Destroy(background);
}
