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

#ifndef GHOSTDB_H
#define GHOSTDB_H

//
// CGHostDB
//

class CBaseCallable;
class CCallableAdminCount;
class CCallableAdminCheck;
class CCallableAdminAdd;
class CCallableAdminRemove;
class CCallableAdminList;
class CCallableBanCount;
class CCallableBanCheck;
class CCallableBanAdd;
class CCallableBanRemove;
class CCallableBanList;
class CCallableGameAdd;
class CCallableGamePlayerAdd;
class CCallableGamePlayerSummaryCheck;
class CCallableDotAGameAdd;
class CCallableDotAPlayerAdd;
class CCallableDotAPlayerSummaryCheck;
class CDBBan;
class CDBGame;
class CDBGamePlayer;
class CDBGamePlayerSummary;
class CDBDotAPlayerSummary;

typedef pair<uint32_t,string> VarP;

class CGHostDB
{
protected:
	bool m_HasError;
	string m_Error;

public:
	CGHostDB( CConfig *CFG );
	virtual ~CGHostDB( );

	bool HasError( )			{ return m_HasError; }
	string GetError( )			{ return m_Error; }

	virtual void RecoverCallable( CBaseCallable *callable );

	// standard (non-threaded) database functions

	virtual bool Begin( );
	virtual bool Commit( );
	virtual uint32_t AdminCount( const string &server );
	virtual bool AdminCheck( string server, string user );
	virtual bool AdminAdd( string server, string user );
	virtual bool AdminRemove( string server, string user );
	virtual vector<string> AdminList( string server );
	virtual uint32_t BanCount( string server );
	virtual CDBBan *BanCheck( string server, string user, string ip );
	virtual bool BanAdd( string server, string user, string ip, string gamename, string admin, string reason );
	virtual bool BanRemove( string server, string user );
	virtual bool BanRemove( string user );
	virtual vector<CDBBan *> BanList( string server );
	virtual uint32_t GameAdd( string server, string map, string gamename, string ownername, uint32_t duration, uint32_t gamestate, string creatorname, string creatorserver );
	virtual uint32_t GamePlayerAdd( uint32_t gameid, string name, string ip, uint32_t spoofed, string spoofedrealm, uint32_t reserved, uint32_t loadingtime, uint32_t left, string leftreason, uint32_t team, uint32_t colour );
	virtual uint32_t GamePlayerCount( string name );
	virtual CDBGamePlayerSummary *GamePlayerSummaryCheck( string name );
	virtual uint32_t DotAGameAdd( uint32_t gameid, uint32_t winner, uint32_t min, uint32_t sec );
	virtual uint32_t DotAPlayerAdd( uint32_t gameid, uint32_t colour, uint32_t kills, uint32_t deaths, uint32_t creepkills, uint32_t creepdenies, uint32_t assists, uint32_t gold, uint32_t neutralkills, string item1, string item2, string item3, string item4, string item5, string item6, string hero, uint32_t newcolour, uint32_t towerkills, uint32_t raxkills, uint32_t courierkills );
	virtual uint32_t DotAPlayerCount( string name );
	virtual CDBDotAPlayerSummary *DotAPlayerSummaryCheck( string name );
	virtual string FromCheck( uint32_t ip );
	virtual bool FromAdd( uint32_t ip1, uint32_t ip2, string country );

	// threaded database functions

	virtual void CreateThread( CBaseCallable *callable );
	virtual CCallableAdminCount *ThreadedAdminCount( const string &server );
	virtual CCallableAdminCheck *ThreadedAdminCheck( string server, string user );
	virtual CCallableAdminAdd *ThreadedAdminAdd( string server, string user );
	virtual CCallableAdminRemove *ThreadedAdminRemove( string server, string user );
	virtual CCallableAdminList *ThreadedAdminList( string server );
	virtual CCallableBanCount *ThreadedBanCount( string server );
	virtual CCallableBanCheck *ThreadedBanCheck( string server, string user, string ip );
	virtual CCallableBanAdd *ThreadedBanAdd( string server, string user, string ip, string gamename, string admin, string reason );
	virtual CCallableBanRemove *ThreadedBanRemove( string server, string user );
	virtual CCallableBanRemove *ThreadedBanRemove( string user );
	virtual CCallableBanList *ThreadedBanList( string server );
	virtual CCallableGameAdd *ThreadedGameAdd( string server, string map, string gamename, string ownername, uint32_t duration, uint32_t gamestate, string creatorname, string creatorserver );
	virtual CCallableGamePlayerAdd *ThreadedGamePlayerAdd( uint32_t gameid, string name, string ip, uint32_t spoofed, string spoofedrealm, uint32_t reserved, uint32_t loadingtime, uint32_t left, string leftreason, uint32_t team, uint32_t colour );
	virtual CCallableGamePlayerSummaryCheck *ThreadedGamePlayerSummaryCheck( string name );
	virtual CCallableDotAGameAdd *ThreadedDotAGameAdd( uint32_t gameid, uint32_t winner, uint32_t min, uint32_t sec );
	virtual CCallableDotAPlayerAdd *ThreadedDotAPlayerAdd( uint32_t gameid, uint32_t colour, uint32_t kills, uint32_t deaths, uint32_t creepkills, uint32_t creepdenies, uint32_t assists, uint32_t gold, uint32_t neutralkills, string item1, string item2, string item3, string item4, string item5, string item6, string hero, uint32_t newcolour, uint32_t towerkills, uint32_t raxkills, uint32_t courierkills );
	virtual CCallableDotAPlayerSummaryCheck *ThreadedDotAPlayerSummaryCheck( string name );
};

//
// Callables
//

// life cycle of a callable:
//  - the callable is created in one of the database's ThreadedXXX functions
//  - initially the callable is NOT ready (i.e. m_Ready = false)
//  - the ThreadedXXX function normally creates a thread to perform some query and (potentially) store some result in the callable
//  - at the time of this writing all threads are immediately detached, the code does not join any threads (the callable's "readiness" is used for this purpose instead)
//  - when the thread completes it will set m_Ready = true
//  - DO NOT DO *ANYTHING* TO THE CALLABLE UNTIL IT'S READY OR YOU WILL CREATE A CONCURRENCY MESS
//  - THE ONLY SAFE FUNCTION IN THE CALLABLE IS GetReady
//  - when the callable is ready you may access the callable's result which will have been set within the (now terminated) thread

// example usage:
//  - normally you will call a ThreadedXXX function, store the callable in a vector, and periodically check if the callable is ready
//  - when the callable is ready you will consume the result then you will pass the callable back to the database via the RecoverCallable function
//  - the RecoverCallable function allows the database to recover some of the callable's resources to be reused later (e.g. MySQL connections)
//  - note that this will NOT free the callable's memory, you must do that yourself after calling the RecoverCallable function
//  - be careful not to leak any callables, it's NOT safe to delete a callable even if you decide that you don't want the result anymore
//  - you should deliver any to-be-orphaned callables to the main vector in CGHost so they can be properly deleted when ready even if you don't care about the result anymore
//  - e.g. if a player does a stats check immediately before a game is deleted you can't just delete the callable on game deletion unless it's ready

class CBaseCallable
{
protected:
	string m_Error;
	volatile bool m_Ready;
	uint32_t m_StartTicks;
	uint32_t m_EndTicks;

public:
	CBaseCallable( ) : m_Error( ), m_Ready( false ), m_StartTicks( 0 ), m_EndTicks( 0 ) { }
	virtual ~CBaseCallable( ) { }

	virtual void operator( )( ) { }

	virtual void Init( );
	virtual void Close( );

	virtual string GetError( )				{ return m_Error; }
	virtual bool GetReady( )				{ return m_Ready; }
	virtual void SetReady( bool nReady )	{ m_Ready = nReady; }
	virtual uint32_t GetElapsed( )			{ return m_Ready ? m_EndTicks - m_StartTicks : 0; }
};

class CCallableAdminCount : virtual public CBaseCallable
{
protected:
	string m_Server;
	uint32_t m_Result;

public:
	CCallableAdminCount( const string &nServer ) : CBaseCallable( ), m_Server( nServer ), m_Result( 0 ) { }
	virtual ~CCallableAdminCount( );

	virtual string GetServer( )					{ return m_Server; }
	virtual uint32_t GetResult( )				{ return m_Result; }
	virtual void SetResult( uint32_t nResult )	{ m_Result = nResult; }
};

class CCallableAdminCheck : virtual public CBaseCallable
{
protected:
	string m_Server;
	string m_User;
	bool m_Result;

public:
	CCallableAdminCheck( string nServer, string nUser ) : CBaseCallable( ), m_Server( nServer ), m_User( nUser ), m_Result( false ) { }
	virtual ~CCallableAdminCheck( );

	virtual string GetServer( )				{ return m_Server; }
	virtual string GetUser( )				{ return m_User; }
	virtual bool GetResult( )				{ return m_Result; }
	virtual void SetResult( bool nResult )	{ m_Result = nResult; }
};

class CCallableAdminAdd : virtual public CBaseCallable
{
protected:
	string m_Server;
	string m_User;
	bool m_Result;

public:
	CCallableAdminAdd( string nServer, string nUser ) : CBaseCallable( ), m_Server( nServer ), m_User( nUser ), m_Result( false ) { }
	virtual ~CCallableAdminAdd( );

	virtual string GetServer( )				{ return m_Server; }
	virtual string GetUser( )				{ return m_User; }
	virtual bool GetResult( )				{ return m_Result; }
	virtual void SetResult( bool nResult )	{ m_Result = nResult; }
};

class CCallableAdminRemove : virtual public CBaseCallable
{
protected:
	string m_Server;
	string m_User;
	bool m_Result;

public:
	CCallableAdminRemove( string nServer, string nUser ) : CBaseCallable( ), m_Server( nServer ), m_User( nUser ), m_Result( false ) { }
	virtual ~CCallableAdminRemove( );

	virtual string GetServer( )				{ return m_Server; }
	virtual string GetUser( )				{ return m_User; }
	virtual bool GetResult( )				{ return m_Result; }
	virtual void SetResult( bool nResult )	{ m_Result = nResult; }
};

class CCallableAdminList : virtual public CBaseCallable
{
protected:
	string m_Server;
	vector<string> m_Result;

public:
	CCallableAdminList( string nServer ) : CBaseCallable( ), m_Server( nServer ) { }
	virtual ~CCallableAdminList( );

	virtual vector<string> GetResult( )					{ return m_Result; }
	virtual void SetResult( vector<string> nResult )	{ m_Result = nResult; }
};

class CCallableBanCount : virtual public CBaseCallable
{
protected:
	string m_Server;
	uint32_t m_Result;

public:
	CCallableBanCount( string nServer ) : CBaseCallable( ), m_Server( nServer ), m_Result( 0 ) { }
	virtual ~CCallableBanCount( );

	virtual string GetServer( )					{ return m_Server; }
	virtual uint32_t GetResult( )				{ return m_Result; }
	virtual void SetResult( uint32_t nResult )	{ m_Result = nResult; }
};

class CCallableBanCheck : virtual public CBaseCallable
{
protected:
	string m_Server;
	string m_User;
	string m_IP;
	CDBBan *m_Result;

public:
	CCallableBanCheck( string nServer, string nUser, string nIP ) : CBaseCallable( ), m_Server( nServer ), m_User( nUser ), m_IP( nIP ), m_Result( NULL ) { }
	virtual ~CCallableBanCheck( );

	virtual string GetServer( )					{ return m_Server; }
	virtual string GetUser( )					{ return m_User; }
	virtual string GetIP( )						{ return m_IP; }
	virtual CDBBan *GetResult( )				{ return m_Result; }
	virtual void SetResult( CDBBan *nResult )	{ m_Result = nResult; }
};

class CCallableBanAdd : virtual public CBaseCallable
{
protected:
	string m_Server;
	string m_User;
	string m_IP;
	string m_GameName;
	string m_Admin;
	string m_Reason;
	bool m_Result;

public:
	CCallableBanAdd( string nServer, string nUser, string nIP, string nGameName, string nAdmin, string nReason ) : CBaseCallable( ), m_Server( nServer ), m_User( nUser ), m_IP( nIP ), m_GameName( nGameName ), m_Admin( nAdmin ), m_Reason( nReason ), m_Result( false ) { }
	virtual ~CCallableBanAdd( );

	virtual string GetServer( )				{ return m_Server; }
	virtual string GetUser( )				{ return m_User; }
	virtual string GetIP( )					{ return m_IP; }
	virtual string GetGameName( )			{ return m_GameName; }
	virtual string GetAdmin( )				{ return m_Admin; }
	virtual string GetReason( )				{ return m_Reason; }
	virtual bool GetResult( )				{ return m_Result; }
	virtual void SetResult( bool nResult )	{ m_Result = nResult; }
};

class CCallableBanRemove : virtual public CBaseCallable
{
protected:
	string m_Server;
	string m_User;
	bool m_Result;

public:
	CCallableBanRemove( string nServer, string nUser ) : CBaseCallable( ), m_Server( nServer ), m_User( nUser ), m_Result( false ) { }
	virtual ~CCallableBanRemove( );

	virtual string GetServer( )				{ return m_Server; }
	virtual string GetUser( )				{ return m_User; }
	virtual bool GetResult( )				{ return m_Result; }
	virtual void SetResult( bool nResult )	{ m_Result = nResult; }
};

class CCallableBanList : virtual public CBaseCallable
{
protected:
	string m_Server;
	vector<CDBBan *> m_Result;

public:
	CCallableBanList( string nServer ) : CBaseCallable( ), m_Server( nServer ) { }
	virtual ~CCallableBanList( );

	virtual vector<CDBBan *> GetResult( )				{ return m_Result; }
	virtual void SetResult( vector<CDBBan *> nResult )	{ m_Result = nResult; }
};

class CCallableGameAdd : virtual public CBaseCallable
{
protected:
	string m_Server;
	string m_Map;
	string m_GameName;
	string m_OwnerName;
	uint32_t m_Duration;
	uint32_t m_GameState;
	string m_CreatorName;
	string m_CreatorServer;
	uint32_t m_Result;

public:
	CCallableGameAdd( string nServer, string nMap, string nGameName, string nOwnerName, uint32_t nDuration, uint32_t nGameState, string nCreatorName, string nCreatorServer ) : CBaseCallable( ), m_Server( nServer ), m_Map( nMap ), m_GameName( nGameName ), m_OwnerName( nOwnerName ), m_Duration( nDuration ), m_GameState( nGameState ), m_CreatorName( nCreatorName ), m_CreatorServer( nCreatorServer ), m_Result( 0 ) { }
	virtual ~CCallableGameAdd( );

	virtual uint32_t GetResult( )				{ return m_Result; }
	virtual void SetResult( uint32_t nResult )	{ m_Result = nResult; }
};

class CCallableGamePlayerAdd : virtual public CBaseCallable
{
protected:
	uint32_t m_GameID;
	string m_Name;
	string m_IP;
	uint32_t m_Spoofed;
	string m_SpoofedRealm;
	uint32_t m_Reserved;
	uint32_t m_LoadingTime;
	uint32_t m_Left;
	string m_LeftReason;
	uint32_t m_Team;
	uint32_t m_Colour;
	uint32_t m_Result;

public:
	CCallableGamePlayerAdd( uint32_t nGameID, string nName, string nIP, uint32_t nSpoofed, string nSpoofedRealm, uint32_t nReserved, uint32_t nLoadingTime, uint32_t nLeft, string nLeftReason, uint32_t nTeam, uint32_t nColour ) : CBaseCallable( ), m_GameID( nGameID ), m_Name( nName ), m_IP( nIP ), m_Spoofed( nSpoofed ), m_SpoofedRealm( nSpoofedRealm ), m_Reserved( nReserved ), m_LoadingTime( nLoadingTime ), m_Left( nLeft ), m_LeftReason( nLeftReason ), m_Team( nTeam ), m_Colour( nColour ), m_Result( 0 ) { }
	virtual ~CCallableGamePlayerAdd( );

	virtual uint32_t GetResult( )				{ return m_Result; }
	virtual void SetResult( uint32_t nResult )	{ m_Result = nResult; }
};

class CCallableGamePlayerSummaryCheck : virtual public CBaseCallable
{
protected:
	string m_Name;
	CDBGamePlayerSummary *m_Result;

public:
	CCallableGamePlayerSummaryCheck( string nName ) : CBaseCallable( ), m_Name( nName ), m_Result( NULL ) { }
	virtual ~CCallableGamePlayerSummaryCheck( );

	virtual string GetName( )								{ return m_Name; }
	virtual CDBGamePlayerSummary *GetResult( )				{ return m_Result; }
	virtual void SetResult( CDBGamePlayerSummary *nResult )	{ m_Result = nResult; }
};

class CCallableDotAGameAdd : virtual public CBaseCallable
{
protected:
	uint32_t m_GameID;
	uint32_t m_Winner;
	uint32_t m_Min;
	uint32_t m_Sec;
	uint32_t m_Result;

public:
	CCallableDotAGameAdd( uint32_t nGameID, uint32_t nWinner, uint32_t nMin, uint32_t nSec ) : CBaseCallable( ), m_GameID( nGameID ), m_Winner( nWinner ), m_Min( nMin ), m_Sec( nSec ), m_Result( 0 ) { }
	virtual ~CCallableDotAGameAdd( );

	virtual uint32_t GetResult( )				{ return m_Result; }
	virtual void SetResult( uint32_t nResult )	{ m_Result = nResult; }
};

class CCallableDotAPlayerAdd : virtual public CBaseCallable
{
protected:
	uint32_t m_GameID;
	uint32_t m_Colour;
	uint32_t m_Kills;
	uint32_t m_Deaths;
	uint32_t m_CreepKills;
	uint32_t m_CreepDenies;
	uint32_t m_Assists;
	uint32_t m_Gold;
	uint32_t m_NeutralKills;
	string m_Item1;
	string m_Item2;
	string m_Item3;
	string m_Item4;
	string m_Item5;
	string m_Item6;
	string m_Hero;
	uint32_t m_NewColour;
	uint32_t m_TowerKills;
	uint32_t m_RaxKills;
	uint32_t m_CourierKills;
	uint32_t m_Result;

public:
	CCallableDotAPlayerAdd( uint32_t nGameID, uint32_t nColour, uint32_t nKills, uint32_t nDeaths, uint32_t nCreepKills, uint32_t nCreepDenies, uint32_t nAssists, uint32_t nGold, uint32_t nNeutralKills, string nItem1, string nItem2, string nItem3, string nItem4, string nItem5, string nItem6, string nHero, uint32_t nNewColour, uint32_t nTowerKills, uint32_t nRaxKills, uint32_t nCourierKills ) : CBaseCallable( ), m_GameID( nGameID ), m_Colour( nColour ), m_Kills( nKills ), m_Deaths( nDeaths ), m_CreepKills( nCreepKills ), m_CreepDenies( nCreepDenies ), m_Assists( nAssists ), m_Gold( nGold ), m_NeutralKills( nNeutralKills ), m_Item1( nItem1 ), m_Item2( nItem2 ), m_Item3( nItem3 ), m_Item4( nItem4 ), m_Item5( nItem5 ), m_Item6( nItem6 ), m_Hero( nHero ), m_NewColour( nNewColour ), m_TowerKills( nTowerKills ), m_RaxKills( nRaxKills ), m_CourierKills( nCourierKills ), m_Result( 0 ) { }
	virtual ~CCallableDotAPlayerAdd( );

	virtual uint32_t GetResult( )				{ return m_Result; }
	virtual void SetResult( uint32_t nResult )	{ m_Result = nResult; }
};

class CCallableDotAPlayerSummaryCheck : virtual public CBaseCallable
{
protected:
	string m_Name;
	CDBDotAPlayerSummary *m_Result;

public:
	CCallableDotAPlayerSummaryCheck( string nName ) : CBaseCallable( ), m_Name( nName ), m_Result( NULL ) { }
	virtual ~CCallableDotAPlayerSummaryCheck( );

	virtual string GetName( )								{ return m_Name; }
	virtual CDBDotAPlayerSummary *GetResult( )				{ return m_Result; }
	virtual void SetResult( CDBDotAPlayerSummary *nResult )	{ m_Result = nResult; }
};

//
// CDBBan
//

class CDBBan
{
private:
	string m_Server;
	string m_Name;
	string m_IP;
	string m_Date;
	string m_GameName;
	string m_Admin;
	string m_Reason;

public:
	CDBBan( string nServer, string nName, string nIP, string nDate, string nGameName, string nAdmin, string nReason );
	~CDBBan( );

	string GetServer( )		{ return m_Server; }
	string GetName( )		{ return m_Name; }
	string GetIP( )			{ return m_IP; }
	string GetDate( )		{ return m_Date; }
	string GetGameName( )	{ return m_GameName; }
	string GetAdmin( )		{ return m_Admin; }
	string GetReason( )		{ return m_Reason; }
};

//
// CDBGame
//

class CDBGame
{
private:
	uint32_t m_ID;
	string m_Server;
	string m_Map;
	string m_DateTime;
	string m_GameName;
	string m_OwnerName;
	uint32_t m_Duration;

public:
	CDBGame( uint32_t nID, string nServer, string nMap, string nDateTime, string nGameName, string nOwnerName, uint32_t nDuration );
	~CDBGame( );

	uint32_t GetID( )		{ return m_ID; }
	string GetServer( )		{ return m_Server; }
	string GetMap( )		{ return m_Map; }
	string GetDateTime( )	{ return m_DateTime; }
	string GetGameName( )	{ return m_GameName; }
	string GetOwnerName( )	{ return m_OwnerName; }
	uint32_t GetDuration( )	{ return m_Duration; }

	void SetDuration( uint32_t nDuration )	{ m_Duration = nDuration; }
};

//
// CDBGamePlayer
//

class CDBGamePlayer
{
private:
	uint32_t m_ID;
	uint32_t m_GameID;
	string m_Name;
	string m_IP;
	uint32_t m_Spoofed;
	string m_SpoofedRealm;
	uint32_t m_Reserved;
	uint32_t m_LoadingTime;
	uint32_t m_Left;
	string m_LeftReason;
	uint32_t m_Team;
	uint32_t m_Colour;

public:
	CDBGamePlayer( uint32_t nID, uint32_t nGameID, string nName, string nIP, uint32_t nSpoofed, string nSpoofedRealm, uint32_t nReserved, uint32_t nLoadingTime, uint32_t nLeft, string nLeftReason, uint32_t nTeam, uint32_t nColour );
	~CDBGamePlayer( );

	uint32_t GetID( )			{ return m_ID; }
	uint32_t GetGameID( )		{ return m_GameID; }
	string GetName( )			{ return m_Name; }
	string GetIP( )				{ return m_IP; }
	uint32_t GetSpoofed( )		{ return m_Spoofed; }
	string GetSpoofedRealm( )	{ return m_SpoofedRealm; }
	uint32_t GetReserved( )		{ return m_Reserved; }
	uint32_t GetLoadingTime( )	{ return m_LoadingTime; }
	uint32_t GetLeft( )			{ return m_Left; }
	string GetLeftReason( )		{ return m_LeftReason; }
	uint32_t GetTeam( )			{ return m_Team; }
	uint32_t GetColour( )		{ return m_Colour; }

	void SetLoadingTime( uint32_t nLoadingTime )	{ m_LoadingTime = nLoadingTime; }
	void SetLeft( uint32_t nLeft )					{ m_Left = nLeft; }
	void SetLeftReason( string nLeftReason )		{ m_LeftReason = nLeftReason; }
};

//
// CDBGamePlayerSummary
//

class CDBGamePlayerSummary
{
private:
	string m_Server;
	string m_Name;
	string m_FirstGameDateTime;		// datetime of first game played
	string m_LastGameDateTime;		// datetime of last game played
	uint32_t m_TotalGames;			// total number of games played
	uint32_t m_MinLoadingTime;		// minimum loading time in milliseconds (this could be skewed because different maps have different load times)
	uint32_t m_AvgLoadingTime;		// average loading time in milliseconds (this could be skewed because different maps have different load times)
	uint32_t m_MaxLoadingTime;		// maximum loading time in milliseconds (this could be skewed because different maps have different load times)
	uint32_t m_MinLeftPercent;		// minimum time at which the player left the game expressed as a percentage of the game duration (0-100)
	uint32_t m_AvgLeftPercent;		// average time at which the player left the game expressed as a percentage of the game duration (0-100)
	uint32_t m_MaxLeftPercent;		// maximum time at which the player left the game expressed as a percentage of the game duration (0-100)
	uint32_t m_MinDuration;			// minimum game duration in seconds
	uint32_t m_AvgDuration;			// average game duration in seconds
	uint32_t m_MaxDuration;			// maximum game duration in seconds

public:
	CDBGamePlayerSummary( string nServer, string nName, string nFirstGameDateTime, string nLastGameDateTime, uint32_t nTotalGames, uint32_t nMinLoadingTime, uint32_t nAvgLoadingTime, uint32_t nMaxLoadingTime, uint32_t nMinLeftPercent, uint32_t nAvgLeftPercent, uint32_t nMaxLeftPercent, uint32_t nMinDuration, uint32_t nAvgDuration, uint32_t nMaxDuration );
	~CDBGamePlayerSummary( );

	string GetServer( )					{ return m_Server; }
	string GetName( )					{ return m_Name; }
	string GetFirstGameDateTime( )		{ return m_FirstGameDateTime; }
	string GetLastGameDateTime( )		{ return m_LastGameDateTime; }
	uint32_t GetTotalGames( )			{ return m_TotalGames; }
	uint32_t GetMinLoadingTime( )		{ return m_MinLoadingTime; }
	uint32_t GetAvgLoadingTime( )		{ return m_AvgLoadingTime; }
	uint32_t GetMaxLoadingTime( )		{ return m_MaxLoadingTime; }
	uint32_t GetMinLeftPercent( )		{ return m_MinLeftPercent; }
	uint32_t GetAvgLeftPercent( )		{ return m_AvgLeftPercent; }
	uint32_t GetMaxLeftPercent( )		{ return m_MaxLeftPercent; }
	uint32_t GetMinDuration( )			{ return m_MinDuration; }
	uint32_t GetAvgDuration( )			{ return m_AvgDuration; }
	uint32_t GetMaxDuration( )			{ return m_MaxDuration; }
};

//
// CDBDotAGame
//

class CDBDotAGame
{
private:
	uint32_t m_ID;
	uint32_t m_GameID;
	uint32_t m_Winner;
	uint32_t m_Min;
	uint32_t m_Sec;

public:
	CDBDotAGame( uint32_t nID, uint32_t nGameID, uint32_t nWinner, uint32_t nMin, uint32_t nSec );
	~CDBDotAGame( );

	uint32_t GetID( )		{ return m_ID; }
	uint32_t GetGameID( )	{ return m_GameID; }
	uint32_t GetWinner( )	{ return m_Winner; }
	uint32_t GetMin( )		{ return m_Min; }
	uint32_t GetSec( )		{ return m_Sec; }
};

//
// CDBDotAPlayer
//

class CDBDotAPlayer
{
private:
	uint32_t m_ID;
	uint32_t m_GameID;
	uint32_t m_Colour;
	uint32_t m_Kills;
	uint32_t m_Deaths;
	uint32_t m_CreepKills;
	uint32_t m_CreepDenies;
	uint32_t m_Assists;
	uint32_t m_Gold;
	uint32_t m_NeutralKills;
	string m_Items[6];
	string m_Hero;
	uint32_t m_NewColour;
	uint32_t m_TowerKills;
	uint32_t m_RaxKills;
	uint32_t m_CourierKills;

public:
	CDBDotAPlayer( );
	CDBDotAPlayer( uint32_t nID, uint32_t nGameID, uint32_t nColour, uint32_t nKills, uint32_t nDeaths, uint32_t nCreepKills, uint32_t nCreepDenies, uint32_t nAssists, uint32_t nGold, uint32_t nNeutralKills, string nItem1, string nItem2, string nItem3, string nItem4, string nItem5, string nItem6, string nHero, uint32_t nNewColour, uint32_t nTowerKills, uint32_t nRaxKills, uint32_t nCourierKills );
	~CDBDotAPlayer( );

	uint32_t GetID( )			{ return m_ID; }
	uint32_t GetGameID( )		{ return m_GameID; }
	uint32_t GetColour( )		{ return m_Colour; }
	uint32_t GetKills( )		{ return m_Kills; }
	uint32_t GetDeaths( )		{ return m_Deaths; }
	uint32_t GetCreepKills( )	{ return m_CreepKills; }
	uint32_t GetCreepDenies( )	{ return m_CreepDenies; }
	uint32_t GetAssists( )		{ return m_Assists; }
	uint32_t GetGold( )			{ return m_Gold; }
	uint32_t GetNeutralKills( )	{ return m_NeutralKills; }
	string GetItem( unsigned int i );
	string GetHero( )			{ return m_Hero; }
	uint32_t GetNewColour( )	{ return m_NewColour; }
	uint32_t GetTowerKills( )	{ return m_TowerKills; }
	uint32_t GetRaxKills( )		{ return m_RaxKills; }
	uint32_t GetCourierKills( )	{ return m_CourierKills; }

	void SetColour( uint32_t nColour )				{ m_Colour = nColour; }
	void SetKills( uint32_t nKills )				{ m_Kills = nKills; }
	void SetDeaths( uint32_t nDeaths )				{ m_Deaths = nDeaths; }
	void SetCreepKills( uint32_t nCreepKills )		{ m_CreepKills = nCreepKills; }
	void SetCreepDenies( uint32_t nCreepDenies )	{ m_CreepDenies = nCreepDenies; }
	void SetAssists( uint32_t nAssists )			{ m_Assists = nAssists; }
	void SetGold( uint32_t nGold )					{ m_Gold = nGold; }
	void SetNeutralKills( uint32_t nNeutralKills )	{ m_NeutralKills = nNeutralKills; }
	void SetItem( unsigned int i, string item );
	void SetHero( string nHero )					{ m_Hero = nHero; }
	void SetNewColour( uint32_t nNewColour )		{ m_NewColour = nNewColour; }
	void SetTowerKills( uint32_t nTowerKills )		{ m_TowerKills = nTowerKills; }
	void SetRaxKills( uint32_t nRaxKills )			{ m_RaxKills = nRaxKills; }
	void SetCourierKills( uint32_t nCourierKills )	{ m_CourierKills = nCourierKills; }
};

//
// CDBDotAPlayerSummary
//

class CDBDotAPlayerSummary
{
private:
	string m_Server;
	string m_Name;
	uint32_t m_TotalGames;			// total number of dota games played
	uint32_t m_TotalWins;			// total number of dota games won
	uint32_t m_TotalLosses;			// total number of dota games lost
	uint32_t m_TotalKills;			// total number of hero kills
	uint32_t m_TotalDeaths;			// total number of deaths
	uint32_t m_TotalCreepKills;		// total number of creep kills
	uint32_t m_TotalCreepDenies;	// total number of creep denies
	uint32_t m_TotalAssists;		// total number of assists
	uint32_t m_TotalNeutralKills;	// total number of neutral kills
	uint32_t m_TotalTowerKills;		// total number of tower kills
	uint32_t m_TotalRaxKills;		// total number of rax kills
	uint32_t m_TotalCourierKills;	// total number of courier kills

public:
	CDBDotAPlayerSummary( string nServer, string nName, uint32_t nTotalGames, uint32_t nTotalWins, uint32_t nTotalLosses, uint32_t nTotalKills, uint32_t nTotalDeaths, uint32_t nTotalCreepKills, uint32_t nTotalCreepDenies, uint32_t nTotalAssists, uint32_t nTotalNeutralKills, uint32_t nTotalTowerKills, uint32_t nTotalRaxKills, uint32_t nTotalCourierKills );
	~CDBDotAPlayerSummary( );

	string GetServer( )					{ return m_Server; }
	string GetName( )					{ return m_Name; }
	uint32_t GetTotalGames( )			{ return m_TotalGames; }
	uint32_t GetTotalWins( )			{ return m_TotalWins; }
	uint32_t GetTotalLosses( )			{ return m_TotalLosses; }
	uint32_t GetTotalKills( )			{ return m_TotalKills; }
	uint32_t GetTotalDeaths( )			{ return m_TotalDeaths; }
	uint32_t GetTotalCreepKills( )		{ return m_TotalCreepKills; }
	uint32_t GetTotalCreepDenies( )		{ return m_TotalCreepDenies; }
	uint32_t GetTotalAssists( )			{ return m_TotalAssists; }
	uint32_t GetTotalNeutralKills( )	{ return m_TotalNeutralKills; }
	uint32_t GetTotalTowerKills( )		{ return m_TotalTowerKills; }
	uint32_t GetTotalRaxKills( )		{ return m_TotalRaxKills; }
	uint32_t GetTotalCourierKills( )	{ return m_TotalCourierKills; }

	float GetAvgKills( )				{ return m_TotalGames > 0 ? (float)m_TotalKills / m_TotalGames : 0; }
	float GetAvgDeaths( )				{ return m_TotalGames > 0 ? (float)m_TotalDeaths / m_TotalGames : 0; }
	float GetAvgCreepKills( )			{ return m_TotalGames > 0 ? (float)m_TotalCreepKills / m_TotalGames : 0; }
	float GetAvgCreepDenies( )			{ return m_TotalGames > 0 ? (float)m_TotalCreepDenies / m_TotalGames : 0; }
	float GetAvgAssists( )				{ return m_TotalGames > 0 ? (float)m_TotalAssists / m_TotalGames : 0; }
	float GetAvgNeutralKills( )			{ return m_TotalGames > 0 ? (float)m_TotalNeutralKills / m_TotalGames : 0; }
	float GetAvgTowerKills( )			{ return m_TotalGames > 0 ? (float)m_TotalTowerKills / m_TotalGames : 0; }
	float GetAvgRaxKills( )				{ return m_TotalGames > 0 ? (float)m_TotalRaxKills / m_TotalGames : 0; }
	float GetAvgCourierKills( )			{ return m_TotalGames > 0 ? (float)m_TotalCourierKills / m_TotalGames : 0; }
};

#endif
