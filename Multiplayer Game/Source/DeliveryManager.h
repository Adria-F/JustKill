#pragma once
#include "ReplicationManagerServer.h"

class DeliveryManager;

class DeliveryDelegate
{
public:
	virtual void onDeliverySuccess(DeliveryManager * deliverymanager) {};
	virtual void onDeliveryFailure(DeliveryManager * deliveryManager) {};
};

class DeliveryDelegateReplication : public DeliveryDelegate
{
public:
	void onDeliveryFailure(DeliveryManager * deliveryManagerr) override;
	void onDeliverySuccess(DeliveryManager * deliveryManager) override;
public:
	std::map<uint32, ReplicationAction> replicationCommands;
	ReplicationManagerServer repManager;
};


struct Delivery
{
	double dispatchTime = 0.0;
	double startingTime = 0.0;
	DeliveryDelegate *deleagate = nullptr;
};

class DeliveryManager
{
public:
	// For senders to write a new seq. number into a packet
	Delivery * writeSequenceNumber(OutputMemoryStream &packet);	

	// For receivers to process the seq. number from an incoming packet
	bool processSequenceNumber(const InputMemoryStream &packet);

	// For receivers to write ack'ed seq. numbers into a packet
	bool hasSequenceNumberPendingAck() const;
	void writeSequenceNumberPendingAck(OutputMemoryStream &packet);

	// For senders to process ack'ed seq. numbers from a packet
	void processAckdSequenceNumbers(const InputMemoryStream &packet);
	void processTimedOutPackets();

	void clear();

private:
	//Private members (sender side)
	// - The next outgoing sequence number
	// - A list of pending deliveries
	uint32 SNCount = 1;
	std::map<uint32, Delivery*> pendingDeliveryMap;

	//Private memebers (receiver side)
	// - The next expected sequence number
	// - A list of sequence numbers pending ack
	uint32 SNExpected = 1;
	std::vector<uint32> pendingACKList;

};





