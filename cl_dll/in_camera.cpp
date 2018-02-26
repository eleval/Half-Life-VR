//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "in_defs.h"
#include "cvardef.h"

extern cvar_t* vr_showPlayer;

extern "C" 
{
	void DLLEXPORT CAM_Think( void );
	int DLLEXPORT CL_IsThirdPerson( void );
	void DLLEXPORT CL_CameraOffset( float *ofs );
}

void DLLEXPORT CAM_Think( void )
{

}

int DLLEXPORT CL_IsThirdPerson( void )
{
	return vr_showPlayer->value == 1.0f;
}

void DLLEXPORT CL_CameraOffset( float *ofs )
{
}
