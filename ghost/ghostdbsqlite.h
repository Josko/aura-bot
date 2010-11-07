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
	vector<string> *GetRow( )	{ return &m_Row; }
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

	// we keep some prepared statements in memory rather than recreating them each function call
	// this is an optimization because preparing statements takes time
	// however it only pays off if you're going to be using the statement extremely often

	void *FromAddStmt;
	void *FromCheckStmt;
	void *BanCheckStmt;
	void *AdminCheckStmt;
public:
	CGHostDBSQLite( CConfig *CFG );
	virtual ~CGHostDBSQLite( );

	virtual bool Begin( );
	virtual bool Commit( );
	virtual uint32_t AdminCount( string server );
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
	// note: these are not actually implemented with threads at the moment, they WILL block until the query is complete
	// todotodo: implement threads here

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
};

#endif
