#include "Networks.h"
#include "ModuleNetworkingServer.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::setListenPort(int port)
{
	listenPort = port;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::onStart()
{
	if (!createSocket()) return;

	// Reuse address
	int enable = 1;
	int res = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingServer::start() - setsockopt");
		disconnect();
		return;
	}

	// Create and bind to local address
	if (!bindSocketToPort(listenPort)) {
		return;
	}

	state = ServerState::Listening;
	App->modGameObject->interpolateEntities = false;

	secondsSinceLastPing = 0.0f;
}

void ModuleNetworkingServer::onGui()
{
	if (ImGui::CollapsingHeader("ModuleNetworkingServer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Connection checking info:");
		ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
		ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

		ImGui::Separator();

		ImGui::Text("Replication");
		ImGui::InputFloat("Delivery interval (s)", &replicationDeliveryIntervalSeconds, 0.01f, 0.1f);
		
		ImGui::Separator();

		ImGui::Text("ZombieSpawnRatio");
		ImGui::InputFloat("Spawning Interval (s)", &zombieSpawnRatio, 0.1f, 10.0f);

		ImGui::Separator();

		if (state == ServerState::Listening)
		{
			int count = 0;

			for (int i = 0; i < MAX_CLIENTS; ++i)
			{
				if (clientProxies[i].name != "")
				{
					ImGui::Text("CLIENT %d", count++);
					ImGui::Text(" - address: %d.%d.%d.%d",
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b1,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b2,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b3,
						clientProxies[i].address.sin_addr.S_un.S_un_b.s_b4);
					ImGui::Text(" - port: %d", ntohs(clientProxies[i].address.sin_port));
					ImGui::Text(" - name: %s", clientProxies[i].name.c_str());
					ImGui::Text(" - id: %d", clientProxies[i].clientId);
					ImGui::Text(" - Last packet time: %.04f", clientProxies[i].lastPacketReceivedTime);
					ImGui::Text(" - Seconds since repl.: %.04f", clientProxies[i].secondsSinceLastReplication);
					
					ImGui::Separator();
				}
			}

			ImGui::Checkbox("Render colliders", &App->modRender->mustRenderColliders);
		}
	}
}

void ModuleNetworkingServer::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	if (state == ServerState::Listening)
	{
		// Register player
		ClientProxy *proxy = getClientProxy(fromAddress);

		// Read the packet type
		ClientMessage message;
		packet >> message;

		// Process the packet depending on its type
		if (message == ClientMessage::Hello)
		{
			bool newClient = false;

			if (proxy == nullptr)
			{
				proxy = createClientProxy();

				newClient = true;

				std::string playerName;
				uint8 spaceshipType;
				packet >> playerName;
				packet >> spaceshipType;

				proxy->address.sin_family = fromAddress.sin_family;
				proxy->address.sin_addr.S_un.S_addr = fromAddress.sin_addr.S_un.S_addr;
				proxy->address.sin_port = fromAddress.sin_port;
				proxy->connected = true;
				proxy->name = playerName;
				proxy->clientId = nextClientId++;

				// Create new network object
				spawnPlayer(*proxy, spaceshipType);

				// Send welcome to the new player
				OutputMemoryStream welcomePacket;
				welcomePacket << ServerMessage::Welcome;
				welcomePacket << proxy->clientId;
				welcomePacket << proxy->gameObject->networkId;
				sendPacket(welcomePacket, fromAddress);

				// Send all network objects to the new player
				uint16 networkGameObjectsCount;
				GameObject *networkGameObjects[MAX_NETWORK_OBJECTS];
				App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
				for (uint16 i = 0; i < networkGameObjectsCount; ++i)
				{
					GameObject *gameObject = networkGameObjects[i];

					// TODO(jesus): Notify the new client proxy's replication manager about the creation of this game object
					proxy->replicationManager.create(gameObject->networkId);
				}

				LOG("Message received: hello - from player %s", playerName.c_str());
			}

			if (!newClient)
			{
				// Send welcome to the new player
				OutputMemoryStream unwelcomePacket;
				unwelcomePacket << ServerMessage::Unwelcome;
				sendPacket(unwelcomePacket, fromAddress);

				WLOG("Message received: UNWELCOMED hello - from player %s", proxy->name.c_str());
			}
		}
		else if (message == ClientMessage::Input)
		{
			// Process the input packet and update the corresponding game object
			if (proxy != nullptr)
			{
				// Read input data
				while (packet.RemainingByteCount() > 0)
				{
					InputPacketData inputData;
					packet >> inputData.sequenceNumber;
					packet >> inputData.horizontalAxis;
					packet >> inputData.verticalAxis;
					packet >> inputData.buttonBits;
					packet >> inputData.mouseX;
					packet >> inputData.mouseY;
					packet >> inputData.leftButton;

					if (inputData.sequenceNumber >= proxy->nextExpectedInputSequenceNumber)
					{
						//Process Keyboard
						proxy->gamepad.horizontalAxis = inputData.horizontalAxis;
						proxy->gamepad.verticalAxis = inputData.verticalAxis;
						unpackInputControllerButtons(inputData.buttonBits, proxy->gamepad);
						proxy->gameObject->behaviour->onInput(proxy->gamepad);

						//Process Mouse
						proxy->mouse.x = inputData.mouseX;
						proxy->mouse.y = inputData.mouseY;
						proxy->mouse.buttons[0] = (ButtonState)inputData.leftButton;
						proxy->gameObject->behaviour->onMouse(proxy->mouse);

						proxy->nextExpectedInputSequenceNumber = inputData.sequenceNumber + 1;
					}
				}
			}
		}
		else if (message == ClientMessage::Ping)
		{
			App->delManager->processAckdSequenceNumbers(packet);
		}

		if (proxy != nullptr)
		{
			proxy->lastPacketReceivedTime = Time.time;
		}
	}
}

void ModuleNetworkingServer::onUpdate()
{
	if (state == ServerState::Listening)
	{
		secondsSinceLastPing += Time.deltaTime;

		// Replication
		for (ClientProxy &clientProxy : clientProxies)
		{
			if (clientProxy.connected)
			{
				clientProxy.secondsSinceLastReplication += Time.deltaTime;

				OutputMemoryStream packet;	
				packet << ServerMessage::Replication;
				packet << clientProxy.nextExpectedInputSequenceNumber-1; //ACK of last received input

				Delivery* delivery = App->delManager->writeSequenceNumber(packet);
				delivery->deleagate = new DeliveryDelegateReplication(); //TODOAdPi
				((DeliveryDelegateReplication*)delivery->deleagate)->replicationCommands = clientProxy.replicationManager.GetCommands();
				// TODO(jesus): If the replication interval passed and the replication manager of this proxy
				//              has pending data, write and send a replication packet to this client.
				if (clientProxy.secondsSinceLastReplication > replicationDeliveryIntervalSeconds)
				{
					clientProxy.secondsSinceLastReplication = 0.0f;
					if (clientProxy.replicationManager.write(packet))
					{
						sendPacket(packet, clientProxy.address);
					}
				}

				//Send ping to clients
				if (secondsSinceLastPing > PING_INTERVAL_SECONDS)
				{
					OutputMemoryStream ping;
					ping << ServerMessage::Ping;

					sendPacket(ping, clientProxy.address);
				}

				//Disconnect client if waited too long
				if (Time.time - clientProxy.lastPacketReceivedTime > DISCONNECT_TIMEOUT_SECONDS)
				{
					onConnectionReset(clientProxy.address);
				}
			}
		}

		//Reset ping timer
		if (secondsSinceLastPing > PING_INTERVAL_SECONDS)
		{
			secondsSinceLastPing = 0.0f;
		}

		//Check for TimeOutPackets DeliveryManager
		App->delManager->processTimedOutPackets();

		//Zombie Spawner WiP
		ZombieSpawner();
	}
}

void ModuleNetworkingServer::onConnectionReset(const sockaddr_in & fromAddress)
{
	// Find the client proxy
	ClientProxy *proxy = getClientProxy(fromAddress);

	if (proxy)
	{
		// Notify game object deletion to replication managers
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (clientProxies[i].connected && proxy->clientId != clientProxies[i].clientId)
			{
				// TODO(jesus): Notify this proxy's replication manager about the destruction of this player's game object
				clientProxies[i].replicationManager.destroy(proxy->gameObject->networkId);
			}
		}

		// Unregister the network identity
		App->modLinkingContext->unregisterNetworkGameObject(proxy->gameObject);

		// Remove its associated game object
		Destroy(proxy->gameObject);

		// Clear the client proxy
		destroyClientProxy(proxy);
	}
}

void ModuleNetworkingServer::onDisconnect()
{
	// Destroy network game objects
	uint16 netGameObjectsCount;
	GameObject *netGameObjects[MAX_NETWORK_OBJECTS];
	App->modLinkingContext->getNetworkGameObjects(netGameObjects, &netGameObjectsCount);
	for (uint32 i = 0; i < netGameObjectsCount; ++i)
	{
		NetworkDestroy(netGameObjects[i]);
	}

	// Clear all client proxies
	for (ClientProxy &clientProxy : clientProxies)
	{
		destroyClientProxy(&clientProxy);
	}
	
	nextClientId = 0;

	state = ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Client proxies
//////////////////////////////////////////////////////////////////////

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::getClientProxy(const sockaddr_in &clientAddress)
{
	// Try to find the client
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].address.sin_addr.S_un.S_addr == clientAddress.sin_addr.S_un.S_addr &&
			clientProxies[i].address.sin_port == clientAddress.sin_port)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

ModuleNetworkingServer::ClientProxy * ModuleNetworkingServer::createClientProxy()
{
	// If it does not exist, pick an empty entry
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clientProxies[i].connected)
		{
			return &clientProxies[i];
		}
	}

	return nullptr;
}

void ModuleNetworkingServer::destroyClientProxy(ClientProxy * proxy)
{
	*proxy = {};
}



//////////////////////////////////////////////////////////////////////
// Spawning
//////////////////////////////////////////////////////////////////////

GameObject * ModuleNetworkingServer::spawnPlayer(ClientProxy &clientProxy, uint8 spaceshipType)
{
	// Create a new game object with the player properties
	clientProxy.gameObject = Instantiate();
	clientProxy.gameObject->size = { 43, 49 };
	clientProxy.gameObject->angle = 45.0f;
	clientProxy.gameObject->order = 3;

	clientProxy.gameObject->texture = App->modResources->robot;

	// Create collider
	clientProxy.gameObject->collider = App->modCollision->addCollider(ColliderType::Player, clientProxy.gameObject);
	clientProxy.gameObject->collider->isTrigger = true;

	// Create behaviour
	clientProxy.gameObject->behaviour = new Player;
	clientProxy.gameObject->behaviour->gameObject = clientProxy.gameObject;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObject(clientProxy.gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(clientProxy.gameObject->networkId);
		}
	}
	
	spawnZombie({ 50,500 });

	return clientProxy.gameObject;
}

GameObject * ModuleNetworkingServer::spawnBullet(GameObject *parent, vec2 offset)
{
	// Create a new game object with the player properties
	GameObject *gameObject = Instantiate();
	gameObject->size = { 8, 14 };
	gameObject->angle = parent->angle;
	gameObject->order = 4;
	vec2 forward = vec2FromDegrees(parent->angle);
	vec2 right = { -forward.y, forward.x };
	gameObject->position = parent->position + offset.x*right + offset.y*forward;
	gameObject->texture = App->modResources->bullet;
	gameObject->collider = App->modCollision->addCollider(ColliderType::Bullet, gameObject);

	// Create behaviour
	gameObject->behaviour = new Laser;
	gameObject->behaviour->gameObject = gameObject;

	// Assign a new network identity to the object
	App->modLinkingContext->registerNetworkGameObject(gameObject);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(gameObject->networkId);
		}
	}

	return gameObject;
}

void ModuleNetworkingServer::ZombieSpawner()
{
	
	timeSinceLastZombieSpawned += Time.deltaTime;
	if (timeSinceLastZombieSpawned > zombieSpawnRatio)
	{		
		spawnZombie({ RandomFloat(-500.0f, 500.0f), RandomFloat(-500.0f, 500.0f) });
		timeSinceLastZombieSpawned = 0.0f;
	}
	

}

float ModuleNetworkingServer::RandomFloat(float min, float max)
{
	return ((float)rand() / RAND_MAX) * (max - min) + min;
}

GameObject * ModuleNetworkingServer::spawnZombie(vec2 position)
{
	GameObject* zombie = Instantiate();
	zombie->size = { 43, 35 };
	zombie->position = position;
	zombie->order = 2;
	zombie->texture = App->modResources->zombie;
	zombie->collider = App->modCollision->addCollider(ColliderType::Zombie, zombie);
	zombie->collider->isTrigger = true;

	zombie->behaviour = new Zombie();
	zombie->behaviour->gameObject = zombie;

	App->modLinkingContext->registerNetworkGameObject(zombie);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(zombie->networkId);
		}
	}

	return zombie;
}

GameObject * ModuleNetworkingServer::spawnExplosion(GameObject* zombie)
{
	GameObject* object = Instantiate();
	object->size = { 60, 60 };
	object->position = zombie->position;
	object->order = 6;
	object->animation = App->modAnimations->useAnimation("explosion");

	Explosion* script = new Explosion();
	script->gameObject = object;
	script->zombie = zombie;
	object->behaviour = script;

	App->modLinkingContext->registerNetworkGameObject(object);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(object->networkId);
		}
	}

	return object;
}

GameObject * ModuleNetworkingServer::spawnBlood(vec2 position, float angle)
{
	GameObject* object = Instantiate();
	object->size = { 50, 50 };
	object->position = position;
	object->angle = angle;
	object->texture = App->modResources->blood;
	object->order = 0;

	object->behaviour = new Explosion();
	object->behaviour->gameObject = object;

	App->modLinkingContext->registerNetworkGameObject(object);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(object->networkId);
		}
	}

	return object;
}

GameObject* ModuleNetworkingServer::spawnRezUI(vec2 position)
{
	GameObject* object = Instantiate();
	object->size = { 66, 85 };
	object->position = position;
	object->animation = App->modAnimations->useAnimation("rez");
	object->order = 5;

	App->modLinkingContext->registerNetworkGameObject(object);

	// Notify all client proxies' replication manager to create the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the creation of this game object
			clientProxies[i].replicationManager.create(object->networkId);
		}
	}

	return object;
}

std::vector<GameObject*> ModuleNetworkingServer::getAllClientPlayers()
{
	std::vector<GameObject*> players;

	for (auto &client : clientProxies)
	{
		if (client.connected)
			players.push_back(client.gameObject);
	}

	return players;
}


//////////////////////////////////////////////////////////////////////
// Update / destruction
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingServer::destroyNetworkObject(GameObject * gameObject)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the destruction of this game object
			clientProxies[i].replicationManager.destroy(gameObject->networkId);
		}
	}

	// Assuming the message was received, unregister the network identity
	App->modLinkingContext->unregisterNetworkGameObject(gameObject);

	// Finally, destroy the object from the server
	Destroy(gameObject);
}

void ModuleNetworkingServer::updateNetworkObject(GameObject * gameObject, ReplicationAction updateType)
{
	// Notify all client proxies' replication manager to destroy the object remotely
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clientProxies[i].connected)
		{
			// TODO(jesus): Notify this proxy's replication manager about the update of this game object
			clientProxies[i].replicationManager.update(gameObject->networkId, updateType);
		}
	}
}


//////////////////////////////////////////////////////////////////////
// Global update / destruction of game objects
//////////////////////////////////////////////////////////////////////

void NetworkUpdate(GameObject * gameObject, ReplicationAction updateType)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetServer->updateNetworkObject(gameObject, updateType);
}

void NetworkDestroy(GameObject * gameObject)
{
	ASSERT(App->modNetServer->isConnected());

	App->modNetServer->destroyNetworkObject(gameObject);
}

std::vector<GameObject*> getPlayers()
{
	return App->modNetServer->getAllClientPlayers();
}
