/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

#pragma once

#include "netdlg.h"

#include <deque>
#include <string>

struct GlobalPacketType;
class IPXAddressClass;

enum {
	NET2_CONSOLE_MESSAGE_CAP = 12,		// Chat lines the console lobby keeps.
	NET2_RESPONSE_JOIN_ENDED = 2,		// The joined game went away or could not start.
};

extern int CurGame;
extern int _netresponse;
extern JoinStateType JoinState;
extern char SerialNumber[23];
extern bool Net2IsGameListActive;
extern bool Net2GameStarted;
extern bool Net2IsConsole;			// The console screens are up in place of the dialogs.
extern std::string Net2ConsoleNotice;	// Why a join ended, for the console screens to show.
extern std::deque<std::string> Net2ConsoleMessages;	// The lobby's chat and notices, newest last.
extern int RulesID;
extern int ArtID;
extern int AIID;

int Net2FirstFreeColor(int reqcolor, int index);
bool Net2Callback(void);
void Net2DisplayUsers(void);
bool Net2Init_Network(void);
void Net2EncodeGameopt(char *out);
void Net2SetAccept(char *who, int status);
int Net2GetAccept(char *who);
int Net2SetHouseAndColor(char *who, int house, int color);
bool Decrypt_Serial(char *buffer);
bool Net2Remote_Connect(void);
bool Net2Console_Remote_Connect(void);
int Request_To_Join(int join_index);
void Unjoin_Game(int game_index);
void Send_Join_Queries(int gamenow, int playernow, int chatnow, int init = 0);
void Get_Join_Responses(void);
void Net2Start_Hosted_Game(void);
void Net2Start_Joined_Game(void);
bool Process_Global_Packet(GlobalPacketType *packet, IPXAddressClass *address);
void Net2DisplayGameList(void);
void Net2ServiceGameList(void);
