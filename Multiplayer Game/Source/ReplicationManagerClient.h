#pragma once

class ReplicationManagerClient
{
public:

	void read(const InputMemoryStream& packet, uint32 clientNetworkId);
};