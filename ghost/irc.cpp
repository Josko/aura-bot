#include "ghost.h"
#include "irc.h"
#include "socket.h"
#include "util.h"
#include "bnetprotocol.h"
#include "bnet.h"

//////////////
//// CIRC ////
//////////////

CIRC :: CIRC( CGHost *nGHost, string nServer, string nNickname, string nUsername, string nPassword, vector<string> nChannels, uint16_t nPort, string nCommandTrigger, vector<string> nLocals ) : m_GHost( nGHost ), m_Locals( nLocals ), m_Channels( nChannels ), m_Server( nServer ), m_Nickname( nNickname ), m_NicknameCpy( nNickname ), m_Username( nUsername ), m_CommandTrigger( nCommandTrigger ), m_Password( nPassword ), m_Port( nPort ), m_Exiting( false ), m_WaitingToConnect( true ), m_OriginalNick( true ), m_LastConnectionAttemptTime( 0 )
{
	m_Socket = new CTCPClient( );

	if( m_Server.empty( ) || m_Username.empty( ) || m_Nickname.empty( ) )
	{
		CONSOLE_Print2( "[IRC] Insufficient input for IRC. Exiting..." );
		m_Exiting = true;
	}
}

CIRC :: ~CIRC( )
{
	delete m_Socket;

	for( vector<CDCC *> :: iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
		delete *i;
}

unsigned int CIRC :: SetFD( void *fd, void *send_fd, int *nfds )
{
	unsigned int NumFDs = 0;

	if( !m_Socket->HasError( ) && m_Socket->GetConnected( ) )
	{
		m_Socket->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
		++NumFDs;
	}

	return NumFDs;
}

bool CIRC :: Update( void *fd, void *send_fd )
{
	if( m_Socket->HasError( ) )
	{
		// the socket has an error

		CONSOLE_Print( "[IRC: " + m_Server + "] disconnected due to socket error,  waiting 30 seconds to reconnect" );
		m_Socket->Reset( );
		m_WaitingToConnect = true;
		m_LastConnectionAttemptTime = GetTime( );
		return m_Exiting;
	}

	if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && !m_WaitingToConnect )
	{
		// the socket was disconnected

		CONSOLE_Print( "[IRC: " + m_Server + "] disconnected, waiting 30 seconds to reconnect" );
		m_Socket->Reset( );
		m_WaitingToConnect = true;
		m_LastConnectionAttemptTime = GetTime( );
		return m_Exiting;
	}

	if( m_Socket->GetConnected( ) )
	{
		// the socket is connected and everything appears to be working properly

		m_Socket->DoRecv( (fd_set *)fd );
		ExtractPackets( );
		m_Socket->DoSend( (fd_set *)send_fd );
		return m_Exiting;
	}

	if( m_Socket->GetConnecting( ) )
	{
		// we are currently attempting to connect to irc

		if( m_Socket->CheckConnect( ) )
		{
			// the connection attempt completed

			CONSOLE_Print( "[IRC: " + m_Server + "] connected" );

			if( !m_OriginalNick )
				m_Nickname = m_NicknameCpy;

			SendIRC( "NICK " + m_Nickname );
			SendIRC( "USER " + m_Username + " " +  m_Nickname + " " + m_Username + " :by h4x0rz88" );

			m_Socket->DoSend( (fd_set *)send_fd );
			return m_Exiting;
		}
		else if( GetTime( ) - m_LastConnectionAttemptTime >= 15 )
		{
			// the connection attempt timed out (15 seconds)

			CONSOLE_Print( "[IRC: " + m_Server + "] connect timed out, waiting 30 seconds to reconnect" );
			m_Socket->Reset( );
			m_LastConnectionAttemptTime = GetTime( );
			m_WaitingToConnect = true;
			return m_Exiting;
		}
	}

	if( !m_Socket->GetConnecting( ) && !m_Socket->GetConnected( ) && ( GetTime( ) - m_LastConnectionAttemptTime >= 30 ) )
	{
		// attempt to connect to irc

		CONSOLE_Print( "[IRC: " + m_Server + "] connecting to server [" + m_Server + "] on port " + UTIL_ToString( m_Port ) );

		if( m_ServerIP.empty( ) )
		{
			m_Socket->Connect( string( ), m_Server, m_Port );

			if( !m_Socket->HasError( ) )
			{
				m_ServerIP = m_Socket->GetIPString( );
				CONSOLE_Print2( "[IRC: " + m_Server + "] resolved and cached server IP address " + m_ServerIP );
			}
		}
		else
		{
			// use cached server IP address since resolving takes time and is blocking

			CONSOLE_Print2( "[IRC: " + m_Server + "] using cached server IP address " + m_ServerIP );
			m_Socket->Connect( string( ), m_ServerIP, m_Port );
		}

		m_WaitingToConnect = false;
		m_LastConnectionAttemptTime = GetTime( );
	}

	return m_Exiting;
}

inline void CIRC :: ExtractPackets( )
{
	string *RecvBuffer = m_Socket->GetBytes( );
	vector<string> Packets = UTIL_Tokenize( *RecvBuffer, '\n' );
	*RecvBuffer = RecvBuffer->erase( );

	for( vector<string> :: iterator i = Packets.begin( ); i != Packets.end( ); ++i )
	{
		vector<string> Parts = UTIL_Tokenize( (*i).substr( 0, (*i).size( ) - 1) , ' ' );

		if( Parts.size( ) >= 3 && Parts[1] == "PRIVMSG" )
		{
			// parse info

			string Message	= (*i).substr( Parts[0].size( ) + Parts[1].size( ) + Parts[2].size( ) + 4, (*i).size( ) - Parts[0].size( ) - Parts[1].size( ) - Parts[2].size( ) - 5  );
                        
			if( Message.size( ) < 2 )
				return;

			string Nickname = Parts[0].substr( 1, Parts[0].find( "!" )-1 );
			string Hostname = Parts[0].substr( Parts[0].find("@")+1 );
			string Target	= Parts[2];

			// ignore ACTIONs for now

			if( Message[0] != '\1' )
			{
				CONSOLE_Print2( "[" + Target + "] " + Nickname + ": " + Message );

				if( Message[0] == m_CommandTrigger[0] )
				{
					// Commands

					string Command, Payload;
					string :: size_type PayloadStart = Message.find( " " );
					bool Root = Hostname.substr( 0, 6 ) == "Aurani" || Hostname.substr( 0, 8 ) == "h4x0rz88";

					if( PayloadStart != string :: npos )
					{
						Command = Message.substr( 1, PayloadStart - 1 );
						Payload = Message.substr( PayloadStart + 1 );
					}
					else
						Command = Message.substr( 1 );

					transform( Command.begin( ), Command.end( ), Command.begin( ), (int(*)(int))tolower );

					if( Command == "nick" && Root )
					{
						SendIRC( "NICK :" + Payload );
						m_Nickname = Payload;
						m_OriginalNick= false;
						return;
					}
					else if( Command == "dcclist" )
					{
						string on;
						string off;

						for( vector<CDCC *> :: iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
						{
							if( (*i)->m_Socket->GetConnected( ) )
								on += (*i)->m_Nickname + " ";
							else
								off += (*i)->m_Nickname + " ";
						}

						SendMessageIRC( "ON: " + on, Target );
						SendMessageIRC( "OFF: " + off, Target );
						return;
					}
					else if( Command == "send" && !Payload.empty( ) && Root )
					{
						SendDCC( Payload );
						return;
					}
                                        else if( Command == "quit" && Root )
                                        {
                                                SendIRC( "QUIT: Quit" );
                                        }
					else if( Command == "irc" && Root )
					{
						m_Socket->Reset( );
						m_WaitingToConnect = true;
						m_LastConnectionAttemptTime = GetTime( );
						return;
					}
					else if( Command == "quit" && Root )
	                                {
	                                        SendIRC( "QUIT: Quit" );
	                                }
					else if( Command == "bnetoff" )
					{
						if( Payload.empty( ) )
						{
							for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
							{
								(*i)->Deactivate( );
								SendMessageIRC( "[BNET: " + (*i)->GetServerAlias( ) + "] deactivated.", Target );
							}
						}
						else
						{
							for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
							{
								if( (*i)->GetServerAlias( ) == Payload )
								{
									(*i)->Deactivate( );
									SendMessageIRC( "[BNET: " + (*i)->GetServerAlias( ) + "] deactivated.", Target );
									return;
								}
							}
						}

						return;
					}
					else if( Command == "bneton" )
					{
						if( Payload.empty( ) )
						{
							for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
							{
								(*i)->Activate( );
								SendMessageIRC( "[BNET: " + (*i)->GetServerAlias( ) + "] activated.", Target );
							}
						}
						else
						{
							for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
							{
								if( (*i)->GetServerAlias( ) == Payload )
								{
									(*i)->Activate( );
									SendMessageIRC( "[BNET: " + (*i)->GetServerAlias( ) + "] activated.", Target );
									return;
								}
							}
						}						
						
						return;
					}
				}

				for( vector<CBNET *> :: iterator i = m_GHost->m_BNETs.begin( ); i != m_GHost->m_BNETs.end( ); ++i )
				{
					if( Message[0] == (*i)->GetCommandTrigger( ) )
					{
						CIncomingChatEvent event = CIncomingChatEvent( CBNETProtocol :: EID_IRC, Nickname, Message );
						(*i)->ProcessChatEvent( &event );
						break;
					}
				}
			}
			else
			{
				// look for DCC CHAT requests

				transform( Message.begin( ), Message.end( ), Message.begin( ), (int(*)(int))tolower );				

				if( Message.size( ) > 25 && Message.substr( 1, 13 ) == "dcc chat chat" )
				{
					string strIP, strPort = Parts[7].substr( 0, Parts[7].find( '\1' ) );
                                        unsigned int Port = UTIL_ToUInt32( strPort );

					for( vector<string> :: iterator i = m_Locals.begin( ); i != m_Locals.end( ); ++i )
					{
						if( Nickname == (*i) )
						{
							strIP = "127.0.0.1";
							break;
						}
					}

					if( strIP.empty( ) )
					{
						unsigned long IP = UTIL_ToUInt32( Parts[6] ), divider = 16777216UL;						
						
						for( int i = 0; i <= 3; ++i )
						{
							stringstream ss;
							ss << (unsigned long) IP / divider;
							
							IP %= divider;
							divider /= 256;
							strIP += ss.str( );
							
							if( i != 3 )
								strIP += '.';
						}						
					}					
					
					for( vector<CDCC *> :: iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
					{
						if( (*i)->m_Nickname == Nickname )
						{
							(*i)->Connect( strIP, Port );
							return;
						}
					}

					m_DCC.push_back( new CDCC( this, strIP, Port, Nickname ) );
				}
			}
		}
		else if( Parts.size( ) >= 2 && Parts[0] == "PING" )
		{
			SendIRC( "PONG " + Parts[1] );
		}
		else if( Parts.size( ) >= 2 && Parts[0] == "NOTICE" )
		{
			CONSOLE_Print( "[IRC: " + m_Server + "] " + (*i) );
		}
		else if( Parts.size( ) >= 2 && Parts[1] == "221" )
		{
			// Q auth if the server is QuakeNet

			if( m_Server.find( "quakenet.org" ) != string :: npos && !m_Password.empty( ) )
			{
				SendMessageIRC( "AUTH " + m_Username + " " + m_Password, "Q@CServe.quakenet.org" );
				SendIRC( "MODE " + m_Nickname + " +x" );
			}

			// join channels

			for( vector<string> :: iterator i = m_Channels.begin( ); i != m_Channels.end( ); ++i )
			{
				SendIRC( "JOIN " + (*i) );
			}
		}
		else if( Parts.size( ) >= 2 && Parts[1] == "353" )
		{
			// we don't actually check if we joined all of the channels

			CONSOLE_Print( "[IRC: " + m_Server + "] joined at least one channel successfully" );
		}
		else if( Parts.size( ) >= 4 && Parts[1] == "KICK" && Parts[3] == m_Nickname )
		{
			// rejoin the channel if we get kicked

			SendIRC( "JOIN " + Parts[2] );
		}
		else if( Parts.size( ) >= 2 && Parts[1] == "433" )
		{
			// nick taken, append _

			m_OriginalNick = false;
			m_Nickname += '_';
			
			SendIRC( "NICK " + m_Nickname );
		}
	}
}

void CIRC :: SendIRC( const string &message )
{
	if( m_Socket->GetConnected( ) )
		m_Socket->PutBytes( message + LF );
}

void CIRC :: SendDCC( const string &message )
{
	for( vector<CDCC *> :: iterator i = m_DCC.begin( ); i != m_DCC.end( ); ++i )
		if( (*i)->m_Socket->GetConnected( ) )
			(*i)->m_Socket->PutBytes( message + '\n' );
}

void CIRC :: SendMessageIRC( const string &message, const string &target )
{
	if( m_Socket->GetConnected( ) )
	{
		if( !target.empty( ) )
			m_Socket->PutBytes( "PRIVMSG " + target + " :" + ( message.size( ) > 320 ? message.substr( 0, 320 ) : message ) + LF );
		else
			for( vector<string> :: iterator i = m_Channels.begin( ); i != m_Channels.end( ); ++i )
				m_Socket->PutBytes( "PRIVMSG " + (*i) + " :" + ( message.size( ) > 320 ? message.substr( 0, 320 ) : message ) + LF );
	}
}

//////////////
//// CDCC ////
//////////////

// Used for establishing a DCC Chat connection to other clients and sending large amounts of data

CDCC :: CDCC( CIRC *nIRC, string nIP, uint16_t nPort, string nNickname ) : m_Nickname( nNickname ), m_IRC( nIRC ), m_IP( nIP ), m_Port( nPort ) 
{
	m_Socket = new CTCPClient( );
	CONSOLE_Print( "[DCC: " + m_IP + ":" + UTIL_ToString( m_Port ) + "] trying to connect to " + m_Nickname );
	m_Socket->Connect( string( ), nIP, nPort );
}

CDCC :: ~CDCC( )
{
	delete m_Socket;
}

unsigned int CDCC :: SetFD( void *fd, void *send_fd, int *nfds )
{
	unsigned int NumFDs = 0;

	if( !m_Socket->HasError( ) && m_Socket->GetConnected( ) )
	{
		m_Socket->SetFD( (fd_set *)fd, (fd_set *)send_fd, nfds );
		++NumFDs;
	}

	return NumFDs;
}

void CDCC :: Update( void *fd, void *send_fd )
{
	if( m_Socket->GetConnecting( ) && m_Socket->CheckConnect( ) )
	{
		m_Socket->PutBytes( "Welcome! :)\n" );
		m_Socket->DoRecv( (fd_set *)fd );		
		m_Socket->DoSend( (fd_set *)send_fd );		
	}	
	else if( m_Socket->HasError( ) )
	{
		m_Socket->Reset( );
	}
	else if( m_Socket->GetConnected( ) )
	{
		m_Socket->DoRecv( (fd_set *)fd );
		m_Socket->DoSend( (fd_set *)send_fd );
	}
}

void CDCC :: Connect( string IP, uint16_t Port )
{
	m_Socket->Reset( );
	m_IP = IP;
	m_Port = Port;
	
	CONSOLE_Print( "[DCC: " + m_IP + ":" + UTIL_ToString( m_Port ) + "] trying to connect to " + m_Nickname );	
			
	m_Socket->Connect( string( ), m_IP, m_Port );
}
