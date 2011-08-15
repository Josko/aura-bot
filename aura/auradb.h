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

#ifndef AURADB_H_
#define AURADB_H_

/**************
 *** SCHEMA ***
 **************

CREATE TABLE admins (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    server TEXT NOT NULL DEFAULT ""
)

CREATE TABLE bans (
    id INTEGER PRIMARY KEY,
    server TEXT NOT NULL,
    name TEXT NOT NULL,
    ip TEXT,
    date TEXT NOT NULL,
    gamename TEXT,
    admin TEXT NOT NULL,
    reason TEXT
)

CREATE TABLE games (
    id INTEGER PRIMARY KEY,
    server TEXT NOT NULL,
    map TEXT NOT NULL,
    datetime TEXT NOT NULL,
    gamename TEXT NOT NULL,
    ownername TEXT NOT NULL,
    duration INTEGER NOT NULL,
    gamestate INTEGER NOT NULL DEFAULT 0,
    creatorname TEXT NOT NULL DEFAULT "",
    creatorserver TEXT NOT NULL DEFAULT ""
)

CREATE TABLE gameplayers (
    id INTEGER PRIMARY KEY,
    gameid INTEGER NOT NULL,
    name TEXT NOT NULL,
    ip TEXT NOT NULL,
    spoofed INTEGER NOT NULL,
    reserved INTEGER NOT NULL,
    loadingtime INTEGER NOT NULL,
    left INTEGER NOT NULL,
    leftreason TEXT NOT NULL,
    team INTEGER NOT NULL,
    colour INTEGER NOT NULL,
    spoofedrealm TEXT NOT NULL DEFAULT ""
)

CREATE TABLE dotagames (
    id INTEGER PRIMARY KEY,
    gameid INTEGER NOT NULL,
    winner INTEGER NOT NULL,
    min INTEGER NOT NULL DEFAULT 0,
    sec INTEGER NOT NULL DEFAULT 0
)

CREATE TABLE dotaplayers (
    id INTEGER PRIMARY KEY,
    gameid INTEGER NOT NULL,
    colour INTEGER NOT NULL,
    kills INTEGER NOT NULL,
    deaths INTEGER NOT NULL,
    creepkills INTEGER NOT NULL,
    creepdenies INTEGER NOT NULL,
    assists INTEGER NOT NULL,
    gold INTEGER NOT NULL,
    neutralkills INTEGER NOT NULL,
    item1 TEXT NOT NULL,
    item2 TEXT NOT NULL,
    item3 TEXT NOT NULL,
    item4 TEXT NOT NULL,
    item5 TEXT NOT NULL,
    item6 TEXT NOT NULL,
    hero TEXT NOT NULL DEFAULT "",
    newcolour NOT NULL DEFAULT 0,
    towerkills NOT NULL DEFAULT 0,
    raxkills NOT NULL DEFAULT 0,
    courierkills NOT NULL DEFAULT 0
)

CREATE TABLE config (
    name TEXT NOT NULL PRIMARY KEY,
    value TEXT NOT NULL
)

CREATE TEMPORARY TABLE iptocountry (
    ip1 INTEGER NOT NULL,
    ip2 INTEGER NOT NULL,
    country TEXT NOT NULL,
    PRIMARY KEY ( ip1, ip2 )
)

CREATE TEMPORARY TABLE rootadmins (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    server TEXT NOT NULL DEFAULT ""
)

CREATE INDEX idx_gameid ON gameplayers ( gameid )
CREATE INDEX idx_gameid_colour ON dotaplayers ( gameid, colour )

 **************
 *** SCHEMA ***
 **************/

//
// CSQLITE3 (wrapper class)
//

class CSQLITE3
{
private:
  void *m_DB;
  bool m_Ready;
  vector<string> m_Row;

public:
  CSQLITE3( string filename );
  ~CSQLITE3( );

  bool GetReady( ) const			{ return m_Ready; }
  vector<string> *GetRow( )		    { return &m_Row; }  
  string GetError( ) const ;

  int Prepare( const string &query, void **Statement );
  int Step( void *Statement );
  int Finalize( void *Statement );
  int Reset( void *Statement );
  int Exec( const string &query );
  uint32_t LastRowID( );
};

//
// CAuraDB
//

class CDBDotAPlayerSummary;
class CDBGamePlayerSummary;
class CDBBan;

class CAuraDB
{
private:
  string m_File;
  CSQLITE3 *m_DB;
  bool m_HasError;
  string m_Error;

  // we keep some prepared statements in memory rather than recreating them each function call
  // this is an optimization because preparing statements takes time
  // however it only pays off if you're going to be using the statement extremely often

  void *FromAddStmt;          // for faster startup time
  void *FromCheckStmt;        // frequently used
  void *BanCheckStmt;         // frequently used
  void *AdminCheckStmt;       // frequently used
  void *RootAdminCheckStmt;   // frequently used

public:
  CAuraDB( CConfig *CFG );
  ~CAuraDB( );

  bool HasError( ) const				{ return m_HasError; }
  string GetError( ) const			    { return m_Error; }

  bool Begin( );
  bool Commit( );
  uint32_t AdminCount( const string &server );
  bool AdminCheck( const string &server, string user );
  bool AdminCheck( string user );
  bool RootAdminCheck( const string &server, string user );
  bool RootAdminCheck( string user );
  bool AdminAdd( const string &server, string user );
  bool RootAdminAdd( const string &server, string user );
  bool AdminRemove( const string &server, string user );
  uint32_t BanCount( const string &server );
  CDBBan *BanCheck( const string &server, string user, const string &ip );
  bool BanAdd( const string &server, string user, const string &ip, const string &gamename, string admin, const string &reason );
  bool BanRemove( const string &server, string user );
  bool BanRemove( string user );
  uint32_t GameAdd( const string &server, const string &map, const string &gamename, const string &ownername, uint32_t duration, uint32_t gamestate, const string &creatorname, const string &creatorserver );
  uint32_t GamePlayerAdd( uint32_t gameid, string name, const string &ip, uint32_t spoofed, const string &spoofedrealm, uint32_t reserved, uint32_t loadingtime, uint32_t left, const string &leftreason, uint32_t team, uint32_t colour );
  uint32_t GamePlayerCount( string name );
  CDBGamePlayerSummary *GamePlayerSummaryCheck( string name );
  uint32_t DotAGameAdd( uint32_t gameid, uint32_t winner, uint32_t min, uint32_t sec );
  uint32_t DotAPlayerAdd( uint32_t gameid, uint32_t colour, uint32_t kills, uint32_t deaths, uint32_t creepkills, uint32_t creepdenies, uint32_t assists, uint32_t gold, uint32_t neutralkills, const string &item1, const string &item2, const string &item3, const string &item4, const string &item5, const string &item6, const string &hero, uint32_t newcolour, uint32_t towerkills, uint32_t raxkills, uint32_t courierkills );
  uint32_t DotAPlayerCount( string name );
  CDBDotAPlayerSummary *DotAPlayerSummaryCheck( string name );
  string FromCheck( uint32_t ip );
  bool FromAdd( uint32_t ip1, uint32_t ip2, const string &country );
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
  CDBBan( const string &nServer, const string &nName, const string &nIP, const string &nDate, const string &nGameName, const string &nAdmin, const string &nReason );
  ~CDBBan( );

  string GetServer( ) const					{ return m_Server; }
  string GetName( ) const					{ return m_Name; }
  string GetIP( ) const						{ return m_IP; }
  string GetDate( ) const					{ return m_Date; }
  string GetGameName( ) const				{ return m_GameName; }
  string GetAdmin( ) const					{ return m_Admin; }
  string GetReason( ) const					{ return m_Reason; }
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
  CDBGamePlayer( uint32_t nID, uint32_t nGameID, const string &nName, string nIP, uint32_t nSpoofed, const string &nSpoofedRealm, uint32_t nReserved, uint32_t nLoadingTime, uint32_t nLeft, const string &nLeftReason, uint32_t nTeam, uint32_t nColour );
  ~CDBGamePlayer( );

  uint32_t GetID( ) const					{ return m_ID; }
  uint32_t GetGameID( ) const				{ return m_GameID; }
  string GetName( ) const					{ return m_Name; }
  string GetIP( ) const						{ return m_IP; }
  uint32_t GetSpoofed( ) const			    { return m_Spoofed; }
  string GetSpoofedRealm( ) const		    { return m_SpoofedRealm; }
  uint32_t GetReserved( ) const			    { return m_Reserved; }
  uint32_t GetLoadingTime( ) const	        { return m_LoadingTime; }
  uint32_t GetLeft( ) const					{ return m_Left; }
  string GetLeftReason( ) const			    { return m_LeftReason; }
  uint32_t GetTeam( ) const					{ return m_Team; }
  uint32_t GetColour( ) const				{ return m_Colour; }
  
  void SetLoadingTime( uint32_t nLoadingTime )		{ m_LoadingTime = nLoadingTime; }
  void SetLeft( uint32_t nLeft )					{ m_Left = nLeft; }
  void SetLeftReason( string nLeftReason )			{ m_LeftReason = nLeftReason; }
};

//
// CDBGamePlayerSummary
//

class CDBGamePlayerSummary
{
private:
  string m_Server;
  string m_Name;
  string m_FirstGameDateTime; // datetime of first game played
  string m_LastGameDateTime;  // datetime of last game played
  uint32_t m_TotalGames;      // total number of games played
  uint32_t m_MinLoadingTime;  // minimum loading time in milliseconds (this could be skewed because different maps have different load times)
  uint32_t m_AvgLoadingTime;  // average loading time in milliseconds (this could be skewed because different maps have different load times)
  uint32_t m_MaxLoadingTime;  // maximum loading time in milliseconds (this could be skewed because different maps have different load times)
  uint32_t m_MinLeftPercent;  // minimum time at which the player left the game expressed as a percentage of the game duration (0-100)
  uint32_t m_AvgLeftPercent;  // average time at which the player left the game expressed as a percentage of the game duration (0-100)
  uint32_t m_MaxLeftPercent;  // maximum time at which the player left the game expressed as a percentage of the game duration (0-100)
  uint32_t m_MinDuration;     // minimum game duration in seconds
  uint32_t m_AvgDuration;     // average game duration in seconds
  uint32_t m_MaxDuration;     // maximum game duration in seconds

public:
  CDBGamePlayerSummary( const string &nServer, const string &nName, const string &nFirstGameDateTime, const string &nLastGameDateTime, uint32_t nTotalGames, uint32_t nMinLoadingTime, uint32_t nAvgLoadingTime, uint32_t nMaxLoadingTime, uint32_t nMinLeftPercent, uint32_t nAvgLeftPercent, uint32_t nMaxLeftPercent, uint32_t nMinDuration, uint32_t nAvgDuration, uint32_t nMaxDuration );
  ~CDBGamePlayerSummary( );

  string GetServer( ) const					{ return m_Server; }
  string GetName( ) const					{ return m_Name; }
  string GetFirstGameDateTime( ) const	    { return m_FirstGameDateTime; }
  string GetLastGameDateTime( ) const		{ return m_LastGameDateTime; }
  uint32_t GetTotalGames( ) const			{ return m_TotalGames; }
  uint32_t GetMinLoadingTime( ) const		{ return m_MinLoadingTime; }
  uint32_t GetAvgLoadingTime( ) const		{ return m_AvgLoadingTime; }
  uint32_t GetMaxLoadingTime( ) const		{ return m_MaxLoadingTime; }
  uint32_t GetMinLeftPercent( ) const		{ return m_MinLeftPercent; }
  uint32_t GetAvgLeftPercent( ) const		{ return m_AvgLeftPercent; }
  uint32_t GetMaxLeftPercent( ) const		{ return m_MaxLeftPercent; }
  uint32_t GetMinDuration( ) const			{ return m_MinDuration; }
  uint32_t GetAvgDuration( ) const			{ return m_AvgDuration; }
  uint32_t GetMaxDuration( ) const			{ return m_MaxDuration; }
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
  CDBDotAPlayer( uint32_t nID, uint32_t nGameID, uint32_t nColour, uint32_t nKills, uint32_t nDeaths, uint32_t nCreepKills, uint32_t nCreepDenies, uint32_t nAssists, uint32_t nGold, uint32_t nNeutralKills, const string &nItem1, const string &nItem2, const string &nItem3, const string &nItem4, const string &nItem5, const string &nItem6, const string &nHero, uint32_t nNewColour, uint32_t nTowerKills, uint32_t nRaxKills, uint32_t nCourierKills );
  ~CDBDotAPlayer( );

	string GetItem( unsigned int i ) const;
  uint32_t GetID( ) const						{ return m_ID; }
  uint32_t GetGameID( ) const					{ return m_GameID; }
  uint32_t GetColour( ) const					{ return m_Colour; }
  uint32_t GetKills( ) const					{ return m_Kills; }
  uint32_t GetDeaths( ) const					{ return m_Deaths; }
  uint32_t GetCreepKills( ) const				{ return m_CreepKills; }
  uint32_t GetCreepDenies( ) const				{ return m_CreepDenies; }
  uint32_t GetAssists( ) const					{ return m_Assists; }
  uint32_t GetGold( ) const						{ return m_Gold; }
  uint32_t GetNeutralKills( ) const				{ return m_NeutralKills; }  
  string GetHero( ) const						{ return m_Hero; }
  uint32_t GetNewColour( ) const				{ return m_NewColour; }
  uint32_t GetTowerKills( ) const				{ return m_TowerKills; }
  uint32_t GetRaxKills( ) const					{ return m_RaxKills; }
  uint32_t GetCourierKills( ) const				{ return m_CourierKills; }
  
  void SetItem( unsigned int i, string item );
  void SetColour( uint32_t nColour )      		  { m_Colour = nColour; }
  void SetKills( uint32_t nKills )				  { m_Kills = nKills; }
  void SetDeaths( uint32_t nDeaths )			  { m_Deaths = nDeaths; }
  void SetCreepKills( uint32_t nCreepKills )	  { m_CreepKills = nCreepKills; }
  void SetCreepDenies( uint32_t nCreepDenies )	  { m_CreepDenies = nCreepDenies; }
  void SetAssists( uint32_t nAssists )			  { m_Assists = nAssists; }
  void SetGold( uint32_t nGold )				  { m_Gold = nGold; }
  void SetNeutralKills( uint32_t nNeutralKills )  { m_NeutralKills = nNeutralKills; }
  void SetHero( string nHero )					  { m_Hero = nHero; }
  void SetNewColour( uint32_t nNewColour )		  { m_NewColour = nNewColour; }
  void SetTowerKills( uint32_t nTowerKills )	  { m_TowerKills = nTowerKills; }
  void SetRaxKills( uint32_t nRaxKills )		  { m_RaxKills = nRaxKills; }
  void SetCourierKills( uint32_t nCourierKills )  { m_CourierKills = nCourierKills; }
};

//
// CDBDotAPlayerSummary
//

class CDBDotAPlayerSummary
{
private:
  string m_Server;
  string m_Name;
  uint32_t m_TotalGames;        // total number of dota games played
  uint32_t m_TotalWins;         // total number of dota games won
  uint32_t m_TotalLosses;       // total number of dota games lost
  uint32_t m_TotalKills;        // total number of hero kills
  uint32_t m_TotalDeaths;       // total number of deaths
  uint32_t m_TotalCreepKills;   // total number of creep kills
  uint32_t m_TotalCreepDenies;  // total number of creep denies
  uint32_t m_TotalAssists;      // total number of assists
  uint32_t m_TotalNeutralKills; // total number of neutral kills
  uint32_t m_TotalTowerKills;   // total number of tower kills
  uint32_t m_TotalRaxKills;     // total number of rax kills
  uint32_t m_TotalCourierKills; // total number of courier kills

public:
  CDBDotAPlayerSummary( const string &nServer, const string &nName, uint32_t nTotalGames, uint32_t nTotalWins, uint32_t nTotalLosses, uint32_t nTotalKills, uint32_t nTotalDeaths, uint32_t nTotalCreepKills, uint32_t nTotalCreepDenies, uint32_t nTotalAssists, uint32_t nTotalNeutralKills, uint32_t nTotalTowerKills, uint32_t nTotalRaxKills, uint32_t nTotalCourierKills );
  ~CDBDotAPlayerSummary( );

  string GetServer( ) const					{ return m_Server; }
  string GetName( ) const					{ return m_Name; }
  uint32_t GetTotalGames( ) const			{ return m_TotalGames; }
  uint32_t GetTotalWins( ) const			{ return m_TotalWins; }
  uint32_t GetTotalLosses( ) const			{ return m_TotalLosses; }
  uint32_t GetTotalKills( ) const			{ return m_TotalKills; }
  uint32_t GetTotalDeaths( ) const			{ return m_TotalDeaths; }
  uint32_t GetTotalCreepKills( ) const		{ return m_TotalCreepKills; }
  uint32_t GetTotalCreepDenies( ) const		{ return m_TotalCreepDenies; }
  uint32_t GetTotalAssists( ) const			{ return m_TotalAssists; }
  uint32_t GetTotalNeutralKills( ) const	{ return m_TotalNeutralKills; }
  uint32_t GetTotalTowerKills( ) const		{ return m_TotalTowerKills; }
  uint32_t GetTotalRaxKills( ) const		{ return m_TotalRaxKills; }
  uint32_t GetTotalCourierKills( ) const	{ return m_TotalCourierKills; }
  float GetAvgKills( ) const				{ return m_TotalGames > 0 ? (float) m_TotalKills / m_TotalGames : 0; }
  float GetAvgDeaths( ) const				{ return m_TotalGames > 0 ? (float) m_TotalDeaths / m_TotalGames : 0; }
  float GetAvgCreepKills( ) const			{ return m_TotalGames > 0 ? (float) m_TotalCreepKills / m_TotalGames : 0; }
  float GetAvgCreepDenies( ) const			{ return m_TotalGames > 0 ? (float) m_TotalCreepDenies / m_TotalGames : 0; }
  float GetAvgAssists( ) const				{ return m_TotalGames > 0 ? (float) m_TotalAssists / m_TotalGames : 0; }
  float GetAvgNeutralKills( ) const			{ return m_TotalGames > 0 ? (float) m_TotalNeutralKills / m_TotalGames : 0; }
  float GetAvgTowerKills( ) const			{ return m_TotalGames > 0 ? (float) m_TotalTowerKills / m_TotalGames : 0; }
  float GetAvgRaxKills( ) const				{ return m_TotalGames > 0 ? (float) m_TotalRaxKills / m_TotalGames : 0; }
  float GetAvgCourierKills( ) const			{ return m_TotalGames > 0 ? (float) m_TotalCourierKills / m_TotalGames : 0; }
};

#endif
