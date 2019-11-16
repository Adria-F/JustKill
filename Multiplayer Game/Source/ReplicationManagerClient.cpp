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
			packet >> go->position.x;
			packet >> go->position.y;
			packet >> go->angle;
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
