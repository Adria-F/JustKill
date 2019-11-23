#include "Networks.h"


Delivery * DeliveryManager::writeSequenceNumber(OutputMemoryStream & packet)
{
	packet << SNCount;
	Delivery * ret = new Delivery();
	ret->dispatchTime = 0.080; //TODOAdPi
	ret->startingTime = Time.time;
	deliveryMap[SNCount++] = ret;	
	return ret;
}


bool DeliveryManager::processSequenceNumber(const InputMemoryStream & packet)
{
	uint32 SNRecieved;
	packet >> SNRecieved;
	if (SNRecieved == SNExpected)
	{
		++SNExpected;
		pendingList.push_back(SNRecieved);
		return true;
	}
	return false;
}

bool DeliveryManager::hasSequenceNumberPendingAck() const
{

	return false;
}

void DeliveryManager::writeSequenceNumberPendingAck(OutputMemoryStream & packet)
{
	for (uint32 &iterator : pendingList)
	{
		packet << iterator;
	}
}

void DeliveryManager::processAckdSequenceNumbers(const InputMemoryStream & packet)
{
	while (packet.RemainingByteCount() > 0)
	{
		uint32 ACK;
		packet >> ACK;

		if (deliveryMap.find(ACK) != deliveryMap.end())
		{
			deliveryMap[ACK]->deleagate->onDeliverySuccess(this);
			deliveryMap.erase(ACK);
		}
	}

}

void DeliveryManager::processTimedOutPackets()
{
	std::vector<uint32> eraseList;
	for (std::map<uint32, Delivery*>::iterator it = deliveryMap.begin(); it != deliveryMap.end(); ++it)
	{
		if (Time.time - (*it).second->startingTime > (*it).second->dispatchTime)
		{
			(*it).second->deleagate->onDeliveryFailure(this);

			eraseList.push_back((*it).first);
		}
	}

	for (uint32 &it : eraseList)
	{
		deliveryMap.erase(it);
	}
}

void DeliveryManager::clear()
{

}

void DeliveryDelegateReplication::onDeliveryFailure(DeliveryManager * deliveryManagerr)
{

	if (replicationCommands.size() > 0)
	{
		for (std::map<uint32, ReplicationAction>::iterator it = replicationCommands.begin(); it != replicationCommands.end(); ++it)
		{			
			std::map<uint32, ReplicationAction> test = repManager.GetCommands();
			if ((*it).second == ReplicationAction::Create)
			{
				repManager.create((*it).first);
			}
			if ((*it).second == ReplicationAction::Destroy)
			{
				repManager.destroy((*it).first);
			}
			if ((*it).second == ReplicationAction::Update_Position)
			{
				repManager.update((*it).first, ReplicationAction::Update_Position);
			}
			std::map<uint32, ReplicationAction> test2 = repManager.GetCommands();
		}		
	}
}

void DeliveryDelegateReplication::onDeliverySuccess(DeliveryManager * deliveryManager)
{
	LOG("HOLA");
}
