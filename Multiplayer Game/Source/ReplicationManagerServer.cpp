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

			packet << go->texture->UID;
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
