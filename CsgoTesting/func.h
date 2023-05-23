#pragma once
#include <vector>
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <unordered_map>
#include "csgo.hpp"
#include "const.h"

struct Angles {
    float pitch;
    float yaw;
};

struct Weapon {
	int id;
    int bullets;
    int rpm;
	float reloadTime;
    //Vector of recoil pattern?
    std::vector<Angles> recoil_pattern;
    std::vector<Angles> abs_recoil_pattern;
};

struct AllWeapons {
	std::unordered_map<int, Weapon> allWeapons;
};

struct Vector3 {
    float x;
    float y;
    float z;
};

struct Vector2 {
    float x;
    float y;
};

static struct ClientStates {
	int lobby = 0, loading = 1, connecting = 2, connected = 5, ingame = 6;
} clientState;



class player
{
public:
    char pad_0000[160]; //0x0000
    Vector3 xyz; //0x00A0
    char pad_00AC[864]; //0x00AC
}; //Size: 0x040C

int GetProcessId(const wchar_t* procName);
DWORD GetProcessBaseAddress(DWORD procId, const wchar_t* modName);
HANDLE GetGameHandle(int procId);
bool WorldToScreen(Vector3 world, Vector2* screen, float viewMatrix[16], int screenWidth, int screenHeight);
uint32_t DynamicAddressWalker(HANDLE handle, uint32_t base_address, std::vector<uint32_t> offsets);
bool GetAngles(Angles& curr_pos, uint32_t pitch_addr, HANDLE game_handle);
uint32_t Dereference(uint32_t address, HANDLE game_handle);
void AsciiGen();
void Status(bool value);
short GetCurrentWeapon();
bool IsValidRecoilIndex(short currWeaponId);
void InitWeaponStruct(AllWeapons* all);
bool IsInGame();
bool IsAmmoZero();
bool IsInReload();

template <typename T>
T readMem(DWORD address);

template <typename T>
void writeMem(DWORD address, T value);

//skinchanger code
void ForceUpdate();
void SkinChanger();
UINT GetModelIndexById(const short knifeID);
UINT GetModelIndex(const char* modelName);

enum knifeDefinitionIndex
{
	WEAPON_KNIFE = 42,
	WEAPON_KNIFE_T = 59,
	WEAPON_KNIFE_BAYONET = 500,
	WEAPON_KNIFE_CSS = 503,
	WEAPON_KNIFE_FLIP = 505,
	WEAPON_KNIFE_GUT = 506,
	WEAPON_KNIFE_KARAMBIT = 507,
	WEAPON_KNIFE_M9_BAYONET = 508,
	WEAPON_KNIFE_TACTICAL = 509,
	WEAPON_KNIFE_FALCHION = 512,
	WEAPON_KNIFE_SURVIVAL_BOWIE = 514,
	WEAPON_KNIFE_BUTTERFLY = 515,
	WEAPON_KNIFE_PUSH = 516,
	WEAPON_KNIFE_CORD = 517,
	WEAPON_KNIFE_CANIS = 518,
	WEAPON_KNIFE_URSUS = 519,
	WEAPON_KNIFE_GYPSY_JACKKNIFE = 520,
	WEAPON_KNIFE_OUTDOOR = 521,
	WEAPON_KNIFE_STILETTO = 522,
	WEAPON_KNIFE_WIDOWMAKER = 523,
	WEAPON_KNIFE_SKELETON = 525
};

enum weaponDefinitionIndex
{
	WEAPON_DEAGLE = 1,
	WEAPON_ELITE = 2,
	WEAPON_FIVESEVEN = 3,
	WEAPON_GLOCK = 4,
	WEAPON_AK47 = 7,
	WEAPON_AUG = 8,
	WEAPON_AWP = 9,
	WEAPON_FAMAS = 10,
	WEAPON_G3SG1 = 11,
	WEAPON_GALILAR = 13,
	WEAPON_M249 = 14,
	WEAPON_M4A1 = 16,
	WEAPON_MAC10 = 17,
	WEAPON_P90 = 19,
	WEAPON_MP5SD = 23,
	WEAPON_UMP45 = 24,
	WEAPON_XM1014 = 25,
	WEAPON_BIZON = 26,
	WEAPON_MAG7 = 27,
	WEAPON_NEGEV = 28,
	WEAPON_SAWEDOFF = 29,
	WEAPON_TEC9 = 30,
	WEAPON_HKP2000 = 32,
	WEAPON_MP7 = 33,
	WEAPON_MP9 = 34,
	WEAPON_NOVA = 35,
	WEAPON_P250 = 36,
	WEAPON_SCAR20 = 38,
	WEAPON_SG556 = 39,
	WEAPON_SSG08 = 40,
	WEAPON_M4A1_SILENCER = 60,
	WEAPON_USP_SILENCER = 61,
	WEAPON_CZ75A = 63,
	WEAPON_REVOLVER = 64
};

typedef struct WeaponPaint {
	int weaponid;
	int paintkit;
	int seed;
	int stattrack;
	float wear;
} WeaponPaint;



