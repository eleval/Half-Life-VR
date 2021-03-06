#include "hud.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"

#include "entity_state.h"
#include "cl_entity.h"
#include "ref_params.h"
#include "in_defs.h" // PITCH YAW ROLL
#include "pm_movevars.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "event_api.h"
#include "pmtrace.h"
#include "screenfade.h"
#include "shake.h"
#include "hltv.h"
#include "keydefs.h"

#include "vr_renderer.h"
#include "vr_helper.h"

#define HARDWARE_MODE
#include "com_model.h"

#include <windows.h>
#include "vr_gl.h"

#include "cl_util.h"


extern globalvars_t *gpGlobals;

VRRenderer gVRRenderer;

cvar_t* vr_renderWorldBackface;
cvar_t* vr_renderControllerAxis;


VRRenderer::VRRenderer()
{
}

VRRenderer::~VRRenderer()
{
}

void VRRenderer::Init()
{
	vr_renderWorldBackface = gEngfuncs.pfnRegisterVariable("vr_renderWorldBackface", "1", 0);
	vr_renderControllerAxis = gEngfuncs.pfnRegisterVariable("vr_renderControllerAxis", "1", 0);
	gVRHelper.Init();
}

void VRRenderer::VidInit()
{
}

void VRRenderer::Frame(double time)
{
	// make sure these are always properly set
	gEngfuncs.pfnClientCmd("fps_max 90");
	gEngfuncs.pfnClientCmd("fps_override 1");
	gEngfuncs.pfnClientCmd("gl_vsync 0");
	gEngfuncs.pfnClientCmd("default_fov 180");
	//gEngfuncs.pfnClientCmd("firstperson");

	if (isInMenu)
	{
		//vrHelper->CaptureMenuTexture();
		//CaptureCurrentScreenToTexture(vrGLMenuTexture);
	}
	else
	{
		//CaptureCurrentScreenToTexture(vrGLHUDTexture);
		isInMenu = true;
	}

	gVRHelper.Update();
}

void VRRenderer::CalcRefdef(struct ref_params_s* pparams)
{
	if (pparams->nextView == 0)
	{
		gVRHelper.UpdatePositions();
		gVRHelper.PrepareVRScene(VREye::Left, pparams);
		gVRHelper.GetViewOrg(pparams->vieworg);
		gVRHelper.GetViewAngles(pparams->viewangles);

		pparams->nextView = 1;
	}
	else if (pparams->nextView == 1)
	{
		gVRHelper.FinishVRScene(pparams);
		gVRHelper.PrepareVRScene(VREye::Right, pparams);
		gVRHelper.GetViewOrg(pparams->vieworg);
		gVRHelper.GetViewAngles(pparams->viewangles);

		pparams->nextView = 2;
	}
	else
	{
		gVRHelper.FinishVRScene(pparams);
		gVRHelper.SubmitImages();

		pparams->nextView = 0;
		//pparams->onlyClientDraw = 1;
	}

	// Update player viewangles from HMD pose
	gEngfuncs.SetViewAngles(pparams->viewangles);
}

void VRRenderer::DrawNormal()
{
	glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);

	if (vr_renderControllerAxis->value == 1.0f)
	{
		gVRHelper.TestRenderControllerPosition(true);
		gVRHelper.TestRenderControllerPosition(false);
	}

	//cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	//localPlayer->origin

	RenderWorldBackfaces();

	glPopAttrib();
}

void VRRenderer::DrawTransparent()
{
}

void VRRenderer::InterceptHUDRedraw(float time, int intermission)
{
	isInMenu = false;
	//glClearColor(0, 0, 0, 0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gHUD.Redraw(time, intermission);
	// TODO: Capture HUD texture
}

void VRRenderer::GetViewAngles(float * angles)
{
	gVRHelper.GetViewAngles(angles);
}

void VRRenderer::GetWalkAngles(float * angles)
{
	gVRHelper.GetWalkAngles(angles);
}

// This method just draws the backfaces of the entire map in black, so the player can't peak "through" walls with their VR headset
void VRRenderer::RenderWorldBackfaces()
{
	if (vr_renderWorldBackface->value == 0.0f)
	{
		return;
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glBindTexture(GL_TEXTURE_2D, 0);
	//glColor4f(1.f, 0.08f, 1.f, 0.58f);
	glColor4f(0, 0, 0, 1);

	cl_entity_s* map = gEngfuncs.GetEntityByIndex(0);
	if (map != nullptr && map->model != nullptr)
	{
		RenderBSPBackfaces(map->model);

		int currentmessagenum = gEngfuncs.GetLocalPlayer()->curstate.messagenum;

		for (int i = 1; i < gpGlobals->maxEntities; i++)
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
			if (ent != nullptr && ent->model != nullptr && ent->model->name != nullptr && ent->model->name[0] == '*' && ent->curstate.messagenum == currentmessagenum)
			{
				glPushMatrix();
				glTranslatef(ent->curstate.origin.x, ent->curstate.origin.y, ent->curstate.origin.z);
				glRotatef(ent->curstate.angles.x, 1, 0, 0);
				glRotatef(ent->curstate.angles.y, 0, 0, 1);
				glRotatef(ent->curstate.angles.z, 0, 1, 0);
				RenderBSPBackfaces(ent->model);
				glPopMatrix();
			}
		}
	}
}

void VRRenderer::RenderBSPBackfaces(model_t* model)
{
	if (vr_renderWorldBackface->value == 0.0f)
	{
		return;
	}

	for (int i = 0; i < model->nummodelsurfaces; i++)
	{
		int surfaceIndex = model->firstmodelsurface + i;
		msurface_t *surface = &model->surfaces[surfaceIndex];
		glBegin(GL_POLYGON);
		for (int k = surface->polys->numverts - 1; k >= 0; k--)
		{
			glVertex3fv(surface->polys->verts[k]);
		}
		glEnd();
	}
}
