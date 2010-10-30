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

#include <boost/thread.hpp>

//
// CQSLITE3 (wrapper class)
//

CSQLITE3 :: CSQLITE3( string filename )
{
	m_Ready = true;

	if( sqlite3_open_v2( filename.c_str( ), (sqlite3 **)&m_DB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL ) != SQLITE_OK )
        // if( sqlite3_open_v2( filename.c_str( ), (sqlite3 **)&m_DB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX , NULL ) != SQLITE_OK )
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

int CSQLITE3 :: Prepare( string query, void **Statement )
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

vector<string> SQLiteGetRow( sqlite3_stmt *Statement )
{
    vector<string> Result;    
    int RC = sqlite3_step( Statement );

    if( RC == SQLITE_ROW )
    {            
            for( int i = 0; i < sqlite3_column_count( Statement ); ++i )
            {
                    char *ColumnText = (char *)sqlite3_column_text( Statement, i );

                    if( ColumnText )
                            Result.push_back( ColumnText );
                    else
                            Result.push_back( string( ) );
            }
    }

    return Result;
}

int CSQLITE3 :: Finalize( void *Statement )
{
	return sqlite3_finalize( (sqlite3_stmt *)Statement );
}

int CSQLITE3 :: Reset( void *Statement )
{
	return sqlite3_reset( (sqlite3_stmt *)Statement );
}

int CSQLITE3 :: Exec( string query )
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

CGHostDBSQLite :: CGHostDBSQLite( CConfig *CFG ) : CGHostDB( CFG ), m_OutstandingCallables( 0 ), m_NumConnections( 0 )
{
	m_File = CFG->GetString( "db_sqlite3_file", "ghost.dbs" );
	CONSOLE_Print( "[SQLITE3] version " + string( SQLITE_VERSION ) );
	CONSOLE_Print( "[SQLITE3] opening database [" + m_File + "]" );
	m_DB = new CSQLITE3( m_File );	

	if( !m_DB->GetReady( ) )
	{
		// setting m_HasError to true indicates there's been a critical error and we want GHost to shutdown
		// this is okay here because we're in the constructor so we're not dropping any games or players

		CONSOLE_Print( string( "[SQLITE3] error opening database [" + m_File + "] - " ) + m_DB->GetError( ) );
		m_HasError = true;
		m_Error = "error opening database";
		return;
	}

	// find the schema number so we can determine whether we need to upgrade or not

	string SchemaNumber;
	sqlite3_stmt *Statement;
	m_DB->Prepare( "SELECT value FROM config WHERE name=\"schema_number\"", (void **) &Statement );

	if( Statement )
	{
		int RC = m_DB->Step( Statement );

		if( RC == SQLITE_ROW )
		{
			vector<string> *Row = m_DB->GetRow( );

			if( Row->size( ) == 1 )
				SchemaNumber = (*Row)[0];
			else
				CONSOLE_Print( "[SQLITE3] error getting schema number - row doesn't have 1 column" );
		}
		else if( RC == SQLITE_ERROR )
			CONSOLE_Print( "[SQLITE3] error getting schema number - " + m_DB->GetError( ) );

		m_DB->Finalize( Statement );
	}
	else
		CONSOLE_Print( "[SQLITE3] prepare error getting schema number - " + m_DB->GetError( ) );

	if( SchemaNumber.empty( ) )
	{
		// couldn't find the schema number
		// unfortunately the very first schema didn't have a config table
		// so we might actually be looking at schema version 1 rather than an empty database
		// try to confirm this by looking for the admins table

		CONSOLE_Print( "[SQLITE3] couldn't find admins table, assuming database is empty" );
		SchemaNumber = "1";
               
                if( m_DB->Exec( "CREATE TABLE admins ( id INTEGER PRIMARY KEY, name TEXT NOT NULL, server TEXT NOT NULL DEFAULT \"\" )" ) != SQLITE_OK )
                                CONSOLE_Print( "[SQLITE3] error creating admins table - " + m_DB->GetError( ) );

                if( m_DB->Exec( "CREATE TABLE bans ( id INTEGER PRIMARY KEY, server TEXT NOT NULL, name TEXT NOT NULL, ip TEXT, date TEXT NOT NULL, gamename TEXT, admin TEXT NOT NULL, reason TEXT )" ) != SQLITE_OK )
                        CONSOLE_Print( "[SQLITE3] error creating bans table - " + m_DB->GetError( ) );

                if( m_DB->Exec( "CREATE TABLE games ( id INTEGER PRIMARY KEY, server TEXT NOT NULL, map TEXT NOT NULL, datetime TEXT NOT NULL, gamename TEXT NOT NULL, ownername TEXT NOT NULL, duration INTEGER NOT NULL, gamestate INTEGER NOT NULL DEFAULT 0, creatorname TEXT NOT NULL DEFAULT \"\", creatorserver TEXT NOT NULL DEFAULT \"\" )" ) != SQLITE_OK )
                        CONSOLE_Print( "[SQLITE3] error creating games table - " + m_DB->GetError( ) );

                if( m_DB->Exec( "CREATE TABLE gameplayers ( id INTEGER PRIMARY KEY, gameid INTEGER NOT NULL, name TEXT NOT NULL, ip TEXT NOT NULL, spoofed INTEGER NOT NULL, reserved INTEGER NOT NULL, loadingtime INTEGER NOT NULL, left INTEGER NOT NULL, leftreason TEXT NOT NULL, team INTEGER NOT NULL, colour INTEGER NOT NULL, spoofedrealm TEXT NOT NULL DEFAULT \"\" )" ) != SQLITE_OK )
                        CONSOLE_Print( "[SQLITE3] error creating gameplayers table - " + m_DB->GetError( ) );

                if( m_DB->Exec( "CREATE TABLE dotagames ( id INTEGER PRIMARY KEY, gameid INTEGER NOT NULL, winner INTEGER NOT NULL, min INTEGER NOT NULL DEFAULT 0, sec INTEGER NOT NULL DEFAULT 0 )" ) != SQLITE_OK )
                        CONSOLE_Print( "[SQLITE3] error creating dotagames table - " + m_DB->GetError( ) );

                if( m_DB->Exec( "CREATE TABLE dotaplayers ( id INTEGER PRIMARY KEY, gameid INTEGER NOT NULL, colour INTEGER NOT NULL, kills INTEGER NOT NULL, deaths INTEGER NOT NULL, creepkills INTEGER NOT NULL, creepdenies INTEGER NOT NULL, assists INTEGER NOT NULL, gold INTEGER NOT NULL, neutralkills INTEGER NOT NULL, item1 TEXT NOT NULL, item2 TEXT NOT NULL, item3 TEXT NOT NULL, item4 TEXT NOT NULL, item5 TEXT NOT NULL, item6 TEXT NOT NULL, hero TEXT NOT NULL DEFAULT \"\", newcolour NOT NULL DEFAULT 0, towerkills NOT NULL DEFAULT 0, raxkills NOT NULL DEFAULT 0, courierkills NOT NULL DEFAULT 0 )" ) != SQLITE_OK )
                        CONSOLE_Print( "[SQLITE3] error creating dotaplayers table - " + m_DB->GetError( ) );

                if( m_DB->Exec( "CREATE TABLE config ( name TEXT NOT NULL PRIMARY KEY, value TEXT NOT NULL )" ) != SQLITE_OK )
                        CONSOLE_Print( "[SQLITE3] error creating config table - " + m_DB->GetError( ) );

                m_DB->Prepare( "INSERT INTO config VALUES ( \"schema_number\", ? )", (void **)&Statement );

                if( Statement )
                {
                        sqlite3_bind_text( Statement, 1, SchemaNumber.c_str( ), -1, SQLITE_TRANSIENT );
                        int RC = m_DB->Step( Statement );

                        if( RC == SQLITE_ERROR )
                                CONSOLE_Print( "[SQLITE3] error inserting schema number [" + SchemaNumber + "] - " + m_DB->GetError( ) );

                        m_DB->Finalize( Statement );
                }
                else
                        CONSOLE_Print( "[SQLITE3] prepare error inserting schema number [" + SchemaNumber + "] - " + m_DB->GetError( ) );


                if( m_DB->Exec( "CREATE INDEX idx_gameid ON gameplayers ( gameid )" ) != SQLITE_OK )
                        CONSOLE_Print( "[SQLITE3] error creating idx_gameid index on gameplayers table - " + m_DB->GetError( ) );

                if( m_DB->Exec( "CREATE INDEX idx_gameid_colour ON dotaplayers ( gameid, colour )" ) != SQLITE_OK )
                        CONSOLE_Print( "[SQLITE3] error creating idx_gameid_colour index on dotaplayers table - " + m_DB->GetError( ) );
	}
	else
		CONSOLE_Print( "[SQLITE3] found schema number [" + SchemaNumber + "]" );

	if( m_DB->Exec( "CREATE TEMPORARY TABLE iptocountry ( ip1 INTEGER NOT NULL, ip2 INTEGER NOT NULL, country TEXT NOT NULL, PRIMARY KEY ( ip1, ip2 ) )" ) != SQLITE_OK )
		CONSOLE_Print( "[SQLITE3] error creating temporary iptocountry table - " + m_DB->GetError( ) );


        // create the first connection

        // m_IdleConnections.push( m_DB );
}

CGHostDBSQLite :: ~CGHostDBSQLite( )
{
        CONSOLE_Print( "[SQLITE3] closing " + UTIL_ToString( m_IdleConnections.size( ) ) + "/" + UTIL_ToString( m_NumConnections ) + " idle MySQL connections" );

	while( !m_IdleConnections.empty( ) )
	{
		sqlite3_close( (sqlite3 *)m_IdleConnections.front( ) );
		m_IdleConnections.pop( );
	}

	if( m_OutstandingCallables > 0 )
		CONSOLE_Print( "[SQLITE3L] " + UTIL_ToString( m_OutstandingCallables ) + " outstanding callables were never recovered" );
        
	CONSOLE_Print( "[SQLITE3] closing database [" + m_File + "]" );
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

string CGHostDBSQLite :: GetStatus( )
{
	return "DB STATUS --- Connections: " + UTIL_ToString( m_IdleConnections.size( ) ) + "/" + UTIL_ToString( m_NumConnections ) + " idle. Outstanding callables: " + UTIL_ToString( m_OutstandingCallables ) + ".";
}

void CGHostDBSQLite :: RecoverCallable( CBaseCallable *callable )
{
	CSQLiteCallable *SQLiteCallable = dynamic_cast<CSQLiteCallable *>( callable );

	if( SQLiteCallable )
	{
                if( m_IdleConnections.size( ) > 15 )
		{
			sqlite3_close( (sqlite3 *)SQLiteCallable->GetConnection( ) );
                        --m_NumConnections;
		}
		else
			m_IdleConnections.push( SQLiteCallable->GetConnection( ) );

		if( m_OutstandingCallables == 0 )
			CONSOLE_Print( "[SQLITE3] recovered a mysql callable with zero outstanding" );
		else
                        --m_OutstandingCallables;

		if( !SQLiteCallable->GetError( ).empty( ) )
			CONSOLE_Print( "[SQLITE3] error --- " + SQLiteCallable->GetError( ) );
	}
	else
		CONSOLE_Print( "[MYSQL] tried to recover a non-mysql callable" );
}

void CGHostDBSQLite :: CreateThread( CBaseCallable *callable )
{
	try
	{
		boost :: thread Thread( boost :: ref( *callable ) );
	}
	catch( boost :: thread_resource_error tre )
	{
		CONSOLE_Print( "[SQLITE3] error spawning thread on attempt #1 [" + string( tre.what( ) ) + "], pausing execution and trying again in 50ms" );
		MILLISLEEP( 30 );

		try
		{
			boost :: thread Thread( boost :: ref( *callable ) );
		}
		catch( boost :: thread_resource_error tre2 )
		{
			CONSOLE_Print( "[SQLITE] error spawning thread on attempt #2 [" + string( tre2.what( ) ) + "], giving up" );
			callable->SetReady( true );
		}
	}
}

void *CGHostDBSQLite :: GetIdleConnection( )
{
	void *Connection = NULL;

	if( !m_IdleConnections.empty( ) )
	{
		Connection = m_IdleConnections.front( );
		m_IdleConnections.pop( );
	}

	return Connection;
}

//
// global helper functions
//

uint32_t SQLiteAdminCount( void *db, string *error, string server )
{
	uint32_t Count = 0;
        sqlite3_stmt *Statement;
	sqlite3_prepare_v2( (sqlite3 *)db, "SELECT COUNT(*) FROM admins WHERE server=?", -1, (sqlite3_stmt **) &Statement, NULL );

        if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_ROW )
			Count = sqlite3_column_int( Statement, 0 );
		else if( RC == SQLITE_ERROR )
			// *error = "[SQLITE3] error counting admins - " + sqlite3_errmsg( (sqlite3 *)db );
                        *error = "[SQLITE3] error counting admins";

		sqlite3_finalize( Statement );
	}
	else
		// *error = "[SQLITE3] prepare error counting admins - " + sqlite3_errmsg( (sqlite3 *)db );
                *error = "[SQLITE3] prepare error counting admins";

	return Count;
}

bool SQLiteAdminCheck( void *db, string *error, string server, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool IsAdmin = false;

        sqlite3_stmt *Statement;
	sqlite3_prepare_v2( (sqlite3 *)db, "SELECT * FROM admins WHERE server=? AND name=?", -1, (sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		
		int RC = sqlite3_step( Statement );

		// we're just checking to see if the query returned a row, we don't need to check the row data itself

		if( RC == SQLITE_ROW )
			IsAdmin = true;
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error checking admin [" + server + " : " + user + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error checking admin [" + server + " : " + user + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return IsAdmin;
}

bool SQLiteAdminAdd( void *db, string *error, string server, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "INSERT INTO admins ( server, name ) VALUES ( ?, ? )", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error adding admin [" + server + " : " + user + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error adding admin [" + server + " : " + user + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Success;
}

bool SQLiteAdminRemove( void *db, string *error, string server, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "DELETE FROM admins WHERE server=? AND name=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error removing admin [" + server + " : " + user + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error removing admin [" + server + " : " + user + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Success;
}

vector<string> SQLiteAdminList( void *db, string *error, string server )
{
	vector<string> AdminList;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "SELECT name FROM admins WHERE server=?", -1,(sqlite3_stmt**) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		while( RC == SQLITE_ROW )
		{
			vector<string> Row = SQLiteGetRow( Statement );

			if( Row.size( ) == 1 )
				AdminList.push_back( Row[0] );

			RC = sqlite3_step( Statement );
		}

		if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error retrieving admin list [" + server + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error retrieving admin list [" + server + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return AdminList;
}

uint32_t SQLiteBanCount( void *db, string *error, string server )
{
	uint32_t Count = 0;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "SELECT COUNT(*) FROM bans WHERE server=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_ROW )
			Count = sqlite3_column_int( Statement, 0 );
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error counting bans [" + server + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error counting bans [" + server + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Count;
}

CDBBan *SQLiteBanCheck( void *db, string *error, string server, string user, string ip )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	CDBBan *Ban = NULL;
        sqlite3_stmt *Statement;
	
	if( ip.empty( ) )
                 sqlite3_prepare_v2( (sqlite3 *)db, "SELECT name, ip, date, gamename, admin, reason FROM bans WHERE server=? AND name=?", -1,(sqlite3_stmt **) &Statement, NULL );
	else
                 sqlite3_prepare_v2( (sqlite3 *)db, "SELECT name, ip, date, gamename, admin, reason FROM bans WHERE (server=? AND name=?) OR ip=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );

		if( !ip.empty( ) )
			sqlite3_bind_text( Statement, 3, ip.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_ROW )
		{
			vector<string> Row = SQLiteGetRow( Statement );

			if( Row.size( ) == 6 )
				Ban = new CDBBan( server, Row[0], Row[1], Row[2], Row[3], Row[4], Row[5] );
			else
				*error = "[SQLITE3] error checking ban [" + server + " : " + user + " : " + ip + "] - row doesn't have 6 columns";
		}
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error checking ban [" + server + " : " + user + " : " + ip + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error checking ban [" + server + " : " + user + " : " + ip + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Ban;
}

bool SQLiteBanAdd( void *db, string *error, string server, string user, string ip, string gamename, string admin, string reason )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "INSERT INTO bans ( server, name, ip, date, gamename, admin, reason ) VALUES ( ?, ?, ?, date('now'), ?, ?, ? )", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 3, ip.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 4, gamename.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 5, admin.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 6, reason.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error adding ban [" + server + " : " + user + " : " + ip + " : " + gamename + " : " + admin + " : " + reason + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error =  "[SQLITE3] prepare error adding ban [" + server + " : " + user + " : " + ip + " : " + gamename + " : " + admin + " : " + reason + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Success;
}

bool SQLiteBanRemove( void *db, string *error, string server, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "DELETE FROM bans WHERE server=? AND name=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		sqlite3_bind_text( Statement, 2, user.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error removing ban [" + server + " : " + user + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error =  "[SQLITE3] prepare error removing ban [" + server + " : " + user + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Success;
}

bool SQLiteBanRemove( void *db, string *error, string user )
{
	transform( user.begin( ), user.end( ), user.begin( ), (int(*)(int))tolower );
	bool Success = false;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "DELETE FROM bans WHERE name=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, user.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error removing ban [" + user + "]";

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error removing ban [" + user + "]";

	return Success;
}

vector<CDBBan *> SQLiteBanList( void *db, string *error, string server )
{
	vector<CDBBan *> BanList;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "SELECT name, ip, date, gamename, admin, reason FROM bans WHERE server=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, server.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		while( RC == SQLITE_ROW )
		{
			vector<string> Row = SQLiteGetRow( Statement );

			if( Row.size( ) == 6 )
				BanList.push_back( new CDBBan( server, Row[0], (Row)[1], (Row)[2], (Row)[3], (Row)[4], (Row)[5] ) );

			RC = sqlite3_step( Statement );
		}

		if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error retrieving ban list [" + server + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error retrieving ban list [" + server + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return BanList;
}

uint32_t SQLiteGameAdd( void *db, string *error, string server, string map, string gamename, string ownername, uint32_t duration, uint32_t gamestate, string creatorname, string creatorserver )
{
	uint32_t RowID = 0;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "INSERT INTO games ( server, map, datetime, gamename, ownername, duration, gamestate, creatorname, creatorserver ) VALUES ( ?, ?, datetime('now'), ?, ?, ?, ?, ?, ? )", -1,(sqlite3_stmt **) &Statement, NULL );

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

		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			RowID = (uint32_t)sqlite3_last_insert_rowid( (sqlite3 *)db );
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error adding game [" + server + " : " + map + " : " + gamename + " : " + ownername + " : " + UTIL_ToString( duration ) + " : " + UTIL_ToString( gamestate ) + " : " + creatorname + " : " + creatorserver + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error adding game [" + server + " : " + map + " : " + gamename + " : " + ownername + " : " + UTIL_ToString( duration ) + " : " + UTIL_ToString( gamestate ) + " : " + creatorname + " : " + creatorserver + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return RowID;
}

uint32_t SQLiteGamePlayerAdd( void *db, string *error, uint32_t gameid, string name, string ip, uint32_t spoofed, string spoofedrealm, uint32_t reserved, uint32_t loadingtime, uint32_t left, string leftreason, uint32_t team, uint32_t colour )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	uint32_t RowID = 0;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "INSERT INTO gameplayers ( gameid, name, ip, spoofed, reserved, loadingtime, left, leftreason, team, colour, spoofedrealm ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )", -1,(sqlite3_stmt **) &Statement, NULL );

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

		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			RowID = (uint32_t)sqlite3_last_insert_rowid( (sqlite3 *)db );
		else if( RC == SQLITE_ERROR )
			*error =  "[SQLITE3] error adding gameplayer [" + UTIL_ToString( gameid ) + " : " + name + " : " + ip + " : " + UTIL_ToString( spoofed ) + " : " + spoofedrealm + " : " + UTIL_ToString( reserved ) + " : " + UTIL_ToString( loadingtime ) + " : " + UTIL_ToString( left ) + " : " + leftreason + " : " + UTIL_ToString( team ) + " : " + UTIL_ToString( colour ) + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error adding gameplayer [" + UTIL_ToString( gameid ) + " : " + name + " : " + ip + " : " + UTIL_ToString( spoofed ) + " : " + spoofedrealm + " : " + UTIL_ToString( reserved ) + " : " + UTIL_ToString( loadingtime ) + " : " + UTIL_ToString( left ) + " : " + leftreason + " : " + UTIL_ToString( team ) + " : " + UTIL_ToString( colour ) + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return RowID;
}

uint32_t SQLiteGamePlayerCount( void *db, string *error, string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	uint32_t Count = 0;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "SELECT COUNT(*) FROM gameplayers LEFT JOIN games ON games.id=gameid WHERE name=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_ROW )
			Count = sqlite3_column_int( Statement, 0 );
		else if( RC == SQLITE_ERROR )
			*error =  "[SQLITE3] error counting gameplayers [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error counting gameplayers [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Count;
}

CDBGamePlayerSummary *SQLiteGamePlayerSummaryCheck( void *db, string *error, string name )
{
	if( SQLiteGamePlayerCount( db, error, name ) == 0 )
		return NULL;

	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	CDBGamePlayerSummary *GamePlayerSummary = NULL;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "SELECT MIN(datetime), MAX(datetime), COUNT(*), MIN(loadingtime), AVG(loadingtime), MAX(loadingtime), MIN(left/CAST(duration AS REAL))*100, AVG(left/CAST(duration AS REAL))*100, MAX(left/CAST(duration AS REAL))*100, MIN(duration), AVG(duration), MAX(duration) FROM gameplayers LEFT JOIN games ON games.id=gameid WHERE name=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_ROW )
		{
			if( sqlite3_column_count( Statement ) == 12 )
			{
				char *First = (char *)sqlite3_column_text( Statement, 0 );
				char *Last = (char *)sqlite3_column_text( Statement, 1 );
				string FirstGameDateTime;
				string LastGameDateTime;

				if( First )
					FirstGameDateTime = First;

				if( Last )
					LastGameDateTime = Last;

				uint32_t TotalGames = sqlite3_column_int( Statement, 2 );
				uint32_t MinLoadingTime = sqlite3_column_int( Statement, 3 );
				uint32_t AvgLoadingTime = sqlite3_column_int( Statement, 4 );
				uint32_t MaxLoadingTime = sqlite3_column_int( Statement, 5 );
				uint32_t MinLeftPercent = sqlite3_column_int( Statement, 6 );
				uint32_t AvgLeftPercent = sqlite3_column_int( Statement, 7 );
				uint32_t MaxLeftPercent = sqlite3_column_int( Statement, 8 );
				uint32_t MinDuration = sqlite3_column_int( Statement, 9 );
				uint32_t AvgDuration = sqlite3_column_int( Statement, 10 );
				uint32_t MaxDuration = sqlite3_column_int( Statement, 11 );
				GamePlayerSummary = new CDBGamePlayerSummary( string( ), name, FirstGameDateTime, LastGameDateTime, TotalGames, MinLoadingTime, AvgLoadingTime, MaxLoadingTime, MinLeftPercent, AvgLeftPercent, MaxLeftPercent, MinDuration, AvgDuration, MaxDuration );
			}
			else
				*error = "[SQLITE3] error checking gameplayersummary [" + name + "] - row doesn't have 12 columns";
		}
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error checking gameplayersummary [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error checking gameplayersummary [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return GamePlayerSummary;
}

uint32_t SQLiteDotAGameAdd( void *db, string *error, uint32_t gameid, uint32_t winner, uint32_t min, uint32_t sec )
{
	uint32_t RowID = 0;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "INSERT INTO dotagames ( gameid, winner, min, sec ) VALUES ( ?, ?, ?, ? )", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_int( Statement, 1, gameid );
		sqlite3_bind_int( Statement, 2, winner );
		sqlite3_bind_int( Statement, 3, min );
		sqlite3_bind_int( Statement, 4, sec );

		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			RowID = (uint32_t)sqlite3_last_insert_rowid( (sqlite3 *)db );
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error adding dotagame [" + UTIL_ToString( gameid ) + " : " + UTIL_ToString( winner ) + " : " + UTIL_ToString( min ) + " : " + UTIL_ToString( sec ) + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error adding dotagame [" + UTIL_ToString( gameid ) + " : " + UTIL_ToString( winner ) + " : " + UTIL_ToString( min ) + " : " + UTIL_ToString( sec ) + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return RowID;
}

uint32_t SQLiteDotAPlayerAdd( void *db, string *error, uint32_t gameid, uint32_t colour, uint32_t kills, uint32_t deaths, uint32_t creepkills, uint32_t creepdenies, uint32_t assists, uint32_t gold, uint32_t neutralkills, string item1, string item2, string item3, string item4, string item5, string item6, string hero, uint32_t newcolour, uint32_t towerkills, uint32_t raxkills, uint32_t courierkills )
{
	uint32_t RowID = 0;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "INSERT INTO dotaplayers ( gameid, colour, kills, deaths, creepkills, creepdenies, assists, gold, neutralkills, item1, item2, item3, item4, item5, item6, hero, newcolour, towerkills, raxkills, courierkills ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )", -1,(sqlite3_stmt **) &Statement, NULL );

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

		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			RowID = (uint32_t)sqlite3_last_insert_rowid( (sqlite3 *)db );
		else if( RC == SQLITE_ERROR )
			*error =  "[SQLITE3] error adding dotaplayer [" + UTIL_ToString( gameid ) + " : " + UTIL_ToString( colour ) + " : " + UTIL_ToString( kills ) + " : " + UTIL_ToString( deaths ) + " : " + UTIL_ToString( creepkills ) + " : " + UTIL_ToString( creepdenies ) + " : " + UTIL_ToString( assists ) + " : " + UTIL_ToString( gold ) + " : " + UTIL_ToString( neutralkills ) + " : " + item1 + " : " + item2 + " : " + item3 + " : " + item4 + " : " + item5 + " : " + item6 + " : " + hero + " : " + UTIL_ToString( newcolour ) + " : " + UTIL_ToString( towerkills ) + " : " + UTIL_ToString( raxkills ) + " : " + UTIL_ToString( courierkills ) + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error =  "[SQLITE3] prepare error adding dotaplayer [" + UTIL_ToString( gameid ) + " : " + UTIL_ToString( colour ) + " : " + UTIL_ToString( kills ) + " : " + UTIL_ToString( deaths ) + " : " + UTIL_ToString( creepkills ) + " : " + UTIL_ToString( creepdenies ) + " : " + UTIL_ToString( assists ) + " : " + UTIL_ToString( gold ) + " : " + UTIL_ToString( neutralkills ) + " : " + item1 + " : " + item2 + " : " + item3 + " : " + item4 + " : " + item5 + " : " + item6 + " : " + hero + " : " + UTIL_ToString( newcolour ) + " : " + UTIL_ToString( towerkills ) + " : " + UTIL_ToString( raxkills ) + " : " + UTIL_ToString( courierkills ) + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return RowID;
}

uint32_t SQLiteDotAPlayerCount( void *db, string *error, string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	uint32_t Count = 0;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "SELECT COUNT(dotaplayers.id) FROM gameplayers LEFT JOIN games ON games.id=gameplayers.gameid LEFT JOIN dotaplayers ON dotaplayers.gameid=games.id AND dotaplayers.colour=gameplayers.colour WHERE name=?", -1,(sqlite3_stmt **) &Statement, NULL );
 
	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_ROW )
			Count = sqlite3_column_int( Statement, 0 );
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error counting dotaplayers [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error counting dotaplayers [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Count;
}

CDBDotAPlayerSummary *SQLiteDotAPlayerSummaryCheck( void *db, string *error, string name )
{
	if( SQLiteDotAPlayerCount( db, error, name ) == 0 )
		return NULL;

	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	CDBDotAPlayerSummary *DotAPlayerSummary = NULL;
	sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "SELECT COUNT(dotaplayers.id), SUM(kills), SUM(deaths), SUM(creepkills), SUM(creepdenies), SUM(assists), SUM(neutralkills), SUM(towerkills), SUM(raxkills), SUM(courierkills) FROM gameplayers LEFT JOIN games ON games.id=gameplayers.gameid LEFT JOIN dotaplayers ON dotaplayers.gameid=games.id AND dotaplayers.colour=gameplayers.colour WHERE name=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		sqlite3_bind_text( Statement, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
		int RC = sqlite3_step( Statement );

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
                                sqlite3_prepare_v2( (sqlite3 *)db, "SELECT COUNT(*) FROM gameplayers LEFT JOIN games ON games.id=gameplayers.gameid LEFT JOIN dotaplayers ON dotaplayers.gameid=games.id AND dotaplayers.colour=gameplayers.colour LEFT JOIN dotagames ON games.id=dotagames.gameid WHERE name=? AND ((winner=1 AND dotaplayers.newcolour>=1 AND dotaplayers.newcolour<=5) OR (winner=2 AND dotaplayers.newcolour>=7 AND dotaplayers.newcolour<=11))", -1,(sqlite3_stmt **) &Statement2, NULL );

				if( Statement2 )
				{
					sqlite3_bind_text( Statement2, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
					int RC2 = sqlite3_step( Statement2 );

					if( RC2 == SQLITE_ROW )
						TotalWins = sqlite3_column_int( Statement2, 0 );
					else if( RC2 == SQLITE_ERROR )
						*error = "[SQLITE3] error counting dotaplayersummary wins [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

					sqlite3_finalize( Statement2 );
				}
				else
					*error = "[SQLITE3] prepare error counting dotaplayersummary wins [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

				// calculate total losses

                                
				sqlite3_stmt *Statement3;
                                sqlite3_prepare_v2( (sqlite3 *)db, "SELECT COUNT(*) FROM gameplayers LEFT JOIN games ON games.id=gameplayers.gameid LEFT JOIN dotaplayers ON dotaplayers.gameid=games.id AND dotaplayers.colour=gameplayers.colour LEFT JOIN dotagames ON games.id=dotagames.gameid WHERE name=? AND ((winner=2 AND dotaplayers.newcolour>=1 AND dotaplayers.newcolour<=5) OR (winner=1 AND dotaplayers.newcolour>=7 AND dotaplayers.newcolour<=11))", -1,(sqlite3_stmt **) &Statement3, NULL );

				if( Statement3 )
				{
					sqlite3_bind_text( Statement3, 1, name.c_str( ), -1, SQLITE_TRANSIENT );
					int RC3 = sqlite3_step( Statement3 );

					if( RC3 == SQLITE_ROW )
						TotalLosses = sqlite3_column_int( Statement3, 0 );
					else if( RC3 == SQLITE_ERROR )
						*error = "[SQLITE3] error counting dotaplayersummary losses [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

					sqlite3_finalize( Statement3 );
				}
				else
					*error = "[SQLITE3] prepare error counting dotaplayersummary losses [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

				// done

				DotAPlayerSummary = new CDBDotAPlayerSummary( string( ), name, TotalGames, TotalWins, TotalLosses, TotalKills, TotalDeaths, TotalCreepKills, TotalCreepDenies, TotalAssists, TotalNeutralKills, TotalTowerKills, TotalRaxKills, TotalCourierKills );
			}
			else
				*error = "[SQLITE3] error checking dotaplayersummary [" + name + "] - row doesn't have 7 columns";
		}
		else if( RC == SQLITE_ERROR )
			*error = "[SQLITE3] error checking dotaplayersummary [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error checking dotaplayersummary [" + name + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return DotAPlayerSummary;
}

string SQLiteFromCheck( void *db, string *error, uint32_t ip )
{
	// a big thank you to tjado for help with the iptocountry feature

	string From = "??";
        sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "SELECT country FROM iptocountry WHERE ip1<=? AND ip2>=?", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		// we bind the ip as an int64 because SQLite treats it as signed

		sqlite3_bind_int64( Statement, 1, ip );
		sqlite3_bind_int64( Statement, 2, ip );
		
		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_ROW )
		{
			vector<string> Row = SQLiteGetRow( Statement );

			if( Row.size( ) == 1 )
				From = (Row)[0];
			else
				*error =  "[SQLITE3] error checking iptocountry [" + UTIL_ToString( ip ) + "] - row doesn't have 1 column";
		}
		else if( RC == SQLITE_ERROR )
			*error =  "[SQLITE3] error checking iptocountry [" + UTIL_ToString( ip ) + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error = "[SQLITE3] prepare error checking iptocountry [" + UTIL_ToString( ip ) + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return From;
}

bool SQLiteFromAdd( void *db, string *error, uint32_t ip1, uint32_t ip2, string country )
{
	// a big thank you to tjado for help with the iptocountry feature

	bool Success = false;
        sqlite3_stmt *Statement;
        sqlite3_prepare_v2( (sqlite3 *)db, "INSERT INTO iptocountry VALUES ( ?, ?, ? )", -1,(sqlite3_stmt **) &Statement, NULL );

	if( Statement )
	{
		// we bind the ip as an int64 because SQLite treats it as signed

		sqlite3_bind_int64( Statement, 1, ip1 );
		sqlite3_bind_int64( Statement, 2, ip2 );
		sqlite3_bind_text( Statement, 3, country.c_str( ), -1, SQLITE_TRANSIENT );

		int RC = sqlite3_step( Statement );

		if( RC == SQLITE_DONE )
			Success = true;
		else if( RC == SQLITE_ERROR )
			*error =  "[SQLITE3] error adding iptocountry [" + UTIL_ToString( ip1 ) + " : " + UTIL_ToString( ip2 ) + " : " + country + "] - " + sqlite3_errmsg( (sqlite3 *)db );

		sqlite3_finalize( Statement );
	}
	else
		*error =  "[SQLITE3] prepare error adding iptocountry [" + UTIL_ToString( ip1 ) + " : " + UTIL_ToString( ip2 ) + " : " + country + "] - " + sqlite3_errmsg( (sqlite3 *)db );

	return Success;
}

CCallableAdminCount *CGHostDBSQLite :: ThreadedAdminCount( string server )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableAdminCount *Callable = new CSQLiteCallableAdminCount( server, Connection, m_File );
        CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableAdminCheck *CGHostDBSQLite :: ThreadedAdminCheck( string server, string user )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableAdminCheck *Callable = new CSQLiteCallableAdminCheck( server, user, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableAdminAdd *CGHostDBSQLite :: ThreadedAdminAdd( string server, string user )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableAdminAdd *Callable = new CSQLiteCallableAdminAdd( server, user, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableFromAdd *CGHostDBSQLite :: ThreadedFromAdd( uint32_t ip1, uint32_t ip2, string country )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableFromAdd *Callable = new CSQLiteCallableFromAdd( ip1, ip2, country, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableFromCheck *CGHostDBSQLite :: ThreadedFromCheck( uint32_t ip )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableFromCheck *Callable = new CSQLiteCallableFromCheck( ip, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableAdminRemove *CGHostDBSQLite :: ThreadedAdminRemove( string server, string user )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableAdminRemove *Callable = new CSQLiteCallableAdminRemove( server, user, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableAdminList *CGHostDBSQLite :: ThreadedAdminList( string server )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableAdminList *Callable = new CSQLiteCallableAdminList( server, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableBanCount *CGHostDBSQLite :: ThreadedBanCount( string server )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableBanCount *Callable = new CSQLiteCallableBanCount( server, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableBanCheck *CGHostDBSQLite :: ThreadedBanCheck( string server, string user, string ip )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableBanCheck *Callable = new CSQLiteCallableBanCheck( server, user, ip, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableBanAdd *CGHostDBSQLite :: ThreadedBanAdd( string server, string user, string ip, string gamename, string admin, string reason )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableBanAdd *Callable = new CSQLiteCallableBanAdd( server, user, ip, gamename, admin, reason, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableBanRemove *CGHostDBSQLite :: ThreadedBanRemove( string server, string user )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableBanRemove *Callable = new CSQLiteCallableBanRemove( server, user, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableBanRemove *CGHostDBSQLite :: ThreadedBanRemove( string user )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableBanRemove *Callable = new CSQLiteCallableBanRemove( string( ), user, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableBanList *CGHostDBSQLite :: ThreadedBanList( string server )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableBanList *Callable = new CSQLiteCallableBanList( server, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableGameAdd *CGHostDBSQLite :: ThreadedGameAdd( string server, string map, string gamename, string ownername, uint32_t duration, uint32_t gamestate, string creatorname, string creatorserver )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableGameAdd *Callable = new CSQLiteCallableGameAdd( server, map, gamename, ownername, duration, gamestate, creatorname, creatorserver, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableGamePlayerAdd *CGHostDBSQLite :: ThreadedGamePlayerAdd( uint32_t gameid, string name, string ip, uint32_t spoofed, string spoofedrealm, uint32_t reserved, uint32_t loadingtime, uint32_t left, string leftreason, uint32_t team, uint32_t colour )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableGamePlayerAdd *Callable = new CSQLiteCallableGamePlayerAdd( gameid, name, ip, spoofed, spoofedrealm, reserved, loadingtime, left, leftreason, team, colour, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableGamePlayerSummaryCheck *CGHostDBSQLite :: ThreadedGamePlayerSummaryCheck( string name )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableGamePlayerSummaryCheck *Callable = new CSQLiteCallableGamePlayerSummaryCheck( name, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableDotAGameAdd *CGHostDBSQLite :: ThreadedDotAGameAdd( uint32_t gameid, uint32_t winner, uint32_t min, uint32_t sec )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableDotAGameAdd *Callable = new CSQLiteCallableDotAGameAdd( gameid, winner, min, sec, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableDotAPlayerAdd *CGHostDBSQLite :: ThreadedDotAPlayerAdd( uint32_t gameid, uint32_t colour, uint32_t kills, uint32_t deaths, uint32_t creepkills, uint32_t creepdenies, uint32_t assists, uint32_t gold, uint32_t neutralkills, string item1, string item2, string item3, string item4, string item5, string item6, string hero, uint32_t newcolour, uint32_t towerkills, uint32_t raxkills, uint32_t courierkills )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;

	CCallableDotAPlayerAdd *Callable = new CSQLiteCallableDotAPlayerAdd( gameid, colour, kills, deaths, creepkills, creepdenies, assists, gold, neutralkills, item1, item2, item3, item4, item5, item6, hero, newcolour, towerkills, raxkills, courierkills, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

CCallableDotAPlayerSummaryCheck *CGHostDBSQLite :: ThreadedDotAPlayerSummaryCheck( string name )
{
        void *Connection = GetIdleConnection( );

	if( !Connection )
                ++m_NumConnections;
        
	CCallableDotAPlayerSummaryCheck *Callable = new CSQLiteCallableDotAPlayerSummaryCheck( name, Connection, m_File );
	CreateThread( Callable );
        ++m_OutstandingCallables;
	return Callable;
}

//
// SQLite Callables
//

void CSQLiteCallable :: Init( )
{
	CBaseCallable :: Init( );

        if( !m_Connection )
	{
            if( sqlite3_open_v2( m_File.c_str( ), (sqlite3 **)&m_Connection, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL ) != SQLITE_OK )
            {
		 m_Error = "[SQLITE3] Couldn't connect to database";
            }
	}
}

void CSQLiteCallable :: Close( )
{	
	CBaseCallable :: Close( );
}

void CSQLiteCallableAdminCount :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteAdminCount( m_Connection, &m_Error, m_Server );

	Close( );
}

void CSQLiteCallableAdminCheck :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteAdminCheck( m_Connection, &m_Error, m_Server, m_User );

	Close( );
}

void CSQLiteCallableAdminAdd :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteAdminAdd( m_Connection, &m_Error, m_Server, m_User );

	Close( );
}

void CSQLiteCallableFromAdd :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteFromAdd( m_Connection, &m_Error, m_IP1, m_IP2, m_Country);

	Close( );
}

void CSQLiteCallableFromCheck :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteFromCheck( m_Connection, &m_Error, m_IP );

	Close( );
}


void CSQLiteCallableAdminRemove :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteAdminRemove( m_Connection, &m_Error, m_Server, m_User );

	Close( );
}

void CSQLiteCallableAdminList :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteAdminList( m_Connection, &m_Error, m_Server );

	Close( );
}

void CSQLiteCallableBanCount :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteBanCount( m_Connection, &m_Error, m_Server );

	Close( );
}

void CSQLiteCallableBanCheck :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteBanCheck( m_Connection, &m_Error, m_Server, m_User, m_IP );

	Close( );
}

void CSQLiteCallableBanAdd :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteBanAdd( m_Connection, &m_Error, m_Server, m_User, m_IP, m_GameName, m_Admin, m_Reason );

	Close( );
}

void CSQLiteCallableBanRemove :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
	{
		if( m_Server.empty( ) )
			m_Result = SQLiteBanRemove( m_Connection, &m_Error, m_User );
		else
			m_Result = SQLiteBanRemove( m_Connection, &m_Error, m_Server, m_User );
	}

	Close( );
}

void CSQLiteCallableBanList :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteBanList( m_Connection, &m_Error, m_Server );

	Close( );
}

void CSQLiteCallableGameAdd :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteGameAdd( m_Connection, &m_Error, m_Server, m_Map, m_GameName, m_OwnerName, m_Duration, m_GameState, m_CreatorName, m_CreatorServer );

	Close( );
}

void CSQLiteCallableGamePlayerAdd :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteGamePlayerAdd( m_Connection, &m_Error, m_GameID, m_Name, m_IP, m_Spoofed, m_SpoofedRealm, m_Reserved, m_LoadingTime, m_Left, m_LeftReason, m_Team, m_Colour );

	Close( );
}

void CSQLiteCallableGamePlayerSummaryCheck :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteGamePlayerSummaryCheck( m_Connection, &m_Error, m_Name );

	Close( );
}

void CSQLiteCallableDotAGameAdd :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteDotAGameAdd( m_Connection, &m_Error, m_GameID, m_Winner, m_Min, m_Sec );

	Close( );
}

void CSQLiteCallableDotAPlayerAdd :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteDotAPlayerAdd( m_Connection, &m_Error, m_GameID, m_Colour, m_Kills, m_Deaths, m_CreepKills, m_CreepDenies, m_Assists, m_Gold, m_NeutralKills, m_Item1, m_Item2, m_Item3, m_Item4, m_Item5, m_Item6, m_Hero, m_NewColour, m_TowerKills, m_RaxKills, m_CourierKills );

	Close( );
}

void CSQLiteCallableDotAPlayerSummaryCheck :: operator( )( )
{
	Init( );

	if( m_Error.empty( ) )
		m_Result = SQLiteDotAPlayerSummaryCheck( m_Connection, &m_Error, m_Name );

	Close( );
}