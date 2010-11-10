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
#include "language.h"
#include "socket.h"
#include "commandpacket.h"
#include "ghostdb.h"
#include "bncsutilinterface.h"
#include "bnetprotocol.h"
#include "bnet.h"
#include "map.h"
#include "gameprotocol.h"
#include "game_base.h"
#include "irc.h"

#include <boost/filesystem.hpp>

using namespace boost :: filesystem;

//
// CBNET
//

CBNET :: CBNET( CGHost *nGHost, string nServer, string nServerAlias, string nCDKeyROC, string nCDKeyTFT, string nCountryAbbrev, string nCountry, uint32_t nLocaleID, string nUserName, string nUserPassword, string nFirstChannel, string nRootAdmin, char nCommandTrigger, bool nPublicCommands, unsigned char nWar3Version, BYTEARRAY nEXEVersion, BYTEARRAY nEXEVersionHash, string nPasswordHashType, uint32_t nMaxMessageLength, uint32_t nHostCounterID ) : m_GHost( nGHost ), m_Exiting( false ), m_Spam( false ), m_Server( nServer ), m_CDKeyROC( nCDKeyROC ), m_CDKeyTFT( nCDKeyTFT ), m_CountryAbbrev( nCountryAbbrev ), m_Country( nCountry ), m_LocaleID( nLocaleID ), m_UserName( nUserName ), m_UserPassword( nUserPassword ), m_FirstChannel( nFirstChannel ), m_RootAdmin( nRootAdmin ), m_CommandTrigger( nCommandTrigger ), m_War3Version( nWar3Version ), m_EXEVersion( nEXEVersion ), m_EXEVersionHash( nEXEVersionHash ), m_PasswordHashType( nPasswordHashType ), m_HostCounterID( nHostCounterID ), m_LastDisconnectedTime( 0 ), m_LastConnectionAttemptTime( 0 ), m_LastNullTime( 0 ), m_LastOutPacketTicks( 0 ), m_LastOutPacketSize( 0 ), m_LastAdminRefreshTime( GetTime( ) ), m_LastBanRefreshTime( GetTime( ) ), m_LastSpamTicks( 0 ), m_FirstConnect( true ), m_WaitingToConnect( true ), m_LoggedIn( false ), m_InChat( false ), m_PublicCommands( nPublicCommands ), m_SpamChannel( "allstars" ), m_Deactivated( false ), m_IRC( false )
{
	m_Socket = new CTCPClient( );
	m_Protocol = new CBNETProtocol( );
	m_BNCSUtil = new CBNCSUtilInterface( nUserName, nUserPassword );
	m_CallableAdminList = m_GHost->m_DB->ThreadedAdminList( nServer );
	m_CallableBanList = m_GHost->m_DB->ThreadedBanList( nServer );
	string LowerServer = m_Server;

	if( !nServerAlias.empty( ) )
		m_ServerAlias = nServerAlias;
	else
		m_ServerAlias = m_Server;

	m_CDKeyROC = nCDKeyROC;
	m_CDKeyTFT = nCDKeyTFT;

	// remove dashes and spaces from CD keys and convert to uppercase

	m_CDKeyROC.erase( remove( m_CDKeyROC.begin( ), m_CDKeyROC.end( ), '-' ), m_CDKeyROC.end( ) );
	m_CDKeyTFT.erase( remove( m_CDKeyTFT.begin( ), m_CDKeyTFT.end( ), '-' ), m_CDKeyTFT.end( ) );
	m_CDKeyROC.erase( remove( m_CDKeyROC.begin( ), m_CDKeyROC.end( ), ' ' ), m_CDKeyROC.end( ) );
	m_CDKeyTFT.erase( remove( m_CDKeyTFT.begin( ), m_CDKeyTFT.end( ), ' ' ), m_CDKeyTFT.end( ) );
	transform( m_CDKeyROC.begin( ), m_CDKeyROC.end( ), m_CDKeyROC.begin( ), (int(*)(int))toupper );
	transform( m_CDKeyTFT.begin( ), m_CDKeyTFT.end( ), m_CDKeyTFT.begin( ), (int(*)(int))toupper );

	if( m_CDKeyROC.size( ) != 26 )
		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] warning - your ROC CD key is not 26 characters long and is probably invalid" );

	if( m_CDKeyTFT.size( ) != 26 )
		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] warning - your TFT CD key is not 26 characters long and is probably invalid" );

	transform( m_RootAdmin.begin( ), m_RootAdmin.end( ), m_RootAdmin.begin( ), (int(*)(int))tolower );	
}

CBNET :: ~CBNET( )
{
	delete m_Socket;
	delete m_Protocol;

	while( !m_Packets.empty( ) )
	{
		delete m_Packets.front( );
		m_Packets.pop( );
	}

	delete m_BNCSUtil;

	for( vector<CIncomingFriendList *> :: iterator i = m_Friends.begin( ); i != m_Friends.end( ); ++i )
		delete *i;

	for( vector<CIncomingClanList *> :: iterator i = m_Clans.begin( ); i != m_Clans.end( ); ++i )
		delete *i;

	for( vector<PairedAdminCount> :: iterator i = m_PairedAdminCounts.begin( ); i != m_PairedAdminCounts.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( vector<PairedAdminAdd> :: iterator i = m_PairedAdminAdds.begin( ); i != m_PairedAdminAdds.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( vector<PairedAdminRemove> :: iterator i = m_PairedAdminRemoves.begin( ); i != m_PairedAdminRemoves.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( vector<PairedBanCount> :: iterator i = m_PairedBanCounts.begin( ); i != m_PairedBanCounts.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( vector<PairedBanAdd> :: iterator i = m_PairedBanAdds.begin( ); i != m_PairedBanAdds.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( vector<PairedBanRemove> :: iterator i = m_PairedBanRemoves.begin( ); i != m_PairedBanRemoves.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( vector<PairedGPSCheck> :: iterator i = m_PairedGPSChecks.begin( ); i != m_PairedGPSChecks.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	for( vector<PairedDPSCheck> :: iterator i = m_PairedDPSChecks.begin( ); i != m_PairedDPSChecks.end( ); ++i )
		m_GHost->m_Callables.push_back( i->second );

	if( m_CallableAdminList )
		m_GHost->m_Callables.push_back( m_CallableAdminList );

	if( m_CallableBanList )
		m_GHost->m_Callables.push_back( m_CallableBanList );

	for( vector<CDBBan *> :: iterator i = m_Bans.begin( ); i != m_Bans.end( ); ++i )
		delete *i;
}

BYTEARRAY CBNET :: GetUniqueName( )
{
	return m_Protocol->GetUniqueName( );
}

unsigned int CBNET :: SetFD( void *fd, void *send_fd, int *nfds )
{
	unsigned int NumFDs = 0;

	if( !m_Socket->HasError( ) && m_Socket->GetConnected( ) )
	{
		m_Socket->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
		++NumFDs;
	}

	return NumFDs;
}

bool CBNET :: Update( void *fd, void *send_fd )
{
	if( m_Deactivated )
		return m_Exiting;

	//
	// update callables
	//

	for( vector<PairedAdminCount> :: iterator i = m_PairedAdminCounts.begin( ); i != m_PairedAdminCounts.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			uint32_t Count = i->second->GetResult( );

			if( Count == 0 )
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ThereAreNoAdmins( m_Server ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ThereAreNoAdmins( m_Server ) );
			}
			else if( Count == 1 )
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ThereIsAdmin( m_Server ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ThereIsAdmin( m_Server ) );
			}
			else
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ThereAreAdmins( m_Server, UTIL_ToString( Count ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ThereAreAdmins( m_Server, UTIL_ToString( Count ) ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedAdminCounts.erase( i );
		}
		else
			++i;
	}

	for( vector<PairedAdminAdd> :: iterator i = m_PairedAdminAdds.begin( ); i != m_PairedAdminAdds.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			if( i->second->GetResult( ) )
			{
				AddAdmin( i->second->GetUser( ) );
				
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->AddedUserToAdminDatabase( m_Server, i->second->GetUser( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->AddedUserToAdminDatabase( m_Server, i->second->GetUser( ) ) );
			}
			else
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ErrorAddingUserToAdminDatabase( m_Server, i->second->GetUser( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ErrorAddingUserToAdminDatabase( m_Server, i->second->GetUser( ) ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedAdminAdds.erase( i );
		}
		else
			++i;
	}

	for( vector<PairedAdminRemove> :: iterator i = m_PairedAdminRemoves.begin( ); i != m_PairedAdminRemoves.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			if( i->second->GetResult( ) )
			{
				RemoveAdmin( i->second->GetUser( ) );
				
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->DeletedUserFromAdminDatabase( m_Server, i->second->GetUser( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->DeletedUserFromAdminDatabase( m_Server, i->second->GetUser( ) ) );
			}
			else
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ErrorDeletingUserFromAdminDatabase( m_Server, i->second->GetUser( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ErrorDeletingUserFromAdminDatabase( m_Server, i->second->GetUser( ) ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedAdminRemoves.erase( i );
		}
		else
			++i;
	}

	for( vector<PairedBanCount> :: iterator i = m_PairedBanCounts.begin( ); i != m_PairedBanCounts.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			uint32_t Count = i->second->GetResult( );

			if( Count == 0 )
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ThereAreNoBannedUsers( m_Server ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ThereAreNoBannedUsers( m_Server ) );
			}
			else if( Count == 1 )
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ThereIsBannedUser( m_Server ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ThereIsBannedUser( m_Server ) );
			}
			else
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ThereAreBannedUsers( m_Server, UTIL_ToString( Count ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ThereAreBannedUsers( m_Server, UTIL_ToString( Count ) ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedBanCounts.erase( i );
		}
		else
			++i;
	}

	for( vector<PairedBanAdd> :: iterator i = m_PairedBanAdds.begin( ); i != m_PairedBanAdds.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			if( i->second->GetResult( ) )
			{
				AddBan( i->second->GetUser( ), i->second->GetIP( ), i->second->GetGameName( ), i->second->GetAdmin( ), i->second->GetReason( ) );
				
				if( !m_IRC )				
					QueueChatCommand( m_GHost->m_Language->BannedUser( i->second->GetServer( ), i->second->GetUser( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->BannedUser( i->second->GetServer( ), i->second->GetUser( ) ) );
			}
			else
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ErrorBanningUser( i->second->GetServer( ), i->second->GetUser( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ErrorBanningUser( i->second->GetServer( ), i->second->GetUser( ) ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedBanAdds.erase( i );
		}
		else
			++i;
	}

	for( vector<PairedBanRemove> :: iterator i = m_PairedBanRemoves.begin( ); i != m_PairedBanRemoves.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			if( i->second->GetResult( ) )
			{
				RemoveBan( i->second->GetUser( ) );
				
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->UnbannedUser( i->second->GetUser( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->UnbannedUser( i->second->GetUser( ) ) );
			}
			else
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->ErrorUnbanningUser( i->second->GetUser( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->ErrorUnbanningUser( i->second->GetUser( ) ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedBanRemoves.erase( i );
		}
		else
			++i;
	}

	for( vector<PairedGPSCheck> :: iterator i = m_PairedGPSChecks.begin( ); i != m_PairedGPSChecks.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			CDBGamePlayerSummary *GamePlayerSummary = i->second->GetResult( );			

			if( GamePlayerSummary )
			{
				string Response = m_GHost->m_Language->HasPlayedGamesWithThisBot( i->second->GetName( ), GamePlayerSummary->GetFirstGameDateTime( ), GamePlayerSummary->GetLastGameDateTime( ), UTIL_ToString( GamePlayerSummary->GetTotalGames( ) ), UTIL_ToString( (float)GamePlayerSummary->GetAvgLoadingTime( ) / 1000, 2 ), UTIL_ToString( GamePlayerSummary->GetAvgLeftPercent( ) ) );
				
				if( !m_IRC )
					QueueChatCommand( Response, i->first, !i->first.empty( ), false );
				else
					IRC_Print( Response );
			}
			else
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->HasntPlayedGamesWithThisBot( i->second->GetName( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->HasntPlayedGamesWithThisBot( i->second->GetName( ) ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedGPSChecks.erase( i );
		}
		else
			++i;
	}

	for( vector<PairedDPSCheck> :: iterator i = m_PairedDPSChecks.begin( ); i != m_PairedDPSChecks.end( ); )
	{
		if( i->second->GetReady( ) )
		{
			CDBDotAPlayerSummary *DotAPlayerSummary = i->second->GetResult( );

			if( DotAPlayerSummary )
			{
				string Summary = m_GHost->m_Language->HasPlayedDotAGamesWithThisBot(	i->second->GetName( ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalGames( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalWins( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalLosses( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalKills( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalDeaths( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalCreepKills( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalCreepDenies( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalAssists( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalNeutralKills( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalTowerKills( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalRaxKills( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetTotalCourierKills( ) ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgKills( ), 2 ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgDeaths( ), 2 ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgCreepKills( ), 2 ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgCreepDenies( ), 2 ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgAssists( ), 2 ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgNeutralKills( ), 2 ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgTowerKills( ), 2 ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgRaxKills( ), 2 ),
                                                                                                        UTIL_ToString( DotAPlayerSummary->GetAvgCourierKills( ), 2 ) );

				if( !m_IRC )
					QueueChatCommand( Summary, i->first, !i->first.empty( ), false );
				else
					IRC_Print( Summary );
			}
			else
			{
				if( !m_IRC )
					QueueChatCommand( m_GHost->m_Language->HasntPlayedDotAGamesWithThisBot( i->second->GetName( ) ), i->first, !i->first.empty( ), false );
				else
					IRC_Print( m_GHost->m_Language->HasntPlayedDotAGamesWithThisBot( i->second->GetName( ) ) );
			}

			m_GHost->m_DB->RecoverCallable( i->second );
			delete i->second;
			i = m_PairedDPSChecks.erase( i );
		}
		else
			++i;
	}

	// refresh the admin list every 5 minutes

	if( !m_CallableAdminList && GetTime( ) - m_LastAdminRefreshTime >= 300 )
		m_CallableAdminList = m_GHost->m_DB->ThreadedAdminList( m_Server );

	if( m_CallableAdminList && m_CallableAdminList->GetReady( ) )
	{
		// CONSOLE_Print( "[BNET: " + m_ServerAlias + "] refreshed admin list (" + UTIL_ToString( m_Admins.size( ) ) + " -> " + UTIL_ToString( m_CallableAdminList->GetResult( ).size( ) ) + " admins)" );
		m_Admins = m_CallableAdminList->GetResult( );
		m_GHost->m_DB->RecoverCallable( m_CallableAdminList );
		delete m_CallableAdminList;
		m_CallableAdminList = NULL;
		m_LastAdminRefreshTime = GetTime( );
	}

	// refresh the ban list every 60 minutes

	if( !m_CallableBanList && GetTime( ) - m_LastBanRefreshTime >= 3600 )
		m_CallableBanList = m_GHost->m_DB->ThreadedBanList( m_Server );

	if( m_CallableBanList && m_CallableBanList->GetReady( ) )
	{
		// CONSOLE_Print( "[BNET: " + m_ServerAlias + "] refreshed ban list (" + UTIL_ToString( m_Bans.size( ) ) + " -> " + UTIL_ToString( m_CallableBanList->GetResult( ).size( ) ) + " bans)" );

		for( vector<CDBBan *> :: iterator i = m_Bans.begin( ); i != m_Bans.end( ); ++i )
			delete *i;

		m_Bans = m_CallableBanList->GetResult( );
		m_GHost->m_DB->RecoverCallable( m_CallableBanList );
		delete m_CallableBanList;
		m_CallableBanList = NULL;
		m_LastBanRefreshTime = GetTime( );
	}

	// we return at the end of each if statement so we don't have to deal with errors related to the order of the if statements
	// that means it might take a few ms longer to complete a task involving multiple steps (in this case, reconnecting) due to blocking or sleeping
	// but it's not a big deal at all, maybe 100ms in the worst possible case (based on a 50ms blocking time)

	if( m_Socket->HasError( ) )
	{
		// the socket has an error

		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] disconnected from battle.net due to socket error" );

		if( m_Socket->GetError( ) == ECONNRESET && GetTime( ) - m_LastConnectionAttemptTime <= 15 )
			CONSOLE_Print( "[BNET: " + m_ServerAlias + "] warning - you are probably temporarily IP banned from battle.net" );

		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] waiting 90 seconds to reconnect" );
		m_BNCSUtil->Reset( m_UserName, m_UserPassword );
		m_Socket->Reset( );
		m_LastDisconnectedTime = GetTime( );
		m_LoggedIn = false;
		m_InChat = false;
		m_WaitingToConnect = true;
		return m_Exiting;
	}

	if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && !m_WaitingToConnect )
	{
		// the socket was disconnected

		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] disconnected from battle.net" );
		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] waiting 90 seconds to reconnect" );
		m_BNCSUtil->Reset( m_UserName, m_UserPassword );
		m_Socket->Reset( );
		m_LastDisconnectedTime = GetTime( );
		m_LoggedIn = false;
		m_InChat = false;
		m_WaitingToConnect = true;
		return m_Exiting;
	}

	if( m_Socket->GetConnected( ) )
	{
		// the socket is connected and everything appears to be working properly

		m_Socket->DoRecv( (fd_set *)fd );
		ExtractPackets( );
		ProcessPackets( );

		// check if at least one packet is waiting to be sent and if we've waited long enough to prevent flooding
		// this formula has changed many times but currently we wait 1 second if the last packet was "small", 3.5 seconds if it was "medium", and 4 seconds if it was "big"

		uint32_t WaitTicks = 0;

		if( m_LastOutPacketSize < 10 )
			WaitTicks = 1150;
		else if( m_LastOutPacketSize < 100 )
			WaitTicks = 3150;
		else
			WaitTicks = 4150;

		if( !m_OutPackets.empty( ) && GetTicks( ) - m_LastOutPacketTicks >= WaitTicks )
		{
			if( m_OutPackets.size( ) > 7 )
				CONSOLE_Print( "[BNET: " + m_ServerAlias + "] packet queue warning - there are " + UTIL_ToString( m_OutPackets.size( ) ) + " packets waiting to be sent" );

			m_Socket->PutBytes( m_OutPackets.front( ) );
			m_LastOutPacketSize = m_OutPackets.front( ).size( );
			m_OutPackets.pop( );
			m_LastOutPacketTicks = GetTicks( );
		}

		// send a null packet every 60 seconds to detect disconnects

		if( GetTime( ) - m_LastNullTime >= 60 && GetTicks( ) - m_LastOutPacketTicks >= 60000 )
		{
			m_Socket->PutBytes( m_Protocol->SEND_SID_NULL( ) );
			m_LastNullTime = GetTime( );
		}
                
		// spam game every 4.1 seconds
		
		if( m_Spam && ( GetTicks( ) - m_LastSpamTicks > 4100 ) )
		{
			if( m_InChat && m_GHost->m_CurrentGame && m_GHost->m_CurrentGame->GetGameName( ).size( ) == 4 )
			{
				m_OutPackets.push( m_Protocol->SEND_SID_CHATCOMMAND( m_GHost->m_CurrentGame->GetGameName( ) ) );
				m_LastSpamTicks = GetTicks( );
			}
			else
				m_Spam = false;
		}

		m_Socket->DoSend( (fd_set *)send_fd );
		return m_Exiting;
	}

	if( m_Socket->GetConnecting( ) )
	{
		// we are currently attempting to connect to battle.net

		if( m_Socket->CheckConnect( ) )
		{
			// the connection attempt completed

			CONSOLE_Print( "[BNET: " + m_ServerAlias + "] connected" );
			m_Socket->PutBytes( m_Protocol->SEND_PROTOCOL_INITIALIZE_SELECTOR( ) );
			m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_INFO( m_War3Version, m_LocaleID, m_CountryAbbrev, m_Country ) );
			m_Socket->DoSend( (fd_set *)send_fd );
			m_LastNullTime = GetTime( );
			m_LastOutPacketTicks = GetTicks( );

			while( !m_OutPackets.empty( ) )
				m_OutPackets.pop( );

			return m_Exiting;
		}
		else if( GetTime( ) - m_LastConnectionAttemptTime >= 15 )
		{
			// the connection attempt timed out (15 seconds)

			CONSOLE_Print( "[BNET: " + m_ServerAlias + "] connect timed out" );
			CONSOLE_Print( "[BNET: " + m_ServerAlias + "] waiting 90 seconds to reconnect" );
			m_Socket->Reset( );
			m_LastDisconnectedTime = GetTime( );
			m_WaitingToConnect = true;
			return m_Exiting;
		}
	}

	if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && ( m_FirstConnect || GetTime( ) - m_LastDisconnectedTime >= 90 ) )
	{
		// attempt to connect to battle.net

		m_FirstConnect = false;
		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] connecting to server [" + m_Server + "] on port 6112" );

		if( !m_GHost->m_BindAddress.empty( ) )
			CONSOLE_Print( "[BNET: " + m_ServerAlias + "] attempting to bind to address [" + m_GHost->m_BindAddress + "]" );

		if( m_ServerIP.empty( ) )
		{
			m_Socket->Connect( m_GHost->m_BindAddress, m_Server, 6112 );

			if( !m_Socket->HasError( ) )
			{
				m_ServerIP = m_Socket->GetIPString( );
				CONSOLE_Print( "[BNET: " + m_ServerAlias + "] resolved and cached server IP address " + m_ServerIP );
			}
		}
		else
		{
			// use cached server IP address since resolving takes time and is blocking

			CONSOLE_Print( "[BNET: " + m_ServerAlias + "] using cached server IP address " + m_ServerIP );
			m_Socket->Connect( m_GHost->m_BindAddress, m_ServerIP, 6112 );
		}

		m_WaitingToConnect = false;
		m_LastConnectionAttemptTime = GetTime( );
	}

	return m_Exiting;
}

inline void CBNET :: ExtractPackets( )
{
	// extract as many packets as possible from the socket's receive buffer and put them in the m_Packets queue

	string *RecvBuffer = m_Socket->GetBytes( );
	BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *)RecvBuffer->c_str( ), RecvBuffer->size( ) );

	// a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

	while( Bytes.size( ) >= 4 )
	{
		// byte 0 is always 255

		if( Bytes[0] == BNET_HEADER_CONSTANT )
		{
			// bytes 2 and 3 contain the length of the packet

			uint16_t Length = UTIL_ByteArrayToUInt16( Bytes, false, 2 );

			if( Length >= 4 )
			{
				if( Bytes.size( ) >= Length )
				{
					m_Packets.push( new CCommandPacket( BNET_HEADER_CONSTANT, Bytes[1], BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) );
					*RecvBuffer = RecvBuffer->substr( Length );
					Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
				}
				else
					return;
			}
			else
			{
				CONSOLE_Print( "[BNET: " + m_ServerAlias + "] error - received invalid packet from battle.net (bad length), disconnecting" );
				m_Socket->Disconnect( );
				return;
			}
		}
		else
		{
			CONSOLE_Print( "[BNET: " + m_ServerAlias + "] error - received invalid packet from battle.net (bad header constant), disconnecting" );
			m_Socket->Disconnect( );
			return;
		}
	}
}

inline void CBNET :: ProcessPackets( )
{
	CIncomingGameHost *GameHost = NULL;
	CIncomingChatEvent *ChatEvent = NULL;
	vector<CIncomingFriendList *> Friends;
	vector<CIncomingClanList *> Clans;

	// process all the received packets in the m_Packets queue
	// this normally means sending some kind of response

	while( !m_Packets.empty( ) )
	{
		CCommandPacket *Packet = m_Packets.front( );
		m_Packets.pop( );

		if( Packet->GetPacketType( ) == BNET_HEADER_CONSTANT )
		{
			switch( Packet->GetID( ) )
			{
			case CBNETProtocol :: SID_NULL:
				// warning: we do not respond to NULL packets with a NULL packet of our own
				// this is because PVPGN servers are programmed to respond to NULL packets so it will create a vicious cycle of useless traffic
				// official battle.net servers do not respond to NULL packets

				m_Protocol->RECEIVE_SID_NULL( Packet->GetData( ) );
				break;

			case CBNETProtocol :: SID_GETADVLISTEX:
				GameHost = m_Protocol->RECEIVE_SID_GETADVLISTEX( Packet->GetData( ) );

				if( GameHost )
					CONSOLE_Print( "[BNET: " + m_ServerAlias + "] joining game [" + GameHost->GetGameName( ) + "]" );

				delete GameHost;
				GameHost = NULL;
				break;

			case CBNETProtocol :: SID_ENTERCHAT:
				if( m_Protocol->RECEIVE_SID_ENTERCHAT( Packet->GetData( ) ) )
				{
					CONSOLE_Print( "[BNET: " + m_ServerAlias + "] joining channel [" + m_FirstChannel + "]" );
					m_InChat = true;
					m_Socket->PutBytes( m_Protocol->SEND_SID_JOINCHANNEL( m_FirstChannel ) );
				}

				break;
				
			case CBNETProtocol :: SID_CHATEVENT:
				ChatEvent = m_Protocol->RECEIVE_SID_CHATEVENT( Packet->GetData( ) );

				if( ChatEvent )
					ProcessChatEvent( ChatEvent );

				delete ChatEvent;
				ChatEvent = NULL;
				break;

			case CBNETProtocol :: SID_CHECKAD:
				m_Protocol->RECEIVE_SID_CHECKAD( Packet->GetData( ) );
				break;

			case CBNETProtocol :: SID_STARTADVEX3:
				if( m_Protocol->RECEIVE_SID_STARTADVEX3( Packet->GetData( ) ) )
				{
					m_InChat = false;
				}
				else
				{
					CONSOLE_Print( "[BNET: " + m_ServerAlias + "] startadvex3 failed" );
					m_GHost->EventBNETGameRefreshFailed( this );
				}

				break;
				
			case CBNETProtocol :: SID_PING:
				m_Socket->PutBytes( m_Protocol->SEND_SID_PING( m_Protocol->RECEIVE_SID_PING( Packet->GetData( ) ) ) );
				break;

			case CBNETProtocol :: SID_AUTH_INFO:
				if( m_Protocol->RECEIVE_SID_AUTH_INFO( Packet->GetData( ) ) )
				{
					if( m_BNCSUtil->HELP_SID_AUTH_CHECK( m_GHost->m_Warcraft3Path, m_CDKeyROC, m_CDKeyTFT, m_Protocol->GetValueStringFormulaString( ), m_Protocol->GetIX86VerFileNameString( ), m_Protocol->GetClientToken( ), m_Protocol->GetServerToken( ) ) )
					{
						// override the exe information generated by bncsutil if specified in the config file
						// apparently this is useful for pvpgn users

						if( m_EXEVersion.size( ) == 4 )
						{
							CONSOLE_Print( "[BNET: " + m_ServerAlias + "] using custom exe version bnet_custom_exeversion = " + UTIL_ToString( m_EXEVersion[0] ) + " " + UTIL_ToString( m_EXEVersion[1] ) + " " + UTIL_ToString( m_EXEVersion[2] ) + " " + UTIL_ToString( m_EXEVersion[3] ) );
							m_BNCSUtil->SetEXEVersion( m_EXEVersion );
						}

						if( m_EXEVersionHash.size( ) == 4 )
						{
							CONSOLE_Print( "[BNET: " + m_ServerAlias + "] using custom exe version hash bnet_custom_exeversionhash = " + UTIL_ToString( m_EXEVersionHash[0] ) + " " + UTIL_ToString( m_EXEVersionHash[1] ) + " " + UTIL_ToString( m_EXEVersionHash[2] ) + " " + UTIL_ToString( m_EXEVersionHash[3] ) );
							m_BNCSUtil->SetEXEVersionHash( m_EXEVersionHash );
						}

						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] attempting to auth as Warcraft III: The Frozen Throne" );					

						m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_CHECK( m_Protocol->GetClientToken( ), m_BNCSUtil->GetEXEVersion( ), m_BNCSUtil->GetEXEVersionHash( ), m_BNCSUtil->GetKeyInfoROC( ), m_BNCSUtil->GetKeyInfoTFT( ), m_BNCSUtil->GetEXEInfo( ), "GHost" ) );

					}
					else
					{
						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon failed - bncsutil key hash failed (check your Warcraft 3 path and cd keys), disconnecting" );
						m_Socket->Disconnect( );
						delete Packet;
						return;
					}
				}

				break;

			case CBNETProtocol :: SID_AUTH_CHECK:
				if( m_Protocol->RECEIVE_SID_AUTH_CHECK( Packet->GetData( ) ) )
				{
					// cd keys accepted

					CONSOLE_Print( "[BNET: " + m_ServerAlias + "] cd keys accepted" );
					m_BNCSUtil->HELP_SID_AUTH_ACCOUNTLOGON( );
					m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGON( m_BNCSUtil->GetClientKey( ), m_UserName ) );
				}
				else
				{
					// cd keys not accepted

					switch( UTIL_ByteArrayToUInt32( m_Protocol->GetKeyState( ), false ) )
					{
					case CBNETProtocol :: KR_ROC_KEY_IN_USE:
						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon failed - ROC CD key in use by user [" + m_Protocol->GetKeyStateDescription( ) + "], disconnecting" );
						break;
					case CBNETProtocol :: KR_TFT_KEY_IN_USE:
						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon failed - TFT CD key in use by user [" + m_Protocol->GetKeyStateDescription( ) + "], disconnecting" );
						break;
					case CBNETProtocol :: KR_OLD_GAME_VERSION:
						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon failed - game version is too old, disconnecting" );
						break;
					case CBNETProtocol :: KR_INVALID_VERSION:
						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon failed - game version is invalid, disconnecting" );
						break;
					default:
						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon failed - cd keys not accepted, disconnecting" );
						break;
					}

					m_Socket->Disconnect( );
					delete Packet;
					return;
				}

				break;
				
			case CBNETProtocol :: SID_AUTH_ACCOUNTLOGON:
				if( m_Protocol->RECEIVE_SID_AUTH_ACCOUNTLOGON( Packet->GetData( ) ) )
				{
					CONSOLE_Print( "[BNET: " + m_ServerAlias + "] username [" + m_UserName + "] accepted" );

					if( m_PasswordHashType == "pvpgn" )
					{
						// pvpgn logon

						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] using pvpgn logon type (for pvpgn servers only)" );
						m_BNCSUtil->HELP_PvPGNPasswordHash( m_UserPassword );
						m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGONPROOF( m_BNCSUtil->GetPvPGNPasswordHash( ) ) );
					}
					else
					{
						// battle.net logon

						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] using battle.net logon type (for official battle.net servers only)" );
						m_BNCSUtil->HELP_SID_AUTH_ACCOUNTLOGONPROOF( m_Protocol->GetSalt( ), m_Protocol->GetServerPublicKey( ) );
						m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGONPROOF( m_BNCSUtil->GetM1( ) ) );
					}
				}
				else
				{
					CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon failed - invalid username, disconnecting" );
					m_Socket->Disconnect( );
					delete Packet;
					return;
				}

				break;

			case CBNETProtocol :: SID_AUTH_ACCOUNTLOGONPROOF:
				if( m_Protocol->RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF( Packet->GetData( ) ) )
				{
					// logon successful

					CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon successful" );
					m_LoggedIn = true;
					m_Socket->PutBytes( m_Protocol->SEND_SID_NETGAMEPORT( m_GHost->m_HostPort ) );
					m_Socket->PutBytes( m_Protocol->SEND_SID_ENTERCHAT( ) );
					m_Socket->PutBytes( m_Protocol->SEND_SID_FRIENDSLIST( ) );
					m_Socket->PutBytes( m_Protocol->SEND_SID_CLANMEMBERLIST( ) );
				}
				else
				{
					CONSOLE_Print( "[BNET: " + m_ServerAlias + "] logon failed - invalid password, disconnecting" );

					// try to figure out if the user might be using the wrong logon type since too many people are confused by this

					string Server = m_Server;
					transform( Server.begin( ), Server.end( ), Server.begin( ), (int(*)(int))tolower );

					if( m_PasswordHashType == "pvpgn" && ( Server == "useast.battle.net" || Server == "uswest.battle.net" || Server == "asia.battle.net" || Server == "europe.battle.net" ) )
						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] it looks like you're trying to connect to a battle.net server using a pvpgn logon type, check your config file's \"battle.net custom data\" section" );
					else if( m_PasswordHashType != "pvpgn" && ( Server != "useast.battle.net" && Server != "uswest.battle.net" && Server != "asia.battle.net" && Server != "europe.battle.net" ) )
						CONSOLE_Print( "[BNET: " + m_ServerAlias + "] it looks like you're trying to connect to a pvpgn server using a battle.net logon type, check your config file's \"battle.net custom data\" section" );

					m_Socket->Disconnect( );
					delete Packet;
					return;
				}

				break;

			case CBNETProtocol :: SID_FRIENDSLIST:
				Friends = m_Protocol->RECEIVE_SID_FRIENDSLIST( Packet->GetData( ) );

				for( vector<CIncomingFriendList *> :: iterator i = m_Friends.begin( ); i != m_Friends.end( ); ++i )
					delete *i;

				m_Friends = Friends;
				break;

			case CBNETProtocol :: SID_CLANMEMBERLIST:
				vector<CIncomingClanList *> Clans = m_Protocol->RECEIVE_SID_CLANMEMBERLIST( Packet->GetData( ) );

				for( vector<CIncomingClanList *> :: iterator i = m_Clans.begin( ); i != m_Clans.end( ); ++i )
					delete *i;

				m_Clans = Clans;
				break;
			}
		}

		delete Packet;
	}
}

void CBNET :: ProcessChatEvent( CIncomingChatEvent *chatEvent )
{
	CBNETProtocol :: IncomingChatEvent Event = chatEvent->GetChatEvent( );
	bool Whisper = ( Event == CBNETProtocol :: EID_WHISPER );
	string User = chatEvent->GetUser( );
	string Message = chatEvent->GetMessage( );
	
	if( Event == CBNETProtocol :: EID_WHISPER && m_GHost->m_CurrentGame )
	{
		if( Message == "s" || Message == "sc" || Message == "spoofcheck" )
		{
			m_GHost->m_CurrentGame->AddToSpoofed( m_Server, User, true );
			return;
		}
	}
	
	m_IRC = ( Event == CBNETProtocol :: EID_IRC );

	if( Event == CBNETProtocol :: EID_WHISPER || Event == CBNETProtocol :: EID_TALK || Event == CBNETProtocol :: EID_IRC )
	{
		// handle spoof checking for current game
		// this case covers whispers - we assume that anyone who sends a whisper to the bot with message "spoofcheck" should be considered spoof checked
		// note that this means you can whisper "spoofcheck" even in a public game to manually spoofcheck if the /whois fails
		
		if( Event == CBNETProtocol :: EID_WHISPER )
		{
			CONSOLE_Print3( "[WHISPER: " + m_ServerAlias + "] [" + User + "] " + Message );
		}
		else
		{
			CONSOLE_Print( "[LOCAL: " + m_ServerAlias + "] [" + User + "] " + Message );
		}

		// handle bot commands

		if( !Message.empty( ) && Message[0] == m_CommandTrigger )
		{
			// extract the command trigger, the command, and the payload
			// e.g. "!say hello world" -> command: "say", payload: "hello world"

			string Command;
			string Payload;
			string :: size_type PayloadStart = Message.find( " " );

			if( PayloadStart != string :: npos )
			{
				Command = Message.substr( 1, PayloadStart - 1 );
				Payload = Message.substr( PayloadStart + 1 );
			}
			else
				Command = Message.substr( 1 );

			transform( Command.begin( ), Command.end( ), Command.begin( ), (int(*)(int))tolower );

			if( IsAdmin( User ) || IsRootAdmin( User ) )
			{
				CONSOLE_Print( "[BNET: " + m_ServerAlias + "] admin [" + User + "] sent command [" + Message + "]" );

				/*****************
				* ADMIN COMMANDS *
				******************/

				//
				// !ADDADMIN
				//

				if( Command == "addadmin" && !Payload.empty( ) )
				{
					if( IsRootAdmin( User ) )
					{
						if( IsAdmin( Payload ) )
							QueueChatCommand( m_GHost->m_Language->UserIsAlreadyAnAdmin( m_Server, Payload ), User, Whisper, m_IRC );
						else
							m_PairedAdminAdds.push_back( PairedAdminAdd( Whisper ? User : string( ), m_GHost->m_DB->ThreadedAdminAdd( m_Server, Payload ) ) );
					}
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}

				//
				// !ADDBAN
				// !BAN
				//

				else if( ( Command == "addban" || Command == "ban" ) && !Payload.empty( ) )
				{
					// extract the victim and the reason
					// e.g. "Varlock leaver after dying" -> victim: "Varlock", reason: "leaver after dying"

					string Victim;
					string Reason;
					stringstream SS;
					SS << Payload;
					SS >> Victim;

					if( !SS.eof( ) )
					{
						getline( SS, Reason );
						string :: size_type Start = Reason.find_first_not_of( " " );

						if( Start != string :: npos )
							Reason = Reason.substr( Start );
					}

					if( IsBannedName( Victim ) )
						QueueChatCommand( m_GHost->m_Language->UserIsAlreadyBanned( m_Server, Victim ), User, Whisper, m_IRC );
					else
						m_PairedBanAdds.push_back( PairedBanAdd( Whisper ? User : string( ), m_GHost->m_DB->ThreadedBanAdd( m_Server, Victim, string( ), string( ), User, Reason ) ) );
				}

				//
				// !CHANNEL (change channel)
				//

				else if( Command == "channel" && !Payload.empty( ) )
					QueueChatCommand( "/join " + Payload );

				//
				// !CHECKADMIN
				//

				else if( Command == "checkadmin" && !Payload.empty( ) )
				{
					if( IsRootAdmin( User ) )
					{
						if( IsAdmin( Payload ) )
							QueueChatCommand( m_GHost->m_Language->UserIsAnAdmin( m_Server, Payload ), User, Whisper, m_IRC );
						else
							QueueChatCommand( m_GHost->m_Language->UserIsNotAnAdmin( m_Server, Payload ), User, Whisper, m_IRC );
					}
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}

				//
				// !CHECKBAN
				//

				else if( Command == "checkban" && !Payload.empty( ) )
				{
					CDBBan *Ban = IsBannedName( Payload );

					if( Ban )
						QueueChatCommand( m_GHost->m_Language->UserWasBannedOnByBecause( m_Server, Payload, Ban->GetDate( ), Ban->GetAdmin( ), Ban->GetReason( ) ), User, Whisper, m_IRC );
					else
						QueueChatCommand( m_GHost->m_Language->UserIsNotBanned( m_Server, Payload ), User, Whisper, m_IRC );
				}

				//
				// !CLOSE (close slot)
				//

				else if( Command == "close" && !Payload.empty( ) && m_GHost->m_CurrentGame )
				{
					if( !m_GHost->m_CurrentGame->GetLocked( ) )
					{
						// close as many slots as specified, e.g. "5 10" closes slots 5 and 10

						stringstream SS;
						SS << Payload;

						while( !SS.eof( ) )
						{
							uint32_t SID;
							SS >> SID;

							if( SS.fail( ) )
							{
								CONSOLE_Print( "[BNET: " + m_ServerAlias + "] bad input to close command" );
								break;
							}
							else
								m_GHost->m_CurrentGame->CloseSlot( (unsigned char)( SID - 1 ), true );
						}
					}
					else
						QueueChatCommand( m_GHost->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
				}

				//
				// !CLOSEALL
				//

				else if( Command == "closeall" && m_GHost->m_CurrentGame )
				{
					if( !m_GHost->m_CurrentGame->GetLocked( ) )
						m_GHost->m_CurrentGame->CloseAllSlots( );
					else
						QueueChatCommand( m_GHost->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
				}

				//
				// !COUNTADMINS
				//

				else if( Command == "countadmins" )
				{
					if( IsRootAdmin( User ) )
						m_PairedAdminCounts.push_back( PairedAdminCount( Whisper ? User : string( ), m_GHost->m_DB->ThreadedAdminCount( m_Server ) ) );
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}

				//
				// !COUNTBANS
				//

				else if( Command == "countbans" )
					m_PairedBanCounts.push_back( PairedBanCount( Whisper ? User : string( ), m_GHost->m_DB->ThreadedBanCount( m_Server ) ) );

				//
				// !DELADMIN
				//

				else if( Command == "deladmin" && !Payload.empty( ) )
				{
					if( IsRootAdmin( User ) )
					{
						if( !IsAdmin( Payload ) )
							QueueChatCommand( m_GHost->m_Language->UserIsNotAnAdmin( m_Server, Payload ), User, Whisper, m_IRC );
						else
							m_PairedAdminRemoves.push_back( PairedAdminRemove( Whisper ? User : string( ), m_GHost->m_DB->ThreadedAdminRemove( m_Server, Payload ) ) );
					}
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}

				//
				// !DELBAN
				// !UNBAN
				//

				else if( ( Command == "delban" || Command == "unban" ) && !Payload.empty( ) )
					m_PairedBanRemoves.push_back( PairedBanRemove( Whisper ? User : string( ), m_GHost->m_DB->ThreadedBanRemove( Payload ) ) );

				//
				// !DISABLE
				//

				else if( Command == "disable" )
				{
					if( IsRootAdmin( User ) )
					{
						QueueChatCommand( m_GHost->m_Language->BotDisabled( ), User, Whisper, m_IRC );
						m_GHost->m_Enabled = false;
					}
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}

				//
				// !DOWNLOADS
				//

				else if( Command == "downloads" && !Payload.empty( ) )
				{
					uint32_t Downloads = UTIL_ToUInt32( Payload );

					if( Downloads == 0 )
					{
						QueueChatCommand( m_GHost->m_Language->MapDownloadsDisabled( ), User, Whisper, m_IRC );
						m_GHost->m_AllowDownloads = 0;
					}
					else if( Downloads == 1 )
					{
						QueueChatCommand( m_GHost->m_Language->MapDownloadsEnabled( ), User, Whisper, m_IRC );
						m_GHost->m_AllowDownloads = 1;
					}
					else if( Downloads == 2 )
					{
						QueueChatCommand( m_GHost->m_Language->MapDownloadsConditional( ), User, Whisper, m_IRC );
						m_GHost->m_AllowDownloads = 2;
					}
				}

				//
				// !ENABLE
				//

				else if( Command == "enable" )
				{
					if( IsRootAdmin( User ) )
					{
						QueueChatCommand( m_GHost->m_Language->BotEnabled( ), User, Whisper, m_IRC );
						m_GHost->m_Enabled = true;
					}
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}

				//
				// !END
				//

				else if( Command == "end" && !Payload.empty( ) )
				{
					// todotodo: what if a game ends just as you're typing this command and the numbering changes?

					uint32_t GameNumber = UTIL_ToUInt32( Payload ) - 1;

					if( GameNumber < m_GHost->m_Games.size( ) )
					{
						// if the game owner is still in the game only allow the root admin to end the game

						if( m_GHost->m_Games[GameNumber]->GetPlayerFromName( m_GHost->m_Games[GameNumber]->GetOwnerName( ), false ) && !IsRootAdmin( User ) )
							QueueChatCommand( m_GHost->m_Language->CantEndGameOwnerIsStillPlaying( m_GHost->m_Games[GameNumber]->GetOwnerName( ) ), User, Whisper, m_IRC );
						else
						{
							QueueChatCommand( m_GHost->m_Language->EndingGame( m_GHost->m_Games[GameNumber]->GetDescription( ) ), User, Whisper, m_IRC );
							CONSOLE_Print( "[GAME: " + m_GHost->m_Games[GameNumber]->GetGameName( ) + "] is over (admin ended game)" );
							m_GHost->m_Games[GameNumber]->StopPlayers( "was disconnected (admin ended game)" );
						}
					}
					else
						QueueChatCommand( m_GHost->m_Language->GameNumberDoesntExist( Payload ), User, Whisper, m_IRC );
				}

				//
				// !EXIT
				// !QUIT
				//

				else if( Command == "exit" || Command == "quit" )
				{
					if( IsRootAdmin( User ) )
					{
						if( Payload == "nice" )
							m_GHost->m_ExitingNice = true;
						else if( Payload == "force" )
							m_Exiting = true;
						else
						{
							if( m_GHost->m_CurrentGame || !m_GHost->m_Games.empty( ) )
								QueueChatCommand( m_GHost->m_Language->AtLeastOneGameActiveUseForceToShutdown( ), User, Whisper, m_IRC );
							else
								m_Exiting = true;
						}
					}
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}									

				//
				// !HOLD (hold a slot for someone)
				//

				else if( Command == "hold" && !Payload.empty( ) && m_GHost->m_CurrentGame )
				{
					// hold as many players as specified, e.g. "Varlock Kilranin" holds players "Varlock" and "Kilranin"

					stringstream SS;
					SS << Payload;

					while( !SS.eof( ) )
					{
						string HoldName;
						SS >> HoldName;

						if( SS.fail( ) )
						{
							CONSOLE_Print( "[BNET: " + m_ServerAlias + "] bad input to hold command" );
							break;
						}
						else
						{
							QueueChatCommand( m_GHost->m_Language->AddedPlayerToTheHoldList( HoldName ), User, Whisper, m_IRC );
							m_GHost->m_CurrentGame->AddToReserved( HoldName );
						}
					}
				}

				//
				// !LOAD (load config file)
				//

				else if( Command == "load" )
				{
					if( Payload.empty( ) )
						QueueChatCommand( m_GHost->m_Language->CurrentlyLoadedMapCFGIs( m_GHost->m_Map->GetCFGFile( ) ), User, Whisper, m_IRC );
					else
					{
						string FoundMapConfigs;

						try
						{
							path MapCFGPath( m_GHost->m_MapCFGPath );
							string Pattern = Payload;
							transform( Pattern.begin( ), Pattern.end( ), Pattern.begin( ), (int(*)(int))tolower );

							if( !exists( MapCFGPath ) )
							{
								CONSOLE_Print( "[BNET: " + m_ServerAlias + "] error listing map configs - map config path doesn't exist" );
								QueueChatCommand( m_GHost->m_Language->ErrorListingMapConfigs( ), User, Whisper, m_IRC );
							}
							else
							{
								directory_iterator EndIterator;
								path LastMatch;
								uint32_t Matches = 0;

								for( directory_iterator i( MapCFGPath ); i != EndIterator; ++i )
								{
									string FileName = i->filename( );
									string Stem = i->path( ).stem( );
									transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(*)(int))tolower );
									transform( Stem.begin( ), Stem.end( ), Stem.begin( ), (int(*)(int))tolower );

									if( !is_directory( i->status( ) ) && i->path( ).extension( ) == ".cfg" && FileName.find( Pattern ) != string :: npos )
									{
										LastMatch = i->path( );
										++Matches;

										if( FoundMapConfigs.empty( ) )
											FoundMapConfigs = i->filename( );
										else
											FoundMapConfigs += ", " + i->filename( );

										// if the pattern matches the filename exactly, with or without extension, stop any further matching

										if( FileName == Pattern || Stem == Pattern )
										{
											Matches = 1;
											break;
										}
									}
								}

								if( Matches == 0 )
									QueueChatCommand( m_GHost->m_Language->NoMapConfigsFound( ), User, Whisper, m_IRC );
								else if( Matches == 1 )
								{
									string File = LastMatch.filename( );
									QueueChatCommand( m_GHost->m_Language->LoadingConfigFile( m_GHost->m_MapCFGPath + File ), User, Whisper, m_IRC );
									CConfig MapCFG;
									MapCFG.Read( LastMatch.string( ) );
									m_GHost->m_Map->Load( &MapCFG, m_GHost->m_MapCFGPath + File );
								}
								else
									QueueChatCommand( m_GHost->m_Language->FoundMapConfigs( FoundMapConfigs ), User, Whisper, m_IRC );
							}
						}
						catch( const exception &ex )
						{
							CONSOLE_Print( "[BNET: " + m_ServerAlias + "] error listing map configs - caught exception [" + ex.what( ) + "]" );
							QueueChatCommand( m_GHost->m_Language->ErrorListingMapConfigs( ), User, Whisper, m_IRC );
						}
					}
				}

				//
				// !MAP (load map file)
				//

				else if( Command == "map" )
				{
					if( Payload.empty( ) )
						QueueChatCommand( m_GHost->m_Language->CurrentlyLoadedMapCFGIs( m_GHost->m_Map->GetCFGFile( ) ), User, Whisper, m_IRC );
					else
					{
						string FoundMaps;

						try
						{
							// path MapPath(  );
							string Pattern = Payload;
							transform( Pattern.begin( ), Pattern.end( ), Pattern.begin( ), (int(*)(int))tolower );

							if( !exists( m_GHost->m_MapPath ) )
							{
								CONSOLE_Print( "[BNET: " + m_ServerAlias + "] error listing maps - map path doesn't exist" );
								QueueChatCommand( m_GHost->m_Language->ErrorListingMaps( ), User, Whisper, m_IRC );
							}
							else
							{
								directory_iterator EndIterator;
								path LastMatch;
								uint32_t Matches = 0;

								for( directory_iterator i( m_GHost->m_MapPath ); i != EndIterator; ++i )
								{
									string FileName = i->filename( );
									string Stem = i->path( ).stem( );
									transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(*)(int))tolower );
									transform( Stem.begin( ), Stem.end( ), Stem.begin( ), (int(*)(int))tolower );

									if( !is_directory( i->status( ) ) && FileName.find( Pattern ) != string :: npos )
									{
										LastMatch = i->path( );
										++Matches;

										if( FoundMaps.empty( ) )
											FoundMaps = i->filename( );
										else
											FoundMaps += ", " + i->filename( );

										// if the pattern matches the filename exactly, with or without extension, stop any further matching

										if( FileName == Pattern || Stem == Pattern )
										{
											Matches = 1;
											break;
										}
									}
								}

								if( Matches == 0 )
									QueueChatCommand( m_GHost->m_Language->NoMapsFound( ), User, Whisper, m_IRC );
								else if( Matches == 1 )
								{
									string File = LastMatch.filename( );
									QueueChatCommand( m_GHost->m_Language->LoadingConfigFile( File ), User, Whisper, m_IRC );

									// hackhack: create a config file in memory with the required information to load the map

									CConfig MapCFG;
									MapCFG.Set( "map_path", "Maps\\Download\\" + File );
									MapCFG.Set( "map_localpath", File );

									if( File.find( "DotA" ) != string :: npos )
										MapCFG.Set( "map_type", "dota" );								

									m_GHost->m_Map->Load( &MapCFG, File );
								}
								else
									QueueChatCommand( m_GHost->m_Language->FoundMaps( FoundMaps ), User, Whisper, m_IRC );
							}
						}
						catch( const exception &ex )
						{
							CONSOLE_Print( "[BNET: " + m_ServerAlias + "] error listing maps - caught exception [" + ex.what( ) + "]" );
							QueueChatCommand( m_GHost->m_Language->ErrorListingMaps( ), User, Whisper, m_IRC );
						}
					}
				}

				//
				// !COUNTMAPS
				//
				else if( Command == "countmaps" || Command == "countmap" )
				{
					directory_iterator EndIterator;
					int Count = 0;

					for( directory_iterator i( m_GHost->m_MapPath ); i != EndIterator; ++i )
					{
						string FileName = i->filename( );
						transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(*)(int))tolower );

						if( FileName.find( ".w3x" ) != string :: npos || FileName.find( ".w3m" ) != string :: npos )
							++Count;
					}

					QueueChatCommand( "There are currently [" + UTIL_ToString( Count ) + "] maps.", User, Whisper, m_IRC );
				}

				//
				// !COUNTCFG(s)
				//
				else if( Command == "countcfg" || Command == "countcfgs" )
				{
					directory_iterator EndIterator;
					int Count = 0;

					for( directory_iterator i( m_GHost->m_MapCFGPath ); i != EndIterator; ++i )
					{
						string FileName = i->filename( );
						transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(*)(int))tolower );

						if( FileName.find( ".cfg" ) != string :: npos  )
							++Count;
					}

					QueueChatCommand( "There are currently [" + UTIL_ToString( Count ) + "] cfgs.", User, Whisper, m_IRC );
				}

				//
				// !DELETECFG
				//
				else if( Command == "deletecfg" && IsRootAdmin( User ) )
				{
					transform( Payload.begin( ), Payload.end( ), Payload.begin( ), (int(*)(int))tolower );

					if( Payload.size( ) > 4 && ( Payload.substr( Payload.size( ) - 4  ) != ".cfg" ) )
						Payload.append( ".cfg" );

					directory_iterator EndIterator;

					for( directory_iterator i( m_GHost->m_MapCFGPath ); i != EndIterator; ++i )
					{
						string FileName = i->filename( );
						transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(*)(int))tolower );

						if( FileName == Payload )
						{
							remove( m_GHost->m_MapCFGPath + i->filename( ) );
							QueueChatCommand( "Deleted [" + i->filename( ) + "]", User, Whisper, m_IRC );
						}
					}
				}

				//
				// !DELETEMAP
				//
				else if( Command == "deletemap" && IsRootAdmin( User ) )
				{
					transform( Payload.begin( ), Payload.end( ), Payload.begin( ), (int(*)(int))tolower );

					if( Payload.size( ) > 4 && ( Payload.substr( Payload.size( ) - 4  ) != ".w3x" || Payload.substr( Payload.size( ) - 4  ) != ".w3m" ) )
						Payload.append( ".w3x" );

					directory_iterator EndIterator;

					for( directory_iterator i( m_GHost->m_MapPath ); i != EndIterator; ++i )
					{
						string FileName = i->filename( );
						transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(*)(int))tolower );

						if( FileName == Payload )
						{
							remove( m_GHost->m_MapPath + i->filename( ) );
							QueueChatCommand( "Deleted [" + i->filename( ) + "]", User, Whisper, m_IRC );
						}
					}
				}

				//
				// !OPEN (open slot)
				//

				else if( Command == "open" && !Payload.empty( ) && m_GHost->m_CurrentGame )
				{
					if( !m_GHost->m_CurrentGame->GetLocked( ) )
					{
						// open as many slots as specified, e.g. "5 10" opens slots 5 and 10

						stringstream SS;
						SS << Payload;

						while( !SS.eof( ) )
						{
							uint32_t SID;
							SS >> SID;

							if( SS.fail( ) )
							{
								CONSOLE_Print( "[BNET: " + m_ServerAlias + "] bad input to open command" );
								break;
							}
							else
								m_GHost->m_CurrentGame->OpenSlot( (unsigned char)( SID - 1 ), true );
						}
					}
					else
						QueueChatCommand( m_GHost->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
				}

				//
				// !OPENALL
				//

				else if( Command == "openall" && m_GHost->m_CurrentGame )
				{
					if( !m_GHost->m_CurrentGame->GetLocked( ) )
						m_GHost->m_CurrentGame->OpenAllSlots( );
					else
						QueueChatCommand( m_GHost->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
				}

				//
				// !PRIV (host private game)
				//

				else if( Command == "priv" && !Payload.empty( ) )
					m_GHost->CreateGame( m_GHost->m_Map, GAME_PRIVATE, Payload, User, User, m_Server, Whisper );

				//
				// !PRIVBY (host private game by other player)
				//

				else if( Command == "privby" && !Payload.empty( ) )
				{
					// extract the owner and the game name
					// e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

					string Owner;
					string GameName;
					string :: size_type GameNameStart = Payload.find( " " );

					if( GameNameStart != string :: npos )
					{
						Owner = Payload.substr( 0, GameNameStart );
						GameName = Payload.substr( GameNameStart + 1 );
						m_GHost->CreateGame( m_GHost->m_Map, GAME_PRIVATE, GameName, Owner, User, m_Server, Whisper );
					}
				}

				//
				// !PUB (host public game)
				//

				else if( Command == "pub" && !Payload.empty( ) )
					m_GHost->CreateGame( m_GHost->m_Map, GAME_PUBLIC, Payload, User, User, m_Server, Whisper );

				//
				// !PUBBY (host public game by other player)
				//

				else if( Command == "pubby" && !Payload.empty( ) )
				{
					// extract the owner and the game name
					// e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

					string Owner;
					string GameName;
					string :: size_type GameNameStart = Payload.find( " " );

					if( GameNameStart != string :: npos )
					{
						Owner = Payload.substr( 0, GameNameStart );
						GameName = Payload.substr( GameNameStart + 1 );
						m_GHost->CreateGame( m_GHost->m_Map, GAME_PUBLIC, GameName, Owner, User, m_Server, Whisper );
					}
				}

				//
				// !RELOAD
				//

				else if( Command == "reload" )
				{
					if( IsRootAdmin( User ) )
					{
						QueueChatCommand( m_GHost->m_Language->ReloadingConfigurationFiles( ), User, Whisper, m_IRC );
						m_GHost->ReloadConfigs( );
					}
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}

				//
				// !SAY
				//

				else if( Command == "say" && !Payload.empty( ) )
				{
					if( IsRootAdmin( User ) || Payload[0] != '/'  )
						QueueChatCommand( Payload );
				}

				//
				// !SAYGAME
				//

				else if( Command == "saygame" && !Payload.empty( ) )
				{
					if( IsRootAdmin( User ) )
					{
						// extract the game number and the message
						// e.g. "3 hello everyone" -> game number: "3", message: "hello everyone"

						uint32_t GameNumber;
						string Message;
						stringstream SS;
						SS << Payload;
						SS >> GameNumber;

						if( SS.fail( ) )
							CONSOLE_Print( "[BNET: " + m_ServerAlias + "] bad input #1 to saygame command" );
						else
						{
							if( SS.eof( ) )
								CONSOLE_Print( "[BNET: " + m_ServerAlias + "] missing input #2 to saygame command" );
							else
							{
								getline( SS, Message );
								string :: size_type Start = Message.find_first_not_of( " " );

								if( Start != string :: npos )
									Message = Message.substr( Start );

								if( GameNumber - 1 < m_GHost->m_Games.size( ) )
									m_GHost->m_Games[GameNumber - 1]->SendAllChat( "ADMIN: " + Message );
								else
									QueueChatCommand( m_GHost->m_Language->GameNumberDoesntExist( UTIL_ToString( GameNumber ) ), User, Whisper, m_IRC );
							}
						}
					}
					else
						QueueChatCommand( m_GHost->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
				}

				//
				// !SAYGAMES
				//

				else if( Command == "saygames" && !Payload.empty( ) )
				{
					if( IsRootAdmin( User ) )
					{
						if( m_GHost->m_CurrentGame )
							m_GHost->m_CurrentGame->SendAllChat( Payload );

						for( vector<CBaseGame *> :: iterator i = m_GHost->m_Games.begin( ); i != m_GHost->m_Games.end( ); ++i )
							(*i)->SendAllChat( "ADMIN: " + Payload );
					}
					else
					{
						if( m_GHost->m_CurrentGame )
							m_GHost->m_CurrentGame->SendAllChat( Payload );

						for( vector<CBaseGame *> :: iterator i = m_GHost->m_Games.begin( ); i != m_GHost->m_Games.end( ); ++i )
							(*i)->SendAllChat( "ADMIN(" + User + "): " + Payload );
					}
				}

				//
				// !SP
				//

				else if( Command == "sp" && m_GHost->m_CurrentGame && !m_GHost->m_CurrentGame->GetCountDownStarted( ) )
				{
					if( !m_GHost->m_CurrentGame->GetLocked( ) )
					{
						m_GHost->m_CurrentGame->SendAllChat( m_GHost->m_Language->ShufflingPlayers( ) );
						m_GHost->m_CurrentGame->ShuffleSlots( );
					}
					else
						QueueChatCommand( m_GHost->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
				}

				//
				// !START
				//

				else if( ( Command == "start" || Command == "s" ) && m_GHost->m_CurrentGame && !m_GHost->m_CurrentGame->GetCountDownStarted( ) && m_GHost->m_CurrentGame->GetNumHumanPlayers( ) > 0 )
				{
					if( !m_GHost->m_CurrentGame->GetLocked( ) )
					{
						// if the player sent "!start force" skip the checks and start the countdown
						// otherwise check that the game is ready to start

						if( Payload == "force" )
							m_GHost->m_CurrentGame->StartCountDown( true );
						else
							m_GHost->m_CurrentGame->StartCountDown( false );
					}
					else
						QueueChatCommand( m_GHost->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
				}

				//
				// !SWAP (swap slots)
				//

				else if( Command == "swap" && !Payload.empty( ) && m_GHost->m_CurrentGame )
				{
					if( !m_GHost->m_CurrentGame->GetLocked( ) )
					{
						uint32_t SID1;
						uint32_t SID2;
						stringstream SS;
						SS << Payload;
						SS >> SID1;

						if( SS.fail( ) )
							CONSOLE_Print( "[BNET: " + m_ServerAlias + "] bad input #1 to swap command" );
						else
						{
							if( SS.eof( ) )
								CONSOLE_Print( "[BNET: " + m_ServerAlias + "] missing input #2 to swap command" );
							else
							{
								SS >> SID2;

								if( SS.fail( ) )
									CONSOLE_Print( "[BNET: " + m_ServerAlias + "] bad input #2 to swap command" );
								else
									m_GHost->m_CurrentGame->SwapSlots( (unsigned char)( SID1 - 1 ), (unsigned char)( SID2 - 1 ) );
							}
						}
					}
					else
						QueueChatCommand( m_GHost->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
				}

				

				//
				// !RESTART
				//

				else if( Command == "restart" )
				{
					if( ( !m_GHost->m_Games.size( ) && !m_GHost->m_CurrentGame ) || Payload == "force" )
					{
						m_GHost->Restart( false );
					}
					else
						QueueChatCommand( "Games in progress, use !restart force", User, Whisper, m_IRC );
				}
				
				//
				// !SPAM
				//
				
				else if( Command == "spam" && m_Server == "europe.battle.net" )
				{
					if( m_Spam )
					{
						m_Spam = false;
						QueueChatCommand( "/j " + m_FirstChannel );
						m_SpamChannel = "allstars";
						CONSOLE_Print3( "Allstars spam is off." );					
					}
					else if( m_GHost->m_CurrentGame && m_GHost->m_CurrentGame->GetGameState( ) == GAME_PRIVATE && m_GHost->m_CurrentGame->GetGameName( ).size( ) == 4 )
					{
						m_Spam = true;
						QueueChatCommand( "/j allstars" );
						CONSOLE_Print3( "[GAME: " + m_GHost->m_CurrentGame->GetGameName( ) + "] Allstars spam is on." );		
					}
					else
						QueueChatCommand( "No current game or it's public or game name isn't 4 letters long.", User, Whisper, m_IRC );
				}

				//
				// !UNHOST
				//

				else if( Command == "unhost" || Command == "uh" )
				{
					if( m_GHost->m_CurrentGame )
					{
						if( m_GHost->m_CurrentGame->GetCountDownStarted( ) )
							QueueChatCommand( m_GHost->m_Language->UnableToUnhostGameCountdownStarted( m_GHost->m_CurrentGame->GetDescription( ) ), User, Whisper, m_IRC );

						// if the game owner is still in the game only allow the root admin to unhost the game

						else if( m_GHost->m_CurrentGame->GetPlayerFromName( m_GHost->m_CurrentGame->GetOwnerName( ), false ) && !IsRootAdmin( User ) )
							QueueChatCommand( m_GHost->m_Language->CantUnhostGameOwnerIsPresent( m_GHost->m_CurrentGame->GetOwnerName( ) ), User, Whisper, m_IRC );
						else
						{
							QueueChatCommand( m_GHost->m_Language->UnhostingGame( m_GHost->m_CurrentGame->GetDescription( ) ), User, Whisper, m_IRC );
							m_GHost->m_CurrentGame->SetExiting( true );
						}
					}
					else
						QueueChatCommand( m_GHost->m_Language->UnableToUnhostGameNoGameInLobby( ), User, Whisper, m_IRC );
				}

				//
				// !W
				//

				else if( Command == "w" && !Payload.empty( ) )
				{
					// extract the name and the message
					// e.g. "Varlock hello there!" -> name: "Varlock", message: "hello there!"

					string Name;
					string Message;
					string :: size_type MessageStart = Payload.find( " " );

					if( MessageStart != string :: npos )
					{
						Name = Payload.substr( 0, MessageStart );
						Message = Payload.substr( MessageStart + 1 );

						for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
							(*i)->QueueChatCommand( Message, Name, true, false );
					}
				}
				
				//
				// !R
				//

				else if( Command == "r" && !Payload.empty( ) )
				{
					QueueChatCommand( "/r " + Payload );				
				}
				
				//
				// !WHOIS
				//

				else if( Command == "whois" && !Payload.empty( ) )
				{
					for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
						(*i)->QueueChatCommand(  "/whois " + Payload );
				}
				
				//
				// !GETCLAN
				//

				else if( Command == "getclan" )
				{
					SendGetClanList( );
					QueueChatCommand( m_GHost->m_Language->UpdatingClanList( ), User, Whisper, m_IRC );
				}

				//
				// !GETFRIENDS
				//

				else if( Command == "getfriends" )
				{
					SendGetFriendsList( );
					QueueChatCommand( m_GHost->m_Language->UpdatingFriendsList( ), User, Whisper, m_IRC );
				}				
			}
			else
				CONSOLE_Print( "[BNET: " + m_ServerAlias + "] non-admin [" + User + "] sent command [" + Message + "]" );

			/*********************
			* NON ADMIN COMMANDS *
			*********************/

			// don't respond to non admins if there are more than 3 messages already in the queue
			// this prevents malicious users from filling up the bot's chat queue and crippling the bot
			// in some cases the queue may be full of legitimate messages but we don't really care if the bot ignores one of these commands once in awhile
			// e.g. when several users join a game at the same time and cause multiple /whois messages to be queued at once

			if( IsAdmin( User ) || IsRootAdmin( User ) || ( m_PublicCommands && m_OutPackets.size( ) <= 3 ) )
			{
				//
				// !STATS
				//

				if( Command == "stats" )
				{
					string StatsUser = User;

					if( !Payload.empty( ) )
						StatsUser = Payload;

					// check for potential abuse

					if( !StatsUser.empty( ) && StatsUser.size( ) < 16 && StatsUser[0] != '/' )
						m_PairedGPSChecks.push_back( PairedGPSCheck( Whisper ? User : string( ), m_GHost->m_DB->ThreadedGamePlayerSummaryCheck( StatsUser ) ) );
				}
				
				//
				// !GETGAME
				//

				else if( Command == "getgame" && !Payload.empty( ) )
				{
					uint32_t GameNumber = UTIL_ToUInt32( Payload ) - 1;

					if( GameNumber < m_GHost->m_Games.size( ) )
						QueueChatCommand( m_GHost->m_Language->GameNumberIs( Payload, m_GHost->m_Games[GameNumber]->GetDescription( ) ), User, Whisper, m_IRC );
					else
						QueueChatCommand( m_GHost->m_Language->GameNumberDoesntExist( Payload ), User, Whisper, m_IRC );
				}		
				
				//
				// !GETGAMES
				//

				else if( Command == "getgames" )
				{
					if( m_GHost->m_CurrentGame )
						QueueChatCommand( m_GHost->m_Language->GameIsInTheLobby( m_GHost->m_CurrentGame->GetDescription( ), UTIL_ToString( m_GHost->m_Games.size( ) ), UTIL_ToString( m_GHost->m_MaxGames ) ), User, Whisper, m_IRC );
					else
						QueueChatCommand( m_GHost->m_Language->ThereIsNoGameInTheLobby( UTIL_ToString( m_GHost->m_Games.size( ) ), UTIL_ToString( m_GHost->m_MaxGames ) ), User, Whisper, m_IRC );
				}
				
				//
				// !GETPLAYERS
				//
				
				else if( Command == "getplayers" && !Payload.empty( ) )
				{
					uint32_t GameNumber = UTIL_ToUInt32( Payload ) - 1;

					if( GameNumber < m_GHost->m_Games.size( ) )
						QueueChatCommand( "Players in game [" + m_GHost->m_Games[GameNumber]->GetGameName( ) + "] are: " + m_GHost->m_Games[GameNumber]->GetPlayers( ), User, Whisper, m_IRC );
					else
						QueueChatCommand( m_GHost->m_Language->GameNumberDoesntExist( Payload ), User, Whisper, m_IRC );				
				
				}

				//
				// !STATSDOTA
				//

				else if( Command == "statsdota" )
				{
					string StatsUser = User;

					if( !Payload.empty( ) )
						StatsUser = Payload;

					// check for potential abuse

					if( !StatsUser.empty( ) && StatsUser.size( ) < 16 && StatsUser[0] != '/' )
						m_PairedDPSChecks.push_back( PairedDPSCheck( Whisper ? User : string( ), m_GHost->m_DB->ThreadedDotAPlayerSummaryCheck( StatsUser ) ) );
				}

				//
				// !VERSION
				//

				else if( Command == "version" )
				{
					if( IsAdmin( User ) || IsRootAdmin( User ) )
						QueueChatCommand( m_GHost->m_Language->VersionAdmin( m_GHost->m_Version ), User, Whisper, m_IRC );
					else
						QueueChatCommand( m_GHost->m_Language->VersionNotAdmin( m_GHost->m_Version ), User, Whisper, m_IRC );
				}
				
				//
				// !STATUS
				//

				else if ( Command == "status" && m_GHost->m_BNETs.size( ) )
				{
					string message = "Status: ";

		       		for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
					{
						message += (*i)->GetServer( ) + ( (*i)->GetLoggedIn( ) ? " [Online], " : " [Offline], " );
					}

					message += m_GHost->m_IRC->m_Server + ( !m_GHost->m_IRC->m_WaitingToConnect ? " [Online]" : " [Offline]" );

					QueueChatCommand( message, User, Whisper, m_IRC );		 
				}
				
				//
				// !ONLINE
				//
				
				else if( Command == "o" || Command == "online" )
				{
					QueueChatCommand( "/w Clan007 -o" );
				}
			}
		}
	}
	else if( Event == CBNETProtocol :: EID_CHANNEL )
	{
		// keep track of current channel so we can rejoin it after hosting a game

		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] joined channel [" + Message + "]" );
		m_CurrentChannel = Message;
		
		if( m_Spam && m_GHost->m_CurrentGame )
		{
			m_GHost->m_CurrentGame->SendAllChat( "Joined channel [" + Message + "] for spamming." ) ;
		}
	}
	else if( Event == CBNETProtocol :: EID_INFO )
	{
		CONSOLE_Print( "[INFO: " + m_ServerAlias + "] " + Message );

		// extract the first word which we hope is the username
		// this is not necessarily true though since info messages also include channel MOTD's and such

		string UserName;
		string :: size_type Split = Message.find( " " );

		if( Split != string :: npos )
			UserName = Message.substr( 0, Split );
		else
			UserName = Message;

		// handle spoof checking for current game
		// this case covers whois results which are used when hosting a public game (we send out a "/whois [player]" for each player)
		// at all times you can still /w the bot with "spoofcheck" to manually spoof check

		if( m_GHost->m_CurrentGame && m_GHost->m_CurrentGame->GetPlayerFromName( UserName, true ) )
		{
			if( Message.find( "is away" ) != string :: npos )
				m_GHost->m_CurrentGame->SendAllChat( m_GHost->m_Language->SpoofPossibleIsAway( UserName ) );
			else if( Message.find( "is unavailable" ) != string :: npos )
				m_GHost->m_CurrentGame->SendAllChat( m_GHost->m_Language->SpoofPossibleIsUnavailable( UserName ) );
			else if( Message.find( "is refusing messages" ) != string :: npos )
				m_GHost->m_CurrentGame->SendAllChat( m_GHost->m_Language->SpoofPossibleIsRefusingMessages( UserName ) );
			else if( Message.find( "is using Warcraft III The Frozen Throne in the channel" ) != string :: npos )
				m_GHost->m_CurrentGame->SendAllChat( m_GHost->m_Language->SpoofDetectedIsNotInGame( UserName ) );
			else if( Message.find( "is using Warcraft III The Frozen Throne in channel" ) != string :: npos )
				m_GHost->m_CurrentGame->SendAllChat( m_GHost->m_Language->SpoofDetectedIsNotInGame( UserName ) );
			else if( Message.find( "is using Warcraft III The Frozen Throne in a private channel" ) != string :: npos )
				m_GHost->m_CurrentGame->SendAllChat( m_GHost->m_Language->SpoofDetectedIsInPrivateChannel( UserName ) );

			if( Message.find( "is using Warcraft III The Frozen Throne in game" ) != string :: npos || Message.find( "is using Warcraft III Frozen Throne and is currently in  game" ) != string :: npos || Message.find( "is using Warcraft III Frozen Throne and is currently in private game" ) != string :: npos )
			{
				// check both the current game name and the last game name against the /whois response
				// this is because when the game is rehosted, players who joined recently will be in the previous game according to battle.net
				// note: if the game is rehosted more than once it is possible (but unlikely) for a false positive because only two game names are checked

				if( Message.find( m_GHost->m_CurrentGame->GetGameName( ) ) != string :: npos || Message.find( m_GHost->m_CurrentGame->GetLastGameName( ) ) != string :: npos )
					m_GHost->m_CurrentGame->AddToSpoofed( m_Server, UserName, false );
				else
					m_GHost->m_CurrentGame->SendAllChat( m_GHost->m_Language->SpoofDetectedIsInAnotherGame( UserName ) );
			}
		}
	}
	else if( Event == CBNETProtocol :: EID_ERROR )
	{
		CONSOLE_Print( "[ERROR: " + m_ServerAlias + "] " + Message );		
		
		if( m_Spam && Message == "Channel is full." )
		{			
			if( m_SpamChannel == "allstars/j allstars/j allstars" )
				m_SpamChannel = "allstars";
			else
				m_SpamChannel += "/j allstars";
				
			QueueChatCommand( "/j " + m_SpamChannel );
		}
	}
}

void CBNET :: SendJoinChannel( const string &channel )
{
	if( m_LoggedIn && m_InChat )
		m_Socket->PutBytes( m_Protocol->SEND_SID_JOINCHANNEL( channel ) );
}

void CBNET :: SendGetFriendsList( )
{
	if( m_LoggedIn )
		m_Socket->PutBytes( m_Protocol->SEND_SID_FRIENDSLIST( ) );
}

void CBNET :: SendGetClanList( )
{
	if( m_LoggedIn )
		m_Socket->PutBytes( m_Protocol->SEND_SID_CLANMEMBERLIST( ) );
}

void CBNET :: QueueEnterChat( )
{
	if( m_LoggedIn )
		m_OutPackets.push( m_Protocol->SEND_SID_ENTERCHAT( ) );
}

void CBNET :: QueueChatCommand( const string &chatCommand )
{
	if( chatCommand.empty( ) )
		return;

	if( m_LoggedIn && m_OutPackets.size( ) <= 10 )
	{
		CONSOLE_Print( "[QUEUED: " + m_ServerAlias + "] " + chatCommand );
		
		if( m_PasswordHashType == "pvpgn" )			
			m_OutPackets.push( m_Protocol->SEND_SID_CHATCOMMAND( chatCommand.substr( 0, 200 ) ) );	
		else if( chatCommand.size( ) > 255 )
			m_OutPackets.push( m_Protocol->SEND_SID_CHATCOMMAND( chatCommand.substr( 0, 255 ) ) );
		else
			m_OutPackets.push( m_Protocol->SEND_SID_CHATCOMMAND( chatCommand ) );	
	}
	else
		CONSOLE_Print( "[BNET: " + m_ServerAlias + "] too many (" + UTIL_ToString( m_OutPackets.size( ) ) + ") packets queued, discarding" );
}


void CBNET :: QueueChatCommand( const string &chatCommand, const string &user, bool whisper, bool irc )
{
	if( chatCommand.empty( ) )
		return;

	// if the destination is IRC send it there
	
	if( irc )
	{
		m_GHost->m_IRC->SendMessageIRC( chatCommand, string( ) );
		return;
	}
	
	// if whisper is true send the chat command as a whisper to user, otherwise just queue the chat command

	if( whisper )
	{
		QueueChatCommand( "/w " + user + " " + chatCommand );
	}
	else
		QueueChatCommand( chatCommand );
}

void CBNET :: QueueGameCreate( unsigned char state, const string &gameName, const string &hostName, CMap *map, uint32_t hostCounter )
{
	if( m_LoggedIn && map )
	{
		if( !m_CurrentChannel.empty( ) )
			m_FirstChannel = m_CurrentChannel;

		m_InChat = false;

		// a game creation message is just a game refresh message with upTime = 0

		QueueGameRefresh( state, gameName, hostName, map, hostCounter );
	}
}

void CBNET :: QueueGameRefresh( unsigned char state, const string &gameName, string hostName, CMap *map, uint32_t hostCounter )
{
	if( hostName.empty( ) )
	{
		BYTEARRAY UniqueName = m_Protocol->GetUniqueName( );
		hostName = string( UniqueName.begin( ), UniqueName.end( ) );
	}

	if( m_LoggedIn && map )
	{
		// construct a fixed host counter which will be used to identify players from this realm
		// the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
		// the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
		// since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
		// when a player joins a game we can obtain the ID from the received host counter
		// note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

		uint32_t MapGameType = map->GetMapGameType( );
		MapGameType |= MAPGAMETYPE_UNKNOWN0;

		if( state == GAME_PRIVATE )
			MapGameType |= MAPGAMETYPE_PRIVATEGAME;

		// use an invalid map width/height to indicate reconnectable games

		BYTEARRAY MapWidth;
		MapWidth.push_back( 192 );
		MapWidth.push_back( 7 );
		BYTEARRAY MapHeight;
		MapHeight.push_back( 192 );
		MapHeight.push_back( 7 );

		if( m_GHost->m_Reconnect )
			m_OutPackets.push( m_Protocol->SEND_SID_STARTADVEX3( state, UTIL_CreateByteArray( MapGameType, false ), map->GetMapGameFlags( ), MapWidth, MapHeight, gameName, hostName, 0, map->GetMapPath( ), map->GetMapCRC( ), map->GetMapSHA1( ), ( ( hostCounter & 0x0FFFFFFF ) | ( m_HostCounterID << 28 ) ) ) );
		else
			m_OutPackets.push( m_Protocol->SEND_SID_STARTADVEX3( state, UTIL_CreateByteArray( MapGameType, false ), map->GetMapGameFlags( ), map->GetMapWidth( ), map->GetMapHeight( ), gameName, hostName, 0, map->GetMapPath( ), map->GetMapCRC( ), map->GetMapSHA1( ), ( ( hostCounter & 0x0FFFFFFF ) | ( m_HostCounterID << 28 ) ) ) );
	}
}

void CBNET :: QueueGameUncreate( )
{
	if( m_LoggedIn )
		m_OutPackets.push( m_Protocol->SEND_SID_STOPADV( ) );
}

void CBNET :: UnqueueGameRefreshes( )
{
	queue<BYTEARRAY> Packets;

	while( !m_OutPackets.empty( ) )
	{
		// todotodo: it's very inefficient to have to copy all these packets while searching the queue

		BYTEARRAY Packet = m_OutPackets.front( );
		m_OutPackets.pop( );

		if( Packet.size( ) >= 2 && Packet[1] != CBNETProtocol :: SID_STARTADVEX3 )
		{
			Packets.push( Packet );
		}
	}

	m_OutPackets = Packets;
	CONSOLE_Print( "[BNET: " + m_ServerAlias + "] unqueued game refresh packets" );
}

bool CBNET :: IsAdmin( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );

	for( vector<string> :: iterator i = m_Admins.begin( ); i != m_Admins.end( ); ++i )
	{
		if( *i == name )
			return true;
	}

	return false;
}

bool CBNET :: IsRootAdmin( string name )
{
	// m_RootAdmin was already transformed to lower case in the constructor

	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );

	// updated to permit multiple root admins seperated by a space, e.g. "Varlock Kilranin Instinct121"
	// note: this function gets called frequently so it would be better to parse the root admins just once and store them in a list somewhere
	// however, it's hardly worth optimizing at this point since the code's already written

	stringstream SS;
	string s;
	SS << m_RootAdmin;

	while( !SS.eof( ) )
	{
		SS >> s;

		if( name == s )
			return true;
	}

	return false;
}

CDBBan *CBNET :: IsBannedName( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );

	// todotodo: optimize this - maybe use a map?

	for( vector<CDBBan *> :: iterator i = m_Bans.begin( ); i != m_Bans.end( ); ++i )
	{
		if( (*i)->GetName( ) == name )
			return *i;
	}

	return NULL;
}

CDBBan *CBNET :: IsBannedIP( const string &ip )
{
	// todotodo: optimize this - maybe use a map?

	for( vector<CDBBan *> :: iterator i = m_Bans.begin( ); i != m_Bans.end( ); ++i )
	{
		if( (*i)->GetIP( ) == ip )
			return *i;
	}

	return NULL;
}

void CBNET :: AddAdmin( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	m_Admins.push_back( name );
}

void CBNET :: AddBan( string name, string ip, string gamename, string admin, string reason )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );
	m_Bans.push_back( new CDBBan( m_Server, name, ip, "N/A", gamename, admin, reason ) );
}

void CBNET :: RemoveAdmin( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );

	for( vector<string> :: iterator i = m_Admins.begin( ); i != m_Admins.end( ); )
	{
		if( *i == name )
			i = m_Admins.erase( i );
		else
			++i;
	}
}

void CBNET :: RemoveBan( string name )
{
	transform( name.begin( ), name.end( ), name.begin( ), (int(*)(int))tolower );

	for( vector<CDBBan *> :: iterator i = m_Bans.begin( ); i != m_Bans.end( ); )
	{
		if( (*i)->GetName( ) == name )
			i = m_Bans.erase( i );
		else
			++i;
	}
}

void CBNET :: HoldFriends( CBaseGame *game )
{
	if( game )
	{
		for( vector<CIncomingFriendList *> :: iterator i = m_Friends.begin( ); i != m_Friends.end( ); ++i )
			game->AddToReserved( (*i)->GetAccount( ) );
	}
}

void CBNET :: HoldClan( CBaseGame *game )
{
	if( game )
	{
		for( vector<CIncomingClanList *> :: iterator i = m_Clans.begin( ); i != m_Clans.end( ); ++i )
			game->AddToReserved( (*i)->GetName( ) );
	}
}
