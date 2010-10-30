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

#ifndef GHOSTDBSQLITE_H
#define GHOSTDBSQLITE_H

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

CREATE TABLE downloads (
	id INTEGER PRIMARY KEY,
	map TEXT NOT NULL,
	mapsize INTEGER NOT NULL,
	datetime TEXT NOT NULL,
	name TEXT NOT NULL,
	ip TEXT NOT NULL,
	spoofed INTEGER NOT NULL,
	spoofedrealm TEXT NOT NULL,
	downloadtime INTEGER NOT NULL
)

CREATE TABLE w3mmdplayers (
	id INTEGER PRIMARY KEY,
	category TEXT NOT NULL,
	gameid INTEGER NOT NULL,
	pid INTEGER NOT NULL,
	name TEXT NOT NULL,
	flag TEXT NOT NULL,
	leaver INTEGER NOT NULL,
	practicing INTEGER NOT NULL
)

CREATE TABLE w3mmdvars (
	id INTEGER PRIMARY KEY,
	gameid INTEGER NOT NULL,
	pid INTEGER NOT NULL,
	varname TEXT NOT NULL,
	value_int INTEGER DEFAULT NULL,
	value_real REAL DEFAULT NULL,
	value_string TEXT DEFAULT NULL
)

CREATE TEMPORARY TABLE iptocountry (
	ip1 INTEGER NOT NULL,
	ip2 INTEGER NOT NULL,
	country TEXT NOT NULL,
	PRIMARY KEY ( ip1, ip2 )
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

	bool GetReady( )			{ return m_Ready; }
	vector<string> *GetRow( )               { return &m_Row; }
	string GetError( );

	int Prepare( string query, void **Statement );
	int Step( void *Statement );
	int Finalize( void *Statement );
	int Reset( void *Statement );
	int Exec( string query );
	uint32_t LastRowID( );
};

//
// CGHostDBSQLite
//

class CGHostDBSQLite : public CGHostDB
{
private:
	string m_File;
	CSQLITE3 *m_DB;
        uint32_t m_OutstandingCallables;
        queue<void *> m_IdleConnections;
	uint32_t m_NumConnections;        
        
public:
	CGHostDBSQLite( CConfig *CFG );
	virtual ~CGHostDBSQLite( );

        virtual void CreateThread( CBaseCallable *callable );

        virtual string GetStatus( );
	virtual void RecoverCallable( CBaseCallable *callable );

        virtual bool Begin( );
	virtual bool Commit( );       

	// threaded database functions

        virtual CCallableFromAdd *ThreadedFromAdd( uint32_t ip1, uint32_t ip2, string country );
        virtual CCallableFromCheck *ThreadedFromCheck( uint32_t ip1 );
	virtual CCallableAdminCount *ThreadedAdminCount( string server );
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

        virtual void *GetIdleConnection( );
};


//
// MySQL Callables
//

class CSQLiteCallable : virtual public CBaseCallable
{
protected:
	void *m_Connection;
        string m_File;
        
public:
	CSQLiteCallable( void *nConnection, string nFile ) : CBaseCallable( ), m_Connection( nConnection ), m_File( nFile ) { }
	virtual ~CSQLiteCallable( ) { }

	virtual void *GetConnection( )	{ return m_Connection; }

	virtual void Init( );
	virtual void Close( );
};

class CSQLiteCallableAdminCount : public CCallableAdminCount, public CSQLiteCallable
{
public:
	CSQLiteCallableAdminCount( string nServer, void *nConnection, string nFile ) : CBaseCallable( ), CCallableAdminCount( nServer ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableAdminCount( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableAdminCheck : public CCallableAdminCheck, public CSQLiteCallable
{
public:
	CSQLiteCallableAdminCheck( string nServer, string nUser, void *nConnection, string nFile ) : CBaseCallable( ), CCallableAdminCheck( nServer, nUser ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableAdminCheck( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableAdminAdd : public CCallableAdminAdd, public CSQLiteCallable
{
public:
	CSQLiteCallableAdminAdd( string nServer, string nUser, void *nConnection, string nFile ) : CBaseCallable( ), CCallableAdminAdd( nServer, nUser ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableAdminAdd( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableFromAdd : public CCallableFromAdd, public CSQLiteCallable
{
public:
	CSQLiteCallableFromAdd( uint32_t nIP1, uint32_t nIP2, string nCountry, void *nConnection, string nFile ) : CBaseCallable( ), CCallableFromAdd( nIP1, nIP2, nCountry ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableFromAdd( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableFromCheck : public CCallableFromCheck, public CSQLiteCallable
{
public:
	CSQLiteCallableFromCheck( uint32_t nIP, void *nConnection, string nFile ) : CBaseCallable( ), CCallableFromCheck( nIP ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableFromCheck( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableAdminRemove : public CCallableAdminRemove, public CSQLiteCallable
{
public:
	CSQLiteCallableAdminRemove( string nServer, string nUser, void *nConnection, string nFile ) : CBaseCallable( ), CCallableAdminRemove( nServer, nUser ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableAdminRemove( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableAdminList : public CCallableAdminList, public CSQLiteCallable
{
public:
	CSQLiteCallableAdminList( string nServer, void *nConnection, string nFile ) : CBaseCallable( ), CCallableAdminList( nServer ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableAdminList( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableBanCount : public CCallableBanCount, public CSQLiteCallable
{
public:
	CSQLiteCallableBanCount( string nServer, void *nConnection, string nFile ) : CBaseCallable( ), CCallableBanCount( nServer ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableBanCount( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableBanCheck : public CCallableBanCheck, public CSQLiteCallable
{
public:
	CSQLiteCallableBanCheck( string nServer, string nUser, string nIP, void *nConnection, string nFile ) : CBaseCallable( ), CCallableBanCheck( nServer, nUser, nIP ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableBanCheck( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableBanAdd : public CCallableBanAdd, public CSQLiteCallable
{
public:
	CSQLiteCallableBanAdd( string nServer, string nUser, string nIP, string nGameName, string nAdmin, string nReason, void *nConnection, string nFile ) : CBaseCallable( ), CCallableBanAdd( nServer, nUser, nIP, nGameName, nAdmin, nReason ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableBanAdd( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableBanRemove : public CCallableBanRemove, public CSQLiteCallable
{
public:
	CSQLiteCallableBanRemove( string nServer, string nUser, void *nConnection, string nFile ) : CBaseCallable( ), CCallableBanRemove( nServer, nUser ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableBanRemove( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableBanList : public CCallableBanList, public CSQLiteCallable
{
public:
	CSQLiteCallableBanList( string nServer, void *nConnection, string nFile ) : CBaseCallable( ), CCallableBanList( nServer ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableBanList( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableGameAdd : public CCallableGameAdd, public CSQLiteCallable
{
public:
	CSQLiteCallableGameAdd( string nServer, string nMap, string nGameName, string nOwnerName, uint32_t nDuration, uint32_t nGameState, string nCreatorName, string nCreatorServer, void *nConnection, string nFile ) : CBaseCallable( ), CCallableGameAdd( nServer, nMap, nGameName, nOwnerName, nDuration, nGameState, nCreatorName, nCreatorServer ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableGameAdd( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableGamePlayerAdd : public CCallableGamePlayerAdd, public CSQLiteCallable
{
public:
	CSQLiteCallableGamePlayerAdd( uint32_t nGameID, string nName, string nIP, uint32_t nSpoofed, string nSpoofedRealm, uint32_t nReserved, uint32_t nLoadingTime, uint32_t nLeft, string nLeftReason, uint32_t nTeam, uint32_t nColour, void *nConnection, string nFile ) : CBaseCallable( ), CCallableGamePlayerAdd( nGameID, nName, nIP, nSpoofed, nSpoofedRealm, nReserved, nLoadingTime, nLeft, nLeftReason, nTeam, nColour ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableGamePlayerAdd( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableGamePlayerSummaryCheck : public CCallableGamePlayerSummaryCheck, public CSQLiteCallable
{
public:
	CSQLiteCallableGamePlayerSummaryCheck( string nName, void *nConnection, string nFile ) : CBaseCallable( ), CCallableGamePlayerSummaryCheck( nName ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableGamePlayerSummaryCheck( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableDotAGameAdd : public CCallableDotAGameAdd, public CSQLiteCallable
{
public:
	CSQLiteCallableDotAGameAdd( uint32_t nGameID, uint32_t nWinner, uint32_t nMin, uint32_t nSec, void *nConnection, string nFile ) : CBaseCallable( ), CCallableDotAGameAdd( nGameID, nWinner, nMin, nSec ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableDotAGameAdd( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableDotAPlayerAdd : public CCallableDotAPlayerAdd, public CSQLiteCallable
{
public:
	CSQLiteCallableDotAPlayerAdd( uint32_t nGameID, uint32_t nColour, uint32_t nKills, uint32_t nDeaths, uint32_t nCreepKills, uint32_t nCreepDenies, uint32_t nAssists, uint32_t nGold, uint32_t nNeutralKills, string nItem1, string nItem2, string nItem3, string nItem4, string nItem5, string nItem6, string nHero, uint32_t nNewColour, uint32_t nTowerKills, uint32_t nRaxKills, uint32_t nCourierKills, void *nConnection, string nFile ) : CBaseCallable( ), CCallableDotAPlayerAdd( nGameID, nColour, nKills, nDeaths, nCreepKills, nCreepDenies, nAssists, nGold, nNeutralKills, nItem1, nItem2, nItem3, nItem4, nItem5, nItem6, nHero, nNewColour, nTowerKills, nRaxKills, nCourierKills ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableDotAPlayerAdd( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

class CSQLiteCallableDotAPlayerSummaryCheck : public CCallableDotAPlayerSummaryCheck, public CSQLiteCallable
{
public:
	CSQLiteCallableDotAPlayerSummaryCheck( string nName, void *nConnection, string nFile ) : CBaseCallable( ), CCallableDotAPlayerSummaryCheck( nName ), CSQLiteCallable( nConnection, nFile ) { }
	virtual ~CSQLiteCallableDotAPlayerSummaryCheck( ) { }

	virtual void operator( )( );
	virtual void Init( ) { CSQLiteCallable :: Init( ); }
	virtual void Close( ) { CSQLiteCallable :: Close( ); }
};

#endif
