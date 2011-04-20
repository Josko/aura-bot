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

#ifndef GAMEPLAYER_H
#define GAMEPLAYER_H

class CTCPSocket;
class CGameProtocol;
class CGame;
class CIncomingJoinPlayer;

//
// CPotentialPlayer
//

class CPotentialPlayer
{
public:
  CGameProtocol *m_Protocol;
  CGame *m_Game;

protected:
  // note: we permit m_Socket to be NULL in this class to allow for the virtual host player which doesn't really exist
  // it also allows us to convert CPotentialPlayers to CGamePlayers without the CPotentialPlayer's destructor closing the socket

  CTCPSocket *m_Socket;
  bool m_DeleteMe;
  CIncomingJoinPlayer *m_IncomingJoinPlayer;

public:
  CPotentialPlayer( CGameProtocol *nProtocol, CGame *nGame, CTCPSocket *nSocket );
  ~CPotentialPlayer( );

  CTCPSocket *GetSocket( ) const								{ return m_Socket; }
  BYTEARRAY GetExternalIP( ) const;
  string GetExternalIPString( ) const;
  bool GetDeleteMe( ) const											{ return m_DeleteMe; }
  CIncomingJoinPlayer *GetJoinPlayer( ) const		{ return m_IncomingJoinPlayer; }
  
  void SetSocket( CTCPSocket *nSocket )					{ m_Socket = nSocket; }
  void SetDeleteMe( bool nDeleteMe )						{ m_DeleteMe = nDeleteMe; }

  // processing functions

  bool Update( void *fd );

  // other functions

  void Send( const BYTEARRAY &data );
};

//
// CGamePlayer
//

class CGamePlayer
{
public:
  CGameProtocol *m_Protocol;
  CGame *m_Game;

protected:
  // note: we permit m_Socket to be NULL in this class to allow for the virtual host player which doesn't really exist

  CTCPSocket *m_Socket;
  bool m_DeleteMe;

private:

  unsigned char m_PID;                      // the player's PID
  string m_Name;                            // the player's name
  BYTEARRAY m_InternalIP;                   // the player's internal IP address as reported by the player when connecting
  vector<uint32_t> m_Pings;                 // store the last few (20) pings received so we can take an average
  queue<uint32_t> m_CheckSums;              // the last few checksums the player has sent (for detecting desyncs)
  string m_LeftReason;                      // the reason the player left the game
  string m_SpoofedRealm;                    // the realm the player last spoof checked on
  string m_JoinedRealm;                     // the realm the player joined on (probable, can be spoofed)
  uint32_t m_LeftCode;                      // the code to be sent in W3GS_PLAYERLEAVE_OTHERS for why this player left the game
  uint32_t m_SyncCounter;                   // the number of keepalive packets received from this player
  uint32_t m_JoinTime;                      // GetTime when the player joined the game (used to delay sending the /whois a few seconds to allow for some lag)
  uint32_t m_LastMapPartSent;               // the last mappart sent to the player (for sending more than one part at a time)
  uint32_t m_LastMapPartAcked;              // the last mappart acknowledged by the player
  uint32_t m_StartedDownloadingTicks;       // GetTicks when the player started downloading the map
  uint32_t m_FinishedDownloadingTime;       // GetTime when the player finished downloading the map
  uint32_t m_FinishedLoadingTicks;          // GetTicks when the player finished loading the game
  uint32_t m_StartedLaggingTicks;           // GetTicks when the player started lagging
  bool m_Spoofed;                           // if the player has spoof checked or not
  bool m_Reserved;                          // if the player is reserved (VIP) or not
  bool m_WhoisShouldBeSent;                 // if a battle.net /whois should be sent for this player or not
  bool m_WhoisSent;                         // if we've sent a battle.net /whois for this player yet (for spoof checking)
  bool m_DownloadAllowed;                   // if we're allowed to download the map or not (used with permission based map downloads)
  bool m_DownloadStarted;                   // if we've started downloading the map or not
  bool m_DownloadFinished;                  // if we've finished downloading the map or not
  bool m_FinishedLoading;                   // if the player has finished loading or not
  bool m_Lagging;                           // if the player is lagging or not (on the lag screen)
  bool m_DropVote;                          // if the player voted to drop the laggers or not (on the lag screen)
  bool m_KickVote;                          // if the player voted to kick a player or not
  bool m_Muted;                             // if the player is muted or not
  bool m_LeftMessageSent;                   // if the playerleave message has been sent or not

public:
  CGamePlayer( CPotentialPlayer *potential, unsigned char nPID, const string &nJoinedRealm, const string &nName, const BYTEARRAY &nInternalIP, bool nReserved );
  ~CGamePlayer( );

  CTCPSocket *GetSocket( ) const								{ return m_Socket; }
  BYTEARRAY GetExternalIP( ) const;
	string GetExternalIPString( ) const;
  bool GetDeleteMe( ) const											{ return m_DeleteMe; }
  unsigned char GetPID( ) const									{ return m_PID; }
  string GetName( ) const												{ return m_Name; }
  BYTEARRAY GetInternalIP( ) const							{ return m_InternalIP; }
  unsigned int GetNumPings( ) const							{ return m_Pings.size( ); }
  unsigned int GetNumCheckSums( ) const					{ return m_CheckSums.size( ); }
  queue<uint32_t> *GetCheckSums( )							{ return &m_CheckSums; }
  string GetLeftReason( ) const									{ return m_LeftReason; }
  string GetSpoofedRealm( ) const								{ return m_SpoofedRealm; }
  string GetJoinedRealm( ) const								{ return m_JoinedRealm; }
  uint32_t GetLeftCode( ) const									{ return m_LeftCode; }
  uint32_t GetSyncCounter( ) const							{ return m_SyncCounter; }
  uint32_t GetJoinTime( ) const									{ return m_JoinTime; }
  uint32_t GetLastMapPartSent( ) const					{ return m_LastMapPartSent; }
  uint32_t GetLastMapPartAcked( ) const					{ return m_LastMapPartAcked; }
  uint32_t GetStartedDownloadingTicks( ) const	{ return m_StartedDownloadingTicks; }
  uint32_t GetFinishedDownloadingTime( ) const	{ return m_FinishedDownloadingTime; }
  uint32_t GetFinishedLoadingTicks( ) const			{ return m_FinishedLoadingTicks; }
  uint32_t GetStartedLaggingTicks( ) const			{ return m_StartedLaggingTicks; }
  bool GetSpoofed( ) const											{ return m_Spoofed; }
  bool GetReserved( ) const											{ return m_Reserved; }
  bool GetWhoisShouldBeSent( ) const						{ return m_WhoisShouldBeSent; }
  bool GetWhoisSent( ) const										{ return m_WhoisSent; }
  bool GetDownloadAllowed( ) const							{ return m_DownloadAllowed; }
  bool GetDownloadStarted( ) const							{ return m_DownloadStarted; }
  bool GetDownloadFinished( ) const							{ return m_DownloadFinished; }
  bool GetFinishedLoading( ) const							{ return m_FinishedLoading; }
  bool GetLagging( ) const											{ return m_Lagging; }
  bool GetDropVote( ) const											{ return m_DropVote; }
  bool GetKickVote( ) const											{ return m_KickVote; }
  bool GetMuted( ) const												{ return m_Muted; }
  bool GetLeftMessageSent( ) const						  { return m_LeftMessageSent; }

  void SetLeftReason( const string &nLeftReason )
  {
    m_LeftReason = nLeftReason;
  }

  void SetSpoofedRealm( const string &nSpoofedRealm )
  {
    m_SpoofedRealm = nSpoofedRealm;
  }

  void SetLeftCode( uint32_t nLeftCode )
  {
    m_LeftCode = nLeftCode;
  }

  void SetSyncCounter( uint32_t nSyncCounter )
  {
    m_SyncCounter = nSyncCounter;
  }

  void SetLastMapPartSent( uint32_t nLastMapPartSent )
  {
    m_LastMapPartSent = nLastMapPartSent;
  }

  void SetLastMapPartAcked( uint32_t nLastMapPartAcked )
  {
    m_LastMapPartAcked = nLastMapPartAcked;
  }

  void SetStartedDownloadingTicks( uint32_t nStartedDownloadingTicks )
  {
    m_StartedDownloadingTicks = nStartedDownloadingTicks;
  }

  void SetFinishedDownloadingTime( uint32_t nFinishedDownloadingTime )
  {
    m_FinishedDownloadingTime = nFinishedDownloadingTime;
  }

  void SetStartedLaggingTicks( uint32_t nStartedLaggingTicks )
  {
    m_StartedLaggingTicks = nStartedLaggingTicks;
  }
  
  void SetSpoofed( bool nSpoofed )
  {
    m_Spoofed = nSpoofed;
  }

  void SetReserved( bool nReserved )
  {
    m_Reserved = nReserved;
  }

  void SetWhoisShouldBeSent( bool nWhoisShouldBeSent )
  {
    m_WhoisShouldBeSent = nWhoisShouldBeSent;
  }

  void SetDownloadAllowed( bool nDownloadAllowed )
  {
    m_DownloadAllowed = nDownloadAllowed;
  }

  void SetDownloadStarted( bool nDownloadStarted )
  {
    m_DownloadStarted = nDownloadStarted;
  }

  void SetDownloadFinished( bool nDownloadFinished )
  {
    m_DownloadFinished = nDownloadFinished;
  }

  void SetLagging( bool nLagging )
  {
    m_Lagging = nLagging;
  }

  void SetDropVote( bool nDropVote )
  {
    m_DropVote = nDropVote;
  }

  void SetKickVote( bool nKickVote )
  {
    m_KickVote = nKickVote;
  }

  void SetMuted( bool nMuted )
  {
    m_Muted = nMuted;
  }

  void SetLeftMessageSent( bool nLeftMessageSent )
  {
    m_LeftMessageSent = nLeftMessageSent;
  }

  // processing functions

  bool Update( void *fd );

  void SetSocket( CTCPSocket *nSocket )
  {
    m_Socket = nSocket;
  }

  void SetDeleteMe( bool nDeleteMe )
  {
    m_DeleteMe = nDeleteMe;
  }

  // other functions

  void Send( const BYTEARRAY &data );
  uint32_t GetPing( bool LCPing );
};

#endif
