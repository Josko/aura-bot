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

#include "ghost.h"
#include "util.h"
#include "config.h"
#include "ghostdb.h"
#include "ghostdbsqlite.h"
#include "sqlite3.h"

//
// CQSLITE3 (wrapper class)
//

CSQLITE3 :: CSQLITE3( string filename )
{
	m_Ready = true;

	if( sqlite3_open_v2( filename.c_str( ), (sqlite3 **)&m_DB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL ) != SQLITE_OK )
		m_Ready = false;
}

CSQLITE3 :: ~CSQLITE3( )
{
	sqlite3_close( (sqlite3 *)m_DB );
}

string CSQLITE3 :: GetError( )
{
	return sqlite3_errmsg( (sqlite3 *)m_DB );
}

int CSQLITE3 :: Prepare( const string &query, void **Statement )
{
	return sqlite3_prepare_v2( (sqlite3 *)m_DB, query.c_str( ), -1, (sqlite3_stmt **)Statement, NULL );
}

int CSQLITE3 :: Step( void *Statement )
{
	int RC = sqlite3_step( (sqlite3_stmt *)Statement );

	if( RC == SQLITE_ROW )
	{
		m_Row.clear( );

		for( int i = 0; i < sqlite3_column_count( (sqlite3_stmt *)Statement ); ++i )
		{
			char *ColumnText = (char *)sqlite3_column_text( (sqlite3_stmt *)Statement, i );

			if( ColumnText )
				m_Row.push_back( ColumnText );
			else
				m_Row.push_back( string( ) );
		}
	}

	return RC;
}

int CSQLITE3 :: Finalize( void *Statement )
{
	return sqlite3_finalize( (sqlite3_stmt *)Statement );
}

int CSQLITE3 :: Reset( void *Statement )
{
	return sqlite3_reset( (sqlite3_stmt *)Statement );
}

int CSQLITE3 :: Exec( const string &query )
{
	return sqlite3_exec( (sqlite3 *)m_DB, query.c_str( ), NULL, NULL, NULL );
}

uint32_t CSQLITE3 :: LastRowID( )
{
	return (uint32_t)sqlite3_last_insert_rowid( (sqlite3 *)m_DB );
}

//
// CGHostDBSQLite
//

CGHostDBSQLite :: CGHostDBSQLite( CConfig *CFG ) : CGHostDB( CFG )
{
	m_File = CFG->GetString( "db_sqlite3_file", "ghost.dbs" );
	Print( "[SQLITE3] version " + string( SQLITE_VERSION ) );
	Print( "[SQLITE3] opening database [" + m_File + "]" );
	m_DB = new CSQLITE3( m_File );

	if( !m_DB->GetReady( ) )
	{
		// setting m_HasError to true indicates there's been a critical error and we want GHost to shutdown
		// this is okay here because we're in the constructor so we're not dropping any games or players

		Print( string( "[SQLITE3] error opening database [" + m_File + "] - " ) + m_DB->GetError( ) );
		m_HasError = true;
		m_Error = "error opening database";
		return;
	}

	// find the schema number so we can determine whether we need to upgrade or not

	string SchemaNumber;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT value FROM config WHERE name=\"schema_number\"", (void **)&Statement );

	if( Statement )
	{
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_ROW )
		{
			vector<string> *Row = m_DB->GetRow( );

			if( Row->size( ) == 1 )
				SchemaNumber = (*Row)[0];
			else
				Print( "[SQLITE3] error getting schema number - row doesn't have 1 column" );
		}
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error getting schema number - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error getting schema number - " + m_DB->GetError( ) );

	if( SchemaNumber.empty( ) )
	{
		// couldn't find the schema number
		// MAEK NEW TABLEZ

		Print( "[SQLITE3] couldn't find schema number, create tables" );
		
		// assume the database is empty
		// note to self: update the SchemaNumber and the database structure when making a new schema

		Print( "[SQLITE3] assuming database is empty" );
		SchemaNumber = "1";

		if( m_DB->Exec( "CREATE TABLE admins ( id INTEGER PRIMARY KEY, name TEXT NOT NULL, server TEXT NOT NULL DEFAULT \"\" )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating admins table - " + m_DB->GetError( ) );

		if( m_DB->Exec( "CREATE TABLE bans ( id INTEGER PRIMARY KEY, server TEXT NOT NULL, name TEXT NOT NULL, ip TEXT, date TEXT NOT NULL, gamename TEXT, admin TEXT NOT NULL, reason TEXT )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating bans table - " + m_DB->GetError( ) );

		if( m_DB->Exec( "CREATE TABLE games ( id INTEGER PRIMARY KEY, server TEXT NOT NULL, map TEXT NOT NULL, datetime TEXT NOT NULL, gamename TEXT NOT NULL, ownername TEXT NOT NULL, duration INTEGER NOT NULL, gamestate INTEGER NOT NULL DEFAULT 0, creatorname TEXT NOT NULL DEFAULT \"\", creatorserver TEXT NOT NULL DEFAULT \"\" )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating games table - " + m_DB->GetError( ) );

		if( m_DB->Exec( "CREATE TABLE gameplayers ( id INTEGER PRIMARY KEY, gameid INTEGER NOT NULL, name TEXT NOT NULL, ip TEXT NOT NULL, spoofed INTEGER NOT NULL, reserved INTEGER NOT NULL, loadingtime INTEGER NOT NULL, left INTEGER NOT NULL, leftreason TEXT NOT NULL, team INTEGER NOT NULL, colour INTEGER NOT NULL, spoofedrealm TEXT NOT NULL DEFAULT \"\" )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating gameplayers table - " + m_DB->GetError( ) );

		if( m_DB->Exec( "CREATE TABLE dotagames ( id INTEGER PRIMARY KEY, gameid INTEGER NOT NULL, winner INTEGER NOT NULL, min INTEGER NOT NULL DEFAULT 0, sec INTEGER NOT NULL DEFAULT 0 )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating dotagames table - " + m_DB->GetError( ) );

		if( m_DB->Exec( "CREATE TABLE dotaplayers ( id INTEGER PRIMARY KEY, gameid INTEGER NOT NULL, colour INTEGER NOT NULL, kills INTEGER NOT NULL, deaths INTEGER NOT NULL, creepkills INTEGER NOT NULL, creepdenies INTEGER NOT NULL, assists INTEGER NOT NULL, gold INTEGER NOT NULL, neutralkills INTEGER NOT NULL, item1 TEXT NOT NULL, item2 TEXT NOT NULL, item3 TEXT NOT NULL, item4 TEXT NOT NULL, item5 TEXT NOT NULL, item6 TEXT NOT NULL, hero TEXT NOT NULL DEFAULT \"\", newcolour NOT NULL DEFAULT 0, towerkills NOT NULL DEFAULT 0, raxkills NOT NULL DEFAULT 0, courierkills NOT NULL DEFAULT 0 )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating dotaplayers table - " + m_DB->GetError( ) );

		if( m_DB->Exec( "CREATE TABLE config ( name TEXT NOT NULL PRIMARY KEY, value TEXT NOT NULL )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating config table - " + m_DB->GetError( ) );

		m_DB->Prepare( "INSERT INTO config VALUES ( \"schema_number\", ? )", (void **)&Statement );

		if( Statement )
		{
			sqlite3_bind_text( Statement, 1, SchemaNumber.c_str( ), -1, SQLITE_TRANSIENT );
			int RC = m_DB->Step( Statement );

			if( RC == SQLITE_ERROR )
				Print( "[SQLITE3] error inserting schema number [" + SchemaNumber + "] - " + m_DB->GetError( ) );

			m_DB->Finalize( Statement );
		}
		else
			Print( "[SQLITE3] prepare error inserting schema number [" + SchemaNumber + "] - " + m_DB->GetError( ) );

		if( m_DB->Exec( "CREATE INDEX idx_gameid ON gameplayers ( gameid )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating idx_gameid index on gameplayers table - " + m_DB->GetError( ) );

		if( m_DB->Exec( "CREATE INDEX idx_gameid_colour ON dotaplayers ( gameid, colour )" ) != SQLITE_OK )
			Print( "[SQLITE3] error creating idx_gameid_colour index on dotaplayers table - " + m_DB->GetError( ) );
		
	}
	else
		Print( "[SQLITE3] found schema number [" + SchemaNumber + "]" );	

	if( m_DB->Exec( "CREATE TEMPORARY TABLE iptocountry ( ip1 INTEGER NOT NULL, ip2 INTEGER NOT NULL, country TEXT NOT NULL, PRIMARY KEY ( ip1, ip2 ) )" ) != SQLITE_OK )
		Print( "[SQLITE3] error creating temporary iptocountry table - " + m_DB->GetError( ) );

	FromAddStmt = NULL;
	FromCheckStmt = NULL;
	BanCheckStmt = NULL;
	AdminCheckStmt = NULL;
}

CGHostDBSQLite :: ~CGHostDBSQLite( )
{
	if( FromAddStmt )
		m_DB->Finalize( FromAddStmt );
		
	if( BanCheckStmt )
		m_DB->Finalize( BanCheckStmt );
		
	if( FromCheckStmt )
		m_DB->Finalize( FromCheckStmt );
		
	if( AdminCheckStmt )
		m_DB->Finalize( AdminCheckStmt );

	Print( "[SQLITE3] closing database [" + m_File + "]" );
	delete m_DB;
}

bool CGHostDBSQLite :: Begin( )
{
	return m_DB->Exec( "BEGIN TRANSACTION" ) == SQLITE_OK;
}

bool CGHostDBSQLite :: Commit( )
{
	return m_DB->Exec( "COMMIT TRANSACTION" ) == SQLITE_OK;
}

uint32_t CGHostDBSQLite :: AdminCount( const string &server )
{
	uint32_t Count = 0;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT COUNT(*) FROM admins WHERE server=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_ROW )
			Count = sqlite3_column_int( Statement, 0 );
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error counting admins [" + server + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error counting admins [" + server + "] - " + m_DB->GetError( ) );

	return Count;
}

bool CGHostDBSQLite :: AdminCheck( const string &server, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool IsAdmin = false;
	
	if( !AdminCheckStmt )
		m_DB->Prepare( "SELECT * FROM admins WHERE server=? AND name=?", (void **)&AdminCheckStmt );

	if( AdminCheckStmt )
	{
		sqlite3_bind_text( (sqlite3_stmt *)AdminCheckStmt, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( (sqlite3_stmt *)AdminCheckStmt, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		
		int RC = m_DB->Step( AdminCheckStmt );

		// we're just checking to see if the query returned a row, we don't need to check the row data itself

		if( RC == SQLITE_ROW )
			IsAdmin = true;
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error checking admin [" + server + " : " + user + "] - " + m_DB->GetError( ) );

		m_DB->Reset( AdminCheckStmt );
	}
	else
		Print( "[SQLITE3] prepare error checking admin [" + server + " : " + user + "] - " + m_DB->GetError( ) );

	return IsAdmin;
}

bool CGHostDBSQLite :: AdminAdd( const string &server, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "INSERT INTO admins ( server, name ) VALUES ( ?, ? )", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error adding admin [" + server + " : " + user + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error adding admin [" + server + " : " + user + "] - " + m_DB->GetError( ) );

	return Success;
}

bool CGHostDBSQLite :: AdminRemove( const string &server, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "DELETE FROM admins WHERE server=? AND name=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error removing admin [" + server + " : " + user + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error removing admin [" + server + " : " + user + "] - " + m_DB->GetError( ) );

	return Success;
}

vector<string> CGHostDBSQLite :: AdminList( const string &server )
{
	vector<string> AdminList;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT name FROM admins WHERE server=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		while( RC == SQLITE_ROW )
		{
			vector<string> *Row = m_DB->GetRow( );

			if( Row->size( ) == 1 )
				AdminList.push_back( (*Row)[0] );

			RC = m_DB->Step( Statement );
		}

		if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error retrieving admin list [" + server + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error retrieving admin list [" + server + "] - " + m_DB->GetError( ) );

	return AdminList;
}

uint32_t CGHostDBSQLite :: BanCount( const string &server )
{
	uint32_t Count = 0;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT COUNT(*) FROM bans WHERE server=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_ROW )
			Count = sqlite3_column_int( Statement, 0 );
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error counting bans [" + server + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error counting bans [" + server + "] - " + m_DB->GetError( ) );

	return Count;
}

CDBBan *CGHostDBSQLite :: BanCheck( const string &server, string user, const string &ip )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	CDBBan *Ban = NULL;
	
	if( !BanCheckStmt )
	{
		if( ip.empty( ) )
			m_DB->Prepare( "SELECT name, ip, date, gamename, admin, reason FROM bans WHERE server=? AND name=?", (void **)&BanCheckStmt );
		else
			m_DB->Prepare( "SELECT name, ip, date, gamename, admin, reason FROM bans WHERE (server=? AND name=?) OR ip=?", (void **)&BanCheckStmt );
	}
	
	if( BanCheckStmt )
	{
		sqlite3_bind_text( (sqlite3_stmt *)BanCheckStmt, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( (sqlite3_stmt *)BanCheckStmt, 2, user.c_str( ), -1, SQLITE_TRANSIENT );

		if( !ip.empty( ) )
			sqlite3_bind_text( (sqlite3_stmt *)BanCheckStmt, 3, ip.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = m_DB->Step( BanCheckStmt );

		if( RC == SQLITE_ROW )
		{
			vector<string> *Row = m_DB->GetRow( );

			if( Row->size( ) == 6 )
				Ban = new CDBBan( server, (*Row)[0], (*Row)[1], (*Row)[2], (*Row)[3], (*Row)[4], (*Row)[5] );
			else
				Print( "[SQLITE3] error checking ban [" + server + " : " + user + " : " + ip + "] - row doesn't have 6 columns" );
		}
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error checking ban [" + server + " : " + user + " : " + ip + "] - " + m_DB->GetError( ) );

		m_DB->Reset( BanCheckStmt );
	}
	else
		Print( "[SQLITE3] prepare error checking ban [" + server + " : " + user + " : " + ip + "] - " + m_DB->GetError( ) );

	return Ban;
}

bool CGHostDBSQLite :: BanAdd( const string &server, string user, const string &ip, const string &gamename, string admin, const string &reason )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "INSERT INTO bans ( server, name, ip, date, gamename, admin, reason ) VALUES ( ?, ?, ?, date('now'), ?, ?, ? )", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 3, ip.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 4, gamename.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 5, admin.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 6, reason.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error adding ban [" + server + " : " + user + " : " + ip + " : " + gamename + " : " + admin + " : " + reason + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error adding ban [" + server + " : " + user + " : " + ip + " : " + gamename + " : " + admin + " : " + reason + "] - " + m_DB->GetError( ) );

	return Success;
}

bool CGHostDBSQLite :: BanRemove( const string &server, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "DELETE FROM bans WHERE server=? AND name=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error removing ban [" + server + " : " + user + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error removing ban [" + server + " : " + user + "] - " + m_DB->GetError( ) );

	return Success;
}

bool CGHostDBSQLite :: BanRemove( string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "DELETE FROM bans WHERE name=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, user.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error removing ban [" + user + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error removing ban [" + user + "] - " + m_DB->GetError( ) );

	return Success;
}

vector<CDBBan *> CGHostDBSQLite :: BanList( const string &server )
{
	vector<CDBBan *> BanList;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT name, ip, date, gamename, admin, reason FROM bans WHERE server=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		while( RC == SQLITE_ROW )
		{
			vector<string> *Row = m_DB->GetRow( );

			if( Row->size( ) == 6 )
				BanList.push_back( new CDBBan( server, (*Row)[0], (*Row)[1], (*Row)[2], (*Row)[3], (*Row)[4], (*Row)[5] ) );

			RC = m_DB->Step( Statement );
		}

		if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error retrieving ban list [" + server + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error retrieving ban list [" + server + "] - " + m_DB->GetError( ) );

	return BanList;
}

uint32_t CGHostDBSQLite :: GameAdd( const string &server, const string &map, const string &gamename, const string &ownername, uint32_t duration, uint32_t gamestate, const string &creatorname, const string &creatorserver )
{
	uint32_t RowID = 0;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "INSERT INTO games ( server, map, datetime, gamename, ownername, duration, gamestate, creatorname, creatorserver ) VALUES ( ?, ?, datetime('now'), ?, ?, ?, ?, ?, ? )", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, map.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 3, gamename.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 4, ownername.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( Statement, 5, duration );
		sqlite3_bind_int( Statement, 6, gamestate );
		sqlite3_bind_text( Statement, 7, creatorname.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 8, creatorserver.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			RowID = m_DB->LastRowID( );
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error adding game [" + server + " : " + map + " : " + gamename + " : " + ownername + " : " + UTIL_ToString( duration ) + " : " + UTIL_ToString( gamestate ) + " : " + creatorname + " : " + creatorserver + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error adding game [" + server + " : " + map + " : " + gamename + " : " + ownername + " : " + UTIL_ToString( duration ) + " : " + UTIL_ToString( gamestate ) + " : " + creatorname + " : " + creatorserver + "] - " + m_DB->GetError( ) );

	return RowID;
}

uint32_t CGHostDBSQLite :: GamePlayerAdd( uint32_t gameid, string name, const string &ip, uint32_t spoofed, const string &spoofedrealm, uint32_t reserved, uint32_t loadingtime, uint32_t left, const string &leftreason, uint32_t team, uint32_t colour )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	uint32_t RowID = 0;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "INSERT INTO gameplayers ( gameid, name, ip, spoofed, reserved, loadingtime, left, leftreason, team, colour, spoofedrealm ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_int( Statement, 1, gameid );
		sqlite3_bind_text( Statement, 2, name.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 3, ip.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( Statement, 4, spoofed );
		sqlite3_bind_int( Statement, 5, reserved );
		sqlite3_bind_int( Statement, 6, loadingtime );
		sqlite3_bind_int( Statement, 7, left );
		sqlite3_bind_text( Statement, 8, leftreason.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( Statement, 9, team );
		sqlite3_bind_int( Statement, 10, colour );
		sqlite3_bind_text( Statement, 11, spoofedrealm.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			RowID = m_DB->LastRowID( );
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error adding gameplayer [" + UTIL_ToString( gameid ) + " : " + name + " : " + ip + " : " + UTIL_ToString( spoofed ) + " : " + spoofedrealm + " : " + UTIL_ToString( reserved ) + " : " + UTIL_ToString( loadingtime ) + " : " + UTIL_ToString( left ) + " : " + leftreason + " : " + UTIL_ToString( team ) + " : " + UTIL_ToString( colour ) + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error adding gameplayer [" + UTIL_ToString( gameid ) + " : " + name + " : " + ip + " : " + UTIL_ToString( spoofed ) + " : " + spoofedrealm + " : " + UTIL_ToString( reserved ) + " : " + UTIL_ToString( loadingtime ) + " : " + UTIL_ToString( left ) + " : " + leftreason + " : " + UTIL_ToString( team ) + " : " + UTIL_ToString( colour ) + "] - " + m_DB->GetError( ) );

	return RowID;
}

uint32_t CGHostDBSQLite :: GamePlayerCount( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	uint32_t Count = 0;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT COUNT(*) FROM gameplayers LEFT JOIN games ON games.id=gameid WHERE name=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_ROW )
			Count = sqlite3_column_int( Statement, 0 );
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error counting gameplayers [" + name + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error counting gameplayers [" + name + "] - " + m_DB->GetError( ) );

	return Count;
}

CDBGamePlayerSummary *CGHostDBSQLite :: GamePlayerSummaryCheck( string name )
{
	if( GamePlayerCount( name ) == 0 )
		return NULL;

	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	CDBGamePlayerSummary *GamePlayerSummary = NULL;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT MIN(datetime), MAX(datetime), COUNT(*), MIN(loadingtime), AVG(loadingtime), MAX(loadingtime), MIN(left/CAST(duration AS REAL))*100, AVG(left/CAST(duration AS REAL))*100, MAX(left/CAST(duration AS REAL))*100, MIN(duration), AVG(duration), MAX(duration) FROM gameplayers LEFT JOIN games ON games.id=gameid WHERE name=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_ROW )
		{
			if( sqlite3_column_count( (sqlite3_stmt *)Statement ) == 12 )
			{
				char *First = (char *)sqlite3_column_text( (sqlite3_stmt *)Statement, 0 );
				char *Last = (char *)sqlite3_column_text( (sqlite3_stmt *)Statement, 1 );
				string FirstGameDateTime;
				string LastGameDateTime;

				if( First )
					FirstGameDateTime = First;

				if( Last )
					LastGameDateTime = Last;

				uint32_t TotalGames = sqlite3_column_int( (sqlite3_stmt *)Statement, 2 );
				uint32_t MinLoadingTime = sqlite3_column_int( (sqlite3_stmt *)Statement, 3 );
				uint32_t AvgLoadingTime = sqlite3_column_int( (sqlite3_stmt *)Statement, 4 );
				uint32_t MaxLoadingTime = sqlite3_column_int( (sqlite3_stmt *)Statement, 5 );
				uint32_t MinLeftPercent = sqlite3_column_int( (sqlite3_stmt *)Statement, 6 );
				uint32_t AvgLeftPercent = sqlite3_column_int( (sqlite3_stmt *)Statement, 7 );
				uint32_t MaxLeftPercent = sqlite3_column_int( (sqlite3_stmt *)Statement, 8 );
				uint32_t MinDuration = sqlite3_column_int( (sqlite3_stmt *)Statement, 9 );
				uint32_t AvgDuration = sqlite3_column_int( (sqlite3_stmt *)Statement, 10 );
				uint32_t MaxDuration = sqlite3_column_int( (sqlite3_stmt *)Statement, 11 );
				GamePlayerSummary = new CDBGamePlayerSummary( string( ), name, FirstGameDateTime, LastGameDateTime, TotalGames, MinLoadingTime, AvgLoadingTime, MaxLoadingTime, MinLeftPercent, AvgLeftPercent, MaxLeftPercent, MinDuration, AvgDuration, MaxDuration );
			}
			else
				Print( "[SQLITE3] error checking gameplayersummary [" + name + "] - row doesn't have 12 columns" );
		}
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error checking gameplayersummary [" + name + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error checking gameplayersummary [" + name + "] - " + m_DB->GetError( ) );

	return GamePlayerSummary;
}

uint32_t CGHostDBSQLite :: DotAGameAdd( uint32_t gameid, uint32_t winner, uint32_t min, uint32_t sec )
{
	uint32_t RowID = 0;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "INSERT INTO dotagames ( gameid, winner, min, sec ) VALUES ( ?, ?, ?, ? )", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_int( Statement, 1, gameid );
		sqlite3_bind_int( Statement, 2, winner );
		sqlite3_bind_int( Statement, 3, min );
		sqlite3_bind_int( Statement, 4, sec );

		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			RowID = m_DB->LastRowID( );
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error adding dotagame [" + UTIL_ToString( gameid ) + " : " + UTIL_ToString( winner ) + " : " + UTIL_ToString( min ) + " : " + UTIL_ToString( sec ) + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error adding dotagame [" + UTIL_ToString( gameid ) + " : " + UTIL_ToString( winner ) + " : " + UTIL_ToString( min ) + " : " + UTIL_ToString( sec ) + "] - " + m_DB->GetError( ) );

	return RowID;
}

uint32_t CGHostDBSQLite :: DotAPlayerAdd( uint32_t gameid, uint32_t colour, uint32_t kills, uint32_t deaths, uint32_t creepkills, uint32_t creepdenies, uint32_t assists, uint32_t gold, uint32_t neutralkills, const string &item1, const string &item2, const string &item3, const string &item4, const string &item5, const string &item6, const string &hero, uint32_t newcolour, uint32_t towerkills, uint32_t raxkills, uint32_t courierkills )
{
	uint32_t RowID = 0;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "INSERT INTO dotaplayers ( gameid, colour, kills, deaths, creepkills, creepdenies, assists, gold, neutralkills, item1, item2, item3, item4, item5, item6, hero, newcolour, towerkills, raxkills, courierkills ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_int( Statement, 1, gameid );
		sqlite3_bind_int( Statement, 2, colour );
		sqlite3_bind_int( Statement, 3, kills );
		sqlite3_bind_int( Statement, 4, deaths );
		sqlite3_bind_int( Statement, 5, creepkills );
		sqlite3_bind_int( Statement, 6, creepdenies );
		sqlite3_bind_int( Statement, 7, assists );
		sqlite3_bind_int( Statement, 8, gold );
		sqlite3_bind_int( Statement, 9, neutralkills );
		sqlite3_bind_text( Statement, 10, item1.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 11, item2.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 12, item3.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 13, item4.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 14, item5.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 15, item6.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 16, hero.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_int( Statement, 17, newcolour );
		sqlite3_bind_int( Statement, 18, towerkills );
		sqlite3_bind_int( Statement, 19, raxkills );
		sqlite3_bind_int( Statement, 20, courierkills );

		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_DONE )
			RowID = m_DB->LastRowID( );
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error adding dotaplayer [" + UTIL_ToString( gameid ) + " : " + UTIL_ToString( colour ) + " : " + UTIL_ToString( kills ) + " : " + UTIL_ToString( deaths ) + " : " + UTIL_ToString( creepkills ) + " : " + UTIL_ToString( creepdenies ) + " : " + UTIL_ToString( assists ) + " : " + UTIL_ToString( gold ) + " : " + UTIL_ToString( neutralkills ) + " : " + item1 + " : " + item2 + " : " + item3 + " : " + item4 + " : " + item5 + " : " + item6 + " : " + hero + " : " + UTIL_ToString( newcolour ) + " : " + UTIL_ToString( towerkills ) + " : " + UTIL_ToString( raxkills ) + " : " + UTIL_ToString( courierkills ) + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error adding dotaplayer [" + UTIL_ToString( gameid ) + " : " + UTIL_ToString( colour ) + " : " + UTIL_ToString( kills ) + " : " + UTIL_ToString( deaths ) + " : " + UTIL_ToString( creepkills ) + " : " + UTIL_ToString( creepdenies ) + " : " + UTIL_ToString( assists ) + " : " + UTIL_ToString( gold ) + " : " + UTIL_ToString( neutralkills ) + " : " + item1 + " : " + item2 + " : " + item3 + " : " + item4 + " : " + item5 + " : " + item6 + " : " + hero + " : " + UTIL_ToString( newcolour ) + " : " + UTIL_ToString( towerkills ) + " : " + UTIL_ToString( raxkills ) + " : " + UTIL_ToString( courierkills ) + "] - " + m_DB->GetError( ) );

	return RowID;
}

uint32_t CGHostDBSQLite :: DotAPlayerCount( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	uint32_t Count = 0;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT COUNT(dotaplayers.id) FROM gameplayers LEFT JOIN games ON games.id=gameplayers.gameid LEFT JOIN dotaplayers ON dotaplayers.gameid=games.id AND dotaplayers.colour=gameplayers.colour WHERE name=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_ROW )
			Count = sqlite3_column_int( Statement, 0 );
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error counting dotaplayers [" + name + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error counting dotaplayers [" + name + "] - " + m_DB->GetError( ) );

	return Count;
}

CDBDotAPlayerSummary *CGHostDBSQLite :: DotAPlayerSummaryCheck( string name )
{
	if( DotAPlayerCount( name ) == 0 )
		return NULL;

	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	CDBDotAPlayerSummary *DotAPlayerSummary = NULL;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT COUNT(dotaplayers.id), SUM(kills), SUM(deaths), SUM(creepkills), SUM(creepdenies), SUM(assists), SUM(neutralkills), SUM(towerkills), SUM(raxkills), SUM(courierkills) FROM gameplayers LEFT JOIN games ON games.id=gameplayers.gameid LEFT JOIN dotaplayers ON dotaplayers.gameid=games.id AND dotaplayers.colour=gameplayers.colour WHERE name=?", (void **)&Statement );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_ROW )
		{
			if( sqlite3_column_count( (sqlite3_stmt *)Statement ) == 10 )
			{
				uint32_t TotalGames = sqlite3_column_int( (sqlite3_stmt *)Statement, 0 );
				uint32_t TotalWins = 0;
				uint32_t TotalLosses = 0;
				uint32_t TotalKills = sqlite3_column_int( (sqlite3_stmt *)Statement, 1 );
				uint32_t TotalDeaths = sqlite3_column_int( (sqlite3_stmt *)Statement, 2 );
				uint32_t TotalCreepKills = sqlite3_column_int( (sqlite3_stmt *)Statement, 3 );
				uint32_t TotalCreepDenies = sqlite3_column_int( (sqlite3_stmt *)Statement, 4 );
				uint32_t TotalAssists = sqlite3_column_int( (sqlite3_stmt *)Statement, 5 );
				uint32_t TotalNeutralKills = sqlite3_column_int( (sqlite3_stmt *)Statement, 6 );
				uint32_t TotalTowerKills = sqlite3_column_int( (sqlite3_stmt *)Statement, 7 );
				uint32_t TotalRaxKills = sqlite3_column_int( (sqlite3_stmt *)Statement, 8 );
				uint32_t TotalCourierKills = sqlite3_column_int( (sqlite3_stmt *)Statement, 9 );

				// calculate total wins

				sqlite3_stmt *Statement2;
				m_DB->Prepare( "SELECT COUNT(*) FROM gameplayers LEFT JOIN games ON games.id=gameplayers.gameid LEFT JOIN dotaplayers ON dotaplayers.gameid=games.id AND dotaplayers.colour=gameplayers.colour LEFT JOIN dotagames ON games.id=dotagames.gameid WHERE name=? AND ((winner=1 AND dotaplayers.newcolour>=1 AND dotaplayers.newcolour<=5) OR (winner=2 AND dotaplayers.newcolour>=7 AND dotaplayers.newcolour<=11))", (void **)&Statement2 );

				if( Statement2 )
				{
					sqlite3_bind_text( Statement2, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
					int RC2 = m_DB->Step( Statement2 );

					if( RC2 == SQLITE_ROW )
						TotalWins = sqlite3_column_int( Statement2, 0 );
					else if( RC2 == SQLITE_ERROR )
						Print( "[SQLITE3] error counting dotaplayersummary wins [" + name + "] - " + m_DB->GetError( ) );

					m_DB->Finalize( Statement2 );
				}
				else
					Print( "[SQLITE3] prepare error counting dotaplayersummary wins [" + name + "] - " + m_DB->GetError( ) );

				// calculate total losses

				sqlite3_stmt *Statement3;
				m_DB->Prepare( "SELECT COUNT(*) FROM gameplayers LEFT JOIN games ON games.id=gameplayers.gameid LEFT JOIN dotaplayers ON dotaplayers.gameid=games.id AND dotaplayers.colour=gameplayers.colour LEFT JOIN dotagames ON games.id=dotagames.gameid WHERE name=? AND ((winner=2 AND dotaplayers.newcolour>=1 AND dotaplayers.newcolour<=5) OR (winner=1 AND dotaplayers.newcolour>=7 AND dotaplayers.newcolour<=11))", (void **)&Statement3 );

				if( Statement3 )
				{
					sqlite3_bind_text( Statement3, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
					int RC3 = m_DB->Step( Statement3 );

					if( RC3 == SQLITE_ROW )
						TotalLosses = sqlite3_column_int( Statement3, 0 );
					else if( RC3 == SQLITE_ERROR )
						Print( "[SQLITE3] error counting dotaplayersummary losses [" + name + "] - " + m_DB->GetError( ) );

					m_DB->Finalize( Statement3 );
				}
				else
					Print( "[SQLITE3] prepare error counting dotaplayersummary losses [" + name + "] - " + m_DB->GetError( ) );

				// done

				DotAPlayerSummary = new CDBDotAPlayerSummary( string( ), name, TotalGames, TotalWins, TotalLosses, TotalKills, TotalDeaths, TotalCreepKills, TotalCreepDenies, TotalAssists, TotalNeutralKills, TotalTowerKills, TotalRaxKills, TotalCourierKills );
			}
			else
				Print( "[SQLITE3] error checking dotaplayersummary [" + name + "] - row doesn't have 7 columns" );
		}
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error checking dotaplayersummary [" + name + "] - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		Print( "[SQLITE3] prepare error checking dotaplayersummary [" + name + "] - " + m_DB->GetError( ) );

	return DotAPlayerSummary;
}

string CGHostDBSQLite :: FromCheck( uint32_t ip )
{
	// a big thank you to tjado for help with the iptocountry feature

	string From = "??";
	
	if( !FromCheckStmt )
		m_DB->Prepare( "SELECT country FROM iptocountry WHERE ip1<=? AND ip2>=?", (void **)&FromCheckStmt );

	if( FromCheckStmt )
	{
		// we bind the ip as an int64 because SQLite treats it as signed

		sqlite3_bind_int64( (sqlite3_stmt *)FromCheckStmt, 1, ip );
		sqlite3_bind_int64( (sqlite3_stmt *)FromCheckStmt, 2, ip );
		
		int RC = m_DB->Step( FromCheckStmt );

		if( RC == SQLITE_ROW )
		{
			vector<string> *Row = m_DB->GetRow( );

			if( Row->size( ) == 1 )
				From = (*Row)[0];
			else
				Print( "[SQLITE3] error checking iptocountry [" + UTIL_ToString( ip ) + "] - row doesn't have 1 column" );
		}
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error checking iptocountry [" + UTIL_ToString( ip ) + "] - " + m_DB->GetError( ) );

		m_DB->Reset( FromCheckStmt );
	}
	else
		Print( "[SQLITE3] prepare error checking iptocountry [" + UTIL_ToString( ip ) + "] - " + m_DB->GetError( ) );

	return From;
}

bool CGHostDBSQLite :: FromAdd( uint32_t ip1, uint32_t ip2, const string &country )
{
	// a big thank you to tjado for help with the iptocountry feature

	bool Success = false;

	if( !FromAddStmt )
		m_DB->Prepare( "INSERT INTO iptocountry VALUES ( ?, ?, ? )", (void **)&FromAddStmt );

	if( FromAddStmt )
	{
		// we bind the ip as an int64 because SQLite treats it as signed

		sqlite3_bind_int64( (sqlite3_stmt *)FromAddStmt, 1, ip1 );
		sqlite3_bind_int64( (sqlite3_stmt *)FromAddStmt, 2, ip2 );
		sqlite3_bind_text( (sqlite3_stmt *)FromAddStmt, 3, country.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = m_DB->Step( FromAddStmt );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			Print( "[SQLITE3] error adding iptocountry [" + UTIL_ToString( ip1 ) + " : " + UTIL_ToString( ip2 ) + " : " + country + "] - " + m_DB->GetError( ) );

		m_DB->Reset( FromAddStmt );
	}
	else
		Print( "[SQLITE3] prepare error adding iptocountry [" + UTIL_ToString( ip1 ) + " : " + UTIL_ToString( ip2 ) + " : " + country + "] - " + m_DB->GetError( ) );

	return Success;
}