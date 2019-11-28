#pragma once

#include "ModuleNetworking.h"

class ModuleNetworkingServer : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingServer public methods
	//////////////////////////////////////////////////////////////////////

	void setListenPort(int port);



private:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	bool isServer() const override { return true; }

	void onStart() override;

	void onGui() override;

	void onPacketReceived(const InputMemoryStream &packet, const sockaddr_in &fromAddress) override;

	void onUpdate() override;

	void onConnectionReset(const sockaddr_in &fromAddress) override;

	void onDisconnect() override;



	//////////////////////////////////////////////////////////////////////
	// Client proxies
	//////////////////////////////////////////////////////////////////////

	uint32 nextClientId = 0;

	struct ClientProxy
	{
		bool connected = false;
		sockaddr_in address;
		uint32 clientId;
		std::string name;
		GameObject *gameObject = nullptr;
		double lastPacketReceivedTime = 0.0f;
		float secondsSinceLastReplication = 0.0f;

		uint32 nextExpectedInputSequenceNumber = 0;
		InputController gamepad;
		MouseController mouse;

		ReplicationManagerServer replicationManager;
	};

	ClientProxy clientProxies[MAX_CLIENTS];

	ClientProxy * createClientProxy();

	ClientProxy * getClientProxy(const sockaddr_in &clientAddress);

	void destroyClientProxy(ClientProxy * proxy);



public:

	//////////////////////////////////////////////////////////////////////
	// Spawning network objects
	//////////////////////////////////////////////////////////////////////

	GameObject * spawnPlayer(ClientProxy &clientProxy, uint8 spaceshipType);

	GameObject * spawnBullet(GameObject *parent, vec2 offset, bool clientInstance = false);

	GameObject* spawnRezUI(vec2 position);

	// NOTE(jesus): Here go spawn methods for each type of network objects

	void ZombieSpawner();
	float RandomFloat(float min, float max);

	//Zombie related objects
	GameObject* spawnZombie(vec2 position);
	GameObject* spawnExplosion(GameObject* zombie);
	GameObject* spawnBlood(vec2 position, float angle);
	
	float zombieSpawnRatio = 2.0;
	float timeSinceLastZombieSpawned = 0;
	bool isSpawnerEnabled = true;
	bool isPlayerConneted = false;

	//Render Order
	// Layer -1 - MAP
	// Layer 0 - BLOOD
	// Layer 1 - DEAD PLAYERS
	// Layer 2 - ZOMBIES
	// Layer 3 - PLAYERS
	// Layer 4 - BULLETS
	// Layer 5 - REZ UI
	// Layer 6 - EXPLOSIONS

private:

	//Get Players

	std::vector<GameObject*> getAllClientPlayers();
	friend std::vector<GameObject*>(getPlayers)();


	//////////////////////////////////////////////////////////////////////
	// Updating / destroying network objects
	//////////////////////////////////////////////////////////////////////

	void destroyNetworkObject(GameObject *gameObject);
	friend void (NetworkDestroy)(GameObject *);

	void updateNetworkObject(GameObject *gameObject, ReplicationAction updateType);
	friend void (NetworkUpdate)(GameObject *, ReplicationAction updateType);



	//////////////////////////////////////////////////////////////////////
	// State
	//////////////////////////////////////////////////////////////////////

	enum class ServerState
	{
		Stopped,
		Listening
	};

	ServerState state = ServerState::Stopped;

	uint16 listenPort = 0;

	float secondsSinceLastPing = 0.0f;
	
	float replicationDeliveryIntervalSeconds = REPLICATION_INTERVAL_SECONDS;
};


// NOTE(jesus): It marks an object for replication updates
void NetworkUpdate(GameObject *gameObject, ReplicationAction updateType);

// NOTE(jesus): For network objects, use this version instead of
// the default Destroy(GameObject *gameObject) one. This one makes
// sure to notify the destruction of the object to all connected
// machines.
void NetworkDestroy(GameObject *gameObject);

std::vector<GameObject*> getPlayers();
