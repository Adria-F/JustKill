#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream & packet)
{
	while (packet.RemainingByteCount() > 0)
	{
		uint32 networkId;
		packet >> networkId;
		ReplicationAction action;
		packet >> action;
		if (action == ReplicationAction::Create)
		{
			if (App->modLinkingContext->getNetworkGameObject(networkId) == nullptr)
			{
				GameObject* go = Instantiate();
				App->modLinkingContext->registerNetworkGameObjectWithNetworkId(go, networkId);

				packet >> go->position.x;
				packet >> go->position.y;
				packet >> go->size.x;
				packet >> go->size.y;
				packet >> go->angle;
				packet >> go->order;

				go->final_position = go->position;
				go->initial_position = go->position;
				go->final_angle = go->angle;
				go->initial_angle = go->angle;
				
				bool haveAnimation = false;
				packet >> haveAnimation;

				if (haveAnimation)
				{
					std::string tag;
					packet >> tag;
					go->animation = App->modAnimations->useAnimation(tag.c_str());
				}
				else
				{
					uint32 UID;
					packet >> UID;
					go->texture = App->modTextures->getTexturebyUID(UID);
				}
			}
			else
			{
				action = ReplicationAction::Update;
			}
		}
		if (action == ReplicationAction::Update)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);
			
			vec2 position;
			float angle;
			packet >> position.x;
			packet >> position.y;
			packet >> angle;
			if (App->modGameObject->interpolateEntities)
			{
				go->newReplicationState(position, angle);
			}
			else
			{
				go->position = position;
				go->angle = angle;
			}

			int32 UID;
			packet >> UID;
			if (UID != -1)
			{
				go->texture = App->modTextures->getTexturebyUID(UID);
				packet >> go->size.x;
				packet >> go->size.y;
				packet >> go->order;
			}
		}
		else if (action == ReplicationAction::Destroy)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);
			if (go)
			{
				App->modLinkingContext->unregisterNetworkGameObject(go);
				Destroy(go);
			}
		}
	}
}
