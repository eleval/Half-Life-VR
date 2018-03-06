#pragma once

#include "vr_system.h"

class VRInput
{
public:
	VRInput();
	~VRInput();

	cvar_t	*vr_control_alwaysforward;
	cvar_t	*vr_control_deadzone;
	cvar_t	*vr_control_teleport;
	cvar_t	*vr_control_hand;
	cvar_t	*vr_control_scheme;


	void HandleButtonPress(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown);
	void HandleTrackpad(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown);

	void HandleButtonPressOld(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown);
	void HandleTrackpadOld(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown);

	void HandleButtonPressNew(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown);
	void HandleTrackpadNew(VRButton button, VRControllerState controllerState, bool isLeftController, bool buttonDown);

	void HandleButtonPressLeft(VRButton button, VRControllerState controllerState, bool buttonDown);
	void HandleButtonPressRight(VRButton button, VRControllerState controllerState, bool buttonDown);
	void HandleTrackpadLeft(VRButton button, VRControllerState controllerState, bool buttonDown);
	void HandleTrackpadRight(VRButton button, VRControllerState controllerState, bool buttonDown);
private:
	bool isTeleActive;
};

extern VRInput g_vrInput;
