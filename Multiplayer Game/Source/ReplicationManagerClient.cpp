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
			{
				go->isPlayer = true;
				go->doInterpolation = false;
			}

			packet >> go->clientInstanceNID;
			if (go->clientInstanceNID != 0) //It will be an instanced object on the client
			{
				//Currently only for bullets
				go->behaviour = new Bullet();
				go->behaviour->gameObject = go;
				go->behaviour->isServer = false;
				go->doInterpolation = false;
				GameObject* clientPlayer = App->modLinkingContext->getNetworkGameObject(go->clientInstanceNID);
				if (clientPlayer)
				{
					vec2 bullet_offset = { 10.0f, 20.0f };
					vec2 forward = vec2FromDegrees(clientPlayer->angle);
					vec2 right = { -forward.y, forward.x };
					go->position = clientPlayer->position + forward * bullet_offset.y + right * bullet_offset.x;
				}
			}

			if (go->isPlayer)
			{
				Player* script = new Player();
				script->gameObject = go;
				script->isServer = false;
				script->laser = App->modNetClient->spawnLaser(go);
				script->laser->doInterpolation = false;
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

			vec2 position;
			float angle;
			packet >> position.x;
			packet >> position.y;
			packet >> angle;
			if (go != nullptr)
			{
				if (go->clientInstanceNID != 0)
				{
					go->doInterpolation = true;
				}
				go->newReplicationState(position, angle);

				if (!App->modGameObject->interpolateEntities || !go->doInterpolation)
				{
					go->position = position;
					go->angle = angle;
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
				App->modNetClient->deadCount++;
			}
			else if (UID == App->modResources->robot->UID)
			{
				if (go->isPlayer)
				{
					go->texture = App->modResources->playerRobot;
					((Player*)go->behaviour)->isDown = false;
				}
				else
				{
					App->modNetClient->alliesRevived++;
				}
			}
			packet >> go->size.x;
			packet >> go->size.y;
			packet >> go->order;
		}
		else if (action == ReplicationAction::Update_Alpha)
		{
			GameObject* go = App->modLinkingContext->getNetworkGameObject(networkId);
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
				if (go->texture && go->texture->UID == App->modResources->zombie->UID) //If it is a zombie
				{
					App->modNetClient->zombieDeathCount++;
				}
				App->modLinkingContext->unregisterNetworkGameObject(go);
				Destroy(go);
			}
		}
	}
}
