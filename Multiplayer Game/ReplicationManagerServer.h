#pragma once
#include <map>

enum class ReplicationAction
{
	None,
	Create,
	Update,
	Destroy
};

class ReplicationManagerServer
{
public:

	void create(uint32 networkId);
	void update(uint32 networkId);
	void destroy(uint32 networkId);

	bool write(OutputMemoryStream& packet);

	std::map<uint32, ReplicationAction> commands;
};