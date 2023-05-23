#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <vector>
#include <thread>
#include <atomic>
#include "csgo.hpp"
#include "func.h"
#include "const.h"


//skinchanger vars

const int procId = GetProcessId(L"csgo.exe");
const HANDLE game_handle = GetGameHandle(procId);

//addresses
const uint32_t client_base_address = GetProcessBaseAddress(procId, L"client.dll");
const uint32_t playerAddressPtr = (client_base_address + hazedumper::signatures::dwLocalPlayer);
const uint32_t viewMatrixAddr = (client_base_address + hazedumper::signatures::dwViewMatrix);
const uint32_t playerAddr = Dereference(playerAddressPtr, game_handle);
const uint32_t engine_addr = GetProcessBaseAddress(procId, L"engine.dll");
const uint32_t client_addr = engine_addr + hazedumper::signatures::dwClientState;
const uint32_t deref_client = Dereference(client_addr, game_handle);
const uint32_t pitch_addr = deref_client + 0x4D90;
const uint32_t yaw_addr = deref_client + 0x4D94;


int main()
{

    AsciiGen();

    if ((procId == -1) || (game_handle == NULL)) {
        printf("Failed to get process\n");
    }


    //initialize weapons up here

    //read config for weapon initialization
    Weapon ak;
    ak.bullets = 30;
    ak.rpm = 600;

    AllWeapons All;
    InitWeaponStruct(&All);
    //--------------------------

    //printf("Player Addr - %.4x\n", playerAddr);

    //loop variables
    bool run = true;
    bool aimbot = false;
    bool skinchanger = false;
    std::atomic<bool> skinExitFlat(false);
    bool recoilComp = false;
    bool gotten = false;
    bool walls = false;

    POINT mouse_pos;
    int count = 0;
    int curr_count = 0;
    Angles curr_pos;
    bool isMouseDown = false;

    std::thread skinT;
    while (run) {
        //loop setters
        if (GetAsyncKeyState(VK_NUMPAD0) & 1) {
            printf("Exitting\n");
            CloseHandle(game_handle);
            run = false;
            std::terminate();
        }
        if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
            skinchanger = !skinchanger;
            printf("Skinchanger ");
            Status(skinchanger);

            if (skinchanger) {
                //FILE*
                skinT = std::thread{ SkinChanger };
            }
            else {
                skinT.join();
                //printf("skinthread joined successfully\n");
            }
         }
        if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
            aimbot = !aimbot;
            printf("Aimbot ");
            Status(aimbot);
        }
        if (GetAsyncKeyState(VK_NUMPAD3) & 1) {
            recoilComp = !recoilComp;
            printf("Recoil Compensator ");
            Status(recoilComp);
        }
        
        //actual code
        if (skinchanger) {
            //load skins config

            //change skins according to config (in new thread)
            //skinchanger = false;
        }
               
        if (aimbot) {
            //aimbot
            if (GetAsyncKeyState(VK_END) & 1) {
                //worldtoview testing

                Vector2 screen = { 0,0 };
                uint32_t entityList = client_base_address + hazedumper::signatures::dwEntityList;
                uint32_t entAddr = DynamicAddressWalker(game_handle, client_base_address, { hazedumper::signatures::dwEntityList, 0x20 });
                uint32_t isEnt = 0;
                player pl;
                uint32_t offset = 0x20;
                ReadProcessMemory(game_handle, (LPVOID)entAddr, &pl, sizeof(pl), NULL);

                do {
                    printf("Player %.2x: %f,%f,%f\n", offset, pl.xyz.x, pl.xyz.y, pl.xyz.z);


                    ReadProcessMemory(game_handle, (LPVOID)entAddr, &pl, sizeof(pl), NULL);
                    offset += 0x10;
                    entAddr = DynamicAddressWalker(game_handle, client_base_address, { hazedumper::signatures::dwEntityList, offset });
                } while (entAddr != 0);
                //std::cout << std::hex << entAddr << " --- WalkTest\n";

                float player_height = 65.0;
                float viewMatrix[16];
                int screenWidth = 1920;
                int screenHeight = 1080;

                INPUT input;
                //setup move input
                for (int i = 0; i < 5; i++) {
                    player p;
                    ReadProcessMemory(game_handle, (LPCVOID)viewMatrixAddr, &viewMatrix, sizeof(viewMatrix), NULL);
                    ReadProcessMemory(game_handle, (LPCVOID)entAddr, &p, sizeof(p), NULL);
                    Vector3 world = { p.xyz.x,p.xyz.y,p.xyz.z + player_height };
                    WorldToScreen(world, &screen, viewMatrix, screenWidth, screenHeight);

                    GetCursorPos(&mouse_pos);
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
                    input.mi.dx = screen.x - mouse_pos.x + 3;
                    input.mi.dy = screen.y - mouse_pos.y;
                    SendInput(1, &input, sizeof(input));

                    Sleep(5);
                }

                //setup click input
                GetCursorPos(&mouse_pos);
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
                input.mi.dx = mouse_pos.x;
                input.mi.dy = mouse_pos.y;
                SendInput(1, &input, sizeof(input));
                //printf("Mouse- (%d, %d)\n", screen.x - mouse_pos.x, screen.y - mouse_pos.y);
            }
        }
        if (recoilComp) {
            if (GetAsyncKeyState(VK_NUMPAD5) & 1) {
                printf("Reset\n");
                count = 0;
            }
            SHORT keystate = GetAsyncKeyState(VK_LBUTTON);

            if (keystate & 0x8000) {
                
                isMouseDown = true;
            }
            else {
                isMouseDown = false;
            }

            int currWeaponId = GetCurrentWeapon();
            //printf("GetAmmo: %d\n", All.allWeapons[currWeaponId].bullets);
            
            //code for getting recoil values to subtract for recoilDeltas
            if (5000 < All.allWeapons[currWeaponId].bullets) {
                
                //printf("%d\n", keystate);
                if (isMouseDown) {
                    printf("Test\n");
                    GetAngles(curr_pos, pitch_addr, game_handle);
                    All.allWeapons[currWeaponId].abs_recoil_pattern.push_back(curr_pos);

                    if (count) {
                        //delta calulation
                        Angles delta;
                        delta.pitch = All.allWeapons[currWeaponId].abs_recoil_pattern.at(count).pitch - All.allWeapons[currWeaponId].abs_recoil_pattern.at(count - 1).pitch;
                        delta.yaw = All.allWeapons[currWeaponId].abs_recoil_pattern.at(count).yaw - All.allWeapons[currWeaponId].abs_recoil_pattern.at(count - 1).yaw;

                        printf("Delta(%f, %f) -- Count: %d\n", delta.pitch, delta.yaw, count);
                        //std::cout << "Delta(" << delta.x << ',' << delta.y << ")\n";
                        All.allWeapons[currWeaponId].recoil_pattern.push_back(delta);
                    }
                    else {
                        printf("First captured\n");
                    }
                    count++;
                }
                Sleep((1000 / (All.allWeapons[currWeaponId].rpm / 60)) * 10);
            }
            else {
                //printf("Test\n");
                //if window in focus**
                if (gotten) {
                    for (int i = 0; i < All.allWeapons[currWeaponId].recoil_pattern.size(); i++) {
                        std::cout << "(" << All.allWeapons[currWeaponId].recoil_pattern.at(i).pitch << "," << All.allWeapons[currWeaponId].recoil_pattern.at(i).yaw << "),";
                    }
                    printf("\n\n");
                    gotten = true;
                }
                
                //testing the recoil using deltas
                if (isMouseDown && IsInGame() && !IsAmmoZero() /* && ActiveGameState()  && !IsInReload()*/ ) {
                    curr_count++;

                    if (!IsValidRecoilIndex(currWeaponId)) {
                        continue;
                    }
                    if (curr_count == All.allWeapons[currWeaponId].bullets - 1) {
                        curr_count = 0;
                        Sleep(All.allWeapons[currWeaponId].reloadTime * 1000);
                        continue;
                    }

                    GetAngles(curr_pos, pitch_addr, game_handle);
                    //get current pos so deltas can be subtracted
                    Angles new_loc = curr_pos;
                    new_loc.pitch += All.allWeapons[currWeaponId].recoil_pattern.at(curr_count - 1).pitch;
                    new_loc.yaw += All.allWeapons[currWeaponId].recoil_pattern.at(curr_count - 1).yaw;

                    if (curr_count != 1) {
                        WriteProcessMemory(game_handle, (LPVOID)pitch_addr, &new_loc, sizeof(new_loc), NULL);

                    }

                    Sleep(1000 / (All.allWeapons[currWeaponId].rpm / 60));
                }
                else {
                    curr_count = 0;
                }
            }
        }
        
        //debug bind
        if (GetAsyncKeyState(VK_NUMPAD4) & 1) {
            //printf("Client Addr = %x -- (offset 108?)", client_addr);
            //gameState = engine + 0x785914

            /*
            DWORD weaponEntity = readMem<DWORD>(localPlayer + m_hMyWeapons + i * 0x4) & 0xfff;
            weaponEntity = readMem<DWORD>(clientBase + dwEntityList + (weaponEntity - 1) * 0x10);
            int accountID = readMem<int>(weaponEntity + m_OriginalOwnerXuidLow);
            if (weaponEntity == 0) { continue; }

            short weaponID = readMem<int>(weaponEntity + m_iItemDefinitionIndex);
            */

            DWORD currAmmo = readMem<DWORD>(client_base_address + hazedumper::signatures::dwRadarBase);

            //currAmmo = Dereference(currAmmo, game_handle);

            currAmmo += 0xC0;
            currAmmo = Dereference(currAmmo, game_handle);

            currAmmo += 0x40;
            currAmmo = Dereference(currAmmo, game_handle);
            currAmmo += 0x64;
            currAmmo = Dereference(currAmmo, game_handle);
            printf("CurrAmmo: %d\n", currAmmo);

            

            //printf("CurrAmmo: %x\n", currAmmo);

            DWORD localPlayer = readMem<DWORD>(client_base_address + hazedumper::signatures::dwLocalPlayer);
            DWORD a = readMem<DWORD>(client_base_address + hazedumper::signatures::dwClientState);
            printf("ClientState: %x\n", client_addr);

            DWORD weaponEntity = readMem<DWORD>(localPlayer + hazedumper::netvars::m_hMyWeapons) & 0xfff;
            weaponEntity = readMem<DWORD>(client_base_address + hazedumper::signatures::dwEntityList + (weaponEntity - 1) * 0x10);

            printf("WeaponEntity: %x\n", weaponEntity);

            printf("PlayerAddr: %x\n", playerAddr);

            //printf("AmmoPtrTest: %x\n", (test + 0xA4));
            //uint32_t derefWeaponEnt


        }
    }

}
