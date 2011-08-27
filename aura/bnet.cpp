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

#include "aura.h"
#include "util.h"
#include "config.h"
#include "language.h"
#include "socket.h"
#include "auradb.h"
#include "bncsutilinterface.h"
#include "bnetprotocol.h"
#include "bnet.h"
#include "map.h"
#include "gameprotocol.h"
#include "game.h"
#include "irc.h"

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

//
// CBNET
//

CBNET::CBNET( CAura *nAura, string nServer, string nServerAlias, string nCDKeyROC, string nCDKeyTFT, string nCountryAbbrev, string nCountry, uint32_t nLocaleID, string nUserName, string nUserPassword, string nFirstChannel, char nCommandTrigger, unsigned char nWar3Version, BYTEARRAY nEXEVersion, BYTEARRAY nEXEVersionHash, string nPasswordHashType, uint32_t nHostCounterID ) : m_Aura( nAura ), m_Exiting( false ), m_Spam( false ), m_Server( nServer ), m_CDKeyROC( nCDKeyROC ), m_CDKeyTFT( nCDKeyTFT ), m_CountryAbbrev( nCountryAbbrev ), m_Country( nCountry ), m_LocaleID( nLocaleID ), m_UserName( nUserName ), m_UserPassword( nUserPassword ), m_FirstChannel( nFirstChannel ), m_CommandTrigger( nCommandTrigger ), m_War3Version( nWar3Version ), m_EXEVersion( nEXEVersion ), m_EXEVersionHash( nEXEVersionHash ), m_PasswordHashType( nPasswordHashType ), m_HostCounterID( nHostCounterID ), m_LastDisconnectedTime( 0 ), m_LastConnectionAttemptTime( 0 ), m_LastNullTime( 0 ), m_LastOutPacketTicks( 0 ), m_LastOutPacketSize( 0 ), m_LastAdminRefreshTime( GetTime( ) ), m_LastBanRefreshTime( GetTime( ) ), m_LastSpamTime( 0 ), m_FirstConnect( true ), m_WaitingToConnect( true ), m_LoggedIn( false ), m_InChat( false )
{
  m_Socket = new CTCPClient( );
  m_Protocol = new CBNETProtocol( );
  m_BNCSUtil = new CBNCSUtilInterface( nUserName, nUserPassword );

  if ( m_PasswordHashType == "pvpgn" || m_EXEVersion.size( ) == 4 || m_EXEVersionHash.size( ) == 4 )
  {
    m_PvPGN = true;
    m_ReconnectDelay = 90;
  }
  else
  {
    m_PvPGN = false;
    m_ReconnectDelay = 240;
  }

  if ( !nServerAlias.empty( ) )
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
  transform( m_CDKeyROC.begin( ), m_CDKeyROC.end( ), m_CDKeyROC.begin( ), (int(* )(int) )toupper );
  transform( m_CDKeyTFT.begin( ), m_CDKeyTFT.end( ), m_CDKeyTFT.begin( ), (int(* )(int) )toupper );

  if ( m_CDKeyROC.size( ) != 26 )
    Print( "[BNET: " + m_ServerAlias + "] warning - your ROC CD key is not 26 characters long and is probably invalid" );

  if ( m_CDKeyTFT.size( ) != 26 )
    Print( "[BNET: " + m_ServerAlias + "] warning - your TFT CD key is not 26 characters long and is probably invalid" );
}

CBNET::~CBNET( )
{
  delete m_Socket;
  delete m_Protocol;
  delete m_BNCSUtil;
}

unsigned int CBNET::SetFD( void *fd, void *send_fd, int *nfds )
{
  if ( !m_Socket->HasError( ) && m_Socket->GetConnected( ) )
  {
    m_Socket->SetFD( (fd_set *) fd, (fd_set *) send_fd, nfds );
    return 1;
  }

  return 0;
}

bool CBNET::Update( void *fd, void *send_fd )
{
  uint32_t Ticks = GetTicks( ), Time = GetTime( );

  // we return at the end of each if statement so we don't have to deal with errors related to the order of the if statements
  // that means it might take a few ms longer to complete a task involving multiple steps (in this case, reconnecting) due to blocking or sleeping
  // but it's not a big deal at all, maybe 100ms in the worst possible case (based on a 50ms blocking time)

  if ( m_Socket->HasError( ) )
  {
    // the socket has an error

    Print2( "[BNET: " + m_ServerAlias + "] disconnected from battle.net due to socket error" );
    Print( "[BNET: " + m_ServerAlias + "] waiting " + UTIL_ToString( m_ReconnectDelay ) + " seconds to reconnect" );
    m_BNCSUtil->Reset( m_UserName, m_UserPassword );
    m_Socket->Reset( );
    m_LastDisconnectedTime = Time;
    m_LoggedIn = false;
    m_InChat = false;
    m_WaitingToConnect = true;
    return m_Exiting;
  }

  if ( m_Socket->GetConnected( ) )
  {
    // the socket is connected and everything appears to be working properly

    m_Socket->DoRecv( (fd_set *) fd );

    // extract as many packets as possible from the socket's receive buffer and put them in the m_Packets queue

    string *RecvBuffer = m_Socket->GetBytes( );
    BYTEARRAY Bytes = UTIL_CreateByteArray( (unsigned char *) RecvBuffer->c_str( ), RecvBuffer->size( ) );

    CIncomingGameHost *GameHost = NULL;
    CIncomingChatEvent *ChatEvent = NULL;

    // a packet is at least 4 bytes so loop as long as the buffer contains 4 bytes

    while ( Bytes.size( ) >= 4 )
    {
      // byte 0 is always 255

      if ( Bytes[0] == BNET_HEADER_CONSTANT )
      {
        // bytes 2 and 3 contain the length of the packet

        uint16_t Length = (uint16_t) ( Bytes[3] << 8 | Bytes[2] );

        if ( Bytes.size( ) >= Length )
        {
          // m_Packets.push( new CCommandPacket( BNET_HEADER_CONSTANT, Bytes[1], BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) );

          switch ( Bytes[1] )
          {
            case CBNETProtocol::SID_NULL:
              // warning: we do not respond to NULL packets with a NULL packet of our own
              // this is because PVPGN servers are programmed to respond to NULL packets so it will create a vicious cycle of useless traffic
              // official battle.net servers do not respond to NULL packets

              m_Protocol->RECEIVE_SID_NULL( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );
              break;

            case CBNETProtocol::SID_GETADVLISTEX:
              GameHost = m_Protocol->RECEIVE_SID_GETADVLISTEX( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );

              if ( GameHost )
                Print( "[BNET: " + m_ServerAlias + "] joining game [" + GameHost->GetGameName( ) + "]" );

              delete GameHost;
              GameHost = NULL;
              break;

            case CBNETProtocol::SID_ENTERCHAT:
              if ( m_Protocol->RECEIVE_SID_ENTERCHAT( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) )
              {
                Print( "[BNET: " + m_ServerAlias + "] joining channel [" + m_FirstChannel + "]" );
                m_InChat = true;
                m_Socket->PutBytes( m_Protocol->SEND_SID_JOINCHANNEL( m_FirstChannel ) );
              }

              break;

            case CBNETProtocol::SID_CHATEVENT:
              ChatEvent = m_Protocol->RECEIVE_SID_CHATEVENT( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );

              if ( ChatEvent )
                ProcessChatEvent( ChatEvent );

              delete ChatEvent;
              ChatEvent = NULL;
              break;

            case CBNETProtocol::SID_CHECKAD:
              m_Protocol->RECEIVE_SID_CHECKAD( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );
              break;

            case CBNETProtocol::SID_STARTADVEX3:
              if ( m_Protocol->RECEIVE_SID_STARTADVEX3( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) )
              {
                m_InChat = false;
              }
              else
              {
                Print( "[BNET: " + m_ServerAlias + "] startadvex3 failed" );
                m_Aura->EventBNETGameRefreshFailed( this );
              }

              break;

            case CBNETProtocol::SID_PING:
              m_Socket->PutBytes( m_Protocol->SEND_SID_PING( m_Protocol->RECEIVE_SID_PING( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) ) );
              break;

            case CBNETProtocol::SID_AUTH_INFO:
              if ( m_Protocol->RECEIVE_SID_AUTH_INFO( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) )
              {
                if ( m_BNCSUtil->HELP_SID_AUTH_CHECK( m_Aura->m_Warcraft3Path, m_CDKeyROC, m_CDKeyTFT, m_Protocol->GetValueStringFormulaString( ), m_Protocol->GetIX86VerFileNameString( ), m_Protocol->GetClientToken( ), m_Protocol->GetServerToken( ) ) )
                {
                  // override the exe information generated by bncsutil if specified in the config file
                  // apparently this is useful for pvpgn users

                  if ( m_EXEVersion.size( ) == 4 )
                  {
                    Print( "[BNET: " + m_ServerAlias + "] using custom exe version bnet_custom_exeversion = " + UTIL_ToString( m_EXEVersion[0] ) + " " + UTIL_ToString( m_EXEVersion[1] ) + " " + UTIL_ToString( m_EXEVersion[2] ) + " " + UTIL_ToString( m_EXEVersion[3] ) );
                    m_BNCSUtil->SetEXEVersion( m_EXEVersion );
                  }

                  if ( m_EXEVersionHash.size( ) == 4 )
                  {
                    Print( "[BNET: " + m_ServerAlias + "] using custom exe version hash bnet_custom_exeversionhash = " + UTIL_ToString( m_EXEVersionHash[0] ) + " " + UTIL_ToString( m_EXEVersionHash[1] ) + " " + UTIL_ToString( m_EXEVersionHash[2] ) + " " + UTIL_ToString( m_EXEVersionHash[3] ) );
                    m_BNCSUtil->SetEXEVersionHash( m_EXEVersionHash );
                  }

                  Print( "[BNET: " + m_ServerAlias + "] attempting to auth as Warcraft III: The Frozen Throne" );

                  m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_CHECK( m_Protocol->GetClientToken( ), m_BNCSUtil->GetEXEVersion( ), m_BNCSUtil->GetEXEVersionHash( ), m_BNCSUtil->GetKeyInfoROC( ), m_BNCSUtil->GetKeyInfoTFT( ), m_BNCSUtil->GetEXEInfo( ), "Aura" ) );

                }
                else
                {
                  Print( "[BNET: " + m_ServerAlias + "] logon failed - bncsutil key hash failed (check your Warcraft 3 path and cd keys), disconnecting" );
                  m_Socket->Disconnect( );
                }
              }

              break;

            case CBNETProtocol::SID_AUTH_CHECK:
              if ( m_Protocol->RECEIVE_SID_AUTH_CHECK( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) )
              {
                // cd keys accepted

                Print( "[BNET: " + m_ServerAlias + "] cd keys accepted" );
                m_BNCSUtil->HELP_SID_AUTH_ACCOUNTLOGON( );
                m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGON( m_BNCSUtil->GetClientKey( ), m_UserName ) );
              }
              else
              {
                // cd keys not accepted

                switch ( UTIL_ByteArrayToUInt32( m_Protocol->GetKeyState( ), false ) )
                {
                  case CBNETProtocol::KR_ROC_KEY_IN_USE:
                    Print( "[BNET: " + m_ServerAlias + "] logon failed - ROC CD key in use by user [" + m_Protocol->GetKeyStateDescription( ) + "], disconnecting" );
                    break;
                  case CBNETProtocol::KR_TFT_KEY_IN_USE:
                    Print( "[BNET: " + m_ServerAlias + "] logon failed - TFT CD key in use by user [" + m_Protocol->GetKeyStateDescription( ) + "], disconnecting" );
                    break;
                  case CBNETProtocol::KR_OLD_GAME_VERSION:
                    Print( "[BNET: " + m_ServerAlias + "] logon failed - game version is too old, disconnecting" );
                    break;
                  case CBNETProtocol::KR_INVALID_VERSION:
                    Print( "[BNET: " + m_ServerAlias + "] logon failed - game version is invalid, disconnecting" );
                    break;
                  default:
                    Print( "[BNET: " + m_ServerAlias + "] logon failed - cd keys not accepted, disconnecting" );
                    break;
                }

                m_Socket->Disconnect( );
              }

              break;

            case CBNETProtocol::SID_AUTH_ACCOUNTLOGON:
              if ( m_Protocol->RECEIVE_SID_AUTH_ACCOUNTLOGON( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) )
              {
                Print( "[BNET: " + m_ServerAlias + "] username [" + m_UserName + "] accepted" );

                if ( m_PasswordHashType == "pvpgn" )
                {
                  // pvpgn logon

                  Print( "[BNET: " + m_ServerAlias + "] using pvpgn logon type (for pvpgn servers only)" );
                  m_BNCSUtil->HELP_PvPGNPasswordHash( m_UserPassword );
                  m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGONPROOF( m_BNCSUtil->GetPvPGNPasswordHash( ) ) );
                }
                else
                {
                  // battle.net logon

                  Print( "[BNET: " + m_ServerAlias + "] using battle.net logon type (for official battle.net servers only)" );
                  m_BNCSUtil->HELP_SID_AUTH_ACCOUNTLOGONPROOF( m_Protocol->GetSalt( ), m_Protocol->GetServerPublicKey( ) );
                  m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_ACCOUNTLOGONPROOF( m_BNCSUtil->GetM1( ) ) );
                }
              }
              else
              {
                Print( "[BNET: " + m_ServerAlias + "] logon failed - invalid username, disconnecting" );
                m_Socket->Disconnect( );
              }

              break;

            case CBNETProtocol::SID_AUTH_ACCOUNTLOGONPROOF:
              if ( m_Protocol->RECEIVE_SID_AUTH_ACCOUNTLOGONPROOF( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) ) )
              {
                // logon successful

                Print( "[BNET: " + m_ServerAlias + "] logon successful" );
                m_LoggedIn = true;
                m_Socket->PutBytes( m_Protocol->SEND_SID_NETGAMEPORT( m_Aura->m_HostPort ) );
                m_Socket->PutBytes( m_Protocol->SEND_SID_ENTERCHAT( ) );
                m_Socket->PutBytes( m_Protocol->SEND_SID_FRIENDSLIST( ) );
                m_Socket->PutBytes( m_Protocol->SEND_SID_CLANMEMBERLIST( ) );
              }
              else
              {
                Print( "[BNET: " + m_ServerAlias + "] logon failed - invalid password, disconnecting" );

                // try to figure out if the user might be using the wrong logon type since too many people are confused by this

                string Server = m_Server;
                transform( Server.begin( ), Server.end( ), Server.begin( ), (int(* )(int) )tolower );

                if ( m_PvPGN && ( Server == "useast.battle.net" || Server == "uswest.battle.net" || Server == "asia.battle.net" || Server == "europe.battle.net" ) )
                  Print( "[BNET: " + m_ServerAlias + "] it looks like you're trying to connect to a battle.net server using a pvpgn logon type, check your config file's \"battle.net custom data\" section" );
                else if ( !m_PvPGN && ( Server != "useast.battle.net" && Server != "uswest.battle.net" && Server != "asia.battle.net" && Server != "europe.battle.net" ) )
                  Print( "[BNET: " + m_ServerAlias + "] it looks like you're trying to connect to a pvpgn server using a battle.net logon type, check your config file's \"battle.net custom data\" section" );

                m_Socket->Disconnect( );
              }

              break;

            case CBNETProtocol::SID_FRIENDSLIST:
              m_Friends = m_Protocol->RECEIVE_SID_FRIENDSLIST( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );
              break;

            case CBNETProtocol::SID_CLANMEMBERLIST:
              m_Clans = m_Protocol->RECEIVE_SID_CLANMEMBERLIST( BYTEARRAY( Bytes.begin( ), Bytes.begin( ) + Length ) );
              break;
          }

          *RecvBuffer = RecvBuffer->substr( Length );
          Bytes = BYTEARRAY( Bytes.begin( ) + Length, Bytes.end( ) );
        }
        else
          break;
      }
    }

    // check if at least one packet is waiting to be sent and if we've waited long enough to prevent flooding
    // this formula has changed many times but currently we wait 1 second if the last packet was "small", 3.5 seconds if it was "medium", and 4 seconds if it was "big"

    uint32_t WaitTicks;

    if ( m_LastOutPacketSize < 10 )
      WaitTicks = 1300;
    else if ( m_LastOutPacketSize < 100 )
      WaitTicks = 3300;
    else
      WaitTicks = 4300;

    if ( !m_OutPackets.empty( ) && Ticks - m_LastOutPacketTicks >= WaitTicks )
    {
      if ( m_OutPackets.size( ) > 7 )
        Print( "[BNET: " + m_ServerAlias + "] packet queue warning - there are " + UTIL_ToString( m_OutPackets.size( ) ) + " packets waiting to be sent" );

      m_Socket->PutBytes( m_OutPackets.front( ) );
      m_LastOutPacketSize = m_OutPackets.front( ).size( );
      m_OutPackets.pop( );
      m_LastOutPacketTicks = Ticks;
    }

    // send a null packet every 60 seconds to detect disconnects

    if ( Time - m_LastNullTime >= 60 && Ticks - m_LastOutPacketTicks >= 60000 )
    {
      m_Socket->PutBytes( m_Protocol->SEND_SID_NULL( ) );
      m_LastNullTime = Time;
    }

    // spam game name every 5 seconds (if enabled)

    if ( m_Spam && Time - m_LastSpamTime > 4 )
    {
      if ( m_InChat )
      {
        if ( m_SpamChannel == m_CurrentChannel )
        {
          if ( m_Aura->m_CurrentGame )
            m_OutPackets.push( m_Protocol->SEND_SID_CHATCOMMAND( m_Aura->m_CurrentGame->GetGameName( ) ) );
        }
        else
          QueueChatCommand( "/j " + m_SpamChannel );
      }

      m_LastSpamTime = Time;
    }

    m_Socket->DoSend( (fd_set *) send_fd );
    return m_Exiting;
  }

  if ( !m_Socket->GetConnected( ) && !m_Socket->GetConnecting( ) && !m_WaitingToConnect )
  {
    // the socket was disconnected

    Print2( "[BNET: " + m_ServerAlias + "] disconnected from battle.net" );
    m_LastDisconnectedTime = Time;
    m_BNCSUtil->Reset( m_UserName, m_UserPassword );
    m_Socket->Reset( );
    m_LoggedIn = false;
    m_InChat = false;
    m_WaitingToConnect = true;
    return m_Exiting;
  }

  if ( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && ( m_FirstConnect || ( Time - m_LastDisconnectedTime >= m_ReconnectDelay ) ) )
  {
    // attempt to connect to battle.net

    m_FirstConnect = false;
    Print( "[BNET: " + m_ServerAlias + "] connecting to server [" + m_Server + "] on port 6112" );

    if ( !m_Aura->m_BindAddress.empty( ) )
      Print( "[BNET: " + m_ServerAlias + "] attempting to bind to address [" + m_Aura->m_BindAddress + "]" );

    if ( m_ServerIP.empty( ) )
    {
      m_Socket->Connect( m_Aura->m_BindAddress, m_Server, 6112 );

      if ( !m_Socket->HasError( ) )
      {
        m_ServerIP = m_Socket->GetIPString( );
        Print( "[BNET: " + m_ServerAlias + "] resolved and cached server IP address " + m_ServerIP );
      }
    }
    else
    {
      // use cached server IP address since resolving takes time and is blocking

      Print( "[BNET: " + m_ServerAlias + "] using cached server IP address " + m_ServerIP );
      m_Socket->Connect( m_Aura->m_BindAddress, m_ServerIP, 6112 );
    }

    m_WaitingToConnect = false;
    m_LastConnectionAttemptTime = Time;
  }

  if ( m_Socket->GetConnecting( ) )
  {
    // we are currently attempting to connect to battle.net

    if ( m_Socket->CheckConnect( ) )
    {
      // the connection attempt completed

      Print2( "[BNET: " + m_ServerAlias + "] connected" );
      m_Socket->PutBytes( m_Protocol->SEND_PROTOCOL_INITIALIZE_SELECTOR( ) );
      m_Socket->PutBytes( m_Protocol->SEND_SID_AUTH_INFO( m_War3Version, m_LocaleID, m_CountryAbbrev, m_Country ) );
      m_Socket->DoSend( (fd_set *) send_fd );
      m_LastNullTime = Time;
      m_LastOutPacketTicks = Ticks;

      while ( !m_OutPackets.empty( ) )
        m_OutPackets.pop( );

      return m_Exiting;
    }
    else if ( Time - m_LastConnectionAttemptTime >= 15 )
    {
      // the connection attempt timed out (15 seconds)

      Print( "[BNET: " + m_ServerAlias + "] connect timed out" );
      Print( "[BNET: " + m_ServerAlias + "] waiting 90 seconds to reconnect" );
      m_Socket->Reset( );
      m_LastDisconnectedTime = Time;
      m_WaitingToConnect = true;
      return m_Exiting;
    }
  }

  return m_Exiting;
}

void CBNET::ProcessChatEvent( CIncomingChatEvent *chatEvent )
{
  CBNETProtocol::IncomingChatEvent Event = chatEvent->GetChatEvent( );
  bool Whisper = ( Event == CBNETProtocol::EID_WHISPER );
  string User = chatEvent->GetUser( );
  string Message = chatEvent->GetMessage( );

  // handle spoof checking for current game
  // this case covers whispers - we assume that anyone who sends a whisper to the bot with message "spoofcheck" should be considered spoof checked
  // note that this means you can whisper "spoofcheck" even in a public game to manually spoofcheck if the /whois fails

  if ( Event == CBNETProtocol::EID_WHISPER && m_Aura->m_CurrentGame )
  {
    if ( Message == "s" || Message == "sc" || Message == "spoofcheck" )
    {
      m_Aura->m_CurrentGame->AddToSpoofed( m_Server, User, true );
      return;
    }
  }

  if ( Event == CBNETProtocol::EID_IRC )
  {
    // extract the irc channel
    
    string::size_type MessageStart = Message.find( " " );
    
    m_IRC = Message.substr( 0, MessageStart );
    Message = Message.substr( MessageStart + 1 );
  }
  else
    m_IRC.clear( );

  if ( Event == CBNETProtocol::EID_WHISPER || Event == CBNETProtocol::EID_TALK || Event == CBNETProtocol::EID_IRC )
  {
    if ( Event == CBNETProtocol::EID_WHISPER )
      Print2( "[WHISPER: " + m_ServerAlias + "] [" + User + "] " + Message );
    else
      Print( "[LOCAL: " + m_ServerAlias + "] [" + User + "] " + Message );

    // handle bot commands

    if ( !Message.empty( ) && Message[0] == m_CommandTrigger )
    {
      // extract the command trigger, the command, and the payload
      // e.g. "!say hello world" -> command: "say", payload: "hello world"

      string Command, Payload;
      string::size_type PayloadStart = Message.find( " " );

      if ( PayloadStart != string::npos )
      {
        Command = Message.substr( 1, PayloadStart - 1 );
        Payload = Message.substr( PayloadStart + 1 );
      }
      else
        Command = Message.substr( 1 );

      transform( Command.begin( ), Command.end( ), Command.begin( ), (int(* )(int) )tolower );

      if ( IsAdmin( User ) || IsRootAdmin( User ) )
      {
        Print( "[BNET: " + m_ServerAlias + "] admin [" + User + "] sent command [" + Message + "]" );

        /*****************
         * ADMIN COMMANDS *
         ******************/

        //
        // !MAP (load map file)
        //

        if ( Command == "map" )
        {
          if ( Payload.empty( ) )
            QueueChatCommand( m_Aura->m_Language->CurrentlyLoadedMapCFGIs( m_Aura->m_Map->GetCFGFile( ) ), User, Whisper, m_IRC );
          else
          {
            string FoundMaps;

            try
            {
              // path MapPath(  );
              string Pattern = Payload;
              transform( Pattern.begin( ), Pattern.end( ), Pattern.begin( ), (int(* )(int) )tolower );

              if ( !exists( m_Aura->m_MapPath ) )
              {
                Print( "[BNET: " + m_ServerAlias + "] error listing maps - map path doesn't exist" );
                QueueChatCommand( m_Aura->m_Language->ErrorListingMaps( ), User, Whisper, m_IRC );
              }
              else
              {
                directory_iterator EndIterator;
                path LastMatch;
                uint32_t Matches = 0;

                for ( directory_iterator i( m_Aura->m_MapPath ); i != EndIterator; ++i )
                {
                  string FileName = i->path( ).filename( ).string( );
                  string Stem = i->path( ).stem( ).string( );
                  transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(* )(int) )tolower );
                  transform( Stem.begin( ), Stem.end( ), Stem.begin( ), (int(* )(int) )tolower );

                  if ( !is_directory( i->status( ) ) && FileName.find( Pattern ) != string::npos )
                  {
                    LastMatch = i->path( );
                    ++Matches;

                    if ( FoundMaps.empty( ) )
                      FoundMaps = i->path( ).filename( ).string( );
                    else
                      FoundMaps += ", " + i->path( ).filename( ).string( );

                    // if the pattern matches the filename exactly, with or without extension, stop any further matching

                    if ( FileName == Pattern || Stem == Pattern )
                    {
                      Matches = 1;
                      break;
                    }
                  }
                }

                if ( Matches == 0 )
                  QueueChatCommand( m_Aura->m_Language->NoMapsFound( ), User, Whisper, m_IRC );
                else if ( Matches == 1 )
                {
                  string File = LastMatch.filename( ).string( );
                  QueueChatCommand( m_Aura->m_Language->LoadingConfigFile( File ), User, Whisper, m_IRC );

                  // hackhack: create a config file in memory with the required information to load the map

                  CConfig MapCFG;
                  MapCFG.Set( "map_path", "Maps\\Download\\" + File );
                  MapCFG.Set( "map_localpath", File );

                  if ( File.find( "DotA" ) != string::npos )
                    MapCFG.Set( "map_type", "dota" );

                  m_Aura->m_Map->Load( &MapCFG, File );
                }
                else
                  QueueChatCommand( m_Aura->m_Language->FoundMaps( FoundMaps ), User, Whisper, m_IRC );
              }
            }
            catch ( const exception &e )
            {
              Print( "[BNET: " + m_ServerAlias + "] error listing maps - caught exception [" + e.what( ) + "]" );
              QueueChatCommand( m_Aura->m_Language->ErrorListingMaps( ), User, Whisper, m_IRC );
            }
          }
        }
        
        //
        // !SPAM
        //

        else if ( Command == "spam" && m_Aura->m_CurrentGame && !m_Aura->m_CurrentGame->GetCountDownStarted( )
                  && m_Aura->m_CurrentGame->GetMap( )->GetMapType( ) == "dota" )
        {
          for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
            if ( !(*i)->GetPvPGN( ) )
              (*i)->SetSpam( );
        }
        
        //
        // !UNHOST
        //

        else if ( Command == "unhost" || Command == "uh" )
        {
          if ( m_Aura->m_CurrentGame )
          {
            if ( m_Aura->m_CurrentGame->GetCountDownStarted( ) )
              QueueChatCommand( m_Aura->m_Language->UnableToUnhostGameCountdownStarted( m_Aura->m_CurrentGame->GetDescription( ) ), User, Whisper, m_IRC );

              // if the game owner is still in the game only allow the root admin to unhost the game

            else if ( m_Aura->m_CurrentGame->GetPlayerFromName( m_Aura->m_CurrentGame->GetOwnerName( ), false ) && !IsRootAdmin( User ) )
              QueueChatCommand( m_Aura->m_Language->CantUnhostGameOwnerIsPresent( m_Aura->m_CurrentGame->GetOwnerName( ) ), User, Whisper, m_IRC );
            else
            {
              QueueChatCommand( m_Aura->m_Language->UnhostingGame( m_Aura->m_CurrentGame->GetDescription( ) ), User, Whisper, m_IRC );
              m_Aura->m_CurrentGame->SetExiting( true );

              if ( m_Spam )
              {
                m_Spam = false;
                QueueChatCommand( "/j " + m_FirstChannel );
              }
            }
          }
          else
            QueueChatCommand( m_Aura->m_Language->UnableToUnhostGameNoGameInLobby( ), User, Whisper, m_IRC );
        }
        
        //
        // !PUB (host public game)
        //

        else if ( Command == "pub" && !Payload.empty( ) )
          m_Aura->CreateGame( m_Aura->m_Map, GAME_PUBLIC, Payload, User, User, m_Server, Whisper );

        //
        // !PRIV (host private game)
        //

        else if ( Command == "priv" && !Payload.empty( ) )
          m_Aura->CreateGame( m_Aura->m_Map, GAME_PRIVATE, Payload, User, User, m_Server, Whisper );

        //
        // !LOAD (load config file)
        //

        else if ( Command == "load" )
        {
          if ( Payload.empty( ) )
            QueueChatCommand( m_Aura->m_Language->CurrentlyLoadedMapCFGIs( m_Aura->m_Map->GetCFGFile( ) ), User, Whisper, m_IRC );
          else
          {
            string FoundMapConfigs;

            try
            {
              path MapCFGPath( m_Aura->m_MapCFGPath );
              string Pattern = Payload;
              transform( Pattern.begin( ), Pattern.end( ), Pattern.begin( ), (int(* )(int) )tolower );

              if ( !exists( MapCFGPath ) )
              {
                Print( "[BNET: " + m_ServerAlias + "] error listing map configs - map config path doesn't exist" );
                QueueChatCommand( m_Aura->m_Language->ErrorListingMapConfigs( ), User, Whisper, m_IRC );
              }
              else
              {
                directory_iterator EndIterator;
                path LastMatch;
                uint32_t Matches = 0;

                for ( directory_iterator i( MapCFGPath ); i != EndIterator; ++i )
                {
                  string FileName = i->path( ).filename( ).string( );
                  string Stem = i->path( ).stem( ).string( );
                  transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(* )(int) )tolower );
                  transform( Stem.begin( ), Stem.end( ), Stem.begin( ), (int(* )(int) )tolower );

                  if ( !is_directory( i->status( ) ) && i->path( ).extension( ) == ".cfg" && FileName.find( Pattern ) != string::npos )
                  {
                    LastMatch = i->path( );
                    ++Matches;

                    if ( FoundMapConfigs.empty( ) )
                      FoundMapConfigs = i->path( ).filename( ).string( );
                    else
                      FoundMapConfigs += ", " + i->path( ).filename( ).string( );

                    // if the pattern matches the filename exactly, with or without extension, stop any further matching

                    if ( FileName == Pattern || Stem == Pattern )
                    {
                      Matches = 1;
                      break;
                    }
                  }
                }

                if ( Matches == 0 )
                  QueueChatCommand( m_Aura->m_Language->NoMapConfigsFound( ), User, Whisper, m_IRC );
                else if ( Matches == 1 )
                {
                  string File = LastMatch.filename( ).string( );
                  QueueChatCommand( m_Aura->m_Language->LoadingConfigFile( m_Aura->m_MapCFGPath + File ), User, Whisper, m_IRC );
                  CConfig MapCFG;
                  MapCFG.Read( LastMatch.string( ) );
                  m_Aura->m_Map->Load( &MapCFG, m_Aura->m_MapCFGPath + File );
                }
                else
                  QueueChatCommand( m_Aura->m_Language->FoundMapConfigs( FoundMapConfigs ), User, Whisper, m_IRC );
              }
            }
            catch ( const exception &e )
            {
              Print( "[BNET: " + m_ServerAlias + "] error listing map configs - caught exception [" + e.what( ) + "]" );
              QueueChatCommand( m_Aura->m_Language->ErrorListingMapConfigs( ), User, Whisper, m_IRC );
            }
          }
        }
        
        //
        // !ADDADMIN
        //

        else if ( Command == "addadmin" && !Payload.empty( ) )
        {
          if ( IsRootAdmin( User ) )
          {
            if ( IsAdmin( Payload ) )
              QueueChatCommand( m_Aura->m_Language->UserIsAlreadyAnAdmin( m_Server, Payload ), User, Whisper, m_IRC );
            else if ( m_Aura->m_DB->AdminAdd( m_Server, Payload ) )
              QueueChatCommand( m_Aura->m_Language->AddedUserToAdminDatabase( m_Server, Payload ), User, Whisper, m_IRC );
            else
              QueueChatCommand( m_Aura->m_Language->ErrorAddingUserToAdminDatabase( m_Server, Payload ), User, Whisper, m_IRC );
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
        
        //
        // !ADDBAN
        // !BAN
        //

        else if ( ( Command == "addban" || Command == "ban" ) && !Payload.empty( ) )
        {
          // extract the victim and the reason
          // e.g. "Varlock leaver after dying" -> victim: "Varlock", reason: "leaver after dying"

          string Victim, Reason;
          stringstream SS;
          SS << Payload;
          SS >> Victim;

          if ( !SS.eof( ) )
          {
            getline( SS, Reason );
            string::size_type Start = Reason.find_first_not_of( " " );

            if ( Start != string::npos )
              Reason = Reason.substr( Start );
          }

          CDBBan *Ban = IsBannedName( Victim );

          if ( Ban )
          {
            QueueChatCommand( m_Aura->m_Language->UserIsAlreadyBanned( m_Server, Victim ), User, Whisper, m_IRC );

            delete Ban;
            Ban = NULL;
          }
          else if ( m_Aura->m_DB->BanAdd( m_Server, Victim, string( ), string( ), User, Reason ) )
            QueueChatCommand( m_Aura->m_Language->BannedUser( m_Server, Victim ), User, Whisper, m_IRC );
          else
            QueueChatCommand( m_Aura->m_Language->ErrorBanningUser( m_Server, Victim ), User, Whisper, m_IRC );
        }
        
        //
        // !CHANNEL (change channel)
        //

        else if ( Command == "channel" && !Payload.empty( ) )
          QueueChatCommand( "/join " + Payload );

        //
        // !CHECKADMIN
        //

        else if ( Command == "checkadmin" && !Payload.empty( ) )
        {
          if ( IsRootAdmin( User ) )
          {
            if ( IsAdmin( Payload ) )
              QueueChatCommand( m_Aura->m_Language->UserIsAnAdmin( m_Server, Payload ), User, Whisper, m_IRC );
            else
              QueueChatCommand( m_Aura->m_Language->UserIsNotAnAdmin( m_Server, Payload ), User, Whisper, m_IRC );
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
        
        //
        // !CHECKBAN
        //

        else if ( Command == "checkban" && !Payload.empty( ) )
        {
          CDBBan *Ban = IsBannedName( Payload );

          if ( Ban )
            QueueChatCommand( m_Aura->m_Language->UserWasBannedOnByBecause( m_Server, Payload, Ban->GetDate( ), Ban->GetAdmin( ), Ban->GetReason( ) ), User, Whisper, m_IRC );
          else
            QueueChatCommand( m_Aura->m_Language->UserIsNotBanned( m_Server, Payload ), User, Whisper, m_IRC );

          delete Ban;
          Ban = NULL;
        }
        
        //
        // !CLOSE (close slot)
        //

        else if ( Command == "close" && !Payload.empty( ) && m_Aura->m_CurrentGame )
        {
          if ( !m_Aura->m_CurrentGame->GetLocked( ) )
          {
            // close as many slots as specified, e.g. "5 10" closes slots 5 and 10

            stringstream SS;
            SS << Payload;

            while ( !SS.eof( ) )
            {
              uint32_t SID;
              SS >> SID;

              if ( SS.fail( ) )
              {
                Print( "[BNET: " + m_ServerAlias + "] bad input to close command" );
                break;
              }
              else
                m_Aura->m_CurrentGame->CloseSlot( (unsigned char) ( SID - 1 ), true );
            }
          }
          else
            QueueChatCommand( m_Aura->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
        }
        
        //
        // !CLOSEALL
        //

        else if ( Command == "closeall" && m_Aura->m_CurrentGame )
        {
          if ( !m_Aura->m_CurrentGame->GetLocked( ) )
            m_Aura->m_CurrentGame->CloseAllSlots( );
          else
            QueueChatCommand( m_Aura->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
        }
        
        //
        // !COUNTADMINS
        //

        else if ( Command == "countadmins" )
        {
          if ( IsRootAdmin( User ) )
          {
            uint32_t Count = m_Aura->m_DB->AdminCount( m_Server );

            if ( Count == 0 )
              QueueChatCommand( m_Aura->m_Language->ThereAreNoAdmins( m_Server ), User, Whisper, m_IRC );
            else if ( Count == 1 )
              QueueChatCommand( m_Aura->m_Language->ThereIsAdmin( m_Server ), User, Whisper, m_IRC );
            else
              QueueChatCommand( m_Aura->m_Language->ThereAreAdmins( m_Server, UTIL_ToString( Count ) ), User, Whisper, m_IRC );
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
        
        //
        // !COUNTBANS
        //

        else if ( Command == "countbans" )
        {
          uint32_t Count = m_Aura->m_DB->BanCount( m_Server );

          if ( Count == 0 )
            QueueChatCommand( m_Aura->m_Language->ThereAreNoBannedUsers( m_Server ), User, Whisper, m_IRC );
          else if ( Count == 1 )
            QueueChatCommand( m_Aura->m_Language->ThereIsBannedUser( m_Server ), User, Whisper, m_IRC );
          else
            QueueChatCommand( m_Aura->m_Language->ThereAreBannedUsers( m_Server, UTIL_ToString( Count ) ), User, Whisper, m_IRC );
        }
        
        //
        // !DELADMIN
        //

        else if ( Command == "deladmin" && !Payload.empty( ) )
        {
          if ( IsRootAdmin( User ) )
          {
            if ( !IsAdmin( Payload ) )
              QueueChatCommand( m_Aura->m_Language->UserIsNotAnAdmin( m_Server, Payload ), User, Whisper, m_IRC );
            else if ( m_Aura->m_DB->AdminRemove( m_Server, Payload ) )
              QueueChatCommand( m_Aura->m_Language->DeletedUserFromAdminDatabase( m_Server, Payload ), User, Whisper, m_IRC );
            else
              QueueChatCommand( m_Aura->m_Language->ErrorDeletingUserFromAdminDatabase( m_Server, Payload ), User, Whisper, m_IRC );
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
        
        //
        // !DELBAN
        // !UNBAN
        //

        else if ( ( Command == "delban" || Command == "unban" ) && !Payload.empty( ) )
        {
          if ( m_Aura->m_DB->BanRemove( Payload ) )
            QueueChatCommand( m_Aura->m_Language->UnbannedUser( Payload ), User, Whisper, m_IRC );
          else
            QueueChatCommand( m_Aura->m_Language->ErrorUnbanningUser( Payload ), User, Whisper, m_IRC );
        }
        
        //
        // !DOWNLOADS
        // !DLS
        //

        else if ( ( Command == "downloads" || Command == "dls" ) )
        {
          if ( Payload.empty( ) )
          {
            if ( m_Aura->m_AllowDownloads == 0 )
              QueueChatCommand( m_Aura->m_Language->MapDownloadsDisabled( ), User, Whisper, m_IRC );
            else if ( m_Aura->m_AllowDownloads == 1 )
              QueueChatCommand( m_Aura->m_Language->MapDownloadsEnabled( ), User, Whisper, m_IRC );
            else if ( m_Aura->m_AllowDownloads == 2 )
              QueueChatCommand( m_Aura->m_Language->MapDownloadsConditional( ), User, Whisper, m_IRC );

            return;
          }

          uint32_t Downloads = UTIL_ToUInt32( Payload );

          if ( Downloads == 0 )
          {
            QueueChatCommand( m_Aura->m_Language->MapDownloadsDisabled( ), User, Whisper, m_IRC );
            m_Aura->m_AllowDownloads = 0;
          }
          else if ( Downloads == 1 )
          {
            QueueChatCommand( m_Aura->m_Language->MapDownloadsEnabled( ), User, Whisper, m_IRC );
            m_Aura->m_AllowDownloads = 1;
          }
          else if ( Downloads == 2 )
          {
            QueueChatCommand( m_Aura->m_Language->MapDownloadsConditional( ), User, Whisper, m_IRC );
            m_Aura->m_AllowDownloads = 2;
          }
        }
        
        //
        // !END
        //

        else if ( Command == "end" && !Payload.empty( ) )
        {
          // TODO: what if a game ends just as you're typing this command and the numbering changes?

          uint32_t GameNumber = UTIL_ToUInt32( Payload ) - 1;

          if ( GameNumber < m_Aura->m_Games.size( ) )
          {
            // if the game owner is still in the game only allow the root admin to end the game

            if ( m_Aura->m_Games[GameNumber]->GetPlayerFromName( m_Aura->m_Games[GameNumber]->GetOwnerName( ), false ) && !IsRootAdmin( User ) )
              QueueChatCommand( m_Aura->m_Language->CantEndGameOwnerIsStillPlaying( m_Aura->m_Games[GameNumber]->GetOwnerName( ) ), User, Whisper, m_IRC );
            else
            {
              QueueChatCommand( m_Aura->m_Language->EndingGame( m_Aura->m_Games[GameNumber]->GetDescription( ) ), User, Whisper, m_IRC );
              Print( "[GAME: " + m_Aura->m_Games[GameNumber]->GetGameName( ) + "] is over (admin ended game)" );
              m_Aura->m_Games[GameNumber]->StopPlayers( "was disconnected (admin ended game)" );
            }
          }
          else
            QueueChatCommand( m_Aura->m_Language->GameNumberDoesntExist( Payload ), User, Whisper, m_IRC );
        }
        
        //
        // !HOLD (hold a slot for someone)
        //

        else if ( Command == "hold" && !Payload.empty( ) && m_Aura->m_CurrentGame )
        {
          // hold as many players as specified, e.g. "Varlock Kilranin" holds players "Varlock" and "Kilranin"

          stringstream SS;
          SS << Payload;

          while ( !SS.eof( ) )
          {
            string HoldName;
            SS >> HoldName;

            if ( SS.fail( ) )
            {
              Print( "[BNET: " + m_ServerAlias + "] bad input to hold command" );
              break;
            }
            else
            {
              QueueChatCommand( m_Aura->m_Language->AddedPlayerToTheHoldList( HoldName ), User, Whisper, m_IRC );
              m_Aura->m_CurrentGame->AddToReserved( HoldName );
            }
          }
        }

        //
        // !SENDLAN
        //
        
        else if ( Command == "sendlan" && m_Aura->m_CurrentGame && !Payload.empty( ) && !m_Aura->m_CurrentGame->GetCountDownStarted( ) )
				{
					// extract the ip and the port
					// e.g. "1.2.3.4 6112" -> ip: "1.2.3.4", port: "6112"

					string IP;
					uint32_t Port = 6112;
					stringstream SS;
					SS << Payload;
					SS >> IP;

					if ( !SS.eof( ) )
						SS >> Port;

					if ( SS.fail( ) )
						QueueChatCommand( "Bad input to sendlan command", User, Whisper, m_IRC );
					else
					{
						// construct a fixed host counter which will be used to identify players from this "realm" (i.e. LAN)
						// the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
						// the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
						// since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
						// when a player joins a game we can obtain the ID from the received host counter
						// note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

						// we send 12 for SlotsTotal because this determines how many PID's Warcraft 3 allocates
						// we need to make sure Warcraft 3 allocates at least SlotsTotal + 1 but at most 12 PID's
						// this is because we need an extra PID for the virtual host player (but we always delete the virtual host player when the 12th person joins)
						// however, we can't send 13 for SlotsTotal because this causes Warcraft 3 to crash when sharing control of units
						// nor can we send SlotsTotal because then Warcraft 3 crashes when playing maps with less than 12 PID's (because of the virtual host player taking an extra PID)
						// we also send 12 for SlotsOpen because Warcraft 3 assumes there's always at least one player in the game (the host)
						// so if we try to send accurate numbers it'll always be off by one and results in Warcraft 3 assuming the game is full when it still needs one more player
						// the easiest solution is to simply send 12 for both so the game will always show up as (1/12) players

						// note: the PrivateGame flag is not set when broadcasting to LAN (as you might expect)
						// note: we do not use m_Map->GetMapGameType because none of the filters are set when broadcasting to LAN (also as you might expect)

						m_Aura->m_UDPSocket->SendTo( IP, Port, m_Aura->m_CurrentGame->GetProtocol( )->SEND_W3GS_GAMEINFO( m_Aura->m_LANWar3Version, UTIL_CreateByteArray( (uint32_t) MAPGAMETYPE_UNKNOWN0, false ), m_Aura->m_CurrentGame->GetMap( )->GetMapGameFlags( ), m_Aura->m_CurrentGame->GetMap( )->GetMapWidth( ), m_Aura->m_CurrentGame->GetMap( )->GetMapHeight( ), m_Aura->m_CurrentGame->GetGameName( ), "Clan 007", 0, m_Aura->m_CurrentGame->GetMap( )->GetMapPath( ), m_Aura->m_CurrentGame->GetMap( )->GetMapCRC( ), 12, 12, m_Aura->m_CurrentGame->GetHostPort( ), m_Aura->m_CurrentGame->GetHostCounter( ) & 0x0FFFFFFF, m_Aura->m_CurrentGame->GetEntryKey( ) ) );
					}
				}

        //
        // !COUNTMAPS
        // !COUNTMAP
        //

        else if ( Command == "countmaps" || Command == "countmap" )
        {
          directory_iterator EndIterator;
          int Count = 0;

          for ( directory_iterator i( m_Aura->m_MapPath ); i != EndIterator; ++i )
          {
            string FileName = i->path( ).filename( ).string( );
            transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(* )(int) )tolower );

            if ( FileName.find( ".w3x" ) != string::npos || FileName.find( ".w3m" ) != string::npos )
              ++Count;
          }

          QueueChatCommand( "There are currently [" + UTIL_ToString( Count ) + "] maps.", User, Whisper, m_IRC );
        }
        
        //
        // !COUNTCFG
        // !COUNTCFGS
        //

        else if ( Command == "countcfg" || Command == "countcfgs" )
        {
          directory_iterator EndIterator;
          int Count = 0;

          for ( directory_iterator i( m_Aura->m_MapCFGPath ); i != EndIterator; ++i )
          {
            string FileName = i->path( ).filename( ).string( );
            transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(* )(int) )tolower );

            if ( FileName.find( ".cfg" ) != string::npos )
              ++Count;
          }

          QueueChatCommand( "There are currently [" + UTIL_ToString( Count ) + "] cfgs.", User, Whisper, m_IRC );
        }
        
        //
        // !DELETECFG
        //

        else if ( Command == "deletecfg" && IsRootAdmin( User ) && !Payload.empty( ) )
        {
          transform( Payload.begin( ), Payload.end( ), Payload.begin( ), (int(* )(int) )tolower );

          if ( Payload.find( ".cfg" ) == string::npos )
            Payload.append( ".cfg" );

          directory_iterator EndIterator;

          for ( directory_iterator i( m_Aura->m_MapCFGPath ); i != EndIterator; ++i )
          {
            string FileName = i->path( ).filename( ).string( );
            transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(* )(int) )tolower );

            if ( FileName == Payload )
            {
              try
              {
                remove( m_Aura->m_MapCFGPath + i->path( ).filename( ).string( ) );
                QueueChatCommand( "Deleted [" + i->path( ).filename( ).string( ) + "]", User, Whisper, m_IRC );
              }
              catch( const exception &e )
              {
                // removal failed
              }
            }
          }
        }
        
        //
        // !DELETEMAP
        //

        else if ( Command == "deletemap" && IsRootAdmin( User ) && !Payload.empty( ) )
        {
          transform( Payload.begin( ), Payload.end( ), Payload.begin( ), (int(* )(int) )tolower );

          if ( Payload.find( ".w3x" ) == string::npos && Payload.find( ".w3m" ) == string::npos )
            Payload.append( ".w3x" );

          directory_iterator EndIterator;

          for ( directory_iterator i( m_Aura->m_MapPath ); i != EndIterator; ++i )
          {
          	string FileName = i->path( ).filename( ).string( );
            transform( FileName.begin( ), FileName.end( ), FileName.begin( ), (int(* )(int) )tolower );

            if ( FileName == Payload )
            {
              try
              {
                remove( m_Aura->m_MapPath + i->path( ).filename( ).string( ) );
                QueueChatCommand( "Deleted [" + i->path( ).filename( ).string( ) + "]", User, Whisper, m_IRC );
              }
              catch( const exception &e )
              {
                // removal failed
              }
            }
          }
        }
        
        //
        // !OPEN (open slot)
        //

        else if ( Command == "open" && !Payload.empty( ) && m_Aura->m_CurrentGame )
        {
          if ( !m_Aura->m_CurrentGame->GetLocked( ) )
          {
            // open as many slots as specified, e.g. "5 10" opens slots 5 and 10

            stringstream SS;
            SS << Payload;

            while ( !SS.eof( ) )
            {
              uint32_t SID;
              SS >> SID;

              if ( SS.fail( ) )
              {
                Print( "[BNET: " + m_ServerAlias + "] bad input to open command" );
                break;
              }
              else
                m_Aura->m_CurrentGame->OpenSlot( (unsigned char) ( SID - 1 ), true );
            }
          }
          else
            QueueChatCommand( m_Aura->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
        }
        
        //
        // !OPENALL
        //

        else if ( Command == "openall" && m_Aura->m_CurrentGame )
        {
          if ( !m_Aura->m_CurrentGame->GetLocked( ) )
            m_Aura->m_CurrentGame->OpenAllSlots( );
          else
            QueueChatCommand( m_Aura->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
        }
        
        //
        // !PRIVBY (host private game by other player)
        //

        else if ( Command == "privby" && !Payload.empty( ) )
        {
          // extract the owner and the game name
          // e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

          string Owner, GameName;
          string::size_type GameNameStart = Payload.find( " " );

          if ( GameNameStart != string::npos )
          {
            Owner = Payload.substr( 0, GameNameStart );
            GameName = Payload.substr( GameNameStart + 1 );
            m_Aura->CreateGame( m_Aura->m_Map, GAME_PRIVATE, GameName, Owner, User, m_Server, Whisper );
          }
        }
        
        //
        // !PUBBY (host public game by other player)
        //

        else if ( Command == "pubby" && !Payload.empty( ) )
        {
          // extract the owner and the game name
          // e.g. "Varlock dota 6.54b arem ~~~" -> owner: "Varlock", game name: "dota 6.54b arem ~~~"

          string Owner, GameName;
          string::size_type GameNameStart = Payload.find( " " );

          if ( GameNameStart != string::npos )
          {
            Owner = Payload.substr( 0, GameNameStart );
            GameName = Payload.substr( GameNameStart + 1 );
            m_Aura->CreateGame( m_Aura->m_Map, GAME_PUBLIC, GameName, Owner, User, m_Server, Whisper );
          }
        }
        
        //
        // !RELOAD
        //

        else if ( Command == "reload" )
        {
          if ( IsRootAdmin( User ) )
          {
            QueueChatCommand( m_Aura->m_Language->ReloadingConfigurationFiles( ), User, Whisper, m_IRC );
            m_Aura->ReloadConfigs( );
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
        
        //
        // !SAY
        //

        else if ( Command == "say" && !Payload.empty( ) )
        {
          if ( IsRootAdmin( User ) || Payload[0] != '/' )
            QueueChatCommand( Payload );
        }
        
        //
        // !SAYGAME
        //

        else if ( Command == "saygame" && !Payload.empty( ) )
        {
          if ( IsRootAdmin( User ) )
          {
            // extract the game number and the message
            // e.g. "3 hello everyone" -> game number: "3", message: "hello everyone"

            uint32_t GameNumber;
            string Message;
            stringstream SS;
            SS << Payload;
            SS >> GameNumber;

            if ( SS.fail( ) )
              Print( "[BNET: " + m_ServerAlias + "] bad input #1 to saygame command" );
            else
            {
              if ( SS.eof( ) )
                Print( "[BNET: " + m_ServerAlias + "] missing input #2 to saygame command" );
              else
              {
                getline( SS, Message );
                string::size_type Start = Message.find_first_not_of( " " );

                if ( Start != string::npos )
                  Message = Message.substr( Start );

                if ( GameNumber - 1 < m_Aura->m_Games.size( ) )
                  m_Aura->m_Games[GameNumber - 1]->SendAllChat( "ADMIN: " + Message );
                else
                  QueueChatCommand( m_Aura->m_Language->GameNumberDoesntExist( UTIL_ToString( GameNumber ) ), User, Whisper, m_IRC );
              }
            }
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
        
        //
        // !SAYGAMES
        //

        else if ( Command == "saygames" && !Payload.empty( ) )
        {
          if ( IsRootAdmin( User ) )
          {
            if ( m_Aura->m_CurrentGame )
              m_Aura->m_CurrentGame->SendAllChat( Payload );

            for ( vector<CGame *> ::const_iterator i = m_Aura->m_Games.begin( ); i != m_Aura->m_Games.end( ); ++i )
              ( *i )->SendAllChat( "ADMIN: " + Payload );
          }
          else
          {
            if ( m_Aura->m_CurrentGame )
              m_Aura->m_CurrentGame->SendAllChat( Payload );

            for ( vector<CGame *> ::const_iterator i = m_Aura->m_Games.begin( ); i != m_Aura->m_Games.end( ); ++i )
              ( *i )->SendAllChat( "ADMIN (" + User + "): " + Payload );
          }
        }
        
        //
        // !SP
        //

        else if ( Command == "sp" && m_Aura->m_CurrentGame && !m_Aura->m_CurrentGame->GetCountDownStarted( ) )
        {
          if ( !m_Aura->m_CurrentGame->GetLocked( ) )
          {
            m_Aura->m_CurrentGame->SendAllChat( m_Aura->m_Language->ShufflingPlayers( ) );
            m_Aura->m_CurrentGame->ShuffleSlots( );
          }
          else
            QueueChatCommand( m_Aura->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
        }
        
        //
        // !START
        // !S
        //

        else if ( ( Command == "start" || Command == "s" ) && m_Aura->m_CurrentGame && !m_Aura->m_CurrentGame->GetCountDownStarted( ) && m_Aura->m_CurrentGame->GetNumHumanPlayers( ) > 0 )
        {
          if ( !m_Aura->m_CurrentGame->GetLocked( ) )
          {
            // if the player sent "!start force" skip the checks and start the countdown
            // otherwise check that the game is ready to start

            if ( Payload == "force" )
              m_Aura->m_CurrentGame->StartCountDown( true );
            else
              m_Aura->m_CurrentGame->StartCountDown( false );
          }
          else
            QueueChatCommand( m_Aura->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
        }
        
        //
        // !SWAP (swap slots)
        //

        else if ( Command == "swap" && !Payload.empty( ) && m_Aura->m_CurrentGame )
        {
          if ( !m_Aura->m_CurrentGame->GetLocked( ) )
          {
            uint32_t SID1, SID2;
            stringstream SS;
            SS << Payload;
            SS >> SID1;

            if ( SS.fail( ) )
              Print( "[BNET: " + m_ServerAlias + "] bad input #1 to swap command" );
            else
            {
              if ( SS.eof( ) )
                Print( "[BNET: " + m_ServerAlias + "] missing input #2 to swap command" );
              else
              {
                SS >> SID2;

                if ( SS.fail( ) )
                  Print( "[BNET: " + m_ServerAlias + "] bad input #2 to swap command" );
                else
                  m_Aura->m_CurrentGame->SwapSlots( (unsigned char) ( SID1 - 1 ), (unsigned char) ( SID2 - 1 ) );
              }
            }
          }
          else
            QueueChatCommand( m_Aura->m_Language->TheGameIsLockedBNET( ), User, Whisper, m_IRC );
        }
        
        //
        // !RESTART
        //

        else if ( Command == "restart" )
        {
          if ( ( !m_Aura->m_Games.size( ) && !m_Aura->m_CurrentGame ) || Payload == "force" )
          {
            m_Exiting = true;

            // gRestart is defined in aura.cpp

            extern bool gRestart;
            gRestart = true;
          }
          else
            QueueChatCommand( "Games in progress, use !restart force", User, Whisper, m_IRC );
        }

        //
        // !W
        //

        else if ( Command == "w" && !Payload.empty( ) )
        {
          // extract the name and the message
          // e.g. "Varlock hello there!" -> name: "Varlock", message: "hello there!"

          string Name;
          string Message;
          string::size_type MessageStart = Payload.find( " " );

          if ( MessageStart != string::npos )
          {
            Name = Payload.substr( 0, MessageStart );
            Message = Payload.substr( MessageStart + 1 );

            for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
              ( *i )->QueueChatCommand( Message, Name, true, string( ) );
          }
        }
        
        //
        // !DISABLE
        //

        else if ( Command == "disable" )
        {
          if ( IsRootAdmin( User ) )
          {
            QueueChatCommand( m_Aura->m_Language->BotDisabled( ), User, Whisper, m_IRC );
            m_Aura->m_Enabled = false;
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
        
        //
        // !ENABLE
        //

        else if ( Command == "enable" )
        {
          if ( IsRootAdmin( User ) )
          {
            QueueChatCommand( m_Aura->m_Language->BotEnabled( ), User, Whisper, m_IRC );
            m_Aura->m_Enabled = true;
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
        
        //
        // !GETCLAN
        //

        else if ( Command == "getclan" )
        {
          SendGetClanList( );
          QueueChatCommand( m_Aura->m_Language->UpdatingClanList( ), User, Whisper, m_IRC );
        }
        
        //
        // !GETFRIENDS
        //

        else if ( Command == "getfriends" )
        {
          SendGetFriendsList( );
          QueueChatCommand( m_Aura->m_Language->UpdatingFriendsList( ), User, Whisper, m_IRC );
        }
        
        //
        // !EXIT
        // !QUIT
        //

        else if ( Command == "exit" || Command == "quit" )
        {
          if ( IsRootAdmin( User ) )
          {
            if ( Payload == "force" )
              m_Exiting = true;
            else
            {
              if ( m_Aura->m_CurrentGame || !m_Aura->m_Games.empty( ) )
                QueueChatCommand( m_Aura->m_Language->AtLeastOneGameActiveUseForceToShutdown( ), User, Whisper, m_IRC );
              else
                m_Exiting = true;
            }
          }
          else
            QueueChatCommand( m_Aura->m_Language->YouDontHaveAccessToThatCommand( ), User, Whisper, m_IRC );
        }
      }
      else
        Print( "[BNET: " + m_ServerAlias + "] non-admin [" + User + "] sent command [" + Message + "]" );

      /*********************
       * NON ADMIN COMMANDS *
       *********************/

      // don't respond to non admins if there are more than 3 messages already in the queue
      // this prevents malicious users from filling up the bot's chat queue and crippling the bot
      // in some cases the queue may be full of legitimate messages but we don't really care if the bot ignores one of these commands once in awhile
      // e.g. when several users join a game at the same time and cause multiple /whois messages to be queued at once

      if ( IsAdmin( User ) || IsRootAdmin( User ) || m_OutPackets.size( ) < 3 )
      {
        //
        // !STATS
        //

        if ( Command == "stats" )
        {
          string StatsUser = User;

          if ( !Payload.empty( ) )
            StatsUser = Payload;

          // check for potential abuse

          if ( !StatsUser.empty( ) && StatsUser.size( ) < 16 && StatsUser[0] != '/' )
          {
            CDBGamePlayerSummary *GamePlayerSummary = m_Aura->m_DB->GamePlayerSummaryCheck( StatsUser );

            if ( GamePlayerSummary )
              QueueChatCommand( m_Aura->m_Language->HasPlayedGamesWithThisBot( StatsUser, GamePlayerSummary->GetFirstGameDateTime( ), GamePlayerSummary->GetLastGameDateTime( ), UTIL_ToString( GamePlayerSummary->GetTotalGames( ) ), UTIL_ToString( (float) GamePlayerSummary->GetAvgLoadingTime( ) / 1000, 2 ), UTIL_ToString( GamePlayerSummary->GetAvgLeftPercent( ) ) ), User, Whisper, m_IRC );
            else
              QueueChatCommand( m_Aura->m_Language->HasntPlayedGamesWithThisBot( StatsUser ), User, Whisper, m_IRC );

            delete GamePlayerSummary;
            GamePlayerSummary = NULL;
          }
        }
        
        //
        // !GETGAME
        // !G
        //

        else if ( ( Command == "getgame" || Command == "g" ) && !Payload.empty( ) )
        {
          uint32_t GameNumber = UTIL_ToUInt32( Payload ) - 1;

          if ( GameNumber < m_Aura->m_Games.size( ) )
            QueueChatCommand( m_Aura->m_Language->GameNumberIs( Payload, m_Aura->m_Games[GameNumber]->GetDescription( ) ), User, Whisper, m_IRC );
          else
            QueueChatCommand( m_Aura->m_Language->GameNumberDoesntExist( Payload ), User, Whisper, m_IRC );
        }
        
        //
        // !GETGAMES
        // !G
        //

        else if ( ( Command == "getgames" || Command == "g" ) && Payload.empty( ) )
        {
          if ( m_Aura->m_CurrentGame )
            QueueChatCommand( m_Aura->m_Language->GameIsInTheLobby( m_Aura->m_CurrentGame->GetDescription( ), UTIL_ToString( m_Aura->m_Games.size( ) ), UTIL_ToString( m_Aura->m_MaxGames ) ), User, Whisper, m_IRC );
          else
            QueueChatCommand( m_Aura->m_Language->ThereIsNoGameInTheLobby( UTIL_ToString( m_Aura->m_Games.size( ) ), UTIL_ToString( m_Aura->m_MaxGames ) ), User, Whisper, m_IRC );
        }
        
        //
        // !GETPLAYERS
        // !GP
        //

        else if ( ( Command == "gp" || Command == "getplayers" ) && !Payload.empty( ) )
        {
          int32_t GameNumber = UTIL_ToInt32( Payload ) - 1;

          if ( -1 < GameNumber && GameNumber < m_Aura->m_Games.size( ) )
            QueueChatCommand( "Players in game [" + m_Aura->m_Games[GameNumber]->GetGameName( ) + "] are: " + m_Aura->m_Games[GameNumber]->GetPlayers( ), User, Whisper, m_IRC );
          else if( GameNumber == -1 && m_Aura->m_CurrentGame )
            QueueChatCommand( "Players in lobby [" + m_Aura->m_CurrentGame->GetGameName( ) + "] are: " + m_Aura->m_CurrentGame->GetPlayers( ), User, Whisper, m_IRC );
          else
            QueueChatCommand( m_Aura->m_Language->GameNumberDoesntExist( Payload ), User, Whisper, m_IRC );

        }
        
        //
        // !STATSDOTA
        // !SD
        //

        else if ( Command == "statsdota" || Command == "sd" )
        {
          string StatsUser = User;

          if ( !Payload.empty( ) )
            StatsUser = Payload;

          // check for potential abuse

          if ( !StatsUser.empty( ) && StatsUser.size( ) < 16 && StatsUser[0] != '/' )
          {
            CDBDotAPlayerSummary *DotAPlayerSummary = m_Aura->m_DB->DotAPlayerSummaryCheck( StatsUser );

            if ( DotAPlayerSummary )
            {
              string Summary = m_Aura->m_Language->HasPlayedDotAGamesWithThisBot(
                      StatsUser,
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
                      UTIL_ToString( DotAPlayerSummary->GetAvgCourierKills( ), 2 )
                      );

              QueueChatCommand( Summary, User, Whisper, m_IRC );

              delete DotAPlayerSummary;
              DotAPlayerSummary = NULL;
            }
            else
              QueueChatCommand( m_Aura->m_Language->HasntPlayedDotAGamesWithThisBot( StatsUser ), User, Whisper, m_IRC );
          }
        }
        
        //
        // !STATUS
        //

        else if ( Command == "status" )
        {
          string message = "Status: ";

          for ( vector<CBNET *> ::const_iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
          {
            message += ( *i )->GetServer( ) + ( ( *i )->GetLoggedIn( ) ? " [Online], " : " [Offline], " );
          }

          message += m_Aura->m_IRC->m_Server + ( !m_Aura->m_IRC->m_WaitingToConnect ? " [Online]" : " [Offline]" );

          QueueChatCommand( message, User, Whisper, m_IRC );
        }
        
        //
        // !VERSION
        //

        else if ( Command == "version" )
        {
          if ( IsAdmin( User ) || IsRootAdmin( User ) )
            QueueChatCommand( m_Aura->m_Language->VersionAdmin( m_Aura->m_Version ), User, Whisper, m_IRC );
          else
            QueueChatCommand( m_Aura->m_Language->VersionNotAdmin( m_Aura->m_Version ), User, Whisper, m_IRC );
        }

      }
    }
  }
  else if ( Event == CBNETProtocol::EID_CHANNEL )
  {
    // keep track of current channel so we can rejoin it after hosting a game

    Print( "[BNET: " + m_ServerAlias + "] joined channel [" + Message + "]" );

    m_CurrentChannel = Message;

    if ( m_Spam && m_Aura->m_CurrentGame && m_SpamChannel != m_CurrentChannel )
    {
      m_SpamChannel = Message;
      m_Aura->m_CurrentGame->SendAllChat( "Joined channel [" + Message + "] for spamming." );
    }
  }
  else if ( Event == CBNETProtocol::EID_INFO )
  {
    Print( "[INFO: " + m_ServerAlias + "] " + Message );

    // extract the first word which we hope is the username
    // this is not necessarily true though since info messages also include channel MOTD's and such

    if ( m_Aura->m_CurrentGame )
    {
      string UserName;
      string::size_type Split = Message.find( " " );

      if ( Split != string::npos )
        UserName = Message.substr( 0, Split );
      else
        UserName = Message;

      if ( m_Aura->m_CurrentGame->GetPlayerFromName( UserName, true ) )
      {
        // handle spoof checking for current game
        // this case covers whois results which are used when hosting a public game (we send out a "/whois [player]" for each player)
        // at all times you can still /w the bot with "spoofcheck" to manually spoof check

        if ( Message.find( "Throne in game" ) != string::npos || Message.find( "currently in  game" ) != string::npos || Message.find( "currently in private game" ) != string::npos )
        {
          // check both the current game name and the last game name against the /whois response
          // this is because when the game is rehosted, players who joined recently will be in the previous game according to battle.net
          // note: if the game is rehosted more than once it is possible (but unlikely) for a false positive because only two game names are checked

          if ( Message.find( m_Aura->m_CurrentGame->GetGameName( ) ) != string::npos || Message.find( m_Aura->m_CurrentGame->GetLastGameName( ) ) != string::npos )
            m_Aura->m_CurrentGame->AddToSpoofed( m_Server, UserName, false );
          else
            m_Aura->m_CurrentGame->SendAllChat( m_Aura->m_Language->SpoofDetectedIsInAnotherGame( UserName ) );

          return;
        }

        if ( Message.find( "is away" ) != string::npos )
          m_Aura->m_CurrentGame->SendAllChat( m_Aura->m_Language->SpoofPossibleIsAway( UserName ) );
        else if ( Message.find( "is unavailable" ) != string::npos )
          m_Aura->m_CurrentGame->SendAllChat( m_Aura->m_Language->SpoofPossibleIsUnavailable( UserName ) );
        else if ( Message.find( "is refusing messages" ) != string::npos )
          m_Aura->m_CurrentGame->SendAllChat( m_Aura->m_Language->SpoofPossibleIsRefusingMessages( UserName ) );
        else if ( Message.find( "is using Warcraft III The Frozen Throne in the channel" ) != string::npos )
          m_Aura->m_CurrentGame->SendAllChat( m_Aura->m_Language->SpoofDetectedIsNotInGame( UserName ) );
        else if ( Message.find( "is using Warcraft III The Frozen Throne in channel" ) != string::npos )
          m_Aura->m_CurrentGame->SendAllChat( m_Aura->m_Language->SpoofDetectedIsNotInGame( UserName ) );
        else if ( Message.find( "is using Warcraft III The Frozen Throne in a private channel" ) != string::npos )
          m_Aura->m_CurrentGame->SendAllChat( m_Aura->m_Language->SpoofDetectedIsInPrivateChannel( UserName ) );
      }
    }
  }
  else if ( Event == CBNETProtocol::EID_ERROR )
  {
    Print( "[ERROR: " + m_ServerAlias + "] " + Message );

    if ( m_Spam && Message == "Channel is full." )
    {
      // if( m_SpamChannel == "allstars/j allstars/j allstars" )
      if ( m_SpamChannel.size( ) > 19 )
        m_SpamChannel = "Allstars";
      else
        m_SpamChannel += "/j Allstars";
    }
  }
}

void CBNET::SetSpam( )
{
  if ( !m_Spam )
  {
    m_SpamChannel = "Allstars";
    m_Spam = true;
    Print2( "[BNET: " + m_ServerAlias + "] Allstars spam is on." );
  }
  else
  {
    m_Spam = false;
    Print2( "[BNET: " + m_ServerAlias + "] Allstars spam is off." );
    QueueChatCommand( "/j " + m_FirstChannel );
  }
}

void CBNET::SendJoinChannel( const string &channel )
{
  if ( m_LoggedIn && m_InChat )
    m_Socket->PutBytes( m_Protocol->SEND_SID_JOINCHANNEL( channel ) );
}

void CBNET::SendGetFriendsList( )
{
  if ( m_LoggedIn )
    m_Socket->PutBytes( m_Protocol->SEND_SID_FRIENDSLIST( ) );
}

void CBNET::SendGetClanList( )
{
  if ( m_LoggedIn )
    m_Socket->PutBytes( m_Protocol->SEND_SID_CLANMEMBERLIST( ) );
}

void CBNET::QueueEnterChat( )
{
  if ( m_LoggedIn )
    m_OutPackets.push( m_Protocol->SEND_SID_ENTERCHAT( ) );
}

void CBNET::QueueChatCommand( const string &chatCommand )
{
  if ( chatCommand.empty( ) )
    return;

  if ( m_LoggedIn && m_OutPackets.size( ) <= 10 )
  {
    Print( "[QUEUED: " + m_ServerAlias + "] " + chatCommand );

    if ( m_PvPGN )
      m_OutPackets.push( m_Protocol->SEND_SID_CHATCOMMAND( chatCommand.substr( 0, 200 ) ) );
    else if ( chatCommand.size( ) > 255 )
      m_OutPackets.push( m_Protocol->SEND_SID_CHATCOMMAND( chatCommand.substr( 0, 255 ) ) );
    else
      m_OutPackets.push( m_Protocol->SEND_SID_CHATCOMMAND( chatCommand ) );
  }
  else
    Print( "[BNET: " + m_ServerAlias + "] too many (" + UTIL_ToString( m_OutPackets.size( ) ) + ") packets queued, discarding" );
}

void CBNET::QueueChatCommand( const string &chatCommand, const string &user, bool whisper, const string &irc )
{
  if ( chatCommand.empty( ) )
    return;

  // if the destination is IRC send it there

  if ( !irc.empty( ) )
  {
    m_Aura->m_IRC->SendMessageIRC( chatCommand, irc );
    return;
  }

  // if whisper is true send the chat command as a whisper to user, otherwise just queue the chat command

  if ( whisper )
  {
    QueueChatCommand( "/w " + user + " " + chatCommand );
  }
  else
    QueueChatCommand( chatCommand );
}

void CBNET::QueueGameCreate( unsigned char state, const string &gameName, CMap *map, uint32_t hostCounter )
{
  if ( m_LoggedIn && map )
  {
    if ( !m_CurrentChannel.empty( ) )
      m_FirstChannel = m_CurrentChannel;

    m_InChat = false;

    // a game creation message is just a game refresh message with upTime = 0

    QueueGameRefresh( state, gameName, map, hostCounter );
  }
}

void CBNET::QueueGameRefresh( unsigned char state, const string &gameName, CMap *map, uint32_t hostCounter )
{
  if ( m_LoggedIn && map )
  {
    BYTEARRAY UniqueName = m_Protocol->GetUniqueName( );

    // construct a fixed host counter which will be used to identify players from this realm
    // the fixed host counter's 4 most significant bits will contain a 4 bit ID (0-15)
    // the rest of the fixed host counter will contain the 28 least significant bits of the actual host counter
    // since we're destroying 4 bits of information here the actual host counter should not be greater than 2^28 which is a reasonable assumption
    // when a player joins a game we can obtain the ID from the received host counter
    // note: LAN broadcasts use an ID of 0, battle.net refreshes use an ID of 1-10, the rest are unused

    uint32_t MapGameType = map->GetMapGameType( );
    MapGameType |= MAPGAMETYPE_UNKNOWN0;

    if ( state == GAME_PRIVATE )
      MapGameType |= MAPGAMETYPE_PRIVATEGAME;

    // use an invalid map width/height to indicate reconnectable games

    BYTEARRAY MapWidth;
    MapWidth.push_back( 192 );
    MapWidth.push_back( 7 );
    BYTEARRAY MapHeight;
    MapHeight.push_back( 192 );
    MapHeight.push_back( 7 );

    m_OutPackets.push( m_Protocol->SEND_SID_STARTADVEX3( state, UTIL_CreateByteArray( MapGameType, false ), map->GetMapGameFlags( ), MapWidth, MapHeight, gameName, m_UserName, 0, map->GetMapPath( ), map->GetMapCRC( ), map->GetMapSHA1( ), ( ( hostCounter & 0x0FFFFFFF ) | ( m_HostCounterID << 28 ) ) ) );  
  }
}

void CBNET::QueueGameUncreate( )
{
  if ( m_LoggedIn )
    m_OutPackets.push( m_Protocol->SEND_SID_STOPADV( ) );
}

void CBNET::UnqueueGameRefreshes( )
{
  queue<BYTEARRAY> Packets;

  while ( !m_OutPackets.empty( ) )
  {
    // TODO: it's very inefficient to have to copy all these packets while searching the queue

    BYTEARRAY Packet = m_OutPackets.front( );
    m_OutPackets.pop( );

    if ( Packet.size( ) >= 2 && Packet[1] != CBNETProtocol::SID_STARTADVEX3 )
      Packets.push( Packet );
  }

  m_OutPackets = Packets;
  Print( "[BNET: " + m_ServerAlias + "] unqueued game refresh packets" );
}

bool CBNET::IsAdmin( string name )
{
  transform( name.begin( ), name.end( ), name.begin( ), (int(* )(int) )tolower );

  if ( m_Aura->m_DB->AdminCheck( m_Server, name ) )
    return true;

  return false;
}

bool CBNET::IsRootAdmin( string name )
{
  transform( name.begin( ), name.end( ), name.begin( ), (int(* )(int) )tolower );

  if ( m_Aura->m_DB->RootAdminCheck( m_Server, name ) )
    return true;

  return false;
}

CDBBan *CBNET::IsBannedName( string name )
{
  transform( name.begin( ), name.end( ), name.begin( ), (int(* )(int) )tolower );

  if ( CDBBan * Ban = m_Aura->m_DB->BanCheck( m_Server, name, string( ) ) )
    return Ban;

  return NULL;
}

void CBNET::HoldFriends( CGame *game )
{
  for ( vector<string> ::const_iterator i = m_Friends.begin( ); i != m_Friends.end( ); ++i )
    game->AddToReserved( *i  );
}

void CBNET::HoldClan( CGame *game )
{
  for ( vector<string> ::const_iterator i = m_Clans.begin( ); i != m_Clans.end( ); ++i )
    game->AddToReserved( *i );
}
