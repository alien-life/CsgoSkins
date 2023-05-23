#include "func.h"

int GetProcessId(const wchar_t* procName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    PROCESSENTRY32 currProcess;
    currProcess.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snapshot, &currProcess)) {
        printf("snapshot failed\n");
        CloseHandle(snapshot);
        return -1;
    }

    do {
        //return process id if found
        if (wcscmp(currProcess.szExeFile, procName) == 0) {
            CloseHandle(snapshot);
            return currProcess.th32ProcessID;
        }
    } while (Process32Next(snapshot, &currProcess));
    //else return -1
    CloseHandle(snapshot);
    return -1;
}

DWORD GetProcessBaseAddress(DWORD procId, const wchar_t* modName) {
    DWORD baseAddress = -1;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

    MODULEENTRY32 currModule;
    currModule.dwSize = sizeof(MODULEENTRY32);

    //i think client.dll is the base
    if (!Module32First(snapshot, &currModule)) {
        //printf("module32first failed");
        CloseHandle(snapshot);
        return -1;
    }

    do {
        if (wcscmp(currModule.szModule, modName) == 0) {
            //std::wcout << currModule.szModule << '\n';
            baseAddress = (DWORD)currModule.modBaseAddr;
        }
    } while (Module32Next(snapshot, &currModule));

    CloseHandle(snapshot);
    return baseAddress;
}


HANDLE GetGameHandle(int procId) {
    if (procId == -1) {
        //printf("procId for csgo.exe not found");
        return NULL;
    }
    //return OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE, false, procId);
    return OpenProcess(PROCESS_ALL_ACCESS, false, procId);

}

bool WorldToScreen(Vector3 world, Vector2* screen, float viewMatrix[16], int screenWidth, int screenHeight) {
    Vector3 clip;
    clip.x = world.x * viewMatrix[0] + world.y * viewMatrix[1] + world.z * viewMatrix[2] + viewMatrix[3];
    clip.y = world.x * viewMatrix[4] + world.y * viewMatrix[5] + world.z * viewMatrix[6] + viewMatrix[7];
    clip.z = world.x * viewMatrix[8] + world.y * viewMatrix[9] + world.z * viewMatrix[10] + viewMatrix[11];
    float w = world.x * viewMatrix[12] + world.y * viewMatrix[13] + world.z * viewMatrix[14] + viewMatrix[15];

    if (w < 0.1f) {
        return false;
    }

    Vector3 normalized;
    normalized.x = clip.x / w;
    normalized.y = clip.y / w;
    normalized.z = clip.z / w;

    screen->x = (screenWidth / 2 * normalized.x) + (normalized.x + screenWidth / 2);
    screen->y = -(screenHeight / 2 * normalized.y) + (normalized.y + screenHeight / 2);

    return true;
}

uint32_t DynamicAddressWalker(HANDLE handle, uint32_t base_address, std::vector<uint32_t> offsets) {
    uint32_t ret = -1;
    uint32_t hold = base_address;

    for (int i = 0; i < offsets.size(); i++) {
        hold += offsets.at(i);
        ReadProcessMemory(handle, (LPVOID)(hold), &ret, sizeof(ret), NULL);
        //std::cout << std::hex << ret<< " -- DMA" << '\n';
    }
    return ret;
}

bool GetAngles(Angles& curr_pos, uint32_t pitch_addr, HANDLE game_handle) {

    ReadProcessMemory(game_handle, (LPVOID)pitch_addr, &curr_pos, sizeof(Angles), NULL);
    return true;
}

uint32_t Dereference(uint32_t address, HANDLE game_handle) {
    uint32_t ret;
    ReadProcessMemory(game_handle, (LPVOID)address, &ret, sizeof(ret), NULL);
    return ret;
}

void AsciiGen() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 13);
    std::cout << R"(
                                 __     __      __    ___    _  _ 
                                (  )   (  )    (  )  (  _)  ( \( )
                                /__\    )(__    )(    ) _)   )  ( 
                               (_)(_)  (____)  (__)  (___)  (_)\_)

                                Numpad1: Toggle skin changer
                                Numpad2: Toggle aimbot
                                Numpad3: Toggle recoil compensator
                                Numpad0: Exit
    )" << '\n';
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 3);
}

void Status(bool status) {
    if (status) {
        printf("Enabled\n");
    }
    else {
        printf("Disabled\n");
    }
}

template <typename T>
T readMem(DWORD address) {
    T buffer;
    ReadProcessMemory(game_handle, (LPVOID)address, &buffer, sizeof(buffer), NULL);
    return buffer;
}

template <typename T>
void writeMem(DWORD address, T value) {
    WriteProcessMemory(game_handle, (LPVOID)address, &value, sizeof(value), NULL);
}

UINT GetModelIndex(const char* modelName) {
    int ClientState = readMem<UINT>(engine_addr + hazedumper::signatures::dwClientState);
    int m_pModelPrecacheTable = readMem<UINT>(ClientState + 0x52A4);
    int nsd = readMem<UINT>(m_pModelPrecacheTable + 0x40);
    int m_pItems = readMem<UINT>(nsd + 0xC);
    int xxxxx = m_pItems + 0xC;

    for (UINT i = 0; i < 1024; i++)
    {
        int nsdi_i = readMem<UINT>(xxxxx + (i * 0x34));
        char str[128] = { 0 };
        if (ReadProcessMemory(game_handle, (LPCVOID)nsdi_i, str, sizeof(str), NULL))
        {
            if (_stricmp(str, modelName) == 0)
            {
                return i;
            }
        }
    }
    return 0;
}

UINT GetModelIndexById(const short knifeID) {
    UINT modelIndex = 0;
    switch (knifeID)
    {
    case WEAPON_KNIFE:
        modelIndex = GetModelIndex("models/weapons/v_knife_default_ct.mdl");
        break;
    case WEAPON_KNIFE_T:
        modelIndex = GetModelIndex("models/weapons/v_knife_default_t.mdl");
        break;
    case WEAPON_KNIFE_BAYONET:
        modelIndex = GetModelIndex("models/weapons/v_knife_bayonet.mdl");
        break;
    case WEAPON_KNIFE_BUTTERFLY:
        modelIndex = GetModelIndex("models/weapons/v_knife_butterfly.mdl");
        break;
    case WEAPON_KNIFE_CANIS:
        modelIndex = GetModelIndex("models/weapons/v_knife_canis.mdl");
        break;
    case WEAPON_KNIFE_CORD:
        modelIndex = GetModelIndex("models/weapons/v_knife_cord.mdl");
        break;
    case WEAPON_KNIFE_CSS:
        modelIndex = GetModelIndex("models/weapons/v_knife_css.mdl");
        break;
    case WEAPON_KNIFE_FALCHION:
        modelIndex = GetModelIndex("models/weapons/v_knife_falchion_advanced.mdl");
        break;
    case WEAPON_KNIFE_FLIP:
        modelIndex = GetModelIndex("models/weapons/v_knife_flip.mdl");
        break;
    case WEAPON_KNIFE_GUT:
        modelIndex = GetModelIndex("models/weapons/v_knife_gut.mdl");
        break;
    case WEAPON_KNIFE_GYPSY_JACKKNIFE:
        modelIndex = GetModelIndex("models/weapons/v_knife_gypsy_jackknife.mdl");
        break;
    case WEAPON_KNIFE_KARAMBIT:
        modelIndex = GetModelIndex("models/weapons/v_knife_karam.mdl");
        break;
    case WEAPON_KNIFE_M9_BAYONET:
        modelIndex = GetModelIndex("models/weapons/v_knife_m9_bay.mdl");
        break;
    case WEAPON_KNIFE_OUTDOOR:
        modelIndex = GetModelIndex("models/weapons/v_knife_outdoor.mdl");
        break;
    case WEAPON_KNIFE_PUSH:
        modelIndex = GetModelIndex("models/weapons/v_knife_push.mdl");
        break;
    case WEAPON_KNIFE_SKELETON:
        modelIndex = GetModelIndex("models/weapons/v_knife_skeleton.mdl");
        break;
    case WEAPON_KNIFE_STILETTO:
        modelIndex = GetModelIndex("models/weapons/v_knife_stiletto.mdl");
        break;
    case WEAPON_KNIFE_SURVIVAL_BOWIE:
        modelIndex = GetModelIndex("models/weapons/v_knife_survival_bowie.mdl");
        break;
    case WEAPON_KNIFE_TACTICAL:
        modelIndex = GetModelIndex("models/weapons/v_knife_tactical.mdl");
        break;
    case WEAPON_KNIFE_URSUS:
        modelIndex = GetModelIndex("models/weapons/v_knife_ursus.mdl");
        break;
    case WEAPON_KNIFE_WIDOWMAKER:
        modelIndex = GetModelIndex("models/weapons/v_knife_widowmaker.mdl");
        break;
    default:
        break;
    }
    return modelIndex;
}

void ForceUpdate() {
    DWORD clientState = readMem<DWORD>(engine_addr + hazedumper::signatures::dwClientState);
    writeMem<int>(clientState + 0x174, -1);
}

//for config parsing
int StrToWeaponId(std::string weapon) {
    if (weapon.compare("deagle") == 0) {
        return WEAPON_DEAGLE;
    }
    else if (weapon.compare("elite") == 0) {
        return WEAPON_ELITE;
    }
    else if (weapon.compare("fiveseven") == 0) {
        return WEAPON_FIVESEVEN;
    }
    else if (weapon.compare("glock") == 0) {
        return WEAPON_GLOCK;
    }
    else if (weapon.compare("ak47") == 0) {
        return WEAPON_AK47;
    }
    else if (weapon.compare("aug") == 0) {
        return WEAPON_AUG;
    }
    else if (weapon.compare("awp") == 0) {
        return WEAPON_AWP;
    }
    else if (weapon.compare("famas") == 0) {
        return WEAPON_FAMAS;
    }
    else if (weapon.compare("g3sg1") == 0) {
        return WEAPON_G3SG1;
    }
    else if (weapon.compare("galil") == 0) {
        return WEAPON_GALILAR;
    }
    else if (weapon.compare("m249") == 0) {
        return WEAPON_M249;
    }
    else if (weapon.compare("m4a1") == 0) {
        return WEAPON_M4A1;
    }
    else if (weapon.compare("mac10") == 0) {
        return WEAPON_MAC10;
    }
    else if (weapon.compare("p90") == 0) {
        return WEAPON_P90;
    }
    else if (weapon.compare("mp5") == 0) {
        return WEAPON_MP5SD;
    }
    else if (weapon.compare("ump45") == 0) {
        return WEAPON_UMP45;
    }
    else if (weapon.compare("xm1014") == 0) {
        return WEAPON_XM1014;
    }
    else if (weapon.compare("bizon") == 0) {
        return WEAPON_BIZON;
    }
    else if (weapon.compare("mag7") == 0) {
        return WEAPON_MAG7;
    }
    else if (weapon.compare("negev") == 0) {
        return WEAPON_NEGEV;
    }
    else if (weapon.compare("sawedoff") == 0) {
        return WEAPON_SAWEDOFF;
    }
    else if (weapon.compare("tec9") == 0) {
        return WEAPON_TEC9;
    }
    else if (weapon.compare("hkp2000") == 0) {
        return WEAPON_HKP2000;
    }
    else if (weapon.compare("mp7") == 0) {
        return WEAPON_MP7;
    }
    else if (weapon.compare("mp9") == 0) {
        return WEAPON_MP9;
    }
    else if (weapon.compare("nova") == 0) {
        return WEAPON_NOVA;
    }
    else if (weapon.compare("p250") == 0) {
        return WEAPON_P250;
    }
    else if (weapon.compare("scar20") == 0) {
        return WEAPON_SCAR20;
    }
    else if (weapon.compare("sg556") == 0) {
        return WEAPON_SG556;
    }
    else if (weapon.compare("ssg08") == 0) {
        return WEAPON_SSG08;
    }
    else if (weapon.compare("m4a1s") == 0) {
        return WEAPON_M4A1_SILENCER;
    }
    else if (weapon.compare("usps") == 0) {
        return WEAPON_USP_SILENCER;
    }
    else if (weapon.compare("cz75a") == 0) {
        return WEAPON_CZ75A;
    }
    else if (weapon.compare("revolver") == 0) {
        return WEAPON_REVOLVER;
    }
    else if (weapon.compare("karambit") == 0) {
        return WEAPON_KNIFE_KARAMBIT;
    }
    else if (weapon.compare("gut") == 0) {
        return WEAPON_KNIFE_GUT;
    }
    else if (weapon.compare("flip") == 0) {
        return WEAPON_KNIFE_FLIP;
    }
    else if (weapon.compare("css") == 0) {
        return WEAPON_KNIFE_CSS;
    }
    else if (weapon.compare("bayonet") == 0) {
        return WEAPON_KNIFE_BAYONET;
    }
    else if (weapon.compare("flip") == 0) {
        return WEAPON_KNIFE_FLIP;
    }
    else if (weapon.compare("m9bayonet") == 0) {
        return WEAPON_KNIFE_M9_BAYONET;
    }
    else if (weapon.compare("huntsman") == 0) {
        return WEAPON_KNIFE_TACTICAL;
    }
    else if (weapon.compare("falchion") == 0) {
        return WEAPON_KNIFE_FALCHION;
    }
    else if (weapon.compare("bowie") == 0) {
        return WEAPON_KNIFE_SURVIVAL_BOWIE;
    }
    else if (weapon.compare("butterfly") == 0) {
        return WEAPON_KNIFE_BUTTERFLY;
    }
    else if (weapon.compare("buttplugs") == 0) {
        return WEAPON_KNIFE_PUSH;
    }
    else if (weapon.compare("paracord") == 0) {
        return WEAPON_KNIFE_CORD;
    }
    else if (weapon.compare("survival") == 0) {
        return WEAPON_KNIFE_CANIS;
    }
    else if (weapon.compare("ursus") == 0) {
        return WEAPON_KNIFE_URSUS;
    }
    else if (weapon.compare("navaja") == 0) {
        return WEAPON_KNIFE_GYPSY_JACKKNIFE;
    }
    else if (weapon.compare("nomad") == 0) {
        return WEAPON_KNIFE_OUTDOOR;
        }
    else if (weapon.compare("stiletto") == 0) {
        return WEAPON_KNIFE_STILETTO;
    }
    else if (weapon.compare("talon") == 0) {
        return WEAPON_KNIFE_WIDOWMAKER;
    }
    else if (weapon.compare("skeleton") == 0) {
        return WEAPON_KNIFE_SKELETON;
    }

    return -1;
}

bool InitPaints(std::unordered_map<int, WeaponPaint> &weaponMap) {
    std::ifstream stream("skinconf.txt");
    if (!stream.is_open()) {
        printf("Failed to open skinConfig\n");
        return false;
    }

    std::string line;
    int count = 0;
    while (std::getline(stream, line)) {
        std::stringstream ss(line);
        std::string weapon;
        int weaponId;
        int paint, seed, stat;
        float wear;

        std::getline(ss, weapon, ',');
        //std::cout << ss.str() << '\n';
        ss >> paint;
        ss.ignore(1, ',');
        ss >> seed;
        ss.ignore(1, ',');
        ss >> stat;
        ss.ignore(1, ',');
        ss >> wear;
        weaponId = StrToWeaponId(weapon);

        WeaponPaint paintStruct = {};
        paintStruct.weaponid = weaponId;
        paintStruct.paintkit = paint;
        paintStruct.seed = seed;
        paintStruct.stattrack = stat;
        paintStruct.wear = wear;
        
        //printf("id: %d, paint: %d, seed: %d, stat: %d, wear: %f\n", weaponId, paint, seed, stat, wear);

        weaponMap[weaponId] =  paintStruct;

    }
}

bool IsKnife(int weaponID) {
    switch (weaponID) {
    case 42:
    case 59:
    case 500:
    case 503:
    case 505:
    case 506:
    case 507:
    case 508:
    case 509:
    case 512:
    case 514:
    case 515:
    case 516:
    case 517:
    case 518:
    case 519:
    case 520:
    case 521:
    case 522:
    case 523:
    case 525:
        return true;
    default:
        return false;
    }
}

void SkinChanger() {
    bool isGetted = false;
    int knifeIndex = 0;
    bool looping = true;

    std::unordered_map<int, WeaponPaint> weaponMap;

    //load hashmap from skinconf
    InitPaints(weaponMap);

    //get the wanted knife from hashmap so that model can be gotten
    int wantedKnife = -1;
    std::unordered_map<int, WeaponPaint>::iterator it;
    for (it = weaponMap.begin(); it != weaponMap.end(); it++) {
        if (IsKnife(it->second.weaponid)) {
            wantedKnife = it->second.weaponid;
            //printf("WantedKnife: %d\n",wantedKnife);
            continue;
        }
    }
    if (wantedKnife != -1) {
        knifeIndex = GetModelIndexById(wantedKnife);
    }

    while (looping)
    {
        if (GetAsyncKeyState(VK_NUMPAD9) & 1) {
            //refresh hud
            ForceUpdate();
        }
        if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
            //printf("Exitting skinThread\n");
            looping = false;
        }
        auto EnginePointer = readMem<DWORD>(engine_addr + hazedumper::signatures::dwClientState);
        auto GameState = readMem<int>(engine_addr + 0x785914);
        DWORD localPlayer = readMem<DWORD>(client_base_address + hazedumper::signatures::dwLocalPlayer);

        int currHealth = readMem<int>(localPlayer + hazedumper::netvars::m_iHealth);
        bool knifeLoaded = false;

        //if spawned in (or in aimbotz)
        if (((currHealth <= 100) && (currHealth >= 1)) || (currHealth == 1337))
        {
            
            /* Skin Changer */
            for (int i = 0; i < 8; i++)
            {
                DWORD weaponEntity = readMem<DWORD>(localPlayer + hazedumper::netvars::m_hMyWeapons + i * 0x4) & 0xfff;
                weaponEntity = readMem<DWORD>(client_base_address + hazedumper::signatures::dwEntityList + (weaponEntity - 1) * 0x10);
                int accountID = readMem<int>(weaponEntity + hazedumper::netvars::m_OriginalOwnerXuidLow);
                if (weaponEntity == 0) { continue; }

                short weaponID = readMem<int>(weaponEntity + hazedumper::netvars::m_iItemDefinitionIndex);

                if (IsKnife(weaponID)) {
                    weaponID = wantedKnife;
                    writeMem<int>(weaponEntity + hazedumper::netvars::m_nModelIndex, knifeIndex);
                    writeMem<short>(weaponEntity + hazedumper::netvars::m_iItemDefinitionIndex, wantedKnife);
                    //printf("Knife Address: %x\n", weaponEntity);
                }
                
                DWORD Paintkit = 0;
                DWORD Seed = 0;
                DWORD Statrak = 0;
                DWORD Quality = 0;
                float Wear = 0;
                char CustomName[20] = "";
                
                
                Paintkit = weaponMap[weaponID].paintkit;
                Seed = weaponMap[weaponID].seed;
                Statrak = weaponMap[weaponID].stattrack;
                Quality = 3;
                Wear = weaponMap[weaponID].wear;
                
                
                
                //printf("ID: %d, Paintkit: %d, Seed: %d, Stat: %d, Wear: %f\n", weaponID, Paintkit, Seed, Statrak, Wear);
                writeMem<int>(weaponEntity + hazedumper::netvars::m_iAccountID, accountID);
                writeMem<DWORD>(weaponEntity + hazedumper::netvars::m_nFallbackPaintKit, Paintkit);
                writeMem<int>(weaponEntity + hazedumper::netvars::m_nFallbackSeed, Seed);
                writeMem<float>(weaponEntity + hazedumper::netvars::m_flFallbackWear, 0.0003f);
                writeMem<int>(weaponEntity + hazedumper::netvars::m_nFallbackStatTrak, Statrak);
                writeMem<int>(weaponEntity + hazedumper::netvars::m_iEntityQuality, 3);

                if (CustomName != "")
                {
                    WriteProcessMemory(game_handle, (LPVOID)(weaponEntity + hazedumper::netvars::m_szCustomName), CustomName, sizeof(char[20]), NULL);
                }

                if (readMem<int>(weaponEntity + hazedumper::netvars::m_iItemIDHigh) != -1)
                {
                    writeMem<int>(weaponEntity + hazedumper::netvars::m_iItemIDHigh, -1);
                }
            }

            /* Knife Changer */
            /* Getting knife model index in other gamestate. */
            if (isGetted == false)
            {
                //knifeIndex = GetModelIndexById(wantedKnife); //Knife Model
                //printf("Changing knife index: %d\n", knifeIndex);
                isGetted = true;
            }
            else {
                //isGetted = !isGetted;
            }

            //if knife, load the model
            UINT iCurWeaponAdress = readMem<UINT>(localPlayer + hazedumper::netvars::m_hActiveWeapon) & 0xFFF;
            UINT m_iBase = readMem<UINT>(client_base_address + hazedumper::signatures::dwEntityList + (iCurWeaponAdress - 1) * 0x10);
            short curwpnID = readMem<short>(m_iBase + hazedumper::netvars::m_iItemDefinitionIndex);
            //printf("CurrWeaponID: %d\n", curwpnID);
            if (IsKnife(curwpnID) && !knifeLoaded)
            {

                DWORD knifeViewModel = readMem<DWORD>(localPlayer + hazedumper::netvars::m_hViewModel) & 0xfff;
                knifeViewModel = readMem<DWORD>(client_base_address + hazedumper::signatures::dwEntityList + (knifeViewModel - 1) * 0x10);

                if (knifeViewModel == 0) { continue; }

                writeMem<DWORD>(knifeViewModel + hazedumper::netvars::m_nModelIndex, knifeIndex);
                //Knife Name

                // Change start knife model
                //writeMem(hazedumper::signatures::dwEntityList + hazedumper::netvars::m_nModelIndex, knifeIndex);

                // Change start view model
                //writeMem(knifeViewModel + hazedumper::netvars::m_hViewModel, knifeIndex);

                knifeLoaded = true;
            }
            
            /*
            UINT iCurWeaponAdress = readMem<UINT>(localPlayer + hazedumper::netvars::m_hActiveWeapon) & 0xFFF;
            UINT m_iBase = readMem<UINT>(client_base_address + hazedumper::signatures::dwEntityList + (iCurWeaponAdress - 1) * 0x10);
            short curwpnID = readMem<short>(m_iBase + hazedumper::netvars::m_iItemDefinitionIndex);

            if (IsKnife(curwpnID))
            {
                DWORD knifeViewModel = readMem<DWORD>(localPlayer + hazedumper::netvars::m_hViewModel) & 0xfff;
                knifeViewModel = readMem<DWORD>(client_base_address + hazedumper::signatures::dwEntityList + (knifeViewModel - 1) * 0x10);

                if (knifeViewModel == 0) { continue; }

                writeMem<DWORD>(knifeViewModel + hazedumper::netvars::m_nModelIndex, knifeIndex);
                // Change start knife model
                //writeMem(hazedumper::signatures::dwEntityList + hazedumper::netvars::m_nModelIndex, knifeIndex);

                // Change start view model
                writeMem(knifeViewModel + hazedumper::netvars::m_hViewModel, knifeIndex);
            }
            */

        }
        else
        {
            knifeLoaded = false;
        }

        Sleep(10);
    }
}

bool IsValidRecoilIndex(short currWeaponId) {
    switch (currWeaponId) {
    case WEAPON_SCAR20:
    case WEAPON_G3SG1:
    case WEAPON_M4A1:
    case WEAPON_M4A1_SILENCER:
    case WEAPON_AUG:
    case WEAPON_FAMAS:
    case WEAPON_AK47:
    case WEAPON_GALILAR:
    case WEAPON_SG556:
    case WEAPON_MP5SD:
    case WEAPON_MAC10:
    case WEAPON_MP7:
    case WEAPON_MP9:
    case WEAPON_UMP45:
    case WEAPON_P90:
    case WEAPON_BIZON:
    case WEAPON_M249:
    case WEAPON_NEGEV:
    case WEAPON_CZ75A:
        return true;
    }

    return false;
}

short GetCurrentWeapon() {
    //read current held weapon memory
    DWORD localPlayer = readMem<DWORD>(client_base_address + hazedumper::signatures::dwLocalPlayer);
    UINT iCurWeaponAdress = readMem<UINT>(localPlayer + hazedumper::netvars::m_hActiveWeapon) & 0xFFF;
    UINT m_iBase = readMem<UINT>(client_base_address + hazedumper::signatures::dwEntityList + (iCurWeaponAdress - 1) * 0x10);
    short curwpnID = readMem<short>(m_iBase + hazedumper::netvars::m_iItemDefinitionIndex);

    //use id & hashmap to get current weapon
    return curwpnID;
}

std::vector<Angles> ParseToVector(std::string stringDelta) {
    //(1.21655,-0.0337931),(1.01379,-1.49012e-08),(1.75724,0.135173),(1.62207,-0.202759),(1.92621,-0.473104),(0.980002,-0.608277),(1.08138,0.033793),(0.270346,1.62207),(0.067585,1.72345),(0.270343,0.270346),(0.540691,0.236552),(-0.270346,0.946208),(0.135172,0.946209),(0.067584,-1.31793),(0.473104,-1.35173),(0.0337925,-1.08138),(0.405519,-1.11517),(-0.270344,-1.58828),(-0.439314,-0.439311),(0.405517,0.371725),(0.0337934,0.0337933),(0.202758,0.608277),(0.168967,-0.371725),(-0.270347,-0.912415),(0.473104,0.54069),(-0.506897,1.55449),(-0.304139,2.33173),(-0.337932,1.52069),(-1.92621,-0.64207),
    std::vector<Angles> result;
    std::istringstream iss(stringDelta);
    char open_paren, comma, close_paren;
    float x, y;
    while (iss >> open_paren >> x >> comma >> y >> close_paren) {
        if (open_paren == '(' && comma == ',' && close_paren == ')') {
            result.push_back({ x,y });
        }
        else {
            throw std::invalid_argument("Invalid argument when parsing stringDelta");
        }

        if (iss.peek() == ',') {
            iss.ignore();
        }
        else {
            break;
        }
    }

    return result;
}


void InitWeaponStruct(AllWeapons* all) {
    Weapon scar;
    Weapon g3sg1;
    Weapon m4a1;
    Weapon m4a1s;
    Weapon aug;
    Weapon famas;
    Weapon ak47;
    Weapon galilar;
    Weapon sg556;
    Weapon mp5;
    Weapon mac10;
    Weapon mp7;
    Weapon mp9;
    Weapon ump45;
    Weapon p90;
    Weapon ppbizon;
    Weapon m249;
    Weapon negev;
    Weapon cz75a;

    scar.bullets = 20;
    scar.reloadTime = 3.1;
    scar.rpm = 240;
    scar.id = WEAPON_SCAR20;
    scar.recoil_pattern = ParseToVector("(0.165211,0.0300384),(0.135173,-0.0150192),(0.0150192,-0.0600767),(0.315403,0.0901151),(0.405518,0.270345),(0.0300384,-0.270345),(0.120154,0.0150192),(0.150192,-0.0600767),(0.165211,-0.18023),(-0.195249,0.270345),(-0.300384,-0.0600767),(0.165211,-0.18023),(0.105134,-0.105134),(-0.0901151,0.0300384),(0.240307,-0.0150192),(0.0901152,0.0750958),(0.18023,0.105134),(-0.36046,0.0150192),(-0.195249,0.0150192)");

    g3sg1.bullets = 20;
    g3sg1.reloadTime = 4.7;
    g3sg1.rpm = 240;
    g3sg1.id = WEAPON_G3SG1;
    g3sg1.recoil_pattern = ParseToVector("(0.0300384,0.255326),(0.0150191,0.120153),(0.0600767,-0.0750959),(0.0750959,-0.0901151),(0.0901151,0.0901151),(0.225288,0.0450576),(0.315403,0.285364),(0.0450577,-0.18023),(0.120154,0.00000000745058),(-0.105134,-0.0600768),(-0.0150189,-0.135173),(-0.0600767,0.105134),(-0.150192,0.0901151),(-0.0150191,-0.18023),(0.315403,0.135173),(0.330422,-0.0750959),(0,0.0300384),(0.0300384,0.210269),(-0.465595,0.0600767)");

    m4a1.bullets = 30;
    m4a1.reloadTime = 3.1;
    m4a1.rpm = 666;
    m4a1.id = WEAPON_M4A1;
    m4a1.recoil_pattern = ParseToVector("(0.608277,-0.101379),(0.168966,0.236552),(0.64207,0.0337932),(0.844829,-0.270345),(2.19656,0.64207),(1.89242,-0.270345),(0.574483,-0.912416),(0.101379,-0.202759),(0.270346,-1.21655),(0.912415,0.912415),(0.473103,2.16276),(-0.675865,0.304139),(-0.304139,2.80483),(0.202759,-0.236552),(0.337932,-0.540691),(0.337933,-0.473104),(-0.270347,1.68966),(-0.10138,0.135173),(0.236552,-1.35173),(0.135173,-1.25035),(0.371725,-1.55449),(0.0675869,-0.946209),(-0.0675888,-0.574484),(0.10138,0.439311),(0.101379,0.202759),(0.0675869,-0.270345),(-0.0337934,-0.304139),(0.0337934,0.101379),(0.0675869,-0.202759)");

    m4a1s.bullets = 20;
    m4a1s.reloadTime = 3.1;
    m4a1s.rpm = 600;
    m4a1s.id = WEAPON_M4A1_SILENCER;
    m4a1s.recoil_pattern = ParseToVector("(0.878622,-0.0000000149012),(-0.236552,-0.101379),(0.811036,0.236552),(0.74345,-0.168966),(1.04759,0.337932),(1.31793,0.101379),(0.506897,-0.675863),(0.675863,-0.270345),(-0.0675864,-1.08138),(0.506897,0.304139),(0.168966,0.912416),(-0.135172,1.41931),(-0.0337925,1.08138),(-0.473102,0.405517),(0.033792,-0.506898),(0.777242,0.10138),(-0.304138,0.709657),(0.0337944,0.202759),(-0.033793,-0.000000238419)");

    aug.bullets = 30;
    aug.reloadTime = 3.8;
    aug.rpm = 600;
    aug.id = WEAPON_AUG;
    aug.recoil_pattern = ParseToVector("(0.439311,-0.337932),(0.405518,0.236552),(0.64207,0.101379),(1.21655,0.371725),(1.65587,-0.371725),(1.04759,-0.270345),(0.878623,-0.811036),(1.08138,-0.168966),(0.168966,-0.777243),(0.405519,1.04759),(0.202759,0.236552),(0.202759,-0.675864),(0.20276,0.10138),(-0.270346,1.14897),(-0.540689,1.75724),(-0.371724,1.4869),(0.20276,-0.270345),(0.405519,0.202759),(-0.0675864,0.337932),(0.304139,-1.21655),(-0.473104,-1.79104),(0.540692,-0.439311),(0.304139,0.337932),(-0.236554,-0.844829),(-0.405519,-1.55449),(-0.202758,-0.64207),(0.270348,1.25035),(0.337931,0.608277),(0.101379,0.0675863)");

    famas.bullets = 25;
    famas.reloadTime = 3.3;
    famas.rpm = 666;
    famas.id = WEAPON_FAMAS;
    famas.recoil_pattern = ParseToVector("(0.439311,-0.168966),(-0.101379,0.270345),(1.55449,0.202759),(1.31793,-0.405518),(0.608277,-0.811036),(0.608278,-0.844829),(0.574484,0.675864),(0.506898,1.18276),(0.101379,0.64207),(0.574483,0.878622),(0.101379,-0.371725),(0.101379,-1.45311),(0.371724,-0.439311),(-0.101379,-1.21655),(0.033793,-0.202759),(-0.168965,-0.74345),(0.473104,-0.337931),(0.101379,0.202759),(-0.270346,1.68966),(0.236552,0.0675861),(0.202759,-0.540691),(-0.506896,-1.25035),(-0.574482,-0.574483),(-1.58828,1.25035)");

    ak47.bullets = 30;
    ak47.reloadTime = 2.4;
    ak47.rpm = 600;
    ak47.id = WEAPON_AK47;
    ak47.recoil_pattern = ParseToVector("(1.21655,-0.0337931),(1.01379,-0.0000000149012),(1.75724,0.135173),(1.62207,-0.202759),(1.92621,-0.473104),(0.980002,-0.608277),(1.08138,0.033793),(0.270346,1.62207),(0.067585,1.72345),(0.270343,0.270346),(0.540691,0.236552),(-0.270346,0.946208),(0.135172,0.946209),(0.067584,-1.31793),(0.473104,-1.35173),(0.0337925,-1.08138),(0.405519,-1.11517),(-0.270344,-1.58828),(-0.439314,-0.439311),(0.405517,0.371725),(0.0337934,0.0337933),(0.202758,0.608277),(0.168967,-0.371725),(-0.270347,-0.912415),(0.473104,0.54069),(-0.506897,1.55449),(-0.304139,2.33173),(-0.337932,1.52069),(-1.92621,-0.64207)");

    galilar.bullets = 35;
    galilar.reloadTime = 3;
    galilar.rpm = 666;
    galilar.id = WEAPON_GALILAR;
    galilar.recoil_pattern = ParseToVector("(-0.135173,-0.236552),(0.405518,0.304138),(0.270345,-0.64207),(0.74345,-0.844829),(2.77104,0.506897),(0.371725,-0.337932),(0.675862,-0.439311),(0.304139,-0.540691),(1.04759,0.844829),(0.101379,1.52069),(-0.304137,1.18276),(-0.608277,1.65586),(0.878623,1.11517),(-0.270344,-0.337932),(0.033793,0.439311),(0.033793,0.033793),(0.506897,-1.08138),(0.878622,-1.58828),(0.135175,-1.55449),(-0.709657,-0.912415),(-0.506897,-0.912416),(0.675863,0.709656),(-0.101379,-0.777243),(-0.236551,-0.574484),(0.270346,0.0337937),(-0.033792,-0.0337937),(-0.304137,0.844829),(-0.405517,2.12897),(0.033793,0.64207),(0.405517,0.0337931),(0.506897,0.439311),(-0.675862,1.21655),(-0.811035,1.55449),(-0.54069,-0.878622)");

    sg556.bullets = 30;
    sg556.reloadTime = 2.8;
    sg556.rpm = 545;
    sg556.id = WEAPON_SG556;
    sg556.recoil_pattern = ParseToVector("(0.405518,0.777243),(1.25035,0.10138),(1.52069,0.405518),(1.52069,0.236552),(1.38552,0.405518),(0.54069,0.878622),(0.675864,-0.946209),(0.43931,0.574483),(-0.168968,0.811036),(0.168967,-0.135173),(0.64207,-0.168966),(-0.10138,0.405518),(0.270345,-0.304139),(-0.270348,0.811036),(-0.777244,1.11517),(-0.506896,0.574485),(0.135173,0.168967),(0.371725,-3.00759),(-0.439311,-2.60207),(-0.304137,-1.89242),(0.878622,-0.168966),(0.844828,-0.405518),(0.0675869,-0.574483),(-0.506897,-1.25035),(-0.337932,-0.270344),(0.371724,0.980003),(0.506897,1.9938),(-0.675865,0.844829),(-1.52069,1.11517)");

    mp5.bullets = 30;
    mp5.reloadTime = 3;
    mp5.rpm = 750;
    mp5.id = WEAPON_MP5SD;
    mp5.recoil_pattern = ParseToVector("(-0.0675863,0.101379),(0.101379,0.0337932),(0.473104,0.337932),(0.54069,0.202759),(1.04759,0.405518),(1.14897,0.473104),(0.473104,0.135173),(0.304138,0.844829),(0.168966,0.168966),(0.506897,-0.405519),(0.473105,-1.35173),(-0.13517,-0.912415),(0.000000953674,-0.811036),(0.135172,-0.168966),(0.371725,0.473104),(0.270345,0.236552),(-0.168966,0.202759),(0.337932,-0.236552),(0.135173,0.135173),(-0.270344,0.980002),(-0.540689,1.04759),(-0.202758,0.304138),(0.168965,0.439311),(0,-0.10138),(-0.270344,-0.0337932),(0.337931,0.168966),(0,0.371725),(-0.135172,-0.304139),(-0.371723,-1.52069)");

    mac10.bullets = 30;
    mac10.reloadTime = 2.6;
    mac10.rpm = 800;
    mac10.id = WEAPON_MAC10;
    mac10.recoil_pattern = ParseToVector("(0.304138,-0.0000000298023),(-0.135173,-0.439311),(0.946209,-0.405518),(0.675863,-0.0675864),(1.92621,-0.912415),(1.08138,-0.811036),(0.980001,0.506898),(0.540692,-0.473104),(0.135173,-0.202759),(0.642069,-0.270345),(0.743451,0.236552),(0.506898,0.135173),(-0.20276,0.540691),(-0.135173,1.41931),(-0.473105,1.38552),(0.202759,0.202759),(-0.236551,0.743449),(0.202759,-0.473104),(-0.202758,0.405518),(-0.506896,0.709657),(0.236552,0.608277),(0.236552,-0.912416),(0.033793,-0.675863),(0.236551,-1.04759),(-0.43931,-1.65586),(0.236552,0.371725),(-0.202758,1.18276),(0.135172,-0.304138),(-0.608277,0.270345)");

    mp7.bullets = 30;
    mp7.reloadTime = 3.1;
    mp7.rpm = 750;
    mp7.id = WEAPON_MP7;
    mp7.recoil_pattern = ParseToVector("(0.337932,0.574484),(-0.135173,0),(0.64207,0.168966),(0.675863,0.574484),(0.64207,-0.540691),(0.980002,1.0138),(0.709656,-0.0675863),(0.270347,0.709656),(0.0675869,0.304139),(0.608276,-0.642071),(0.439311,-1.11517),(0.168966,-1.55449),(-0.0675869,-0.506898),(0.202759,0.506898),(0.236551,0.304138),(0.033793,0.202759),(0.033793,-0.135173),(0.473104,0),(-0.337931,1.01379),(-0.743448,1.38552),(0.135173,-0.000000238419),(-0.270344,0.777243),(0.101379,-0.337932),(0.033793,-0.0337932),(0,0.0675864),(0,0.0675864),(0,0.135173),(-0.337932,-1.31793),(-0.168965,-0.304139)");

    mp9.bullets = 30;
    mp9.reloadTime = 2.1;
    mp9.rpm = 857;
    mp9.id = WEAPON_MP9;
    mp9.recoil_pattern = ParseToVector("(0.371725,0.0337932),(0.0675863,-0.0675864),(0.777243,-0.202759),(1.65587,-0.371725),(1.25035,0.202759),(1.14897,0.10138),(0.74345,-0.270345),(0.371725,-1.55449),(0.337931,-1.79104),(-0.33793,-0.608277),(0.135172,-0.0337934),(0.777242,-0.0675869),(0.743451,1.25035),(0.20276,0.777243),(0.473105,1.45311),(0.439312,0.439311),(0.067585,1.55449),(-0.743451,1.52069),(0.0337925,-0.574484),(0.236552,-0.946209),(0.135172,-1.14897),(-0.0337925,0.101379),(-0.304138,1.21655),(-0.10138,1.14897),(-0.0337934,0.304139),(0.304138,0.64207),(0.371725,-0.675864),(0.135173,-0.236552),(-1.85862,-1.62207)");

    ump45.bullets = 25;
    ump45.reloadTime = 3.5;
    ump45.rpm = 666;
    ump45.id = WEAPON_UMP45;
    ump45.recoil_pattern = ParseToVector("(0.64207,0.101379),(0.506897,0.304139),(0.74345,-0.135173),(1.11517,0.337932),(1.79104,0.608277),(1.31793,-0.033793),(0.74345,-0.540691),(0.506897,0.168966),(1.4869,-0.74345),(-0.20276,-0.912415),(0.405517,-0.64207),(0.168964,0.0337933),(0.506896,-0.439311),(0.101379,-0.135172),(-0.337932,-0.540691),(-0.0675859,0.0337932),(0.371724,1.11517),(-0.10138,1.04759),(-0.0675869,-0.473104),(-0.135173,-1.08138),(0.0337944,-0.439311),(-0.337931,0.473104),(-0.00000190735,1.01379),(-0.0675859,0.168966)");

    p90.bullets = 50;
    p90.reloadTime = 3.3;
    p90.rpm = 857;
    p90.id = WEAPON_P90;
    p90.recoil_pattern = ParseToVector("(0.675863,0.101379),(0.236552,-0.168966),(0.371725,0.0337932),(0.709656,0.777243),(0.608277,0.811036),(0.811036,0.439311),(0.946209,-0.540691),(0.912415,-0.608277),(0.168966,-0.202759),(0.980002,-0.135173),(0.371725,0.337932),(-0.168966,0.64207),(0.304138,0.337932),(0.168965,0.608277),(-0.033793,0.506897),(0.0675869,-1.01379),(0.0675859,-0.912416),(0.0675859,-0.540691),(0.0675859,-0.574484),(0.0675864,-0.878622),(-0.337931,-0.811036),(0.101379,0.777243),(0,-0.777243),(0.236551,0.946209),(-0.371724,0.980002),(0.0000043051,1.04759),(0.101379,-0.101379),(0.304138,-0.304139),(0,0.608277),(0.101379,-0.371725),(0.101379,-0.74345),(-0.168965,-0.777243),(0.168965,0.337932),(0.033793,0.168966),(-0.202758,0.574484),(0.0675859,0.64207),(-0.033793,0.0675864),(0.0675859,-0.777243),(-0.43931,-0.844829),(0.202758,-0.675863),(-0.101379,-0.878622),(-0.033793,-0.844829),(-0.202758,-0.54069),(0,-0.574484),(0.135172,0.304139),(0.0675859,0.777243),(0.033793,-0.371725),(-0.101379,0.84483),(-0.371723,0.777243)");

    ppbizon.bullets = 64;
    ppbizon.reloadTime = 2.4;
    ppbizon.rpm = 750;
    ppbizon.id = WEAPON_BIZON;
    ppbizon.recoil_pattern = ParseToVector("(0.270345,0.0675863),(0.540691,0.202759),(0.236552,0.540691),(0.912416,0.64207),(0.946208,-0.168966),(1.55449,-0.64207),(0.675864,0.101379),(0.337932,-1.25035),(0.337931,-0.574484),(0.202758,-0.371725),(0.168966,-0.844829),(0.64207,0.439312),(-0.371724,1.62207),(0.270346,-0.168966),(0.000000476837,-0.74345),(0.101379,-0.202759),(-0.0675859,0.236552),(0.371724,-0.574484),(-0.168966,-0.54069),(-0.168966,-0.844829),(0.101379,-0.946208),(-0.270344,-0.371725),(-0.0337934,-0.168966),(0.101379,0.202758),(-0.0675859,0.236551),(0.135172,0.168966),(0.202759,0.946209),(0.371725,0.506897),(-0.101379,0.0675864),(-0.135172,-0.675864),(0.236551,0.912416),(-0.270344,0.000000238419),(-0.270345,-0.202759),(0,0.64207),(0.236552,-0.337932),(0,0.608277),(0.168965,0.0337932),(-0.371723,-0.84483),(-0.168965,-0.878623),(-0.608278,-0.912416),(-0.0337934,-0.0675864),(0.439311,0.337932),(0.101378,-0.304139),(-0.608276,-1.4869),(0.304137,1.45311),(0.236551,1.28414),(0.236552,0.0337932),(-0.101379,-0.270345),(-0.135172,-0.304139),(0,0.0675864),(0.033793,-0.506898),(0.202758,0.74345),(0.033793,0.912416),(0.202759,1.25035),(-0.371724,0.946209),(-0.101379,0.608277),(0.135172,-0.371725),(0.270345,-0.878622),(-0.0675859,0.506897),(0,0.304138),(-0.337931,0.168966),(-0.405516,-0.168966),(-0.980001,0)");

    m249.bullets = 100;
    m249.reloadTime = 5.7;
    m249.rpm = 750;
    m249.id = WEAPON_M249;
    m249.recoil_pattern = ParseToVector("(0.337932,0.0000000298023),(1.38552,1.11517),(1.52069,0.878622),(1.28414,1.08138),(1.38552,0.270346),(1.58828,-0.811036),(1.52069,-1.96),(0.236551,-1.55449),(0.473104,-0.811036),(0.84483,-0.743449),(0.878621,0.439312),(0.270344,0.608277),(0.439312,-0.439311),(0.405519,-0.337932),(0.506899,-0.405518),(0.0337915,-0.304138),(-0.574484,-0.0337932),(0.270344,0.642071),(-0.10138,1.14897),(0.067585,0.574484),(0.405516,2.02759),(-0.0337944,0.304138),(-0.337931,0.844829),(-0.135172,0.135172),(-0.135174,-0.74345),(-0.135174,0.101379),(0.101378,0.135173),(0.168967,-0.608277),(-0.0337934,-2.06138),(-0.236552,-0.337932),(0.168966,0.439311),(0.10138,0.236552),(-0.236554,0.135173),(0,-0.371725),(0.236551,-0.202759),(-0.337934,0.270345),(0.473104,-0.0337932),(0.202761,-0.202759),(0.405519,-0.337932),(-0.0675879,0.709656),(-0.236552,-0.64207),(-0.101379,-0.64207),(-0.000000953674,0.709657),(0.337933,-0.270345),(-0.000000953674,-0.236552),(0.168967,0.439311),(-0.202759,0.236552),(-0.270346,-0.405518),(-0.0675869,-0.64207),(0.304139,0.202759),(0.00000190735,-0.439312),(0,-0.10138),(-0.10138,-0.236552),(-0.270346,-0.371725),(-0.0675869,-0.54069),(-0.405519,-0.608277),(0.067585,0.506897),(0.304137,0.135173),(0.0675869,-0.10138),(0.371724,0.608277),(-0.168967,1.31793),(-0.811037,1.62207),(-0.74345,1.28414),(-0.439312,0.337932),(-0.506899,0.540691),(0.135174,-0.878622),(0.337934,-1.14897),(0.439312,-1.01379),(0.405519,-0.675864),(-0.168967,-0.574484),(0.473105,-0.371725),(0.236553,0.304139),(0.0675831,0.574484),(0.101379,-0.371725),(-0.0337934,-0.878622),(0.135172,-0.84483),(0,0.135173),(-0.20276,0.337932),(0.135172,0.0675864),(-0.405519,0.405519),(0.473104,0.912416),(0.101379,0.844829),(-0.101379,0.236552),(0.236551,0.540691),(-0.168965,-0.101379),(0.0675869,-1.08138),(-0.0337934,-1.01379),(0.135171,0.777243),(-0.168967,-0.439311),(-0.168966,-0.0675864),(-0.405521,0.878622),(-0.912417,0.946209),(0.202759,0.135173),(0.101379,-0.371725),(-0.506897,-0.168966),(-0.642071,0.135173),(-1.18276,0.202759),(-0.94621,-0.608277),(-0.202759,0.168966)");

    negev.bullets = 150;
    negev.reloadTime = 5.7;
    negev.rpm = 800;
    negev.id = WEAPON_NEGEV;
    negev.recoil_pattern = ParseToVector("(0.946209,0.101379),(0.135173,0),(0.608277,-0.304138),(1.52069,0.236552),(1.68966,-0.135173),(1.75724,-0.0337932),(0.540691,0.0337932),(0.574484,0.202759),(0.878622,-0.168966),(0.202759,0.0675863),(0.439312,-0.0675863),(0.236552,0),(0,0),(0,0.101379),(-0.0337934,0),(0.405519,-0.0675863),(0.168966,-0.0337932),(0,0),(-0.304138,0.0675863),(-0.270346,-0.0337932),(0.371725,0.0675863),(-0.10138,-0.101379),(-0.0337934,0.0337932),(0,0),(0,0),(0.236553,-0.135173),(0.439311,0.337932),(-0.202761,-0.270345),(-0.0337934,0.101379),(0,0),(0.202759,0),(0.20276,-0.101379),(0.337931,0.0675863),(0.0337934,0.0337932),(-0.10138,0.0337932),(0,0),(-0.202759,0),(-0.202759,-0.202759),(-0.10138,0.135173),(0.0337934,0.0337932),(0,-0.202759),(0.337932,0.0675863),(-0.0337925,0.101379),(-0.202759,-0.0337932),(0,0),(0,0),(0,0),(-0.0675869,0.0675863),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0.0337934,-0.101379),(-0.405518,0.0675863),(-0.777244,-0.0337932),(-0.675864,0.0337932),(-0.405519,0),(0.236552,-0.0675863),(0.777242,0.0675863),(0.236552,-0.0337932),(-0.0675869,0),(0.101378,0.0337932),(0.506898,-0.0337932),(-0.0337925,0.0337932),(-0.0675869,0.135173),(-0.20276,-0.135173),(0,0),(0.236551,-0.0337932),(0.337933,0.168966),(-0.0337934,-0.101379),(-0.236552,0),(-0.20276,-0.0337932),(0,0),(0,0),(0,0),(0,0),(0.0675869,-0.0675863),(0.0337934,0.0675863),(0.135173,0),(0.337933,0.168966),(-0.0337934,-0.101379),(0,0),(-0.337932,-0.236552),(0.608276,0.135173),(0.405518,0.0337932),(-0.101379,0),(-0.0675869,0.0337932),(-0.236552,0),(0.20276,0.0337932),(-0.101379,0),(-0.371725,0.0337932),(-0.0337934,-0.0675863),(0.0337915,-0.0337932),(0.439312,0.0675863),(0.0337934,0),(-0.0675869,-0.0337932),(-0.337933,0),(0.101379,-0.0337932),(0.202759,0.0337932),(0,0),(-0.236552,0),(0,0),(0.0337915,-0.0337932),(0.236551,0.0337932),(0,0),(-0.0337934,0),(-0.20276,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0,0),(0.135174,0),(0.135172,0.0675863),(-0.574483,-0.168966),(-1.21655,0.168966),(-0.337931,0.0337932),(-0.168966,0),(-0.371725,-0.135173),(0.405519,0.135173),(0.743451,-0.135173),(0.10138,0),(0.135172,0),(0.473104,0.135173),(0,-0.168966),(0,0),(0,0),(-0.0337934,0),(0,0),(0,0),(0,0),(0.202759,0.0675863),(-0.0675859,0),(-0.168965,0),(-0.439312,0.0337932),(-1.04759,-0.0337932),(-1.38552,-0.101379),(-0.743448,0.101379)");

    cz75a.bullets = 12;
    cz75a.reloadTime = 2.8;
    cz75a.rpm = 600;
    cz75a.id = WEAPON_CZ75A;
    cz75a.recoil_pattern = ParseToVector("(0.0337932,0.0337932),(0.777243,0.675863),(0.878623,-0.675863),(-0.304139,-1.25035),(2.36552,0.135173),(-0.371725,0.574484),(1.38552,1.89242),(0.337932,1.31793),(2.39931,-0.506898),(0.675865,0.371725),(0.304137,-0.202759)");

    all->allWeapons[WEAPON_SCAR20] = scar;
    all->allWeapons[WEAPON_G3SG1] = g3sg1;
    all->allWeapons[WEAPON_M4A1] = m4a1;
    all->allWeapons[WEAPON_M4A1_SILENCER] = m4a1s;
    all->allWeapons[WEAPON_AUG] = aug;
    all->allWeapons[WEAPON_FAMAS] = famas;
    all->allWeapons[WEAPON_AK47] = ak47;
    all->allWeapons[WEAPON_GALILAR] = galilar;
    all->allWeapons[WEAPON_SG556] = sg556;
    all->allWeapons[WEAPON_MP5SD] = mp5;
    all->allWeapons[WEAPON_MAC10] = mac10;
    all->allWeapons[WEAPON_MP7] = mp7;
    all->allWeapons[WEAPON_MP9] = mp9;
    all->allWeapons[WEAPON_UMP45] = ump45;
    all->allWeapons[WEAPON_P90] = p90;
    all->allWeapons[WEAPON_BIZON] = ppbizon;
    all->allWeapons[WEAPON_M249] = m249;
    all->allWeapons[WEAPON_NEGEV] = negev;
    all->allWeapons[WEAPON_CZ75A] = cz75a;
}

bool IsInGame() {
    DWORD localPlayer = readMem<DWORD>(client_base_address + hazedumper::signatures::dwLocalPlayer);

    int currHealth = readMem<int>(localPlayer + hazedumper::netvars::m_iHealth);

    //if spawned in (or in aimbotz)
    if (currHealth <= 100 && currHealth >= 1 || currHealth == 1337) {
        return true;
    }
    return false;
}

bool IsAmmoZero() {
    DWORD currAmmo = readMem<DWORD>(client_base_address + hazedumper::signatures::dwRadarBase);

    if (currAmmo == NULL) {
        return false;
    }

    currAmmo += 0xC0;
    currAmmo = Dereference(currAmmo, game_handle);
    
    if (currAmmo == NULL) {
        return false;
    }

    currAmmo += 0x40;
    currAmmo = Dereference(currAmmo, game_handle);
    if (currAmmo == NULL) {
        return false;
    }
    currAmmo += 0x64;
    currAmmo = Dereference(currAmmo, game_handle);

   // printf("currAmmo: %d\n", currAmmo);
    if (currAmmo == 0) {
        return true;
    }
    return false;
}

bool IsInReload() {
    DWORD localPlayer = readMem<DWORD>(client_base_address + hazedumper::signatures::dwLocalPlayer);
    
    bool isInReload = readMem<bool>(localPlayer + hazedumper::netvars::m_bInReload);

    printf("IsInReload: %d\n", isInReload);

    return true;
}


