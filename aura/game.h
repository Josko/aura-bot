/*

   Copyright [2010] [Josko Nikolic]

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

#include "gameslot.h"

//
// CGame
//

class CTCPServer;
class CGameProtocol;
class CPotentialPlayer;
class CGamePlayer;
class CMap;
class CIncomingJoinPlayer;
class CIncomingAction;
class CIncomingChatPlayer;
class CIncomingMapSize;
class CDBBan;
class CDBGame;
class CDBGamePlayer;
class CStats;
class CStatsDOTA;
class CIRC;

class CGame
{
public:
	CAura *m_Aura;

protected:
	CTCPServer *m_Socket;				// listening socket
	CDBBan *m_DBBanLast;				// last ban for the !banlast command - this is a pointer to one of the items in m_DBBans
	vector<CDBBan *> m_DBBans;			// vector of potential ban data for the database
	CDBGame *m_DBGame;				// potential game data for the database	
	CStats *m_Stats;				// class to keep track of game stats such as kills/deaths/assists in dota
	uint32_t m_GameID;				// GameID stored in the database
	CGameProtocol *m_Protocol;			// game protocol
	vector<CGameSlot> m_Slots;			// vector of slots
	vector<CPotentialPlayer *> m_Potentials;	// vector of potential players (connections that haven't sent a W3GS_REQJOIN packet yet)
	vector<CDBGamePlayer *> m_DBGamePlayers;	// vector of potential gameplayer data for the database
	vector<CGamePlayer *> m_Players;		// vector of players
	queue<CIncomingAction *> m_Actions;		// queue of actions to be sent
	vector<string> m_Reserved;			// vector of player names with reserved slots (from the !hold command)
	set<string> m_IgnoredNames;			// set of player names to NOT print ban messages for when joining because they've already been printed
	CMap *m_Map;					// map data
	bool m_Exiting;					// set to true and this class will be deleted next update
	bool m_Saving;					// if we're currently saving game data to the database
	uint16_t m_HostPort;				// the port to host games on
	unsigned char m_GameState;			// game state, public or private
	unsigned char m_VirtualHostPID;			// host's PID
	vector<unsigned char> m_FakePlayers;		// the fake player's PIDs (if present)
	unsigned char m_GProxyEmptyActions;
	string m_GameName;				// game name
	string m_LastGameName;				// last game name (the previous game name before it was rehosted)
	string m_VirtualHostName;			// host's name
	string m_OwnerName;				// name of the player who owns this game (should be considered an admin)
	string m_CreatorName;				// name of the player who created this game
	string m_CreatorServer;				// battle.net server the player who created this game was on	
	string m_KickVotePlayer;			// the player to be kicked with the currently running kick vote
	string m_HCLCommandString;			// the "HostBot Command Library" command string, used to pass a limited amount of data to specially designed maps
	uint32_t m_RandomSeed;				// the random seed sent to the Warcraft III clients
	uint32_t m_HostCounter;				// a unique game number
	uint32_t m_Latency;				// the number of ms to wait between sending action packets (we queue any received during this time)
	uint32_t m_SyncLimit;				// the maximum number of packets a player can fall out of sync before starting the lag screen
	uint32_t m_SyncCounter;				// the number of actions sent so far (for determining if anyone is lagging)
	uint32_t m_GameTicks;				// ingame ticks
	uint32_t m_CreationTime;			// GetTime when the game was created
	uint32_t m_LastPingTime;			// GetTime when the last ping was sent
	uint32_t m_LastRefreshTime;			// GetTime when the last game refresh was sent
	uint32_t m_LastDownloadTicks;			// GetTicks when the last map download cycle was performed
	uint32_t m_DownloadCounter;			// # of map bytes downloaded in the last second
	uint32_t m_LastDownloadCounterResetTicks;	// GetTicks when the download counter was last reset
	uint32_t m_LastCountDownTicks;			// GetTicks when the last countdown message was sent
	uint32_t m_CountDownCounter;			// the countdown is finished when this reaches zero
	uint32_t m_StartedLoadingTicks;			// GetTicks when the game started loading
	uint32_t m_StartPlayers;			// number of players when the game started
	uint32_t m_LastLagScreenResetTime;		// GetTime when the "lag" screen was last reset
	uint32_t m_LastActionSentTicks;			// GetTicks when the last action packet was sent
	uint32_t m_LastActionLateBy;			// the number of ticks we were late sending the last action packet by
	uint32_t m_StartedLaggingTime;			// GetTime when the last lag screen started
	uint32_t m_LastLagScreenTime;			// GetTime when the last lag screen was active (continuously updated)
	uint32_t m_LastReservedSeen;			// GetTime when the last reserved player was seen in the lobby
	uint32_t m_StartedKickVoteTime;			// GetTime when the kick vote was started
	uint32_t m_GameOverTime;			// GetTime when the game was over
	uint32_t m_LastPlayerLeaveTicks;		// GetTicks when the most recent player left the game
	bool m_SlotInfoChanged;				// if the slot info has changed and hasn't been sent to the players yet (optimization)
	bool m_Locked;					// if the game owner is the only one allowed to run game commands or not
	bool m_RefreshError;				// if the game had a refresh error
	bool m_MuteAll;					// if we should stop forwarding ingame chat messages targeted for all players or not
	bool m_MuteLobby;				// if we should stop forwarding lobby chat messages
	bool m_CountDownStarted;			// if the game start countdown has started or not
	bool m_GameLoading;				// if the game is currently loading or not
	bool m_GameLoaded;				// if the game has loaded or not
	bool m_Lagging;					// if the lag screen is active or not

public:
	CGame( CAura *nAura, CMap *nMap, uint16_t nHostPort, unsigned char nGameState, string &nGameName, string &nOwnerName, string &nCreatorName, string &nCreatorServer );
	~CGame( );

	uint16_t GetHostPort( )					{ return m_HostPort; }
	unsigned char GetGameState( )				{ return m_GameState; }
	unsigned char GetGProxyEmptyActions( )			{ return m_GProxyEmptyActions; }
	string GetGameName( )					{ return m_GameName; }
	string GetLastGameName( )				{ return m_LastGameName; }
	string GetVirtualHostName( )				{ return m_VirtualHostName; }
	string GetOwnerName( )					{ return m_OwnerName; }
	string GetCreatorName( )				{ return m_CreatorName; }
	string GetCreatorServer( )				{ return m_CreatorServer; }
	uint32_t GetHostCounter( )				{ return m_HostCounter; }
	uint32_t GetLastLagScreenTime( )			{ return m_LastLagScreenTime; }
	bool GetLocked( )					{ return m_Locked; }
	bool GetCountDownStarted( )				{ return m_CountDownStarted; }
	bool GetGameLoading( )					{ return m_GameLoading; }
	bool GetGameLoaded( )					{ return m_GameLoaded; }
	bool GetLagging( )					{ return m_Lagging; }

	void SetExiting( bool nExiting )			{ m_Exiting = nExiting; }
	void SetRefreshError( bool nRefreshError )		{ m_RefreshError = nRefreshError; }

	uint32_t GetNextTimedActionTicks( );
	uint32_t GetSlotsOccupied( );
	uint32_t GetSlotsOpen( );
	uint32_t GetNumPlayers( );
	uint32_t GetNumHumanPlayers( );
	string GetDescription( );
	string GetPlayers( );

	// processing functions

	unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	bool Update( void *fd, void *send_fd );
	void UpdatePost( void *send_fd );

	// generic functions to send packets to players

	inline void Send( CGamePlayer *player, const BYTEARRAY &data );
	inline void Send( unsigned char PID, const BYTEARRAY &data );
	inline void Send( const BYTEARRAY &PIDs, const BYTEARRAY &data );
	inline void SendAll( const BYTEARRAY &data );

	// functions to send packets to players

	void SendChat( unsigned char fromPID, CGamePlayer *player, const string &message );
	void SendChat( unsigned char fromPID, unsigned char toPID, const string &message );
	void SendChat( CGamePlayer *player, const string &message );
	void SendChat( unsigned char toPID, const string &message );
	void SendAllChat( unsigned char fromPID, const string &message );
	void SendAllChat( const string &message );
	inline void SendAllSlotInfo( );
	inline void SendVirtualHostPlayerInfo( CGamePlayer *player );
	inline void SendFakePlayerInfo( CGamePlayer *player );
	inline void SendAllActions( );

	// events
	// note: these are only called while iterating through the m_Potentials or m_Players vectors
	// therefore you can't modify those vectors and must use the player's m_DeleteMe member to flag for deletion

	void EventPlayerDeleted( CGamePlayer *player );
	void EventPlayerDisconnectTimedOut( CGamePlayer *player );
	void EventPlayerDisconnectSocketError( CGamePlayer *player );
	void EventPlayerDisconnectConnectionClosed( CGamePlayer *player );
	void EventPlayerJoined( CPotentialPlayer *potential, CIncomingJoinPlayer *joinPlayer );
	void EventPlayerLeft( CGamePlayer *player, uint32_t reason );
	void EventPlayerLoaded( CGamePlayer *player );
	void EventPlayerAction( CGamePlayer *player, CIncomingAction *action );
	void EventPlayerKeepAlive( );
	void EventPlayerChatToHost( CGamePlayer *player, CIncomingChatPlayer *chatPlayer );
	bool EventPlayerBotCommand( CGamePlayer *player, string &command, string &payload );
	void EventPlayerChangeTeam( CGamePlayer *player, unsigned char team );
	void EventPlayerChangeColour( CGamePlayer *player, unsigned char colour );
	void EventPlayerChangeRace( CGamePlayer *player, unsigned char race );
	void EventPlayerChangeHandicap( CGamePlayer *player, unsigned char handicap );
	void EventPlayerDropRequest( CGamePlayer *player );
	void EventPlayerMapSize( CGamePlayer *player, CIncomingMapSize *mapSize );
	void EventPlayerPongToHost( CGamePlayer *player, uint32_t pong );

	// these events are called outside of any iterations
	
	inline void EventGameStarted( );
	inline void EventGameLoaded( );

	// other functions

	unsigned char GetSIDFromPID( unsigned char PID );
	CGamePlayer *GetPlayerFromPID( unsigned char PID );
	CGamePlayer *GetPlayerFromSID( unsigned char SID );
	CGamePlayer *GetPlayerFromName( string name, bool sensitive );
	uint32_t GetPlayerFromNamePartial( string name, CGamePlayer **player );
	CGamePlayer *GetPlayerFromColour( unsigned char colour );
	inline unsigned char GetNewPID( );
	inline unsigned char GetNewColour( );
	BYTEARRAY GetPIDs( );
	BYTEARRAY GetPIDs( unsigned char excludePID );
	inline unsigned char GetHostPID( );
	inline unsigned char GetEmptySlot( bool reserved );
	inline unsigned char GetEmptySlot( unsigned char team, unsigned char PID );
	void SwapSlots( unsigned char SID1, unsigned char SID2 );
	void OpenSlot( unsigned char SID, bool kick );
	void CloseSlot( unsigned char SID, bool kick );
	void ComputerSlot( unsigned char SID, unsigned char skill, bool kick );
	void ColourSlot( unsigned char SID, unsigned char colour );
	void OpenAllSlots( );
	void CloseAllSlots( );
	void ShuffleSlots( );
	void AddToSpoofed( const string &server, const string &name, bool sendMessage );
	void AddToReserved( string name );
	bool IsOwner( string name );
	bool IsReserved( string name );
	bool IsDownloading( );
	void StartCountDown( bool force );
	void StopPlayers( const string &reason );
	inline void StopLaggers( const string &reason );
	inline void CreateVirtualHost( );
	inline void DeleteVirtualHost( );
	inline void CreateFakePlayer( );
	inline void DeleteFakePlayers( );
	inline bool IsAdmin( string name );
};

#endif
