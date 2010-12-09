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
#include "crc32.h"
#include "sha1.h"
#include "csvparser.h"
#include "config.h"
#include "language.h"
#include "socket.h"
#include "ghostdb.h"
#include "ghostdbsqlite.h"
#include "bnet.h"
#include "map.h"
#include "gameplayer.h"
#include "gameprotocol.h"
#include "gpsprotocol.h"
#include "game_base.h"
#include "game.h"
#include "irc.h"

#include <csignal>
#include <cstdlib>
#include <ctime>

#define __STORMLIB_SELF__
#include <stormlib/StormLib.h>

#ifdef WIN32
 #include <ws2tcpip.h>
 #include <winsock.h>
 #include <process.h>
#else
 #include <sys/time.h>
#endif

#ifdef __APPLE__
 #include <mach/mach_time.h>
#endif

CGHost *gGHost = NULL;

uint32_t GetTime( )
{
	#ifdef WIN32
	// don't use GetTickCount anymore because it's not accurate enough (~16ms resolution)
	// don't use QueryPerformanceCounter anymore because it isn't guaranteed to be strictly increasing on some systems and thus requires "smoothing" code
	// use timeGetTime instead, which typically has a high resolution (5ms or more) but we request a lower resolution on startup

	return timeGetTime( ) / 1000;
#elif __APPLE__
	uint64_t current = mach_absolute_time( );
	static mach_timebase_info_data_t info = { 0, 0 };
	// get timebase info
	if( info.denom == 0 )
		mach_timebase_info( &info );
	uint64_t elapsednano = current * ( info.numer / info.denom );
	// convert ns to ms
	return elapsednano / 1e9;
#else
	uint32_t ticks;
	struct timespec t;
	clock_gettime( CLOCK_MONOTONIC, &t );
	ticks = t.tv_sec * 1000;
	ticks += t.tv_nsec / 1e9;
	return ticks;
#endif
}

uint32_t GetTicks( )
{
#ifdef WIN32
	// don't use GetTickCount anymore because it's not accurate enough (~16ms resolution)
	// don't use QueryPerformanceCounter anymore because it isn't guaranteed to be strictly increasing on some systems and thus requires "smoothing" code
	// use timeGetTime instead, which typically has a high resolution (5ms or more) but we request a lower resolution on startup

	return timeGetTime( );
#elif __APPLE__
	uint64_t current = mach_absolute_time( );
	static mach_timebase_info_data_t info = { 0, 0 };
	// get timebase info
	if( info.denom == 0 )
		mach_timebase_info( &info );
	uint64_t elapsednano = current * ( info.numer / info.denom );
	// convert ns to ms
	return elapsednano / 1e6;
#else
	uint32_t ticks;
	struct timespec t;
	clock_gettime( CLOCK_MONOTONIC, &t );
	ticks = t.tv_sec * 1000;
	ticks += t.tv_nsec / 1000000;
	return ticks;
#endif
}

void SignalCatcher( int s )
{
	CONSOLE_Print( "[!!!] caught signal " + UTIL_ToString( s ) + ", exiting NOW" );

	if( gGHost )
	{
		if( gGHost->m_Exiting )
			exit( 1 );
		else
			gGHost->m_Exiting = true;
	}
	else
		exit( 1 );
}

void CONSOLE_Print( const string &message )
{
	cout << message << endl;	

	if( gGHost )
		if( gGHost->m_IRC )
			gGHost->m_IRC->SendDCC( message );
}

void CONSOLE_Print2( const string &message )
{
	cout << message << endl;
	
}

void CONSOLE_Print3( const string &message )
{
	cout << message << endl;
	
	if( gGHost )
	{
		if( gGHost->m_IRC )
		{
			gGHost->m_IRC->SendMessageIRC( message, string( ) );
			gGHost->m_IRC->SendDCC( message );
		}
	}
}

void IRC_Print( const string &message )
{
	if( gGHost )
		if( gGHost->m_IRC )
			gGHost->m_IRC->SendMessageIRC( message, string( ) );
}

//
// main
//

int main( )
{
	srand( time( NULL ) );

	// read config file

	CConfig CFG;
	CFG.Read( "default.cfg" );

	CONSOLE_Print( "[AURA] starting up" );

	// signal( SIGABRT, SignalCatcher );
	signal( SIGINT, SignalCatcher );

#ifndef WIN32
	// disable SIGPIPE since some systems like OS X don't define MSG_NOSIGNAL

	signal( SIGPIPE, SIG_IGN );
#endif

#ifdef WIN32
	// initialize timer resolution
	// attempt to set the resolution as low as possible from 1ms to 5ms

	unsigned int TimerResolution = 0;

	for( unsigned int i = 1; i <= 5; ++i )
	{
		if( timeBeginPeriod( i ) == TIMERR_NOERROR )
		{
			TimerResolution = i;
			break;
		}
		else if( i < 5 )
			CONSOLE_Print( "[AURA] error setting Windows timer resolution to " + UTIL_ToString( i ) + " milliseconds, trying a higher resolution" );
		else
		{
			CONSOLE_Print( "[AURA] error setting Windows timer resolution" );
			return 1;
		}
	}

	CONSOLE_Print( "[AURA] using Windows timer with resolution " + UTIL_ToString( TimerResolution ) + " milliseconds" );
#elif __APPLE__
	// not sure how to get the resolution
#else
	// print the timer resolution

	struct timespec Resolution;

	if( clock_getres( CLOCK_MONOTONIC, &Resolution ) == -1 )
		CONSOLE_Print( "[AURA] error getting monotonic timer resolution" );
	else
		CONSOLE_Print( "[AURA] using monotonic timer with resolution " + UTIL_ToString( (double)( Resolution.tv_nsec / 1000 ), 2 ) + " microseconds" );
#endif

#ifdef WIN32
	// initialize winsock

	CONSOLE_Print( "[AURA] starting winsock" );
	WSADATA wsadata;

	if( WSAStartup( MAKEWORD( 2, 2 ), &wsadata ) != 0 )
	{
		CONSOLE_Print( "[AURA] error starting winsock" );
		return 1;
	}

	// increase process priority

	CONSOLE_Print( "[AURA] setting process priority to \"high\"" );
	SetPriorityClass( GetCurrentProcess( ), HIGH_PRIORITY_CLASS );
#endif

	// initialize ghost

	gGHost = new CGHost( &CFG );

	while( true )
	{
		// block for 50ms on all sockets - if you intend to perform any timed actions more frequently you should change this
		// that said it's likely we'll loop more often than this due to there being data waiting on one of the sockets but there aren't any guarantees

		if( gGHost->Update( 50000 ) )
			break;
	}

	// shutdown ghost

	CONSOLE_Print( "[AURA] shutting down" );
	delete gGHost;
	gGHost = NULL;

#ifdef WIN32
	// shutdown winsock

	CONSOLE_Print( "[AURA] shutting down winsock" );
	WSACleanup( );

	// shutdown timer

	timeEndPeriod( TimerResolution );
#endif

	return 0;
}

//
// CGHost
//

CGHost :: CGHost( CConfig *CFG ) : m_IRC( NULL ), m_ReconnectSocket( NULL ), m_CurrentGame( NULL ), m_Language( NULL ), m_Exiting( false ), m_Enabled( true ), m_Version( "0.84" ), m_HostCounter( 1 )
{
	vector<string> channels, locals;

	for( int i = 0; i < 10; ++i )
	{
		string line;

		if( i == 1 )
			line = CFG->GetString( "irc_channel", string( ) );
		else
			line = CFG->GetString( "irc_channel" + UTIL_ToString( i ), string( ) );

		if( !line.empty( ) )
			channels.push_back( "#" + line );

		line.clear( );

		if( i == 1 )
			line = CFG->GetString( "dcc_local", string( ) );
		else
			line = CFG->GetString( "dcc_local" + UTIL_ToString( i ), string( ) );

		if( !line.empty( ) )
			locals.push_back( line );
	}

	m_IRC = new CIRC( this, CFG->GetString( "irc_server", string( ) ), CFG->GetString( "irc_nickname", string( ) ), CFG->GetString( "irc_username", string( ) ), CFG->GetString( "irc_password", string( ) ), channels, CFG->GetInt( "irc_port", 6667 ), CFG->GetString( "irc_commandtrigger", "!" ), locals );
	
        m_UDPSocket = new CUDPSocket( );
	m_UDPSocket->SetBroadcastTarget( CFG->GetString( "udp_broadcasttarget", string( ) ) );
	m_UDPSocket->SetDontRoute( CFG->GetInt( "udp_dontroute", 0 ) == 0 ? false : true );
	m_GPSProtocol = new CGPSProtocol( );
	m_CRC = new CCRC32( );
	m_CRC->Initialize( );
	m_SHA = new CSHA1( );	
	m_HostPort = CFG->GetInt( "bot_hostport", 6112 );
	m_Reconnect = CFG->GetInt( "bot_reconnect", 1 ) == 0 ? false : true;
	m_ReconnectPort = CFG->GetInt( "bot_reconnectport", 6114 );
	m_DefaultMap = CFG->GetString( "bot_defaultmap", "dota" );
	m_LANWar3Version = CFG->GetInt( "lan_war3version", 24 );
	
	CONSOLE_Print( "[AURA] opening primary database" );
	m_DB = new CGHostDBSQLite( CFG );
	
	SetConfigs( CFG );

	// load the battle.net connections
	// we're just loading the config data and creating the CBNET classes here, the connections are established later (in the Update function)

	for( int i = 1; i < 10; ++i )
	{
		string Prefix;

		if( i == 1 )
			Prefix = "bnet_";
		else
			Prefix = "bnet" + UTIL_ToString( i ) + "_";

		string Server = CFG->GetString( Prefix + "server", string( ) );
		string ServerAlias = CFG->GetString( Prefix + "serveralias", string( ) );
		string CDKeyROC = CFG->GetString( Prefix + "cdkeyroc", string( ) );
		string CDKeyTFT = CFG->GetString( Prefix + "cdkeytft", string( ) );
		string CountryAbbrev = CFG->GetString( Prefix + "countryabbrev", "USA" );
		string Country = CFG->GetString( Prefix + "country", "United States" );
		string Locale = CFG->GetString( Prefix + "locale", "system" );
		uint32_t LocaleID;

		if( Locale == "system" )
		{
			LocaleID = 1033;
		}
		else
			LocaleID = UTIL_ToUInt32( Locale );

		string UserName = CFG->GetString( Prefix + "username", string( ) );
		string UserPassword = CFG->GetString( Prefix + "password", string( ) );
		string FirstChannel = CFG->GetString( Prefix + "firstchannel", "The Void" );
		string RootAdmin = CFG->GetString( Prefix + "rootadmin", string( ) );
		string BNETCommandTrigger = CFG->GetString( Prefix + "commandtrigger", "!" );

		if( BNETCommandTrigger.empty( ) )
			BNETCommandTrigger = "!";
		
		unsigned char War3Version = CFG->GetInt( Prefix + "custom_war3version", 24 );
		BYTEARRAY EXEVersion = UTIL_ExtractNumbers( CFG->GetString( Prefix + "custom_exeversion", string( ) ), 4 );
		BYTEARRAY EXEVersionHash = UTIL_ExtractNumbers( CFG->GetString( Prefix + "custom_exeversionhash", string( ) ), 4 );
		string PasswordHashType = CFG->GetString( Prefix + "custom_passwordhashtype", string( ) );
		uint32_t MaxMessageLength = CFG->GetInt( Prefix + "custom_maxmessagelength", 200 );

		if( Server.empty( ) )
			break;

		CONSOLE_Print( "[GHOST] found battle.net connection #" + UTIL_ToString( i ) + " for server [" + Server + "]" );

		if( Locale == "system" )
		{
			CONSOLE_Print( "[AURA] using system locale of " + UTIL_ToString( LocaleID ) );
		}

		m_BNETs.push_back( new CBNET( this, Server, ServerAlias, CDKeyROC, CDKeyTFT, CountryAbbrev, Country, LocaleID, UserName, UserPassword, FirstChannel, RootAdmin, BNETCommandTrigger[0], War3Version, EXEVersion, EXEVersionHash, PasswordHashType, MaxMessageLength, i ) );
	}

	if( m_BNETs.empty( ) )
		CONSOLE_Print( "[AURA] warning - no battle.net connections found in config file" );

	// extract common.j and blizzard.j from War3Patch.mpq if we can
	// these two files are necessary for calculating "map_crc" when loading maps so we make sure to do it before loading the default map
	// see CMap :: Load for more information

	ExtractScripts( );

	// load the default maps (note: make sure to run ExtractScripts first)

	if( m_DefaultMap.size( ) < 4 || m_DefaultMap.substr( m_DefaultMap.size( ) - 4 ) != ".cfg" )	
		m_DefaultMap += ".cfg";		

	CConfig MapCFG;
	MapCFG.Read( m_MapCFGPath + m_DefaultMap );
	m_Map = new CMap( this, &MapCFG, m_MapCFGPath + m_DefaultMap );

	// load the iptocountry data

	LoadIPToCountryData( );

	if( m_BNETs.empty( ) )
		CONSOLE_Print( "[AURA] warning - no battle.net connections found" );

	CONSOLE_Print( "[AURA] Aura++ Version " + m_Version + " (with over 9000 ECONNRESETs)" );
}

CGHost :: ~CGHost( )
{
	delete m_UDPSocket;
	delete m_ReconnectSocket;

	for( vector<CTCPSocket *> :: iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); ++i )
		delete *i;

	delete m_GPSProtocol;
	delete m_CRC;
	delete m_SHA;

	for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		delete *i;

	delete m_CurrentGame;

	for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
		delete *i;

	delete m_DB;
	delete m_Language;
	delete m_Map;
	delete m_IRC;
}

inline bool CGHost :: Update( unsigned long usecBlock )
{
	// create the GProxy++ reconnect listener

	if( m_Reconnect )
	{
		if( !m_ReconnectSocket )
		{
			m_ReconnectSocket = new CTCPServer( );

			if( m_ReconnectSocket->Listen( m_BindAddress, m_ReconnectPort ) )
				CONSOLE_Print( "[AURA] listening for GProxy++ reconnects on port " + UTIL_ToString( m_ReconnectPort ) );
			else
			{
				CONSOLE_Print( "[AURA] error listening for GProxy++ reconnects on port " + UTIL_ToString( m_ReconnectPort ) );
				delete m_ReconnectSocket;
				m_ReconnectSocket = NULL;
				m_Reconnect = false;
			}
		}
		else if( m_ReconnectSocket->HasError( ) )
		{
			CONSOLE_Print( "[AURA] GProxy++ reconnect listener error (" + m_ReconnectSocket->GetErrorString( ) + ")" );
			delete m_ReconnectSocket;
			m_ReconnectSocket = NULL;
			m_Reconnect = false;
		}
	}

	unsigned int NumFDs = 0;

	// take every socket we own and throw it in one giant select statement so we can block on all sockets

	int nfds = 0;
	fd_set fd;
	fd_set send_fd;
	FD_ZERO( &fd );
	FD_ZERO( &send_fd );	

	// 1. the current game's server and player sockets

	if( m_CurrentGame )
		NumFDs += m_CurrentGame->SetFD( &fd, &send_fd, &nfds );

	// 2. all running games' player sockets

	for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
		NumFDs += (*i)->SetFD( &fd, &send_fd, &nfds );

	// 3. all battle.net sockets

	for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		NumFDs += (*i)->SetFD( &fd, &send_fd, &nfds );

	// 4. IRC and DCC

	NumFDs += m_IRC->SetFD( &fd, &send_fd, &nfds );

	for( vector<CDCC *> :: iterator i = m_IRC->m_DCC.begin( ); i != m_IRC->m_DCC.end( ); ++i )
		NumFDs += (*i)->SetFD( &fd, &send_fd, &nfds );

	// 5. the GProxy++ reconnect socket(s)

	if( m_Reconnect && m_ReconnectSocket )
	{
		m_ReconnectSocket->SetFD( &fd, &send_fd, &nfds );
		++NumFDs;
	}

	for( vector<CTCPSocket *> :: iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); ++i )
	{
		(*i)->SetFD( &fd, &send_fd, &nfds );
		++NumFDs;
	}

	// before we call select we need to determine how long to block for
	// previously we just blocked for a maximum of the passed usecBlock microseconds
	// however, in an effort to make game updates happen closer to the desired latency setting we now use a dynamic block interval
	// note: we still use the passed usecBlock as a hard maximum

	for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); ++i )
	{
		if( (*i)->GetNextTimedActionTicks( ) * 1000 < usecBlock )
			usecBlock = (*i)->GetNextTimedActionTicks( ) * 1000;
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = usecBlock;

	struct timeval send_tv;
	send_tv.tv_sec = 0;
	send_tv.tv_usec = 0;

#ifdef WIN32
	select( 1, &fd, NULL, NULL, &tv );
	select( 1, NULL, &send_fd, NULL, &send_tv );
#else
	select( nfds + 1, &fd, NULL, NULL, &tv );
	select( nfds + 1, NULL, &send_fd, NULL, &send_tv );
#endif

	if( NumFDs == 0 )
	{
		// we don't have any sockets (i.e. we aren't connected to battle.net maybe due to a lost connection and there aren't any games running)
		// select will return immediately and we'll chew up the CPU if we let it loop so just sleep for 200ms to kill some time

		MILLISLEEP( 200 );
	}
	
	bool BNETExit = false, IRCExit = false;	
	
	// update running games

	for( vector<CBaseGame *> :: iterator i = m_Games.begin( ); i != m_Games.end( ); )
	{
		if( (*i)->Update( &fd, &send_fd ) )
		{
			CONSOLE_Print3( "[AURA] deleting game [" + (*i)->GetGameName( ) + "]" );
			EventGameDeleted( *i );
			delete *i;
			i = m_Games.erase( i );
		}
		else
		{
			(*i)->UpdatePost( &send_fd );
			++i;
		}
	}

	// update current game

	if( m_CurrentGame )
	{
		if( m_CurrentGame->Update( &fd, &send_fd ) )
		{
			CONSOLE_Print3( "[AURA] deleting current game [" + m_CurrentGame->GetGameName( ) + "]" );
			delete m_CurrentGame;
			m_CurrentGame = NULL;

			for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
			{
				(*i)->QueueGameUncreate( );
				(*i)->QueueEnterChat( );
			}
		}
		else if( m_CurrentGame )
			m_CurrentGame->UpdatePost( &send_fd );
	}	

	// update battle.net connections

	for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
	{
		if( (*i)->Update( &fd, &send_fd ) )
			BNETExit = true;
	}

	// update IRC and DCC

	if( m_IRC->Update( &fd, &send_fd ) )
		IRCExit = true;

	for( vector<CDCC *> :: iterator i = m_IRC->m_DCC.begin( ); i != m_IRC->m_DCC.end( ); ++i )
		(*i)->Update( &fd, &send_fd );

	// update GProxy++ reliable reconnect sockets

	if( m_Reconnect && m_ReconnectSocket )
	{
		CTCPSocket *NewSocket = m_ReconnectSocket->Accept( &fd );

		if( NewSocket )
			m_ReconnectSockets.push_back( NewSocket );
	}

	for( vector<CTCPSocket *> :: iterator i = m_ReconnectSockets.begin( ); i != m_ReconnectSockets.end( ); )
	{
		if( (*i)->HasError( ) || !(*i)->GetConnected( ) || GetTime( ) - (*i)->GetLastRecv( ) >= 12 )
		{
			delete *i;
			i = m_ReconnectSockets.erase( i );
			continue;
		}

		(*i)->DoRecv( &fd );
		string *RecvBuffer = (*i)->GetBytes( );
		BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *)RecvBuffer->c_str( ), RecvBuffer->size( ) );

		// a packet is at least 4 bytes

		if( Bytes.size( ) >= 4 )
		{
			if( Bytes[0] == GPS_HEADER_CONSTANT )
			{
				// bytes 2 and 3 contain the length of the packet

				uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

				if( Length >= 4 )
				{
					if( Bytes.size( ) >= Length )
					{
						if( Bytes[1] == CGPSProtocol :: GPS_RECONNECT && Length == 13 )
						{
							unsigned char PID = Bytes[4];
							uint32_t ReconnectKey = UTIL_ByteArrayToUInt32( Bytes, false, 5 );
							uint32_t LastPacket = UTIL_ByteArrayToUInt32( Bytes, false, 9 );

							// look for a matching player in a running game

							CGamePlayer *Match = NULL;

							for( vector<CBaseGame *> :: iterator j = m_Games.begin( ); j != m_Games.end( ); ++j )
							{
								if( (*j)->GetGameLoaded( ) )
								{
									CGamePlayer *Player = (*j)->GetPlayerFromPID( PID );

									if( Player && Player->GetGProxy( ) && Player->GetGProxyReconnectKey( ) == ReconnectKey )
									{
										Match = Player;
										break;
									}
								}
							}

							if( Match )
							{
								// reconnect successful!

								*RecvBuffer = RecvBuffer->substr( Length );
								Match->EventGProxyReconnect( *i, LastPacket );
								i = m_ReconnectSockets.erase( i );
								continue;
							}
							else
							{
								(*i)->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_NOTFOUND ) );
								(*i)->DoSend( &send_fd );
								delete *i;
								i = m_ReconnectSockets.erase( i );
								continue;
							}
						}
						else
						{
							(*i)->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
							(*i)->DoSend( &send_fd );
							delete *i;
							i = m_ReconnectSockets.erase( i );
							continue;
						}
					}
				}
				else
				{
					(*i)->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
					(*i)->DoSend( &send_fd );
					delete *i;
					i = m_ReconnectSockets.erase( i );
					continue;
				}
			}
			else
			{
				(*i)->PutBytes( m_GPSProtocol->SEND_GPSS_REJECT( REJECTGPS_INVALID ) );
				(*i)->DoSend( &send_fd );
				delete *i;
				i = m_ReconnectSockets.erase( i );
				continue;
			}
		}

		(*i)->DoSend( &send_fd );
		++i;
	}		

	return m_Exiting || BNETExit || IRCExit;
}

void CGHost :: EventBNETGameRefreshFailed( CBNET *bnet )
{
	if( m_CurrentGame )
	{
		m_CurrentGame->SendAllChat( m_Language->UnableToCreateGameTryAnotherName( bnet->GetServer( ), m_CurrentGame->GetGameName( ) ) );
		
		CONSOLE_Print3( "[GAME: " + m_CurrentGame->GetGameName( ) + "] Unable to create game on server [" + bnet->GetServer( ) + "]. Try another name." );

		// we take the easy route and simply close the lobby if a refresh fails
		// it's possible at least one refresh succeeded and therefore the game is still joinable on at least one battle.net (plus on the local network) but we don't keep track of that
		// we only close the game if it has no players since we support game rehosting (via !priv and !pub in the lobby)

		if( m_CurrentGame->GetNumHumanPlayers( ) == 0 )
			m_CurrentGame->SetExiting( true );

		m_CurrentGame->SetRefreshError( true );
	}
}

void CGHost :: EventGameDeleted( CBaseGame *game )
{
	for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
	{
		(*i)->QueueChatCommand( m_Language->GameIsOver( game->GetDescription( ) ) );

		if( (*i)->GetServer( ) == game->GetCreatorServer( ) )
			(*i)->QueueChatCommand( m_Language->GameIsOver( game->GetDescription( ) ), game->GetCreatorName( ), true, false );
	}
}

void CGHost :: ReloadConfigs( )
{
	CConfig CFG;
	CFG.Read( "default.cfg" );
	SetConfigs( &CFG );
}

void CGHost :: SetConfigs( CConfig *CFG )
{
	// this doesn't set EVERY config value since that would potentially require reconfiguring the battle.net connections
	// it just set the easily reloadable values

	m_LanguageFile = CFG->GetString( "bot_language", "language.cfg" );
	delete m_Language;
	m_Language = new CLanguage( m_LanguageFile );
	m_Warcraft3Path = UTIL_AddPathSeperator( CFG->GetString( "bot_war3path", "C:\\Program Files\\Warcraft III\\" ) );
	m_BindAddress = CFG->GetString( "bot_bindaddress", string( ) );
	m_ReconnectWaitTime = CFG->GetInt( "bot_reconnectwaittime", 3 );
	m_MaxGames = CFG->GetInt( "bot_maxgames", 10 );
	string BotCommandTrigger = CFG->GetString( "bot_commandtrigger", "!" );
	m_CommandTrigger = BotCommandTrigger[0];
	
	m_MapCFGPath = UTIL_AddPathSeperator( CFG->GetString( "bot_mapcfgpath", string( ) ) );
	m_MapPath = UTIL_AddPathSeperator( CFG->GetString( "bot_mappath", string( ) ) );
	m_VirtualHostName = CFG->GetString( "bot_virtualhostname", "|cFF4080C0Aura" );

	if( m_VirtualHostName.size( ) > 15 )
	{
		m_VirtualHostName = "|cFF4080C0Aura";
		CONSOLE_Print( "[AURA] warning - bot_virtualhostname is longer than 15 characters, using default virtual host name" );
	}

	m_AutoLock = CFG->GetInt( "bot_autolock", 0 ) == 0 ? false : true;
	m_AllowDownloads = CFG->GetInt( "bot_allowdownloads", 0 );
	m_MaxDownloaders = CFG->GetInt( "bot_maxdownloaders", 3 );
	m_MaxDownloadSpeed = CFG->GetInt( "bot_maxdownloadspeed", 100 );
	m_LCPings = CFG->GetInt( "bot_lcpings", 1 ) == 0 ? false : true;
	m_AutoKickPing = CFG->GetInt( "bot_autokickping", 300 );
	m_LobbyTimeLimit = CFG->GetInt( "bot_lobbytimelimit", 2 );
	
	if( m_LobbyTimeLimit < 0 )
		m_LobbyTimeLimit = 2;
	
	m_Latency = CFG->GetInt( "bot_latency", 100 );
	m_SyncLimit = CFG->GetInt( "bot_synclimit", 50 );
	m_VoteKickPercentage = CFG->GetInt( "bot_votekickpercentage", 70 );

	if( m_VoteKickPercentage > 100 )
	{
		m_VoteKickPercentage = 100;
	}
}

void CGHost :: ExtractScripts( )
{
	string PatchMPQFileName = m_Warcraft3Path + "War3Patch.mpq";
	HANDLE PatchMPQ;

	if( SFileOpenArchive( PatchMPQFileName.c_str( ), 0, MPQ_OPEN_FORCE_MPQ_V1, &PatchMPQ ) )
	{
		CONSOLE_Print( "[AURA] loading MPQ file [" + PatchMPQFileName + "]" );
		HANDLE SubFile;

		// common.j

		if( SFileOpenFileEx( PatchMPQ, "Scripts\\common.j", 0, &SubFile ) )
		{
			uint32_t FileLength = SFileGetFileSize( SubFile, NULL );

			if( FileLength > 0 && FileLength != 0xFFFFFFFF )
			{
				char *SubFileData = new char[FileLength];
				DWORD BytesRead = 0;

				if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead ) )
				{
					CONSOLE_Print( "[AURA] extracting Scripts\\common.j from MPQ file to [" + m_MapCFGPath + "common.j]" );
					UTIL_FileWrite( m_MapCFGPath + "common.j", (unsigned char *)SubFileData, BytesRead );
				}
				else
					CONSOLE_Print( "[AURA] warning - unable to extract Scripts\\common.j from MPQ file" );

				delete [] SubFileData;
			}

			SFileCloseFile( SubFile );
		}
		else
			CONSOLE_Print( "[AURA] couldn't find Scripts\\common.j in MPQ file" );

		// blizzard.j

		if( SFileOpenFileEx( PatchMPQ, "Scripts\\blizzard.j", 0, &SubFile ) )
		{
			uint32_t FileLength = SFileGetFileSize( SubFile, NULL );

			if( FileLength > 0 && FileLength != 0xFFFFFFFF )
			{
				char *SubFileData = new char[FileLength];
				DWORD BytesRead = 0;

				if( SFileReadFile( SubFile, SubFileData, FileLength, &BytesRead ) )
				{
					CONSOLE_Print( "[AURA] extracting Scripts\\blizzard.j from MPQ file to [" + m_MapCFGPath + "blizzard.j]" );
					UTIL_FileWrite( m_MapCFGPath + "blizzard.j", (unsigned char *)SubFileData, BytesRead );
				}
				else
					CONSOLE_Print( "[AURA] warning - unable to extract Scripts\\blizzard.j from MPQ file" );

				delete [] SubFileData;
			}

			SFileCloseFile( SubFile );
		}
		else
			CONSOLE_Print( "[AURA] couldn't find Scripts\\blizzard.j in MPQ file" );

		SFileCloseArchive( PatchMPQ );
	}
	else
		CONSOLE_Print( "[AURA] warning - unable to load MPQ file [" + PatchMPQFileName + "] - error code " + UTIL_ToString( GetLastError( ) ) );
}

inline void CGHost :: LoadIPToCountryData( )
{
	ifstream in;
	in.open( "ip-to-country.csv" );

	if( in.fail( ) )
		CONSOLE_Print( "[AURA] warning - unable to read file [ip-to-country.csv], iptocountry data not loaded" );
	else
	{
		CONSOLE_Print( "[AURA] started loading [ip-to-country.csv]" );

		// the begin and commit statements are optimizations
		// we're about to insert ~4 MB of data into the database so if we allow the database to treat each insert as a transaction it will take a LONG time
		// todotodo: handle begin/commit failures a bit more gracefully

		if( !m_DB->Begin( ) )
			CONSOLE_Print( "[AURA] warning - failed to begin database transaction, iptocountry data not loaded" );
		else
		{
			unsigned char Percent = 0;
			string Line, IP1, IP2, Country;
			CSVParser parser;

			// get length of file for the progress meter

			in.seekg( 0, ios :: end );
			uint32_t FileLength = in.tellg( );
			in.seekg( 0, ios :: beg );

			while( !in.eof( ) )
			{
				getline( in, Line );

				if( Line.empty( ) )
					continue;

				parser << Line;
				parser >> IP1;
				parser >> IP2;
				parser >> Country;
				m_DB->FromAdd( UTIL_ToUInt32( IP1 ), UTIL_ToUInt32( IP2 ), Country );

				// it's probably going to take awhile to load the iptocountry data (~10 seconds on my 3.2 GHz P4 when using SQLite3)
				// so let's print a progress meter just to keep the user from getting worried

				unsigned char NewPercent = (unsigned char)( (float)in.tellg( ) / FileLength * 100 );

				if( NewPercent != Percent && NewPercent % 10 == 0 )
				{
					Percent = NewPercent;
					CONSOLE_Print( "[AURA] iptocountry data: " + UTIL_ToString( Percent ) + "% loaded" );
				}
			}

			if( !m_DB->Commit( ) )
				CONSOLE_Print( "[AURA] warning - failed to commit database transaction, iptocountry data not loaded" );
			else
				CONSOLE_Print( "[AURA] finished loading [ip-to-country.csv]" );
		}

		in.close( );
	}
}

void CGHost :: CreateGame( CMap *map, unsigned char gameState, string gameName, string ownerName, string creatorName, string creatorServer, bool whisper )
{
	if( !m_Enabled )
	{
		for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameDisabled( gameName ), creatorName, whisper, false );
		}

		return;
	}

	if( gameName.size( ) > 31 )
	{
		for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameNameTooLong( gameName ), creatorName, whisper, false );
		}

		return;
	}

	if( !map->GetValid( ) )
	{
		for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameInvalidMap( gameName ), creatorName, whisper, false );
		}

		return;
	}

	if( m_CurrentGame )
	{
		for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameAnotherGameInLobby( gameName, m_CurrentGame->GetDescription( ) ), creatorName, whisper, false );
		}

		return;
	}

	if( m_Games.size( ) >= m_MaxGames )
	{
		for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetServer( ) == creatorServer )
				(*i)->QueueChatCommand( m_Language->UnableToCreateGameMaxGamesReached( gameName, UTIL_ToString( m_MaxGames ) ), creatorName, whisper, false );
		}

		return;
	}

	CONSOLE_Print3( "[AURA] creating game [" + gameName + "]" );

	m_CurrentGame = new CGame( this, map, m_HostPort, gameState, gameName, ownerName, creatorName, creatorServer );

	// todotodo: check if listening failed and report the error to the user

	for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
	{
		if( whisper && (*i)->GetServer( ) == creatorServer )
		{
			// note that we send this whisper only on the creator server

			if( gameState == GAME_PRIVATE )
				(*i)->QueueChatCommand( m_Language->CreatingPrivateGame( gameName, ownerName ), creatorName, whisper, false );
			else if( gameState == GAME_PUBLIC )
				(*i)->QueueChatCommand( m_Language->CreatingPublicGame( gameName, ownerName ), creatorName, whisper, false );
		}
		else
		{
			// note that we send this chat message on all other bnet servers

			if( gameState == GAME_PRIVATE )
				(*i)->QueueChatCommand( m_Language->CreatingPrivateGame( gameName, ownerName ) );
			else if( gameState == GAME_PUBLIC )
				(*i)->QueueChatCommand( m_Language->CreatingPublicGame( gameName, ownerName ) );
		}

		(*i)->QueueGameCreate( gameState, gameName, string( ), map, m_CurrentGame->GetHostCounter( ) );
	}

	// if we're creating a private game we don't need to send any game refresh messages so we can rejoin the chat immediately
	// unfortunately this doesn't work on PVPGN servers because they consider an enterchat message to be a gameuncreate message when in a game
	// so don't rejoin the chat if we're using PVPGN

	if( gameState == GAME_PRIVATE )
	{
		for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
		{
			if( (*i)->GetPasswordHashType( ) != "pvpgn" )
				(*i)->QueueEnterChat( );
		}
	}

	// hold friends and/or clan members

	for( vector<CBNET *> :: iterator i = m_BNETs.begin( ); i != m_BNETs.end( ); ++i )
	{
		(*i)->HoldFriends( m_CurrentGame );
		(*i)->HoldClan( m_CurrentGame );
	}
}

void CGHost :: Restart ( bool force )
{
	// shutdown ghost

	CONSOLE_Print3( "[AURA] restarting" );
	delete gGHost;
	gGHost = NULL;	

#ifdef WIN32
	// shutdown winsock

	CONSOLE_Print( "[AURA] shutting down winsock" );
	WSACleanup( );

	_spawnl( _P_OVERLAY, "ghost.exe", "ghost.exe", NULL );
#else		
	execl( "ghost", "ghost", NULL );
#endif
}