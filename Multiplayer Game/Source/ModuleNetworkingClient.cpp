#include "Networks.h"
#include "ModuleNetworkingClient.h"



//////////////////////////////////////////////////////////////////////
// ModuleNetworkingClient public methods
//////////////////////////////////////////////////////////////////////


void ModuleNetworkingClient::setServerAddress(const char * pServerAddress, uint16 pServerPort)
{
	serverAddressStr = pServerAddress;
	serverPort = pServerPort;
}

void ModuleNetworkingClient::setPlayerInfo(const char * pPlayerName, uint8 pSpaceshipType)
{
	playerName = pPlayerName;
	spaceshipType = pSpaceshipType;
}

GameObject * ModuleNetworkingClient::spawnLaser(GameObject * player)
{
	GameObject *gameObject = Instantiate();
	gameObject->size = { 7, 1000 };
	gameObject->angle = player->angle;
	gameObject->order = 2;
	vec2 forward = vec2FromDegrees(player->angle);
	vec2 right = { -forward.y, forward.x };
	vec2 offset = { 10.0f, 500.0f };
	gameObject->position = player->position + offset.x*right + offset.y*forward;
	gameObject->texture = App->modResources->laser;

	return gameObject;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

void ModuleNetworkingClient::onStart()
{
	if (!createSocket()) return;

	if (!bindSocketToPort(0)) {
		disconnect();
		return;
	}

	// Create remote address
	serverAddress = {};
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);
	int res = inet_pton(AF_INET, serverAddressStr.c_str(), &serverAddress.sin_addr);
	if (res == SOCKET_ERROR) {
		reportError("ModuleNetworkingClient::startClient() - inet_pton");
		disconnect();
		return;
	}

	state = ClientState::Start;

	inputDataFront = 0;
	inputDataBack = 0;

	secondsSinceLastInputDelivery = 0.0f;
	secondsSinceLastPing = 0.0f;
	lastPacketReceivedTime = Time.time;

	//App->modGameObject->interpolateEntities = true;
}

void ModuleNetworkingClient::onGui()
{
	if (state == ClientState::Stopped) return;

	if (ImGui::CollapsingHeader("ModuleNetworkingClient", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (state == ClientState::WaitingWelcome)
		{
			ImGui::Text("Waiting for server response...");
		}
		else if (state == ClientState::Playing)
		{
			ImGui::Text("Connected to server");
			ImGui::Text(" - Replication Ping: %f", replicationPing);			

			ImGui::Separator();

			ImGui::Text("Player info:");
			ImGui::Text(" - Id: %u", playerId);
			ImGui::Text(" - Name: %s", playerName.c_str());

			ImGui::Separator();

			ImGui::Text("Player info:");
			ImGui::Text(" - Type: %u", spaceshipType);
			ImGui::Text(" - Network id: %u", networkId);

			vec2 playerPosition = {};
			GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
			if (playerGameObject != nullptr) {
				playerPosition = playerGameObject->position;
			}
			ImGui::Text(" - Coordinates: (%f, %f)", playerPosition.x, playerPosition.y);

			ImGui::Separator();

			ImGui::Text("Connection checking info:");
			ImGui::Text(" - Ping interval (s): %f", PING_INTERVAL_SECONDS);
			ImGui::Text(" - Disconnection timeout (s): %f", DISCONNECT_TIMEOUT_SECONDS);

			ImGui::Separator();

			ImGui::Text("Input:");
			ImGui::InputFloat("Delivery interval (s)", &inputDeliveryIntervalSeconds, 0.01f, 0.1f, 4);

			ImGui::Separator();

			ImGui::Checkbox("Entity interpolation", &App->modGameObject->interpolateEntities);
			ImGui::Checkbox("Client prediction", &clientPrediction);
		}
	}
}

void ModuleNetworkingClient::onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress)
{
	lastPacketReceivedTime = Time.time;
	ServerMessage message;
	packet >> message;

	if (state == ClientState::WaitingWelcome)
	{
		if (message == ServerMessage::Welcome)
		{
			packet >> playerId;
			packet >> networkId;
			LOG("ModuleNetworkingClient::onPacketReceived() - Welcome from server");
			state = ClientState::Playing;
		}
		else if (message == ServerMessage::Unwelcome)
		{
			WLOG("ModuleNetworkingClient::onPacketReceived() - Unwelcome from server :-(");
			disconnect();
		}
	}
	else if (state == ClientState::Playing)
	{
		// TODO(jesus): Handle incoming messages from server		
		if (message == ServerMessage::Replication)
		{
			replicationPing = Time.time - lastReplicationTime;
			lastReplicationTime = Time.time;
			//Receive ACK of input
			uint32 lastPackedProccessed;
			packet >> lastPackedProccessed;
			inputDataFront = lastPackedProccessed + 1;
			
			App->delManager->processSequenceNumber(packet);
			replicationManager.read(packet, networkId);
		}
	}
}

void ModuleNetworkingClient::onUpdate()
{
	if (state == ClientState::Stopped) return;

	if (state == ClientState::Start)
	{
		// Send the hello packet with player data

		OutputMemoryStream stream;
		stream << ClientMessage::Hello;
		stream << playerName;
		stream << spaceshipType;

		sendPacket(stream, serverAddress);

		state = ClientState::WaitingWelcome;
	}
	else if (state == ClientState::WaitingWelcome)
	{
	}
	else if (state == ClientState::Playing)
	{
		secondsSinceLastInputDelivery += Time.deltaTime;
		secondsSinceLastMouseDelivery += Time.deltaTime;
		secondsSinceLastPing += Time.deltaTime;

		if (inputDataBack/*end()*/ - inputDataFront/*begin()*/ < ArrayCount(inputData))
		{
			uint32 currentInputData = inputDataBack++;
			InputPacketData &inputPacketData = inputData[currentInputData % ArrayCount(inputData)];
			inputPacketData.sequenceNumber = currentInputData;
			inputPacketData.horizontalAxis = Input.horizontalAxis;
			inputPacketData.verticalAxis = Input.verticalAxis;
			inputPacketData.buttonBits = packInputControllerButtons(Input);
			inputPacketData.mouseX = Mouse.x - Window.width / 2;
			inputPacketData.mouseY = Mouse.y - Window.height / 2;
			inputPacketData.leftButton = Mouse.buttons[0];
			
			// Create packet (if there's input and the input delivery interval exceeded)
			if (secondsSinceLastInputDelivery > inputDeliveryIntervalSeconds)
			{
				secondsSinceLastInputDelivery = 0.0f;

				OutputMemoryStream packet;
				packet << ClientMessage::Input;

				for (uint32 i = inputDataFront; i < inputDataBack; ++i)
				{
					InputPacketData &inputPacketData = inputData[i % ArrayCount(inputData)];

					packet << inputPacketData.sequenceNumber;
					packet << inputPacketData.horizontalAxis;
					packet << inputPacketData.verticalAxis;
					packet << inputPacketData.buttonBits;
					packet << inputPacketData.mouseX;
					packet << inputPacketData.mouseY;
					packet << inputPacketData.leftButton;
										
				}

				// Clear the queue
				//inputDataFront = inputDataBack;



				sendPacket(packet, serverAddress);
			}
		}

		//Client Prediction
		GameObject* playerClientGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
		if (clientPrediction && playerClientGameObject)
		{
			if (playerClientGameObject->lastServerInputSN != 0)
			{
				//Get the server position
				Player* ret = (Player*)&playerClientGameObject->behaviour;
				ret->serverPosition;

				//Get the Client position based on the SN recieved by the server
				InputController clientPosition = inputControllerFromInputPacketData(inputData[playerClientGameObject->lastServerInputSN], Input); // Last argumnet Input is not necesary
				vec2 clientPos = { 0,0 };
				clientPos = ClientOnInput(clientPosition);
				vec2 dif_pos;
				dif_pos.x = clientPos.x - ret->serverPosition.x;
				dif_pos.y = clientPos.y - ret->serverPosition.y;
				LOG("%f %f", dif_pos.x, dif_pos.y);

				// Check if server and client position are not in sync
				if (clientPos.x != ret->serverPosition.x ||
					clientPos.y != ret->serverPosition.y)
				{

					//Reapplies the inputs not yet processed by the server						
					for (uint32 i = playerClientGameObject->lastServerInputSN; i <= inputDataBack; i++)
					{
						//check the max array for the input
						if (inputDataBack/*end()*/ - playerClientGameObject->lastServerInputSN/*begin()*/ < ArrayCount(inputData))
						{
							uint32 currentInputData = i;// Current input since position changed
							InputPacketData &inputPacketData = inputData[currentInputData % ArrayCount(inputData)];
							inputPacketData.sequenceNumber = currentInputData;
							inputPacketData.horizontalAxis = Input.horizontalAxis;
							inputPacketData.verticalAxis = Input.verticalAxis;
							inputPacketData.buttonBits = packInputControllerButtons(Input);
							inputPacketData.mouseX = Mouse.x - Window.width / 2;
							inputPacketData.mouseY = Mouse.y - Window.height / 2;
							inputPacketData.leftButton = Mouse.buttons[0];
						}
					}					
				}
			}
			// Get first input of the inputData
			InputPacketData currentInput = inputData[inputDataFront % ArrayCount(inputData)];
			InputController ret = inputControllerFromInputPacketData(currentInput, Input);

			MouseController mouse;
			mouse.x = Mouse.x - Window.width / 2;
			mouse.y = Mouse.y - Window.height / 2;
			mouse.buttons[0] = Mouse.buttons[0];
			playerClientGameObject->behaviour->onInput(ret); //Apply it
			playerClientGameObject->behaviour->onMouse(mouse);
		}

		//Send pings to server
		if (secondsSinceLastPing > PING_INTERVAL_SECONDS)
		{
			secondsSinceLastPing = 0.0f;

			OutputMemoryStream ping;
			ping << ClientMessage::Ping;
			App->delManager->writeSequenceNumberPendingAck(ping);
			sendPacket(ping, serverAddress);
			ping.Clear();
		}

		//Disconnect if waited too long
		if (Time.time - lastPacketReceivedTime > DISCONNECT_TIMEOUT_SECONDS)
		{
			disconnect();
		}
	}

	// Make the camera focus the player game object
	GameObject *playerGameObject = App->modLinkingContext->getNetworkGameObject(networkId);
	if (playerGameObject != nullptr)
	{
		App->modRender->cameraPosition = playerGameObject->position;
	}
}

void ModuleNetworkingClient::onConnectionReset(const sockaddr_in & fromAddress)
{
	disconnect();
}

void ModuleNetworkingClient::onDisconnect()
{
	state = ClientState::Stopped;

	// Get all network objects and clear the linking context
	uint16 networkGameObjectsCount;
	GameObject *networkGameObjects[MAX_NETWORK_OBJECTS] = {};
	App->modLinkingContext->getNetworkGameObjects(networkGameObjects, &networkGameObjectsCount);
	App->modLinkingContext->clear();

	// Destroy all network objects
	for (uint32 i = 0; i < networkGameObjectsCount; ++i)
	{
		Destroy(networkGameObjects[i]);
	}

	App->modRender->cameraPosition = {};
}

vec2 ModuleNetworkingClient::ClientOnInput(const InputController &input)
{
	vec2 ret;
	if (input.horizontalAxis != 0.0f || input.verticalAxis != 0.0f)
	{
		const float advanceSpeed = 200.0f;
		ret += vec2{ 1,0 } *input.horizontalAxis * advanceSpeed * Time.deltaTime;
		ret += vec2{ 0,-1 } *input.verticalAxis * advanceSpeed * Time.deltaTime;
	}
	return ret;
}
