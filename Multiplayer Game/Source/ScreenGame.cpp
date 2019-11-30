#include "Networks.h"

GameObject *background = nullptr;
GameObject *collider_north = nullptr;
GameObject *collider_south = nullptr;
GameObject *collider_east = nullptr;
GameObject *collider_west = nullptr;

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
		App->modNetClient->setEnabled(true);
	}

	background = Instantiate();
	background->texture = App->modResources->parking_lot;
	background->order = -1;

	collider_north = Instantiate();
	collider_north->texture = App->modResources->collider_north;
	collider_north->position.y = -1075.0f;
	App->modCollision->addCollider(ColliderType::Wall, collider_north);	

	collider_south = Instantiate();
	collider_south->texture = App->modResources->collider_south;
	collider_south->position.y = 1075.0f;
	App->modCollision->addCollider(ColliderType::Wall, collider_south);

	collider_east = Instantiate();
	collider_east->texture = App->modResources->collider_east;
	collider_east->position.x = 1050.0f;
	App->modCollision->addCollider(ColliderType::Wall, collider_east);

	collider_west = Instantiate();
	collider_west->texture = App->modResources->collider_west;
	collider_west->position.x = -1017.0f;
	App->modCollision->addCollider(ColliderType::Wall, collider_west);	
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
