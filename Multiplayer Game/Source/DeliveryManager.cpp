#include "Networks.h"
#include "ReplicationManagerServer.h"
//Server (Replication)
Delivery * DeliveryManager::writeSequenceNumber(OutputMemoryStream & packet)
{
	packet << SNCount;
	Delivery * ret = new Delivery();
	ret->dispatchTime = PING_INTERVAL_SECONDS * 4.0F; //We will wait for 4 ACK before dispatching
	ret->startingTime = Time.time;
	pendingDeliveryMap[SNCount++] = ret;		
	return ret;
}
//Client (Replication)
bool DeliveryManager::processSequenceNumber(const InputMemoryStream & packet)
{
	uint32 SNRecieved;
	packet >> SNRecieved;

	if (SNRecieved >= SNExpected)
	{
		SNExpected = SNRecieved + 1;
		pendingACKList.push_back(SNRecieved);
		//LOG("Packet %i has been Recieved by Client", SNRecieved);
		return true;
	}
	else
	{
		//LOG("Difference between SNReieved and SNExpected: %i ", SNRecieved - SNExpected);
	}
		
	return false;
}

bool DeliveryManager::hasSequenceNumberPendingAck() const
{

	return false;
}
//Client (Replication)
void DeliveryManager::writeSequenceNumberPendingAck(OutputMemoryStream & packet)
{
	if (packet.GetSize() > 0)
	{
		for (uint32 &iterator : pendingACKList)
		{
			packet << iterator;
		}
	}
	pendingACKList.clear();

}
//Server (Replication)
void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream & packet)
{
	while (packet.RemainingByteCount() > 0)
	{
		uint32 ACK;
		packet >> ACK;

		if (pendingDeliveryMap.find(ACK) != pendingDeliveryMap.end())
		{
			pendingDeliveryMap[ACK]->deleagate->onDeliverySuccess(this);			
			pendingDeliveryMap.erase(ACK);
			//LOG("Packet %i has been ACK", ACK);
		}
	}

}
//Server (Replication)
void DeliveryManager::processTimedOutPackets()
{
	std::vector<uint32> eraseList;
	for (std::map<uint32, Delivery*>::iterator it = pendingDeliveryMap.begin(); it != pendingDeliveryMap.end(); ++it)
	{
		if (Time.time - (*it).second->startingTime > (*it).second->dispatchTime)
		{
			(*it).second->deleagate->onDeliveryFailure(this);

			eraseList.push_back((*it).first);

			//LOG("Packet %i has been discarted by Time out", (*it).first);
		}
	}

	for (uint32 &it : eraseList)
	{
		pendingDeliveryMap.erase(it);
	}
}

void DeliveryManager::clear()
{

}

void DeliveryDelegateReplication::onDeliveryFailure(DeliveryManager* deliveryManagerr)
{
	if (replicationCommands.size() > 0)
	{
		for (std::map<uint32, ReplicationAction>::iterator it = replicationCommands.begin(); it != replicationCommands.end(); ++it)
		{						
			if ((*it).second == ReplicationAction::Create)
			{
				repManager.create((*it).first);
			}
			else if ((*it).second == ReplicationAction::Destroy)
			{
				repManager.destroy((*it).first);
			}
			else if ((*it).second == ReplicationAction::Update_Position)
			{
				repManager.update((*it).first, ReplicationAction::Update_Position);
			}			
		}
	}
	replicationCommands.clear();
}

void DeliveryDelegateReplication::onDeliverySuccess(DeliveryManager* deliveryManager)
{
	
}
