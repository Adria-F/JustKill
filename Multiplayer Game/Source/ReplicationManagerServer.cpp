#include "Networks.h"
#include "ReplicationManagerServer.h"

void ReplicationManagerServer::create(uint32 networkId)
{
	commands[networkId] = ReplicationAction::Create;
}

void ReplicationManagerServer::update(uint32 networkId, ReplicationAction updateType)
{
	if (commands.find(networkId) == commands.end() || commands[networkId] == ReplicationAction::Update_Position)
	{
		commands[networkId] = updateType;
	}
}

void ReplicationManagerServer::destroy(uint32 networkId)
{
	commands[networkId] = ReplicationAction::Destroy;
}

std::map<uint32, ReplicationAction> ReplicationManagerServer::GetCommands()
{
	return commands;
}

void ReplicationManagerServer::InsertCommands(std::pair<uint32, ReplicationAction> command)
{
	commands.insert(command);
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
			packet << go->order;
			packet << (go->animation != nullptr); //Boolean to know if there is animation

			if (go->animation)
			{
				packet << go->animation->tag;
			}
			else
			{
				packet << go->texture->UID;
			}
		}
		else if ((*it_c).second == ReplicationAction::Update_Position)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject((*it_c).first);
			packet << go->position.x;
			packet << go->position.y;
			packet << go->angle;
		}
		else if ((*it_c).second == ReplicationAction::Update_Texture)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject((*it_c).first);
			if (go->texture != nullptr)
			{
				packet << go->texture->UID;
				packet << go->size.x;
				packet << go->size.y;
				packet << go->order;
			}
		}
		else if ((*it_c).second == ReplicationAction::Update_Animation)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject((*it_c).first);
			if (go->animation != nullptr)
			{
				packet << go->animation->spriteDuration;
			}
		}
	}

	commands.clear();
	return true;
}
