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
#include "irc.h"
#include "socket.h"
#include "util.h"
#include "bnetprotocol.h"
#include "bnet.h"

//////////////
//// CIRC ////
//////////////

CIRC::CIRC( CAura *nAura, const string &nServer, const string &nNickname, const string &nUsername, const string &nPassword, vector<string> *nChannels, uint16_t nPort, const string &nCommandTrigger, vector<string> *nLocals ) : m_Aura( nAura ), m_Locals( *nLocals ), m_Channels( *nChannels ), m_Server( nServer ), m_Nickname( nNickname ), m_NicknameCpy( nNickname ), m_Username( nUsername ), m_CommandTrigger( nCommandTrigger ), m_Password( nPassword ), m_Port( nPort ), m_Exiting( false ), m_WaitingToConnect( true ), m_OriginalNick( true ), m_LastConnectionAttemptTime( 0 ), m_LastPacketTime( GetTime( ) ), m_LastAntiIdleTime( GetTime( ) )
{
  m_Socket = new CTCPClient( );
}

CIRC::~CIRC( )
{
  delete m_Socket;

  for ( vector<CDCC *> ::iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
    delete *i;
}

unsigned int CIRC::SetFD( void *fd, void *send_fd, int *nfds )
{
  unsigned int NumFDs = 0;

  // irc socket
  
  if ( !m_Socket->HasError( ) && m_Socket->GetConnected( ) )
  {
    m_Socket->SetFD( (fd_set *) fd, (fd_set *) send_fd, nfds );
    ++NumFDs;
  }

  // dcc sockets

  for ( vector<CDCC *> ::iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
  {
    if ( !( *i )->GetError( ) && ( *i )->GetConnected( ) )
    {
      ( *i )->SetFD( (fd_set *) fd, (fd_set *) send_fd, nfds );
      ++NumFDs;
    }
  }

  return NumFDs;
}

bool CIRC::Update( void *fd, void *send_fd )
{
  uint32_t Time = GetTime( );

  for ( vector<CDCC *> ::iterator i = m_DCC.begin( ); i != m_DCC.end( ); )
  {
    if( ( *i )->Update( &fd, &send_fd ) )
    {      
      delete *i;
      i = m_DCC.erase( i );
    }
    else
      ++i;
  }

  if ( m_Socket->HasError( ) )
  {
    // the socket has an error

    Print( "[IRC: " + m_Server + "] disconnected due to socket error,  waiting 60 seconds to reconnect" );
    m_Socket->Reset( );
    m_WaitingToConnect = true;
    m_LastConnectionAttemptTime = Time;
    return m_Exiting;
  }

  if ( m_Socket->GetConnected( ) )
  {
    // the socket is connected and everything appears to be working properly

    if ( Time - m_LastPacketTime > 210 )
    {
      Print( "[IRC: " + m_Server + "] ping timeout,  reconnecting" );
      m_Socket->Reset( );
      m_WaitingToConnect = true;
      return m_Exiting;
    }

    if ( Time - m_LastAntiIdleTime > 60 )
    {
      SendIRC( "TIME" );
      m_LastAntiIdleTime = Time;
    }

    m_Socket->DoRecv( (fd_set *) fd );
    ExtractPackets( );
    m_Socket->DoSend( (fd_set *) send_fd );
    return m_Exiting;
  }

  if ( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && !m_WaitingToConnect )
  {
    // the socket was disconnected

    Print( "[IRC: " + m_Server + "] disconnected, waiting 60 seconds to reconnect" );
    m_Socket->Reset( );
    m_WaitingToConnect = true;
    m_LastConnectionAttemptTime = Time;
    return m_Exiting;
  }

  if ( m_Socket->GetConnecting( ) )
  {
    // we are currently attempting to connect to irc

    if ( m_Socket->CheckConnect( ) )
    {
      // the connection attempt completed

      if ( !m_OriginalNick )
        m_Nickname = m_NicknameCpy;

      SendIRC( "NICK " + m_Nickname );
      SendIRC( "USER " + m_Username + " " + m_Nickname + " " + m_Username + " :by h4x0rz88" );

      m_Socket->DoSend( (fd_set *) send_fd );

      Print( "[IRC: " + m_Server + "] connected" );

      m_LastPacketTime = Time;

      return m_Exiting;
    }
    else if ( Time - m_LastConnectionAttemptTime > 15 )
    {
      // the connection attempt timed out (15 seconds)

      Print( "[IRC: " + m_Server + "] connect timed out, waiting 60 seconds to reconnect" );
      m_Socket->Reset( );
      m_LastConnectionAttemptTime = Time;
      m_WaitingToConnect = true;
      return m_Exiting;
    }
  }

  if ( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && ( Time - m_LastConnectionAttemptTime > 60 ) )
  {
    // attempt to connect to irc

    Print( "[IRC: " + m_Server + "] connecting to server [" + m_Server + "] on port " + UTIL_ToString( m_Port ) );

    if ( m_ServerIP.empty( ) )
    {
      m_Socket->Connect( string( ), m_Server, m_Port );

      if ( !m_Socket->HasError( ) )
      {
        m_ServerIP = m_Socket->GetIPString( );
      }
    }
    else
    {
      // use cached server IP address since resolving takes time and is blocking

      m_Socket->Connect( string( ), m_ServerIP, m_Port );
    }

    m_WaitingToConnect = false;
    m_LastConnectionAttemptTime = Time;
  }

  return m_Exiting;
}

inline void CIRC::ExtractPackets( )
{
  string Token, PreviousToken, Recv = *( m_Socket->GetBytes( ) );
  uint32_t Time = GetTime( );
  unsigned int i;

  /* loop through whole recv buffer */

  for ( i = 0; i < Recv.size( ); ++i )
  {
    // add chars to token

    if ( Recv[i] != ' ' && Recv[i] != CR && Recv[i] != LF )
    {
      Token += Recv[i];
    }
    else if ( Recv[i] == ' ' || Recv[i] == LF )
    {
      // end of token, examine

      if ( Token == "PRIVMSG" )
      {
        // parse the PreviousToken as it holds the user info and then the Token for the message itself

        string Nickname, Hostname, Message, Command, Payload;
        bool IsCommand = true;

        unsigned int j = 1;

        // get nickname

        for (; PreviousToken[j] != '!'; ++j )
          Nickname += PreviousToken[j];

        // skip username

        for ( j += 2; PreviousToken[j] != '@'; ++j );

        // get hostname

        for ( ++j; j < PreviousToken.size( ); ++j )
          Hostname += PreviousToken[j];

        // skip channel

        for ( i += 3; Recv[i] != ':'; ++i );

        // process message

        for ( ++i; Recv[i] != CR; ++i )
        {
          Message += Recv[i];

          if ( Recv[i] == ' ' && IsCommand )
          {
            IsCommand = false;
            continue;
          }

          if ( Message.size( ) != 1 )
          {
            if ( IsCommand )
              Command += tolower( Recv[i] );
            else
              Payload += Recv[i];
          }
        }

        i += 2;

        if ( Message.empty( ) )
        {
          PreviousToken = Token;
          Token.clear( );
          continue;
        }

        if ( Message[0] != SOH )
        {
          for ( vector<CBNET *> ::iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
          {
            if ( Message[0] == ( *i )->GetCommandTrigger( ) )
            {
              CIncomingChatEvent event = CIncomingChatEvent( CBNETProtocol::EID_IRC, Nickname, Message );
              ( *i )->ProcessChatEvent( &event );
              break;
            }
          }

          if ( Message[0] == m_CommandTrigger[0] )
          {
            bool Root = false;

            if( Hostname.size( ) >= 6 )
              Root = ( Hostname.substr( 0, 6 ) == "Aurani" ) || ( Hostname.substr( 0, 8 ) == "h4x0rz88" );

            //
            // !NICK
            //

            if ( Command == "nick" && Root )
            {
              SendIRC( "NICK :" + Payload );
              m_Nickname = Payload;
              m_OriginalNick = false;
            }

            //
            // !DCCLIST
            //

            else if ( Command == "dcclist" )
            {
              string on, off;

              for ( vector<CDCC *> ::iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
              {
                if ( ( *i )->GetConnected( ) )
                  on += ( *i )->GetNickname( ) + "[" + UTIL_ToString( ( *i )->GetPort( ) ) + "] ";
                else
                  off += ( *i )->GetNickname( ) + "[" + UTIL_ToString( ( *i )->GetPort( ) ) + "] ";
              }
              
              SendMessageIRC( "ON: " + on, string( ) );
              SendMessageIRC( "OFF: " + off, string( ) );
            }

            //
            // !BNETOFF
            //

            else if ( Command == "bnetoff" )
            {
              if ( Payload.empty( ) )
              {
                for ( vector<CBNET *> ::iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
                {
                  ( *i )->Deactivate( );
                  SendMessageIRC( "[BNET: " + ( *i )->GetServerAlias( ) + "] deactivated.", string( ) );
                }
              }
              else
              {
                for ( vector<CBNET *> ::iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
                {
                  if ( ( *i )->GetServerAlias( ) == Payload )
                  {
                    ( *i )->Deactivate( );
                    SendMessageIRC( "[BNET: " + ( *i )->GetServerAlias( ) + "] deactivated.", string( ) );
                    break;
                  }
                }
              }
            }

            //
            // !BNETON
            //

            else if ( Command == "bneton" )
            {
              if ( Payload.empty( ) )
              {
                for ( vector<CBNET *> ::iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
                {
                  ( *i )->Activate( );
                  SendMessageIRC( "[BNET: " + ( *i )->GetServerAlias( ) + "] activated.", string( ) );
                }
              }
              else
              {
                for ( vector<CBNET *> ::iterator i = m_Aura->m_BNETs.begin( ); i != m_Aura->m_BNETs.end( ); ++i )
                {
                  if ( ( *i )->GetServerAlias( ) == Payload )
                  {
                    ( *i )->Activate( );
                    SendMessageIRC( "[BNET: " + ( *i )->GetServerAlias( ) + "] activated.", string( ) );
                    break;
                  }
                }
              }
            }
          }
        }
        else if ( Payload.size( ) > 12 && Payload.substr( 0, 4 ) == "CHAT" )
        {
          // CHAT chat 3162588924 1025

          string strIP, strPort;
          bool IsPort = false;

          for ( unsigned int j = 10; j < ( Payload.size( ) - 1 ); ++j )
          {
            if ( !IsPort && Payload[j] == ' ' )
            {
              IsPort = true;
              continue;
            }

            if ( !IsPort )
              strIP += Payload[j];
            else
              strPort += Payload[j];
          }

          unsigned int Port = UTIL_ToUInt16( strPort );

          if ( Port < 1024 || 1026 < Port )
            Port = 1024;

          bool Local = false;

          for ( vector<string> ::iterator i = m_Locals.begin( ); i != m_Locals.end( ); ++i )
          {
            if ( Nickname == ( *i ) )
            {
              Local = true;
              strIP = "127.0.0.1";
              break;
            }
          }

          if ( !Local )
          {
            unsigned long IP = UTIL_ToUInt32( strIP ), divider = 16777216UL;
            strIP = "";

            for ( int i = 0; i <= 3; ++i )
            {
              stringstream SS;

              SS << (unsigned long) IP / divider;
              IP %= divider;
              divider /= 256;
              strIP += SS.str( );

              if ( i != 3 )
                strIP += '.';
            }
          }

          for ( vector<CDCC *> ::iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
          {
            if ( ( *i )->GetNickname( ) == Nickname )
            {              
              delete *i;
              m_DCC.erase( i );
              break;
            }
          }

          m_DCC.push_back( new CDCC( this, strIP, Port, Nickname ) );
        }

        // remember last packet time

        m_LastPacketTime = Time;
      }
      else if ( Token == "391" )
      {
        for (; Recv[i] != LF; ++i );

        // remember last packet time

        m_LastPacketTime = Time;
      }
      else if ( Token == "PING" )
      {
        string Packet;

        // PING :blabla

        for ( ++i; Recv[i] != ':'; ++i );

        for ( ++i; Recv[i] != CR; ++i )
          Packet += Recv[i];

        SendIRC( "PONG :" + Packet );

        ++i;

        // remember last packet time

        m_LastPacketTime = Time;
      }
      else if ( Token == "NOTICE" )
      {
        for (; Recv[i] != LF; ++i );

        // remember last packet time

        m_LastPacketTime = Time;
      }
      else if ( Token == "221" )
      {
        // Q auth if the server is QuakeNet

        if ( m_Server.find( "quakenet.org" ) != string::npos && !m_Password.empty( ) )
        {
          SendMessageIRC( "AUTH " + m_Username + " " + m_Password, "Q@CServe.quakenet.org" );
          SendIRC( "MODE " + m_Nickname + " +x" );
        }

        // join channels

        for ( vector<string> ::iterator j = m_Channels.begin( ); j != m_Channels.end( ); ++j )
        {
          SendIRC( "JOIN " + ( *j ) );
        }

        for (; Recv[i] != LF; ++i );

        // remember last packet time

        m_LastPacketTime = Time;
      }
      else if ( Token == "433" )
      {
        // nick taken, append _

        m_OriginalNick = false;
        m_Nickname += '_';

        SendIRC( "NICK " + m_Nickname );

        for (; Recv[i] != LF; ++i );

        // remember last packet time

        m_LastPacketTime = Time;
      }
      else if ( Token == "353" )
      {
        for (; Recv[i] != LF; ++i );

        // remember last packet time

        m_LastPacketTime = Time;
      }
      else if ( Token == "KICK" )
      {
        string Channel, Victim;
        bool Space = false;

        // get channel

        for ( ++i; Recv[i] != ' '; ++i )
          Channel += Recv[i];

        // get the victim

        for ( ++i; i < Recv.size( ); ++i )
        {
          if ( Recv[i] == ' ' )
            Space = true;
          else if ( Recv[i] == CR )
            break;
          else if ( Space && Recv[i] != ':' )
            Victim += Recv[i];
        }

        // we're the victim here! rejoin

        if ( Victim == m_Nickname )
        {
          SendIRC( "JOIN " + Channel );
        }

        // move position after the \n

        i += 2;

        // remember last packet time

        m_LastPacketTime = Time;
      }

      // empty the token

      PreviousToken = Token;
      Token.clear( );
    }
  }

  m_Socket->ClearRecvBuffer( );
}

void CIRC::SendIRC( const string &message )
{
  if ( m_Socket->GetConnected( ) )
    m_Socket->PutBytes( message + LF );
}

void CIRC::SendDCC( const string &message )
{
  for ( vector<CDCC *> ::iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
    if ( ( *i )->GetConnected( ) )
      ( *i )->PutBytes( message + LF );
}

void CIRC::SendMessageIRC( const string &message, const string &target )
{
  if ( m_Socket->GetConnected( ) )
  {
    if ( target.empty( ) )
      for ( vector<string> ::iterator i = m_Channels.begin( ); i != m_Channels.end( ); ++i )
        m_Socket->PutBytes( "PRIVMSG " + ( *i ) + " :" + ( message.size( ) > 341 ? message.substr( 0, 341 ) : message ) + LF );
    else
      m_Socket->PutBytes( "PRIVMSG " + target + " :" + ( message.size( ) > 341 ? message.substr( 0, 341 ) : message ) + LF );
  }
}

//////////////
//// CDCC ////
//////////////

// Used for establishing a DCC Chat connection to other clients and sending large amounts of data

CDCC::CDCC( CIRC *nIRC, string nIP, uint16_t nPort, const string &nNickname ) : m_Nickname( nNickname ), m_IRC( nIRC ), m_IP( nIP ), m_Port( nPort ), m_DeleteMe( false )
{
  m_Socket = new CTCPClient( );
  m_Socket->SetNoDelay( );
  m_Socket->Connect( string( ), nIP, nPort );

  Print( "[DCC: " + m_IP + ":" + UTIL_ToString( m_Port ) + "] trying to connect to " + m_Nickname );
}

CDCC::~CDCC( )
{
  delete m_Socket;
}

void CDCC::SetFD( void *fd, void *send_fd, int *nfds )
{
    m_Socket->SetFD( (fd_set *) fd, (fd_set *) send_fd, nfds );
}

bool CDCC::Update( void* fd, void *send_fd )
{
  if ( m_Socket->HasError( ) )
  {
    m_Socket->Reset( );
    return true;
  }
  else if ( m_Socket->GetConnected( ) )
  {
    m_Socket->FlushRecv( (fd_set *) fd );
    m_Socket->DoSend( (fd_set *) send_fd );
  }
  else if ( m_Socket->GetConnecting( ) && m_Socket->CheckConnect( ) )
  {
    m_Socket->FlushRecv( (fd_set *) fd );
    Print( "[DCC: " + m_IP + ":" + UTIL_ToString( m_Port ) + "] connected to " + m_Nickname + "!" );
    m_Socket->DoSend( (fd_set *) send_fd );
  }

  return false;
}