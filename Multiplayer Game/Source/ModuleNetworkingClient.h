#pragma once

#include "ModuleNetworking.h"

class ModuleNetworkingClient : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingClient public methods
	//////////////////////////////////////////////////////////////////////

	void setServerAddress(const char *serverAddress, uint16 serverPort);

	void setPlayerInfo(const char *playerName);

	GameObject* spawnLaser(GameObject* player);

	// Replication Delay
	float replicationPing = 0.0f;

	std::list<GameObject*> players;

	//Game Statistics
	uint32 zombieDeathCount = 0;
	uint32 deadCount = 0;
	uint32 alliesRevived = 0;

private:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	bool isClient() const override { return true; }

	void onStart() override;

	void onGui() override;

	void onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress) override;

	void onUpdate() override;

	void onConnectionReset(const sockaddr_in &fromAddress) override;

	void onDisconnect() override;

	void floatingUI() override;

	void processAllInputs();

	//////////////////////////////////////////////////////////////////////
	// Client state
	//////////////////////////////////////////////////////////////////////

	enum class ClientState
	{
		Stopped,
		Start,
		WaitingWelcome,
		Playing
	};

	ClientState state = ClientState::Stopped;

	std::string serverAddressStr = "";
	uint16 serverPort = 0;

	sockaddr_in serverAddress = {};
	std::string playerName = "player";

	uint32 playerId = 0;
	uint32 networkId = 0;

	ReplicationManagerClient replicationManager;

	bool clientPrediction = true;
	bool serverReconciliation = true;

	// Input ///////////
	static const int MAX_INPUT_DATA_SIMULTANEOUS_PACKETS = 64;

	// Queue of input data
	InputPacketData inputData[MAX_INPUT_DATA_SIMULTANEOUS_PACKETS];
	uint32 inputDataFront = 0; // list.begin()
	uint32 inputDataBack = 0; // list.end()

	float inputDeliveryIntervalSeconds = 0.05f;
	float secondsSinceLastInputDelivery = 0.0f;
	float mouseDeliveryIntervalSeconds = 0.05f;
	float secondsSinceLastMouseDelivery = 0.0f;


	// Timeout / ping

	double lastPacketReceivedTime = 0.0f; // NOTE(jesus): Use this to implement client timeout
	float secondsSinceLastPing = 0.0f;    // NOTE(jesus): Use this to implement ping to server
	double lastReplicationTime = 0.0f;
};

