#pragma once
#include <map>

enum class ReplicationAction
{
	None,
	Create,
	Update_Position,
	Update_Texture,
	Update_Alpha,
	Update_Animation,
	Destroy
};

class ReplicationManagerServer
{
public:

	void create(uint32 networkId);
	void update(uint32 networkId, ReplicationAction updateType);
	void destroy(uint32 networkId);

	std::map<uint32, ReplicationAction> GetCommands();
	void InsertCommands(std::pair<uint32, ReplicationAction> command);

	bool write(OutputMemoryStream& packet);

public:
	std::map<uint32, ReplicationAction> commands;
};