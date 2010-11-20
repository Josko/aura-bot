/*

   Copyright [2008] [Trevor Hogan]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   CODE PORTED FROM THE ORIGINAL GHOST PROJECT: http://ghost.pwner.org/

*/

#ifndef GAME_H
#define GAME_H

//
// CGame
//

class CDBBan;
class CDBGame;
class CDBGamePlayer;
class CStats;
class CIRC;

class CGame : public CBaseGame
{
protected:
	CDBBan *m_DBBanLast;				// last ban for the !banlast command - this is a pointer to one of the items in m_DBBans
	vector<CDBBan *> m_DBBans;			// vector of potential ban data for the database
	CDBGame *m_DBGame;				// potential game data for the database
	vector<CDBGamePlayer *> m_DBGamePlayers;	// vector of potential gameplayer data for the database
	CStats *m_Stats;				// class to keep track of game stats such as kills/deaths/assists in dota
	uint32_t m_GameID;				// GameID stored in the database

public:
	CGame( CGHost *nGHost, CMap *nMap, uint16_t nHostPort, unsigned char nGameState, string &nGameName, string &nOwnerName, string &nCreatorName, string &nCreatorServer );
	virtual ~CGame( );

	virtual bool Update( void *fd, void *send_fd );
	virtual void EventPlayerDeleted( CGamePlayer *player );
	virtual void EventPlayerAction( CGamePlayer *player, CIncomingAction *action );
	virtual bool EventPlayerBotCommand( CGamePlayer *player, string &command, string &payload );
	virtual void EventGameStarted( );
	virtual void SaveGameData( );
};

#endif
