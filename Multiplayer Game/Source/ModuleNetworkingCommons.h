#pragma once

struct InputPacketData
{
	uint32 sequenceNumber = 0;
	real32 horizontalAxis = 0.0f;
	real32 verticalAxis = 0.0f;
	uint16 buttonBits = 0;

	int16  mouseX = 0;
	int16 mouseY = 0;
	int leftButton = 0;
};

uint16 packInputControllerButtons(const InputController &input);

InputController inputControllerFromInputPacketData(const InputPacketData &inputPacketData, const InputController &previousGamepad);

void unpackInputControllerButtons(uint16 buttonBits, InputController &input);
