#ifndef CONST_H
#define CONST_H

#include <Windows.h>
#include <atomic>

//initialize weapons up here

//--------------------------

//globals
extern const int procId;
extern const HANDLE game_handle;

//addresses
extern const uint32_t client_base_address;
extern const uint32_t playerAddressPtr;
extern const uint32_t viewMatrixAddr;
extern const uint32_t playerAddr;
extern const uint32_t engine_addr;
extern const uint32_t client_addr;
extern const uint32_t deref_client;
extern const uint32_t pitch_addr;
extern const uint32_t yaw_addr;

//thread shit

#endif
