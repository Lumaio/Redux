#ifndef HOOKER_H
#define HOOKER_H

#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <map>
#include <memory>
#include <functional>
#include <d3dx9.h>
#include "SDK\InterfaceManager\InterfaceManager.hpp"
#include "SDK\SourceEngine\SDK.hpp"
#include "SDK\SourceEngine\IInputSystem.h"
#include "SDK\CSGOStructs.hpp"
#include "SDK\VFTableHook.hpp"
#include "SDK\NetvarManager.hpp"
#include "SDK\Utils.hpp"
#include "CBaseAttributeItem.hpp"
#include "skins.hpp"
#include "interfaces.hpp"
#include "ImGUI\imgui.h"
#include "ImGUI\DX9\imgui_impl_dx9.h"

static bool ColorPicker(const char* label, float col[3])
{
	static const float HUE_PICKER_WIDTH = 20.0f;
	static const float CROSSHAIR_SIZE = 7.0f;
	static const ImVec2 SV_PICKER_SIZE = ImVec2(200, 200);

	ImColor color(col[0], col[1], col[2]);
	bool value_changed = false;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	ImVec2 picker_pos = ImGui::GetCursorScreenPos();

	ImColor colors[] = { ImColor(255, 0, 0),
		ImColor(255, 255, 0),
		ImColor(0, 255, 0),
		ImColor(0, 255, 255),
		ImColor(0, 0, 255),
		ImColor(255, 0, 255),
		ImColor(255, 0, 0) };

	for (int i = 0; i < 6; ++i)
	{
		draw_list->AddRectFilledMultiColor(
			ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 10, picker_pos.y + i * (SV_PICKER_SIZE.y / 6)),
			ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 10 + HUE_PICKER_WIDTH,
				   picker_pos.y + (i + 1) * (SV_PICKER_SIZE.y / 6)),
			colors[i],
			colors[i],
			colors[i + 1],
			colors[i + 1]);
	}

	float hue, saturation, value;
	ImGui::ColorConvertRGBtoHSV(
		color.Value.x, color.Value.y, color.Value.z, hue, saturation, value);

	draw_list->AddLine(
		ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 8, picker_pos.y + hue * SV_PICKER_SIZE.y),
		ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 12 + HUE_PICKER_WIDTH, picker_pos.y + hue * SV_PICKER_SIZE.y),
		ImColor(255, 255, 255));

	{
		const int step = 5;
		ImVec2 pos = ImVec2(0, 0);

		ImVec4 c00(1, 1, 1, 1);
		ImVec4 c10(1, 1, 1, 1);
		ImVec4 c01(1, 1, 1, 1);
		ImVec4 c11(1, 1, 1, 1);
		for (int y = 0; y < step; y++) {
			for (int x = 0; x < step; x++) {
				float s0 = (float)x / (float)step;
				float s1 = (float)(x + 1) / (float)step;
				float v0 = 1.0 - (float)(y) / (float)step;
				float v1 = 1.0 - (float)(y + 1) / (float)step;

				ImGui::ColorConvertHSVtoRGB(hue, s0, v0, c00.x, c00.y, c00.z);
				ImGui::ColorConvertHSVtoRGB(hue, s1, v0, c10.x, c10.y, c10.z);
				ImGui::ColorConvertHSVtoRGB(hue, s0, v1, c01.x, c01.y, c01.z);
				ImGui::ColorConvertHSVtoRGB(hue, s1, v1, c11.x, c11.y, c11.z);

				draw_list->AddRectFilledMultiColor(
					ImVec2(picker_pos.x + pos.x, picker_pos.y + pos.y), 
					ImVec2(picker_pos.x + pos.x + SV_PICKER_SIZE.x / step, picker_pos.y + pos.y + SV_PICKER_SIZE.y / step),
					ImGui::ColorConvertFloat4ToU32(c00),
					ImGui::ColorConvertFloat4ToU32(c10),
					ImGui::ColorConvertFloat4ToU32(c11),
					ImGui::ColorConvertFloat4ToU32(c01));

				pos.x += SV_PICKER_SIZE.x / step;
			}
			pos.x = 0;
			pos.y += SV_PICKER_SIZE.y / step;
		}
	}

	float x = saturation * SV_PICKER_SIZE.x;
	float y = (1 -value) * SV_PICKER_SIZE.y;
	ImVec2 p(picker_pos.x + x, picker_pos.y + y);
	draw_list->AddLine(ImVec2(p.x - CROSSHAIR_SIZE, p.y), ImVec2(p.x - 2, p.y), ImColor(255, 255, 255));
	draw_list->AddLine(ImVec2(p.x + CROSSHAIR_SIZE, p.y), ImVec2(p.x + 2, p.y), ImColor(255, 255, 255));
	draw_list->AddLine(ImVec2(p.x, p.y + CROSSHAIR_SIZE), ImVec2(p.x, p.y + 2), ImColor(255, 255, 255));
	draw_list->AddLine(ImVec2(p.x, p.y - CROSSHAIR_SIZE), ImVec2(p.x, p.y - 2), ImColor(255, 255, 255));

	ImGui::InvisibleButton("saturation_value_selector", SV_PICKER_SIZE);

	if (ImGui::IsItemActive() && ImGui::GetIO().MouseDown[0])
	{
		ImVec2 mouse_pos_in_canvas = ImVec2(
			ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y);

		/**/ if( mouse_pos_in_canvas.x <                     0 ) mouse_pos_in_canvas.x = 0;
		else if( mouse_pos_in_canvas.x >= SV_PICKER_SIZE.x - 1 ) mouse_pos_in_canvas.x = SV_PICKER_SIZE.x - 1;

		/**/ if( mouse_pos_in_canvas.y <                     0 ) mouse_pos_in_canvas.y = 0;
		else if( mouse_pos_in_canvas.y >= SV_PICKER_SIZE.y - 1 ) mouse_pos_in_canvas.y = SV_PICKER_SIZE.y - 1;

		value = 1 - (mouse_pos_in_canvas.y / (SV_PICKER_SIZE.y - 1));
		saturation = mouse_pos_in_canvas.x / (SV_PICKER_SIZE.x - 1);
		value_changed = true;
	}

	ImGui::SetCursorScreenPos(ImVec2(picker_pos.x + SV_PICKER_SIZE.x + 10, picker_pos.y));
	ImGui::InvisibleButton("hue_selector", ImVec2(HUE_PICKER_WIDTH, SV_PICKER_SIZE.y));

	if( (ImGui::IsItemHovered()||ImGui::IsItemActive()) && ImGui::GetIO().MouseDown[0])
	{
		ImVec2 mouse_pos_in_canvas = ImVec2(
			ImGui::GetIO().MousePos.x - picker_pos.x, ImGui::GetIO().MousePos.y - picker_pos.y);

		/* Previous horizontal bar will represent hue=1 (bottom) as hue=0 (top). Since both colors are red, we clamp at (-2, above edge) to avoid visual continuities */
		/**/ if( mouse_pos_in_canvas.y <                     0 ) mouse_pos_in_canvas.y = 0;
		else if( mouse_pos_in_canvas.y >= SV_PICKER_SIZE.y - 2 ) mouse_pos_in_canvas.y = SV_PICKER_SIZE.y - 2;

		hue = mouse_pos_in_canvas.y / (SV_PICKER_SIZE.y - 1 );
		value_changed = true;
	}

	color = ImColor::HSV(hue > 0 ? hue : 1e-6, saturation > 0 ? saturation : 1e-6, value > 0 ? value : 1e-6);
	col[0] = color.Value.x;
	col[1] = color.Value.y;
	col[2] = color.Value.z;
	return value_changed | ImGui::ColorEdit3(label, col);
}

static const char* HAIRSTYLE[]=
{
	"PLUS",
	"CROSS",
	"CIRCLE",
	"SQUARE",
	"DOTROUND",
	"DOTSQUARE"
};

struct KNIFE {
	int id; char* name; char* display; char* simple;
	KNIFE() { id = 0; name = "$NAME"; display = "$DISPLAY"; simple = "$SIMPLE"; }
	KNIFE(int id, char* name, char* simple, char* display) { this->id = id; this->name = display; this->simple = simple; this->display = name; }
};
struct SKIN {
	int id; char* name;
	SKIN() { id = 0; name = "DEFAULT"; }
	SKIN(int id, char* name) { this->id = id; this->name = name; }
};

static std::vector<SKIN> AK_SKINS = {
	SKIN{ 341, "First Class" },
	SKIN{ 14,  "Red Laminate" },
	SKIN{ 22,  "Contrast Spray" },
	SKIN{ 44,  "Case Hardened" },
	SKIN{ 72,  "Safari Mesh" },
	SKIN{ 122, "Jungle Spray" },
	SKIN{ 170, "Predator" },
	SKIN{ 172, "Black Laminate" },
	SKIN{ 180, "Fire Serpent" },
	SKIN{ 394, "Cartel" },
	SKIN{ 300, "Emerald Pinstripe" },
	SKIN{ 226, "Blue Laminate" },
	SKIN{ 282, "Redline" },
	SKIN{ 302, "Vulcan" },
	SKIN{ 316, "Jaguar" },
	SKIN{ 340, "Jet Set" },
	SKIN{ 380, "Wasteland Rebel" },
	SKIN{ 422, "Elite Build" },
	SKIN{ 456, "Hydroponic" },
	SKIN{ 474, "Aquamarine Revenge" }
};

static std::vector<SKIN> A1_SKINS = {
	SKIN{59,  "Slaughter" },
	SKIN{33,  "Hot Rod" },
	SKIN{60,  "Dark Water" },
	SKIN{430, "Hyper Beast" },
	SKIN{77,  "Boreal Forest" },
	SKIN{235, "VariCamo" },
	SKIN{254, "Nitro" },
	SKIN{189, "Bright Water" },
	SKIN{301, "Atomic Alloy" },
	SKIN{217, "Blood Tiger" },
	SKIN{257, "Guardian" },
	SKIN{321, "Master Piece" },
	SKIN{326, "Knight" },
	SKIN{360, "Cyrex" },
	SKIN{383, "Basilisk" },
	SKIN{440, "Icarus Fell" },
	SKIN{445, "Hot Rod" },
};

static std::vector<SKIN> USPS_SKINS = {
	SKIN{59,  "Slaughter" },
	SKIN{25,  "Forest Leaves" },
	SKIN{60,  "Dark Water" },
	SKIN{235, "VariCamo" },
	SKIN{183, "Overgrowth" },
	SKIN{339, "Caiman" },
	SKIN{217, "Blood Tiger" },
	SKIN{221, "Serum" },
	SKIN{236, "Night Ops" },
	SKIN{277, "Stainless" },
	SKIN{290, "Guardian" },
	SKIN{313, "Orion"},
	SKIN{318, "Road Rash" },
	SKIN{332, "Royal Blue" },
	SKIN{364, "Business Class" },
	SKIN{454, "Para Green" },
	SKIN{489, "Torque" }
};

static std::vector<SKIN> A4_SKINS = {
	SKIN{8,    "Desert Storm"},
	SKIN{101,  "Tornado"},
	SKIN{5,    "Forest DDPAT"},
	SKIN{167,  "Radiation Hazard"},
	SKIN{164,  "Modern Hunter"},
	SKIN{16,   "Jungle Tiger"},
	SKIN{17,   "Urban DDPAT"},
	SKIN{155,  "Bullet Rain"},
	SKIN{170,  "Predator"},
	SKIN{176,  "Faded Zebra"},
	SKIN{187,  "Zirka"},
	SKIN{255,  "Asiimov"},
	SKIN{309,  "Howl"},
	SKIN{215,  "X - Ray"},
	SKIN{336,  "Desert - Strike"},
	SKIN{384,  "Griffin"},
	SKIN{400,  "çã(Dragon King)"},
	SKIN{449,  "Poseidon"},
	SKIN{471,  "Daybreak"},
	SKIN{480,  "Evil Daimyo"}
};

static std::vector<SKIN> DEAGLE_SKINS = {
	SKIN{37 , "Blaze"},
	SKIN{347, "Pilot"},
	SKIN{468, "Midnight Storm"},
	SKIN{469, "Sunset Storm Ò"},
	SKIN{5,   "Forest DDPAT"},
	SKIN{12,  "Crimson Web"},
	SKIN{17,  "Urban DDPAT"},
	SKIN{40,  "Night"},
	SKIN{61,  "Hypnotic"},
	SKIN{90,  "Mudder"},
	SKIN{235, "VariCamo"},
	SKIN{185, "Golden Koi"},
	SKIN{248, "Red Quartz"},
	SKIN{231, "Cobalt Disruption"},
	SKIN{232, "Crimson Web"},
	SKIN{237, "Urban Rubble"},
	SKIN{397, "Naga"},
	SKIN{328, "Hand Cannon"},
	SKIN{273, "Heirloom"},
	SKIN{296, "Meteorite"},
	SKIN{351, "Conspiracy"},
	SKIN{425, "Bronze Deco"},
	SKIN{470, "Sunset Storm "}
};

static std::vector<SKIN> AWP_SKINS = {
	SKIN{174, "BOOM"} ,
	SKIN{344, "Dragon Lore"},
	SKIN{5,   "Forest DDPAT"},
	SKIN{84,  "Pink DDPAT"},
	SKIN{30,  "Snake Camo"},
	SKIN{51,  "Lightning Strike"},
	SKIN{72,  "Safari Mesh"},
	SKIN{181, "Corticera"},
	SKIN{259, "Redline"},
	SKIN{395, "Man - o'-war"},
	SKIN{212, "Graphite"},
	SKIN{214, "Graphite"},
	SKIN{227, "Electric Hive"},
	SKIN{251, "Pit Viper"},
	SKIN{279, "Asiimov"},
	SKIN{424, "Worm God"},
	SKIN{446, "Medusa"},
	SKIN{451, "Sun in Leo"},
	SKIN{475, "Hyper Beast"}
};

static std::vector<SKIN> FIVESEVEN_SKINS = {
	SKIN{3,   "Candy Apple"},
	SKIN{27,  "Bone Mask"},
	SKIN{44,  "Case Hardened"},
	SKIN{46,  "Contractor"},
	SKIN{78,  "Forest Night"},
	SKIN{141, "Orange Peel"},
	SKIN{151, "Jungle"},
	SKIN{254, "Nitro"},
	SKIN{248, "Red Quartz"},
	SKIN{210, "Anodized Gunmetal"},
	SKIN{223, "Nightshade"},
	SKIN{252, "Silver Quartz"},
	SKIN{265, "Kami"},
	SKIN{274, "Copper Galaxy"},
	SKIN{464, "Neon Kimono"},
	SKIN{352, "Fowl Play"},
	SKIN{377, "Hot Shot"},
	SKIN{387, "Urban Hazard"},
	SKIN{427, "Monkey Business"}
};

static std::vector<KNIFE> KNIFE_MODELS = {
	KNIFE{0, "DEFAULT", "DEFAULT", "Default"},
	KNIFE{0, "models/weapons/v_knife_bayonet.mdl",           "knife_bayonet",           "Bayonet"},
	KNIFE{0, "models/weapons/v_knife_m9_bay.mdl",            "knife_m9_bay",            "M9 Bayonet"},
	KNIFE{0, "models/weapons/v_knife_flip.mdl",              "knife_flip",              "Flip Knife"},
	KNIFE{0, "models/weapons/v_knife_gut.mdl",               "knife_gut",               "Gut Knife"},
	KNIFE{0, "models/weapons/v_knife_karam.mdl",             "knife_karam",             "Karambit"},
	KNIFE{0, "models/weapons/v_knife_tactical.mdl",          "knife_tactical",          "Tactical"},
	KNIFE{0, "models/weapons/v_knife_butterfly.mdl",         "knife_butterfly",         "Butterfly"},
	KNIFE{0, "models/weapons/v_knife_falchion_advanced.mdl", "knife_falchion_advanced", "Fachion Knife"},
	KNIFE{0, "models/weapons/v_knife_push.mdl",              "knife_push",              "Shadow Daggers"},
	KNIFE{0, "models/weapons/v_knife_survival_bowie.mdl",    "knife_survival_bowie",    "Bowie Knife"},
};

static std::vector<SKIN> KNIFE_SKINS = {

};

namespace Settings
{
	static bool AIMBOT_ENABLED    = false;
	static bool ESP_ENABLED       = false;
	static bool BHOP_ENABLED      = false;
	static bool CROSSHAIR_ENABLED = false;
	static bool RADAR_ENABLED     = false;
	static bool NOVISRECOIL       = false;
	static bool WATERMARK         =  true;
	static bool ANTIAIM           = false;
	static bool BONEESP           = false;
	static bool CSCOCOMPAT        = false;
	static bool NOFLASH           = false;

	static char               CLANTAG[15]  = "";
	static bool               ANIMATED     = false;
	static std::vector<std::string> FRAMES;

	static float ESP_ENEMY_COLOR[4] = {1, 0, 0,   1};
	static float  ESP_TEAM_COLOR[4] = {0,   1, 0, 1};
	static float ESP_BONES_COLOR[4] = {0,   1, 1, 1};

	static float HAIRCOLOR[4] = {1, 1, 0, 1};
	static int   HAIRSIZE     = 100;
	static int   STYLE        = 0;

	namespace Skins
	{
		static int ak;
	}
}

struct MenuSetting {
	bool* to_edit;
	std::string name;
	MenuSetting(bool* t, std::string n) {
		name += n;
		to_edit = t;
	}
};

static bool drop = false;
static POINT Cur;

/* Shamelessly stolen from stackoverflow */
class KeyToggle {
public:
	KeyToggle(ButtonCode_t key) :mKey(key), mActive(false) {}
	operator bool() {
		if (InputSystem->IsButtonDown(mKey) && !mActive) {
			mActive = true;
			return true;
		}
		else if (!InputSystem->IsButtonDown(mKey)) mActive = false;
		return false;
	}
private:
	ButtonCode_t mKey;
	bool mActive;
};

typedef bool(__thiscall* CreateMove_t)(SourceEngine::IClientMode*, float, SourceEngine::CUserCmd*);
typedef void(__thiscall* PaintTraverse_t)(SourceEngine::IPanel*, unsigned int, bool, bool);
typedef void(__thiscall* FrameStageNotify_t)(void*, SourceEngine::ClientFrameStage_t); 
typedef long(__stdcall* EndScene_t)(IDirect3DDevice9*);
typedef long(__stdcall* Reset_t)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
typedef void(__fastcall* SendClanTag_t)(const char*,const char*);

static SourceEngine::HFont font;

static bool anim_enabled=false;
static bool menu_enabled=true;
static std::vector<MenuSetting> menu;

static std::unique_ptr<Hooks::VFTableHook> PaintTraverseHook_p = nullptr;
static PaintTraverse_t PaintTraverse_o = nullptr;

static std::unique_ptr<Hooks::VFTableHook> CreateMoveHook_p = nullptr;
static CreateMove_t CreateMove_o = nullptr;

static std::unique_ptr<Hooks::VFTableHook> FrameStageNotify_p = nullptr;
static FrameStageNotify_t FrameStageNotify_o = nullptr;

static std::unique_ptr<Hooks::VFTableHook> EndScene_p = nullptr;
static EndScene_t EndScene_o = nullptr;

static std::unique_ptr<Hooks::VFTableHook> Reset_p = nullptr;
static Reset_t Reset_o = nullptr;

static WNDPROC WndProc_o = nullptr;
static HWND Window_o = nullptr;

static bool sliding=false;
static float FOV_SLIDE = 100;

static int ak_ind = 0, a1_ind = 0, a4_ind = 0, deag_ind = 0, usps_ind = 0;

static KeyToggle toggle_home(ButtonCode_t::KEY_HOME);
static KeyToggle toggle_click(ButtonCode_t::MOUSE_LEFT);
static KeyToggle toggle_end(ButtonCode_t::KEY_END);

void DText(const wchar_t* text, int x, int y);
void DrawRect(int x, int y, int w, int h);
void DrawFillRect(int x, int y, int w, int h);
void DrawMouse();

bool IsButtonPressed(ButtonCode_t code);

void initHook();
void unloadHook();

bool __stdcall hook_createmove(float, SourceEngine::CUserCmd*);
void __stdcall hook_painttraverse(unsigned int, bool, bool);
void __fastcall hook_framestagenotify(void* ecx, void* edx, SourceEngine::ClientFrameStage_t);
HRESULT __stdcall hook_endscene(IDirect3DDevice9*);
HRESULT __stdcall hook_reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
LRESULT __stdcall hook_wndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool WINAPI hook_getcursorpos(LPPOINT lpPoint);

/* Hack functions asjibsdihvcbasdcvhlibasdcv */
void bhop(SourceEngine::CUserCmd*);
bool aimbot(SourceEngine::CUserCmd*);
bool antiaim(SourceEngine::CUserCmd*);
void esp();
void radar();

inline float DEG2RAD(float deg) { return deg*(3.14159f/180.f); }
void CorrectMovement(SourceEngine::QAngle, SourceEngine::CUserCmd*, float oldfor, float oldside);


/* UI SHIT ASIHSDOLNVCASLKJVNASLKVNSAKL:VNSAVD */
template <typename TYPE>
struct DropMenu {
	std::vector<TYPE>* skins;
	const wchar_t* name;
	int index = 0;
	bool thisdrop = false;
	int x, y;
	SourceEngine::EItemDefinitionIndex skinid;
	bool needsupdate=true;
	KeyToggle drop_click;
	std::function<void(DropMenu*)> funct;

	DropMenu(std::vector<TYPE>* _skins, const wchar_t* _name, std::function<void(DropMenu*)> _funct, SourceEngine::EItemDefinitionIndex _skinid, int _x, int _y) :
		drop_click(ButtonCode_t::MOUSE_LEFT),
		skinid(_skinid)
	{
		skins = _skins;
		name = _name;
		x = _x;
		y = _y;
		funct = _funct;
	}
	void Draw() {
		Surface->DrawSetTextColor(255, 255, 255, 255);
		Surface->DrawSetColor(0, 0, 120, 180);
		DrawFillRect(x, y, 150, 20);

		Surface->DrawSetColor(0, 0, 0, 255);
		DrawRect(x, y, 150, 20);
		DText(name, x + 5, y - 16);
		std::string z((*skins)[index].name);
		DText(std::wstring(z.begin(), z.end()).c_str(), x, y);

		if (thisdrop)
		{
			for (int i = 0; i < skins->size(); i++) {
				TYPE skin = (*skins)[i];

				if (Cur.x > x && Cur.x < x + 150 && Cur.y > y + 20 + i * 20 && Cur.y < y + 20 + i * 20 + 20) {
					Surface->DrawSetColor(0, 120, 0, 180);
					if (drop_click) {
						index = i;
						needsupdate = true;
						thisdrop = false;
						drop = false;
					}
				}
				else 
					Surface->DrawSetColor(0, 120, 120, 180);

				DrawFillRect(x, y + 20 + i * 20, 150, 20);
				Surface->DrawSetColor(0, 0, 0, 255);
				DrawRect(x, y + 20 + i * 20, 150, 20);
				Surface->DrawSetTextColor(255, 255, 255, 255);
				std::string h(skin.name);
				DText(std::wstring(h.begin(), h.end()).c_str(), x, y + 20 + i * 20);

			}
		}

		if (drop_click) {
			if (drop)
				if (!thisdrop)
					return;

			if (Cur.x > this->x && Cur.x < this->x + 150 && Cur.y > this->y && Cur.y < this->y + 20) {
				if (!drop && !thisdrop) {
					this->thisdrop = true;
					drop = true;
				}
			}
			else {
				drop = false;
				this->thisdrop = false;
			}
		}

		if (needsupdate) {
			//set_skins(skinid, (*skins)[index].id);
			funct(this);
			needsupdate = false;
		}
	}
};

static DropMenu<SKIN>* ak47;
static DropMenu<SKIN>* m4a4;
static DropMenu<SKIN>* m4a1;
static DropMenu<SKIN>* usps;
static DropMenu<SKIN>* deag;
static DropMenu<SKIN>* awps;
static DropMenu<SKIN>* five;
static DropMenu<KNIFE>*knife;

static std::vector<DropMenu<SKIN>**> Sorter;

#endif /* end of include guard: HOOKER_H */
