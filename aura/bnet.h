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

#ifndef BNET_H
#define BNET_H

//
// CBNET
//

class CTCPClient;
class CBNCSUtilInterface;
class CBNETProtocol;
class CIncomingFriendList;
class CIncomingClanList;
class CIncomingChatEvent;
class CDBBan;
class CIRC;
class CDCC;

class CBNET
{
public:
	CAura *m_Aura;

private:
	CTCPClient *m_Socket;					// the connection to battle.net
	CBNETProtocol *m_Protocol;				// battle.net protocol	
	CBNCSUtilInterface *m_BNCSUtil;				// the interface to the bncsutil library (used for logging into battle.net)
	queue<BYTEARRAY> m_OutPackets;				// queue of outgoing packets to be sent (to prevent getting kicked for flooding)
	vector<CIncomingFriendList *> m_Friends;		// vector of friends
	vector<CIncomingClanList *> m_Clans;			// vector of clan members
	bool m_Exiting;						// set to true and this class will be deleted next update
	bool m_Spam;						// spam game in allstars
	string m_Server;					// battle.net server to connect to
	string m_ServerIP;					// battle.net server to connect to (the IP address so we don't have to resolve it every time we connect)
	string m_ServerAlias;					// battle.net server alias (short name, e.g. "USEast")
	string m_CDKeyROC;					// ROC CD key
	string m_CDKeyTFT;					// TFT CD key
	string m_CountryAbbrev;					// country abbreviation
	string m_Country;					// country
	uint32_t m_LocaleID;					// see: http://msdn.microsoft.com/en-us/library/0h88fahh%28VS.85%29.aspx
	string m_UserName;					// battle.net username
	string m_UserPassword;					// battle.net password
	string m_FirstChannel;					// the first chat channel to join upon entering chat (note: store the last channel when entering a game)
	string m_CurrentChannel;				// the current chat channel
	string m_RootAdmin;					// the root admin
	char m_CommandTrigger;					// the character prefix to identify commands
	unsigned char m_War3Version;				// custom warcraft 3 version for PvPGN users
	BYTEARRAY m_EXEVersion;					// custom exe version for PvPGN users
	BYTEARRAY m_EXEVersionHash;				// custom exe version hash for PvPGN users
	string m_PasswordHashType;				// password hash type for PvPGN users
	uint32_t m_HostCounterID;				// the host counter ID to identify players from this realm
	uint32_t m_LastDisconnectedTime;			// GetTime when we were last disconnected from battle.net
	uint32_t m_LastConnectionAttemptTime;			// GetTime when we last attempted to connect to battle.net
	uint32_t m_LastNullTime;				// GetTime when the last null packet was sent for detecting disconnects
	uint32_t m_LastOutPacketTicks;				// GetTicks when the last packet was sent for the m_OutPackets queue
	uint32_t m_LastOutPacketSize;
	uint32_t m_LastAdminRefreshTime;			// GetTime when the admin list was last refreshed from the database
	uint32_t m_LastBanRefreshTime;				// GetTime when the ban list was last refreshed from the database
	uint32_t m_LastSpamTime;
	uint32_t m_ReconnectDelay;
	bool m_FirstConnect;					// if we haven't tried to connect to battle.net yet
	bool m_WaitingToConnect;				// if we're waiting to reconnect to battle.net after being disconnected
	bool m_LoggedIn;					// if we've logged into battle.net or not
	bool m_InChat;						// if we've entered chat or not (but we're not necessarily in a chat channel yet)
	string m_SpamChannel;
	bool m_Deactivated;	
	bool m_IRC;

public:
	CBNET( CAura *nAura, string nServer, string nServerAlias, string nCDKeyROC, string nCDKeyTFT, string nCountryAbbrev, string nCountry, uint32_t nLocaleID, string nUserName, string nUserPassword, string nFirstChannel, string nRootAdmin, char nCommandTrigger, unsigned char nWar3Version, BYTEARRAY nEXEVersion, BYTEARRAY nEXEVersionHash, string nPasswordHashType, uint32_t nMaxMessageLength, uint32_t nHostCounterID );
	~CBNET( );

	bool GetExiting( )				{ return m_Exiting; }
	string GetServer( )				{ return m_Server; }
	string GetServerAlias( )			{ return m_ServerAlias; }
	string GetCDKeyROC( )				{ return m_CDKeyROC; }
	string GetCDKeyTFT( )				{ return m_CDKeyTFT; }
	string GetUserName( )				{ return m_UserName; }
	string GetUserPassword( )			{ return m_UserPassword; }
	string GetFirstChannel( )			{ return m_FirstChannel; }
	string GetCurrentChannel( )			{ return m_CurrentChannel; }
	string GetRootAdmin( )				{ return m_RootAdmin; }
	char GetCommandTrigger( )			{ return m_CommandTrigger; }
	BYTEARRAY GetEXEVersion( )			{ return m_EXEVersion; }
	BYTEARRAY GetEXEVersionHash( )			{ return m_EXEVersionHash; }
	string GetPasswordHashType( )			{ return m_PasswordHashType; }
	uint32_t GetHostCounterID( )			{ return m_HostCounterID; }
	bool GetLoggedIn( )				{ return m_LoggedIn; }
	bool GetInChat( )				{ return m_InChat; }	
	uint32_t GetOutPacketsQueued( )			{ return m_OutPackets.size( ); }
        bool GetSpam( )                                 { return m_Spam; }
	BYTEARRAY GetUniqueName( );        

        void SetSpam( bool spam )                       { m_Spam = spam; }	
	void SetSpam( );

	// processing functions

	unsigned int SetFD( void *fd, void *send_fd, int *nfds );
	bool Update( void *fd, void *send_fd );	
	void ProcessChatEvent( CIncomingChatEvent *chatEvent );

	// functions to send packets to battle.net

	void SendJoinChannel( const string &channel );
	void SendGetFriendsList( );
	void SendGetClanList( );
	void QueueEnterChat( );
	void QueueChatCommand( const string &chatCommand );
	void QueueChatCommand( const string &chatCommand, const string &user, bool whisper, bool irc );
	void QueueGameCreate( unsigned char state, const string &gameName, const string &hostName, CMap *map, uint32_t hostCounter );
	void QueueGameRefresh( unsigned char state, const string &gameName, string hostName, CMap *map, uint32_t hostCounter );
	void QueueGameUncreate( );

	void UnqueueGameRefreshes( );

	// other functions

	void Deactivate( );
	void Activate( )			{ m_Deactivated = false; m_Spam = false; }
	bool IsAdmin( string name );
	bool IsRootAdmin( string name );
	CDBBan *IsBannedName( string name );
	void HoldFriends( CGame *game );
	void HoldClan( CGame *game );
};

#endif
