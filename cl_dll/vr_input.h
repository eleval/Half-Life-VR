#pragma once

#include "vr_interface.h"

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


	void HandleButtonPress(VRButton button, VRControllerState controllerState, bool leftOrRight, bool downOrUp);
	void HandleTrackpad(VRButton button, VRControllerState controllerState, bool leftOrRight, bool downOrUp);

	void HandleButtonPressOld(VRButton button, VRControllerState controllerState, bool leftOrRight, bool downOrUp);
	void HandleTrackpadOld(VRButton button, VRControllerState controllerState, bool leftOrRight, bool downOrUp);

	void HandleButtonPressNew(VRButton button, VRControllerState controllerState, bool leftOrRight, bool downOrUp);
	void HandleTrackpadNew(VRButton button, VRControllerState controllerState, bool leftOrRight, bool downOrUp);

	void HandleButtonPressLeft(VRButton button, VRControllerState controllerState, bool downOrUp);
	void HandleButtonPressRight(VRButton button, VRControllerState controllerState, bool downOrUp);
	void HandleTrackpadLeft(VRButton button, VRControllerState controllerState, bool downOrUp);
	void HandleTrackpadRight(VRButton button, VRControllerState controllerState, bool downOrUp);
private:
	bool isTeleActive;
};

extern VRInput g_vrInput;
