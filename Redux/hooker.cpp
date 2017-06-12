#include "hooker.hpp"
#include <d3d9.h>
#include <d3dx9.h>
#include "DrawManager.h"

static BOOL(WINAPI* gpGetCursorPos)(LPPOINT lpPoint);
extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool vecPressedKeys[256] = {};
bool init = false;
std::unique_ptr<DrawManager> g_pRenderer = nullptr;
char CURFRAME[15]="";
int FRAMEINDEX=0;

void initHook()
{
	printf("DX Version: %d\n", D3D_SDK_VERSION);

	printf("Init hooks\n");
	Client      = InterfaceManager::BruteForce<SourceEngine::IBaseClientDLL>    ("client.dll",         "VClient");
	Engine      = InterfaceManager::BruteForce<SourceEngine::IVEngineClient>    ("engine.dll",         "VEngineClient");
	Panel       = InterfaceManager::BruteForce<SourceEngine::IPanel>            ("vgui2.dll",          "VGUI_Panel"); // WORKS
	Surface     = InterfaceManager::BruteForce<SourceEngine::ISurface>          ("vguimatsurface.dll", "VGUI_Surface"); // WORKS
	EntityList  = InterfaceManager::BruteForce<SourceEngine::IClientEntityList> ("client.dll",         "VClientEntityList");
	EngineTrace = InterfaceManager::BruteForce<SourceEngine::IEngineTrace>      ("engine.dll",         "EngineTraceClient");
	InputSystem = InterfaceManager::BruteForce<IInputSystem>                    ("inputsystem.dll",    "InputSystemVersion");

	NetvarManager::Instance()->CreateDatabase();
	NetvarManager::Instance()->Dump("C:\\Users\\Lumaio\\Desktop\\Projects\\netvars.txt");
	
	void** pct = *reinterpret_cast<void***>(Client);
	ClientMode = **reinterpret_cast<SourceEngine::IClientMode***>(reinterpret_cast<DWORD>(pct[10])+5);
	
	PaintTraverseHook_p = std::make_unique<Hooks::VFTableHook>((Hooks::PPDWORD)Panel, true);
	PaintTraverse_o = PaintTraverseHook_p->Hook(41, (PaintTraverse_t)hook_painttraverse);
	
	CreateMoveHook_p = std::make_unique<Hooks::VFTableHook>((Hooks::PPDWORD)ClientMode, true);
	CreateMove_o = CreateMoveHook_p->Hook(24, (CreateMove_t)hook_createmove);

	FrameStageNotify_p = std::make_unique<Hooks::VFTableHook>((Hooks::PPDWORD)Client, true);
	FrameStageNotify_o = FrameStageNotify_p->Hook(36, (FrameStageNotify_t)hook_framestagenotify);

	auto dwDevice = **(uint32_t**)(Utils::FindSignature(XorStr("shaderapidx9.dll"), XorStr("A1 ? ? ? ? 50 8B 08 FF 51 0C")) + 1);
	EndScene_p = std::make_unique<Hooks::VFTableHook>((Hooks::PPDWORD)dwDevice, true);
	EndScene_o = EndScene_p->Hook(42, (EndScene_t)hook_endscene);

	Reset_p = std::make_unique<Hooks::VFTableHook>((Hooks::PPDWORD)dwDevice, true);
	Reset_o = Reset_p->Hook(16, (Reset_t)hook_reset);

	/// Setup GUI and other hooks
	while (!(Window_o = FindWindowA(XorStr("Valve001"), NULL))) Sleep(200);
	if (Window_o)
		WndProc_o = (WNDPROC)SetWindowLongPtr(Window_o, GWLP_WNDPROC, (LONG_PTR)hook_wndproc);

	font = Surface->CreateFont();
	Surface->SetFontGlyphSet(font, "Consolas", 16, 400, 0, 0, (int)FontFlags_t::FONTFLAG_OUTLINE, 0, 0);

}

void unloadHook()
{
	SetWindowLongPtr(Window_o, GWLP_WNDPROC, (LONG_PTR)WndProc_o);

	g_pRenderer->InvalidateObjects();

	PaintTraverseHook_p->Unhook(41);
	CreateMoveHook_p->Unhook(24);
	FrameStageNotify_p->Unhook(36);
	EndScene_p->Unhook(42);
	Reset_p->Unhook(16);

	/*
	CreateMoveHook_p->RestoreTable();
	EndScene_p->RestoreTable();
	Reset_p->RestoreTable();
	PaintTraverseHook_p->RestoreTable();
	FrameStageNotify_p->RestoreTable();
	*/

	printf("Uninjected hack (safe to close)\n");
}

void DText(const wchar_t* text, int x, int y)
{
	//Surface->DrawSetTextColor(color);
	Surface->DrawSetTextFont(font);
	Surface->DrawSetTextPos(x, y);
	Surface->DrawPrintText(text, wcslen(text));
}
void DrawRect(int x, int y, int w, int h) {
	Surface->DrawOutlinedRect(x, y, x + w, y + h);
}
void DrawFillRect(int x, int y, int w, int h) {
	Surface->DrawFilledRect(x, y, x + w, y + h);
}
void DrawMouse()
{

	Surface->DrawSetColor(255, 255, 255, 255);
	for (int i = 0; i <= 9; i++)
	{
		Surface->DrawLine(Cur.x, Cur.y, Cur.x + i, Cur.y + 11);
	}
	for (int i = 0; i <= 7; i++)
	{
		Surface->DrawLine(Cur.x, Cur.y + 9 + i, Cur.x + i, Cur.y + 9);
	}
	for (int i = 0; i <= 3; i++)
	{
		Surface->DrawLine(Cur.x + 6 + i, Cur.y + 11, Cur.x, Cur.y + i);
	}
	Surface->DrawLine(Cur.x + 5, Cur.y + 11, Cur.x + 8, Cur.y + 18);
	Surface->DrawLine(Cur.x + 4, Cur.y + 11, Cur.x + 7, Cur.y + 18);

	Surface->DrawSetColor(0, 0, 0, 255);
	Surface->DrawLine(Cur.x, Cur.y, Cur.x, Cur.y + 17);
	Surface->DrawLine(Cur.x, Cur.y + 17, Cur.x + 3, Cur.y + 14);

	Surface->DrawLine(Cur.x + 4, Cur.y + 14, Cur.x + 7, Cur.y + 19);
	Surface->DrawLine(Cur.x + 5, Cur.y + 14, Cur.x + 7, Cur.y + 19);

	Surface->DrawLine(Cur.x + 7, Cur.y + 18, Cur.x + 9, Cur.y + 18);

	Surface->DrawLine(Cur.x + 10, Cur.y + 18, Cur.x + 7, Cur.y + 12);

	Surface->DrawLine(Cur.x + 7, Cur.y + 12, Cur.x + 12, Cur.y + 12);

	Surface->DrawLine(Cur.x + 12, Cur.y + 12, Cur.x, Cur.y);
}
bool IsButtonPressed(ButtonCode_t code)
{
	static int buttonPressedTick = 0;

	if (InputSystem->IsButtonDown(code) && (GetTickCount64() - buttonPressedTick) > 100)
	{
		buttonPressedTick = GetTickCount64();
		return true;
	}
	return false;
}

static float time=0;
bool __stdcall hook_createmove(float dt, SourceEngine::CUserCmd* pcmd)
{
	time+=dt;
	bool orig = CreateMove_o(ClientMode, dt, pcmd);

	static auto SendClanTag = reinterpret_cast<SendClanTag_t>(Utils::FindSignature(XorStr("engine.dll"), XorStr("53 56 57 8B DA 8B F9 FF 15")));

	if (Settings::ANIMATED && Settings::FRAMES.size()>0)
	{
		if (time > 0.5f) {
			time = 0;
			if (FRAMEINDEX+1 > Settings::FRAMES.size()-1)
				FRAMEINDEX=0;
			else
				FRAMEINDEX++;
			SendClanTag(Settings::FRAMES[FRAMEINDEX].c_str(),"");
		}
	}

	if (pcmd&&pcmd->command_number&&strcmp(Settings::CLANTAG, "")!=0&&!Settings::ANIMATED) { SendClanTag(Settings::CLANTAG,""); }

	// mouse events

	QAngle oldangle;// = pcmd->viewangles;
	Engine->GetViewAngles(oldangle);
	float oldfor = pcmd->forwardmove;
	float oldside= pcmd->sidemove;

	radar();
	bhop(pcmd);
	bool ant = antiaim(pcmd);
	bool aim = aimbot(pcmd);

	CorrectMovement(oldangle, pcmd, oldfor, oldside);
	if (aim || ant) { // Aimbot not enabled
		return false;
	}
	else return false;
}

static int menu_index = 0;

static unsigned int top_panel = 0;
void __stdcall hook_painttraverse(unsigned int panel, bool repaint, bool force)
{
	//NetvarManager::Instance()->Dump("C:\\Users\\Lumaio\\Desktop\\Projects\\netvars.txt");
	PaintTraverse_o(Panel, panel, repaint, force);
	if (!top_panel) {
		if (!strcmp(Panel->GetName(panel), "MatSystemTopPanel")) { top_panel = panel; }
		return;
	}
	if (panel != top_panel) return;

	int cx = 0, cy = 0;
	Surface->SurfaceGetCursorPos(cx, cy);
	Cur.x = cx;
	Cur.y = cy;

	// info shit
	Surface->DrawSetTextColor(255, 255, 255, 255);
	if (Settings::WATERMARK)
		DText(L"Redux for CS:GO { 0xA }", 0, 0);

	if (!Engine->IsInGame()) return;
	esp();

}

void __fastcall hook_framestagenotify(void* ecx, void* edx, SourceEngine::ClientFrameStage_t stage)
{
	if (!Engine->IsInGame()) return;
	
	// skin changer
	while (stage == SourceEngine::ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_START)
	{
		auto local = C_CSPlayer::GetLocalPlayer();
		if (!local || !local->IsAlive()) break;

		UINT* weapons = local->GetWeapons();
		if (!weapons) break;

		for (int i = 0; weapons[i]; i++) {
			// model
			CBaseAttributeItem* pweapon = (CBaseAttributeItem*)EntityList->GetClientEntityFromHandle(weapons[i]);
			if (!pweapon) continue;

			int windex =  *pweapon->GetItemDefinitionIndex();
			apply_model(local, pweapon, windex);

			// skin
			apply_skin(weapons[i]);
		}
		break;
	}

	// no visible recoil (shamelessly stolen from unknowncheats) (i fixed it tho, shit was broke)
	static QAngle qAimPunch, qViewPunch;
	static QAngle *qpAimPunch = nullptr, *qpViewPunch = nullptr;
	if (Settings::NOVISRECOIL && Engine->IsInGame())
	{
		if (stage == SourceEngine::ClientFrameStage_t::FRAME_RENDER_START)
		{
			auto local = C_CSPlayer::GetLocalPlayer();
			if (local && local->IsAlive())
			{
				qpAimPunch = (QAngle*)local->AimPunch();//MakePtr(QAngle *, pLocal, OFFSET_VIEWPUNCH);
				qpViewPunch = (QAngle*)local->ViewPunch();//MakePtr(QAngle *, pLocal, OFFSET_AIMPUNCH);

				if (qpAimPunch && qpViewPunch) {
					qAimPunch = *qpAimPunch;
					qViewPunch = *qpViewPunch;

					qpAimPunch->Init(0.f, 0.f, 0.f);
					qpViewPunch->Init(0.f, 0.f, 0.f);
				}
			}
		}
	}

	if (Settings::NOFLASH) *C_CSPlayer::GetLocalPlayer()->GetMaxAlpha() = 10;
	else *C_CSPlayer::GetLocalPlayer()->GetMaxAlpha() = 255;

	FrameStageNotify_o(ecx, stage);

	if (Settings::NOVISRECOIL && stage == SourceEngine::ClientFrameStage_t::FRAME_RENDER_START) {
		if (qpAimPunch && qpViewPunch) {
			//(*qpAimPunch).Init(qAimPunch.x, qAimPunch.y, qAimPunch.z);
			*qpAimPunch = qAimPunch;
			*qpViewPunch = qViewPunch;
		}
	}
}

static int selected=0;
HRESULT __stdcall hook_endscene(IDirect3DDevice9* pDevice)
{
	static SourceEngine::ConVar* convar;
	if (!init) {
		ImGui_ImplDX9_Init(Window_o, pDevice);
		g_pRenderer = std::make_unique<DrawManager>(pDevice);
		g_pRenderer->CreateObjects();
		init = true;
		convar = SourceEngine::Interfaces::CVar()->FindVar("cl_mouseenable");

		// THEEEEEME
		ImGuiStyle * style = &ImGui::GetStyle();

		style->WindowPadding = ImVec2(5, 5);
		style->WindowRounding = 0.f;
		style->FramePadding = ImVec2(1, 1);
		style->FrameRounding = 0.0f;
		style->ItemSpacing = ImVec2(8, 2);
		style->ItemInnerSpacing = ImVec2(8, 8);
		style->IndentSpacing = 25.0f;
		style->ScrollbarSize = 5.0f;
		style->ScrollbarRounding = 0.0f;
		style->GrabMinSize = 5.0f;
		style->GrabRounding = 3.0f;
		style->AntiAliasedLines = false;
		style->AntiAliasedShapes = false;

		style->Colors[ImGuiCol_Text]                 = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		style->Colors[ImGuiCol_TextDisabled]         = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_WindowBg]             = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_ChildWindowBg]        = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style->Colors[ImGuiCol_PopupBg]              = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style->Colors[ImGuiCol_Border]               = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		style->Colors[ImGuiCol_BorderShadow]         = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		style->Colors[ImGuiCol_FrameBg]              = ImVec4(0.20f, 0.19f, 0.22f, 1.00f);
		style->Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_TitleBg]              = ImVec4(0.20f, 0.19f, 0.22f, 1.00f);
		style->Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		style->Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style->Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.20f, 0.19f, 0.22f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.20f, 0.19f, 0.22f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_ComboBg]              = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
		style->Colors[ImGuiCol_CheckMark]            = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_SliderGrab]           = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_Button]               = ImVec4(0.20f, 0.19f, 0.22f, 1.00f);
		style->Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_ButtonActive]         = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.19f, 0.22f, 1.00f);
		style->Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_HeaderActive]         = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_Column]               = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ColumnHovered]        = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_ColumnActive]         = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ResizeGrip]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style->Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_CloseButton]          = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
		style->Colors[ImGuiCol_CloseButtonHovered]   = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
		style->Colors[ImGuiCol_CloseButtonActive]    = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
		style->Colors[ImGuiCol_PlotLines]            = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

		ImGui::GetIO().Fonts->AddFontFromFileTTF("slkscrb.ttf", 10);
	}
	else
	{
		ImGui::GetIO().MouseDrawCursor = menu_enabled;

		ImGui_ImplDX9_NewFrame();

		if (menu_enabled)
		{
			ImGui::Begin("Redux for CSGO", &menu_enabled, ImVec2(250, 500), 0.75f);
			{
				ImGui::Text("%f", time);
				ImGui::Checkbox("Watermark", &Settings::WATERMARK);
				ImGui::Separator();
				ImGui::Text(""\
							"		####           \n"\
							"       ####           \n"\
							"     #############    \n"\
							"    ;##########       \n"\
							"    ##########'       \n"\
							"   ###########        \n"\
							"   ######## ;         \n"\
							"   ;#######           \n"\
							"   ########           \n"\
							"   `######            \n"\
							"   #######`           \n"\
							"   ########           \n"\
							"   ###  ####          \n"\
							"   ###   ###          \n"\
							"  ,##     ##          \n"\
							"  ##      ##          \n"\
							" :##      ##          \n"\
							" :#       ##          \n"\
							" ##       ##.         \n"\
							" ##       ,'##        ");
				ImGui::Separator();
				if (ImGui::CollapsingHeader("Visuals")) {
					ImGui::Checkbox("Boxes", &Settings::ESP_ENABLED);
					ImGui::Checkbox("Bones", &Settings::BONEESP);
					ImGui::Checkbox("Crosshair", &Settings::CROSSHAIR_ENABLED);
					ImGui::Checkbox("No Flash", &Settings::NOFLASH);
				}
				if (ImGui::CollapsingHeader("Aimbot")) {
					ImGui::Checkbox("Enabled", &Settings::AIMBOT_ENABLED);
					ImGui::Checkbox("Anti Aim", &Settings::ANTIAIM);
				}
				if (ImGui::CollapsingHeader("Misc")) {
					ImGui::Checkbox("Bunnyhop", &Settings::BHOP_ENABLED);
					ImGui::Checkbox("No Visual Recoil", &Settings::NOVISRECOIL);
					ImGui::Checkbox("CS:CO Compatibility", &Settings::CSCOCOMPAT);
					ImGui::InputText("Clan Tag", Settings::CLANTAG, 15);
					ImGui::Checkbox("Animated", &Settings::ANIMATED);
				}
				ImGui::Separator();
				if (ImGui::CollapsingHeader("ESP Colors")) {
					ImGui::ColorEdit3("Team Color",  Settings::ESP_TEAM_COLOR );
					ImGui::ColorEdit3("Enemy Color", Settings::ESP_ENEMY_COLOR);
					ImGui::ColorEdit3("Bones Color", Settings::ESP_BONES_COLOR);
				}
				if (ImGui::CollapsingHeader("Crosshair Settings")) {
					ImGui::Combo("Style", &Settings::STYLE, HAIRSTYLE, IM_ARRAYSIZE(HAIRSTYLE));
					ImGui::SliderInt("Size", &Settings::HAIRSIZE, 0, 100);
					ImGui::ColorEdit4("Color", Settings::HAIRCOLOR, true);
				}
			}
			ImGui::End();

			if (Settings::ANIMATED)
			{
				ImGui::SetNextWindowPos(ImVec2(680, 300), ImGuiSetCond_Once);
				ImGui::Begin("Animation", &Settings::ANIMATED, ImVec2(340, 280), 0.75f);
				{

					ImGui::InputText("Frame", CURFRAME, 15);
					ImGui::SameLine();
					if (ImGui::Button("Add Frame")) { Settings::FRAMES.push_back(std::string(CURFRAME)); sprintf(CURFRAME, ""); }
					ImGui::Separator();
					if (ImGui::CollapsingHeader("Frames")) {
						std::string text;
						for (int i = 0; i < Settings::FRAMES.size(); i++) {
							text+=Settings::FRAMES[i];
							text+="\n";
						}
						ImGui::Text(text.c_str());
					}

				}
				ImGui::End();
			}
		}

		g_pRenderer->BeginRendering();
		ImGui::Render();
		g_pRenderer->EndRendering();

	}

	return EndScene_o(pDevice);
}

HRESULT __stdcall hook_reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	//Correctly handling Reset calls is very important if you have a DirectX hook.
	//IDirect3DDevice9::Reset is called when you minimize the game, Alt-Tab or change resolutions.
	//When it is called, the IDirect3DDevice9 is placed on "lost" state and many related resources are released
	//This means that we need to recreate our own resources when that happens. If we dont, we crash.

	//GUI wasnt initialized yet, just call Reset and return
	if(!init) return Reset_o(pDevice, pPresentationParameters);

	//Device is on LOST state.

	ImGui_ImplDX9_InvalidateDeviceObjects(); //Invalidate GUI resources
	g_pRenderer->InvalidateObjects();

	//Call original Reset.
	auto hr = Reset_o(pDevice, pPresentationParameters);

	g_pRenderer->CreateObjects();
	ImGui_ImplDX9_CreateDeviceObjects(); //Recreate GUI resources
	return hr;
}

LRESULT __stdcall hook_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//Captures the keys states
	switch(uMsg) {
	case WM_LBUTTONDOWN:
		vecPressedKeys[VK_LBUTTON] = true;
		break;
	case WM_LBUTTONUP:
		vecPressedKeys[VK_LBUTTON] = false;
		break;
	case WM_RBUTTONDOWN:
		vecPressedKeys[VK_RBUTTON] = true;
		break;
	case WM_RBUTTONUP:
		vecPressedKeys[VK_RBUTTON] = false;
		break;
	case WM_KEYDOWN:
		vecPressedKeys[wParam] = true;
		break;
	case WM_KEYUP:
		vecPressedKeys[wParam] = false;
		break;
	default: break;
	}

	//Insert toggles the menu
	{
		//Maybe there is a better way to do this? Not sure
		//We want to toggle when the key is pressed (i.e when it receives a down and then up signal)
		static bool isDown = false;
		static bool isClicked = false;
		if(vecPressedKeys[VK_INSERT]) {
			isClicked = false;
			isDown = true;
		} else if(!vecPressedKeys[VK_INSERT] && isDown) {
			isClicked = true;
			isDown = false;
		} else {
			isClicked = false;
			isDown = false;
		}

		if(isClicked) {
			menu_enabled = !menu_enabled;

			//Use cl_mouseenable convar to disable the mouse when the window is open 
			static auto cl_mouseenable = SourceEngine::Interfaces::CVar()->FindVar(XorStr("cl_mouseenable"));
			cl_mouseenable->SetValue(!menu_enabled);
		}
	}

	//Processes the user input using ImGui_ImplDX9_WndProcHandler
	if(init && menu_enabled && ImGui_ImplDX9_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true; //Input was consumed, return

					 //Input was not consumed by the GUI, call original WindowProc to pass the input to the game
	return CallWindowProc(WndProc_o, hWnd, uMsg, wParam, lParam);
}

bool WINAPI hook_getcursorpos(LPPOINT lpPoint)
{
	if (menu_enabled) return FALSE;

}


/* Hack functions sadcjonsadijc;basdjc;obasndc;asdc */


void bhop(SourceEngine::CUserCmd* pcmd)
{
	if (!Settings::BHOP_ENABLED) return;
	if (pcmd->command_number == 0) return;

	auto local_player = C_CSPlayer::GetLocalPlayer();
	if (!local_player) return;

	if ((pcmd->buttons & IN_JUMP) && !(local_player->GetFlags() & (int)SourceEngine::EntityFlags::FL_ONGROUND)) {
		pcmd->buttons &= ~IN_JUMP;
	}
}


bool aimbot(SourceEngine::CUserCmd* pcmd)
{

	using SourceEngine::Vector;
	/*  AIMBOT  */
	for (int i = 1; i < Engine->GetMaxClients(); i++)
	{
		if (i == Engine->GetLocalPlayer()) continue;
		auto entity = static_cast<C_CSPlayer*>(EntityList->GetClientEntity(i));
		if (!entity) continue;
		if (!entity->IsAlive() || entity->IsDormant()) continue;

		if (entity->GetClientClass()->m_ClassID == SourceEngine::EClassIds::CCSPlayer)
		{
			auto local_player = C_CSPlayer::GetLocalPlayer();
			if (!local_player) continue;
			if (local_player->GetTeamNum() == entity->GetTeamNum()) continue;

			int iScreenWidth, iScreenHeight;
			Engine->GetScreenSize(iScreenWidth, iScreenHeight);

			int head = 8;
			if (Settings::CSCOCOMPAT) head=6;
			Vector start, end;
			end = Utils::GetEntityBone(entity, (SourceEngine::ECSPlayerBones)head);
			start = local_player->GetEyePos();

			Vector screen;
			bool in_fov = false;
			if (Utils::WorldToScreen(end, screen)) {
				in_fov = powf((screen.x - iScreenWidth / 2), 2) + powf((screen.y - iScreenHeight / 2), 2) < pow(FOV_SLIDE, 2);
			}


			SourceEngine::Ray_t ray;
			SourceEngine::trace_t trace;
			ray.Init(start, end);
			SourceEngine::CTraceFilter filter;
			filter.pSkip = local_player;
			EngineTrace->TraceRay(ray, MASK_SHOT, &filter, &trace);

			if (trace.m_pEnt == entity && in_fov) {
				if (Settings::AIMBOT_ENABLED) {
					Vector dir = end - start;
					Vector new_angles;

					dir = dir.Normalized();
					Utils::VectorAngles(dir, new_angles);

					Vector punch = *local_player->AimPunch() * 2;
					new_angles -= punch;
					//new_angles.MulSub(new_angles, punch, 2.f);

					Utils::Clamp(new_angles);

					if (pcmd->buttons & IN_ATTACK)
						pcmd->viewangles = new_angles;
					return false;
				}
			}
			else {
				continue;
			}
		}
	}
}

static int state=0;
bool antiaim(SourceEngine::CUserCmd* pcmd)
{
	if (Settings::ANTIAIM && !(pcmd->buttons & IN_ATTACK) && !(pcmd->buttons & IN_USE)) {
		Vector angle;
		angle.y=45*(state%8); // yaw
		angle.x=89; // pitch
		angle.z=0; // roll
		if (Utils::Clamp(angle))
			pcmd->viewangles=angle;
		state++;
		return false;
	}
}


void esp()
{
	int iScreenWidth, iScreenHeight;
	Engine->GetScreenSize(iScreenWidth, iScreenHeight);

	if (Settings::AIMBOT_ENABLED) { // FOV
		Surface->DrawSetColor(SourceEngine::Color(255, 255, 0, 255));
		Surface->DrawOutlinedCircle(iScreenWidth / 2, iScreenHeight / 2, FOV_SLIDE, 100);
	}
	if (Settings::CROSSHAIR_ENABLED) {
		switch (Settings::STYLE) {
		case 0: // Plus
			Surface->DrawSetColor(SourceEngine::Color(Settings::HAIRCOLOR[0]*255, Settings::HAIRCOLOR[1]*255, Settings::HAIRCOLOR[2]*255, Settings::HAIRCOLOR[3]*255));
			Surface->DrawLine(iScreenWidth / 2 - Settings::HAIRSIZE, iScreenHeight / 2, iScreenWidth / 2 + Settings::HAIRSIZE, iScreenHeight / 2);
			Surface->DrawLine(iScreenWidth / 2, iScreenHeight / 2 - Settings::HAIRSIZE, iScreenWidth / 2, iScreenHeight / 2 + Settings::HAIRSIZE);
			break;
		case 1: // Cross
			Surface->DrawSetColor(SourceEngine::Color(Settings::HAIRCOLOR[0]*255, Settings::HAIRCOLOR[1]*255, Settings::HAIRCOLOR[2]*255, Settings::HAIRCOLOR[3]*255));
			Surface->DrawLine(iScreenWidth / 2 - Settings::HAIRSIZE, iScreenHeight / 2 - Settings::HAIRSIZE, iScreenWidth / 2 + Settings::HAIRSIZE, iScreenHeight / 2 + Settings::HAIRSIZE);
			Surface->DrawLine(iScreenWidth / 2 + Settings::HAIRSIZE, iScreenHeight / 2 - Settings::HAIRSIZE, iScreenWidth / 2 - Settings::HAIRSIZE, iScreenHeight / 2 + Settings::HAIRSIZE);
			break;
		case 2: // Circle
			Surface->DrawSetColor(SourceEngine::Color(Settings::HAIRCOLOR[0]*255, Settings::HAIRCOLOR[1]*255, Settings::HAIRCOLOR[2]*255, Settings::HAIRCOLOR[3]*255));
			Surface->DrawOutlinedCircle(iScreenWidth / 2, iScreenHeight / 2, Settings::HAIRSIZE, 100);
			break;
		case 3: // Square
			Surface->DrawSetColor(SourceEngine::Color(Settings::HAIRCOLOR[0]*255, Settings::HAIRCOLOR[1]*255, Settings::HAIRCOLOR[2]*255, Settings::HAIRCOLOR[3]*255));
			DrawRect(iScreenWidth / 2 - Settings::HAIRSIZE / 2, iScreenHeight / 2 - Settings::HAIRSIZE / 2, Settings::HAIRSIZE, Settings::HAIRSIZE);
			break;
		case 4: // DotRound
			Surface->DrawSetColor(SourceEngine::Color(Settings::HAIRCOLOR[0]*255, Settings::HAIRCOLOR[1]*255, Settings::HAIRCOLOR[2]*255, Settings::HAIRCOLOR[3]*255));
			Surface->DrawLine(iScreenWidth / 2 - FOV_SLIDE, iScreenHeight / 2, iScreenWidth / 2 + FOV_SLIDE, iScreenHeight / 2);
			Surface->DrawLine(iScreenWidth / 2, iScreenHeight / 2 - FOV_SLIDE, iScreenWidth / 2, iScreenHeight / 2 + FOV_SLIDE);
			break;

		}
	}
	
	for (int i = 1; i < Engine->GetMaxClients(); i++)
	{
		if (i == Engine->GetLocalPlayer()) continue;

		auto entity = static_cast<C_CSPlayer*>(EntityList->GetClientEntity(i));
		if (!entity) continue;
		if (!entity->IsAlive() || entity->IsDormant()) continue;

		if (entity->GetClientClass()->m_ClassID == SourceEngine::EClassIds::CCSPlayer)
		{

			// BONE ESP
			/*
			pelvis = 2,
			spine_0, // 3
			spine_1, // 4
			spine_2, // 5
			spine_3, // 6
			neck_0, // 7
			head_0, // 8
			clavicle_L, // 9
			arm_upper_L, // 10
			arm_lower_L, // 11
			hand_L, // 12
			arm_upper_R, // 38
			arm_lower_R, // 39
			hand_R, // 40
			*/

			Surface->DrawSetColor(Settings::ESP_BONES_COLOR[0]*255, Settings::ESP_BONES_COLOR[1]*255, Settings::ESP_BONES_COLOR[2]*255, Settings::ESP_BONES_COLOR[3]*255);

			const studiohdr_t* studio = SourceEngine::Interfaces::ModelInfo()->FindModel(entity->GetModel());
			if (studio && Settings::BONEESP)
			{
				static SourceEngine::matrix3x4_t bone2world[128];
				if (entity->SetupBones(bone2world, 128, 256, SourceEngine::Interfaces::Engine()->GetLastTimeStamp()))
				{
					for (int i = 0; i < studio->numbones; i++)
					{
						mstudiobone_t* bone = studio->GetBone(i);
						if (!bone || !(bone->flags&256) || bone->parent==-1)
							continue;

						Vector bone1;
						if (!Utils::WorldToScreen(Vector(bone2world[i][0][3], bone2world[i][1][3],bone2world[i][2][3]), bone1))
							continue;
						Vector bone2;
						if (!Utils::WorldToScreen(Vector(bone2world[bone->parent][0][3], bone2world[bone->parent][1][3],bone2world[bone->parent][2][3]), bone2))
							continue;
						
						Surface->DrawLine(bone1.x,bone1.y,bone2.x,bone2.y);
					}
				}
			}


			// BOX ESP
			if (!Settings::ESP_ENABLED) continue;
			auto local_player = C_CSPlayer::GetLocalPlayer();
			if (!local_player) continue;

			bool is_enemy = local_player->GetTeamNum() != entity->GetTeamNum();

			Surface->DrawSetColor(is_enemy ? 
								  SourceEngine::Color(Settings::ESP_ENEMY_COLOR[0]*255, Settings::ESP_ENEMY_COLOR[1]*255, Settings::ESP_ENEMY_COLOR[2]*255, Settings::ESP_ENEMY_COLOR[3]*255) 
								  : 
								  SourceEngine::Color(Settings::ESP_TEAM_COLOR[0]*255, Settings::ESP_TEAM_COLOR[1]*255, Settings::ESP_TEAM_COLOR[2]*255, Settings::ESP_TEAM_COLOR[3]*255));

			Surface->DrawSetTextColor(is_enemy ? 
									  SourceEngine::Color(Settings::ESP_ENEMY_COLOR[0]*255, Settings::ESP_ENEMY_COLOR[1]*255, Settings::ESP_ENEMY_COLOR[2]*255, Settings::ESP_ENEMY_COLOR[3]*255) 
									  : 
									  SourceEngine::Color(Settings::ESP_TEAM_COLOR[0]*255, Settings::ESP_TEAM_COLOR[1]*255, Settings::ESP_TEAM_COLOR[2]*255, Settings::ESP_TEAM_COLOR[3]*255));

			auto origin = entity->GetOrigin();
			int _head = 8;
			if (Settings::CSCOCOMPAT) _head=6;
			auto head = Utils::GetEntityBone(entity, (SourceEngine::ECSPlayerBones)_head);
			head.z += 15.f;

			SourceEngine::Vector screen_origin, screen_head;
			if (Utils::WorldToScreen(head, screen_head) && Utils::WorldToScreen(origin, screen_origin)) {
				float height = abs(screen_head.y - screen_origin.y);
				float width = height*0.65f;

				Surface->DrawOutlinedRect(screen_origin.x - width / 2, screen_origin.y - height, (screen_origin.x - width / 2) + width, screen_origin.y);
				head.z -= 15.f;

				Utils::WorldToScreen(head, screen_head);
				Surface->DrawOutlinedCircle(screen_head.x, screen_head.y, height*0.1, 10);

				bool in_fov = false;
				in_fov = powf((screen_head.x - iScreenWidth / 2), 2) + powf((screen_head.y - iScreenHeight / 2), 2) < pow(FOV_SLIDE, 2);

				if (in_fov && Settings::AIMBOT_ENABLED) {
					Surface->DrawOutlinedCircle(screen_head.x, screen_head.y, 5, 10);
					Surface->DrawOutlinedCircle(screen_head.x, screen_head.y, 4, 10);
					Surface->DrawOutlinedCircle(screen_head.x, screen_head.y, 3, 10);
					Surface->DrawOutlinedCircle(screen_head.x, screen_head.y, 2, 10);
					Surface->DrawOutlinedCircle(screen_head.x, screen_head.y, 1, 10);
					Surface->DrawLine(iScreenWidth / 2, iScreenHeight / 2, screen_head.x, screen_head.y);
				}
				// name
				SourceEngine::player_info_t player_info;
				Engine->GetPlayerInfo(i, &player_info);
				std::string pname(player_info.szName);
				DText(std::wstring(pname.begin(), pname.end()).c_str(), screen_origin.x - width / 2, screen_origin.y);

				// health
				std::string hp;
				hp += "HP: ";
				hp += std::to_string(entity->GetHealth());
				DText(std::wstring(hp.begin(), hp.end()).c_str(), screen_origin.x - width / 2, screen_origin.y - height - 16);
			}
		}
	}
}

void radar()
{
	if (!Settings::RADAR_ENABLED) return;
	for (int i = 1; i < Engine->GetMaxClients(); i++)
	{
		auto entity = static_cast<C_CSPlayer*>(EntityList->GetClientEntity(i));
		if (!entity) continue;
		if (!entity->IsAlive() || entity->IsDormant()) continue;
	}
}

void CorrectMovement(QAngle vOldAngles, SourceEngine::CUserCmd* pCmd, float fOldForward, float fOldSidemove)
{
	//side/forward move correction
	float deltaView = pCmd->viewangles.y - vOldAngles.y;
	float f1;
	float f2;
	if (vOldAngles.y < 0.f)
		f1 = 360.0f + vOldAngles.y;
	else
		f1 = vOldAngles.y;
	if (pCmd->viewangles.y < 0.0f)
		f2 = 360.0f + pCmd->viewangles.y;
	else
		f2 = pCmd->viewangles.y;
	if (f2 < f1)
		deltaView = abs(f2 - f1);
	else
		deltaView = 360.0f - abs(f1 - f2);
	deltaView = 360.0f - deltaView;
	pCmd->forwardmove = cos(DEG2RAD(deltaView)) * fOldForward + cos(DEG2RAD(deltaView + 90.f)) * fOldSidemove;
	pCmd->sidemove = sin(DEG2RAD(deltaView)) * fOldForward + sin(DEG2RAD(deltaView + 90.f)) * fOldSidemove;
}

