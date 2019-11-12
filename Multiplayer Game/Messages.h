#pragma once

enum class ClientMessage
{
	Hello,
	Input,
	Mouse,
	Ping
};

enum class ServerMessage
{
	Welcome,
	Unwelcome,
	Replication,
	Ping
};
