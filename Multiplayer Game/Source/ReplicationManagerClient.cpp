#include "Networks.h"
#include "ReplicationManagerClient.h"

void ReplicationManagerClient::read(const InputMemoryStream & packet, uint32 clientNetworkId)
{
	while (packet.RemainingByteCount() > 0)
	{
		uint32 networkId;
		packet >> networkId;
		ReplicationAction action;
		packet >> action;
		if (action == ReplicationAction::Create)
		{
			GameObject* go = nullptr;
			go = App->modLinkingContext->getNetworkGameObject(networkId, true);
			if (go != nullptr)
			{
				App->modLinkingContext->unregisterNetworkGameObject(go);
				Destroy(go);
			}

			go = Instantiate();
			App->modLinkingContext->registerNetworkGameObjectWithNetworkId(go, networkId);

			packet >> go->position.x;
			packet >> go->position.y;
			packet >> go->size.x;
			packet >> go->size.y;
			packet >> go->angle;
			packet >> go->order;
			packet >> go->color.r;
			packet >> go->color.g;
			packet >> go->color.b;
			packet >> go->color.a;
			if (networkId == clientNetworkId)
				go->isPlayer = true;

			if (go->isPlayer)
			{
				Player* script = new Player();
				script->gameObject = go;
				script->isServer = false;
				script->laser = App->modNetClient->spawnLaser(go);
				go->behaviour = script;
			}

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
				if (go->isPlayer)
				{
					go->texture = App->modResources->playerRobot;
				}
				else
				{
					go->texture = App->modTextures->getTexturebyUID(UID);
				}
			}
		}
		else if (action == ReplicationAction::Update_Position)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);

			vec2 serverposition;
			float serverangle;
			uint32 lastInputSN;
			packet >> serverposition.x;
			packet >> serverposition.y;
			packet >> serverangle;
			packet >> lastInputSN;
			if (go != nullptr)
			{
				//Client Side Predicition
				if (go->isPlayer)
				{					
					Player* ret = (Player*)&go->behaviour;
					go->lastServerInputSN = lastInputSN;
					ret->serverPosition.x = serverposition.x;
					ret->serverPosition.y = serverposition.y;									
				}		

				// Interpolation (finalPos init)
				go->newReplicationState(serverposition, serverangle);

				// Disable entity interpolation
				if (!App->modGameObject->interpolateEntities || go->isPlayer)
				{
					go->position = serverposition;
					go->angle = serverangle;
				}
				

			}
		}
		else if (action == ReplicationAction::Update_Texture)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);

			int32 UID;
			packet >> UID;
			go->texture = App->modTextures->getTexturebyUID(UID);
			if (go->isPlayer && UID == App->modResources->dead->UID) //To update player dead/alive texture
			{
				go->texture = App->modResources->playerDead;
				((Player*)go->behaviour)->isDown = true;
			}
			else if (go->isPlayer && UID == App->modResources->robot->UID)
			{
				go->texture = App->modResources->playerRobot;
				((Player*)go->behaviour)->isDown = false;
			}
			packet >> go->size.x;
			packet >> go->size.y;
			packet >> go->order;
			packet >> go->color.r;
			packet >> go->color.g;
			packet >> go->color.b;
			packet >> go->color.a;
		}
		else if (action == ReplicationAction::Update_Animation)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);
			if (go->animation != nullptr)
			{
				packet >> go->animation->spriteDuration;
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
