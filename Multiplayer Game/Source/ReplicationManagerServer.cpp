#include "Networks.h"
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::create(uint32 networkId)
{
	commands[networkId] = ReplicationAction::Create;
}

void ReplicationManagerServer::update(uint32 networkId)
{
	if (commands.find(networkId) == commands.end())
	{
		commands[networkId] = ReplicationAction::Update;
	}
}

void ReplicationManagerServer::destroy(uint32 networkId)
{
	commands[networkId] = ReplicationAction::Destroy;
}

bool ReplicationManagerServer::write(OutputMemoryStream & packet)
{
	if (commands.size() == 0)
		return false;

	for (std::map<uint32, ReplicationAction>::iterator it_c = commands.begin(); it_c != commands.end(); ++it_c)
	{	
		packet << (*it_c).first;
		packet << (*it_c).second;
		if ((*it_c).second == ReplicationAction::Create)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject((*it_c).first);
			packet << go->position.x;
			packet << go->position.y;
			packet << go->size.x;
			packet << go->size.y;
			packet << go->angle;
			packet << (go->animation != nullptr); //Boolean to know if there is animation

			if (go->animation)
			{
				packet << go->animation->tag;
			}
			else
			{
				int texture = 0;
				if (go->texture == App->modResources->spacecraft1)
					texture = 1;
				else if (go->texture == App->modResources->spacecraft2)
					texture = 2;
				else if (go->texture == App->modResources->spacecraft3)
					texture = 3;
				else if (go->texture == App->modResources->laser)
					texture = 4;
				else if (go->texture == App->modResources->asteroid1)
					texture = 5;
				else if (go->texture == App->modResources->asteroid2)
					texture = 6;

				packet << texture;
			}
		}
		if ((*it_c).second == ReplicationAction::Update)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject((*it_c).first);
			packet << go->position.x;
			packet << go->position.y;
			packet << go->angle;
		}
	}

	commands.clear();
	return true;
}
