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

#ifndef LANGUAGE_H
#define LANGUAGE_H

//
// CLanguage
//

class CLanguage
{
private:
	CConfig *m_CFG;

public:
	CLanguage( const string &nCFGFile );
	~CLanguage( );

	string UnableToCreateGameTryAnotherName( const string &server, const string &gamename );
	string UserIsAlreadyAnAdmin( const string &server, const string &user );
	string AddedUserToAdminDatabase( const string &server, const string &user );
	string ErrorAddingUserToAdminDatabase( const string &server, const string &user );
	string YouDontHaveAccessToThatCommand( );
	string UserIsAlreadyBanned( const string &server, const string &victim );
	string BannedUser( const string & server, const string & victim );
	string ErrorBanningUser( const string & server, const string & victim );
	string UserIsAnAdmin( const string & server, const string & user );
	string UserIsNotAnAdmin( const string & server, const string & user );
	string UserWasBannedOnByBecause( const string & server, const string & victim, const string & date, const string & admin, const string & reason );
	string UserIsNotBanned( const string & server, const string & victim );
	string ThereAreNoAdmins( const string & server );
	string ThereIsAdmin( const string & server );
	string ThereAreAdmins( const string & server, const string & count );
	string ThereAreNoBannedUsers( const string & server );
	string ThereIsBannedUser( const string & server );
	string ThereAreBannedUsers( const string & server, const string & count );
	string YouCantDeleteTheRootAdmin( );
	string DeletedUserFromAdminDatabase( const string & server, const string & user );
	string ErrorDeletingUserFromAdminDatabase( const string & server, const string & user );
	string UnbannedUser( const string & victim );
	string ErrorUnbanningUser( const string & victim );
	string GameNumberIs( const string & number, const string & description );
	string GameNumberDoesntExist( const string & number );
	string GameIsInTheLobby( const string & description, const string & current, const string & max );
	string ThereIsNoGameInTheLobby( const string & current, const string & max );
	string UnableToLoadConfigFilesOutside( );
	string LoadingConfigFile( const string & file );
	string UnableToLoadConfigFileDoesntExist( const string & file );
	string CreatingPrivateGame( const string & gamename, const string & user );
	string CreatingPublicGame( const string & gamename, const string & user );
	string UnableToUnhostGameCountdownStarted( const string & description );
	string UnhostingGame( const string & description );
	string UnableToUnhostGameNoGameInLobby( );
	string VersionAdmin( const string & version );
	string VersionNotAdmin( const string & version );
	string UnableToCreateGameAnotherGameInLobby( const string & gamename, const string & description );
	string UnableToCreateGameMaxGamesReached( const string & gamename, const string & max );
	string GameIsOver( const string & description );
	string SpoofCheckByReplying( );
	string GameRefreshed( );
	string SpoofPossibleIsAway( const string & user );
	string SpoofPossibleIsUnavailable( const string & user );
	string SpoofPossibleIsRefusingMessages( const string & user );
	string SpoofDetectedIsNotInGame( const string & user );
	string SpoofDetectedIsInPrivateChannel( const string & user );
	string SpoofDetectedIsInAnotherGame( const string & user );
	string CountDownAborted( );
	string TryingToJoinTheGameButBanned( const string & victim );
	string UnableToBanNoMatchesFound( const string & victim );
	string PlayerWasBannedByPlayer( const string & server, const string & victim, const string & user );
	string UnableToBanFoundMoreThanOneMatch( const string & victim );
	string AddedPlayerToTheHoldList( const string & user );
	string UnableToKickNoMatchesFound( const string & victim );
	string UnableToKickFoundMoreThanOneMatch( const string & victim );
	string SettingLatencyToMinimum( const string & min );
	string SettingLatencyToMaximum( const string & max );
	string SettingLatencyTo( const string & latency );
	string KickingPlayersWithPingsGreaterThan( const string & total, const string & ping );
	string HasPlayedGamesWithThisBot( const string & user, const string & firstgame, const string & lastgame, const string & totalgames, const string & avgloadingtime, const string & avgstay );
	string HasntPlayedGamesWithThisBot( const string & user );
	string AutokickingPlayerForExcessivePing( const string & victim, const string & ping );
	string SpoofCheckAcceptedFor( const string & server, const string & user );
	string PlayersNotYetSpoofChecked( const string & notspoofchecked );
	string ManuallySpoofCheckByWhispering( const string & hostname );
	string SpoofCheckByWhispering( const string & hostname );
	string EveryoneHasBeenSpoofChecked( );
	string PlayersNotYetPinged( const string & notpinged );
	string EveryoneHasBeenPinged( );
	string ShortestLoadByPlayer( const string & user, const string & loadingtime );
	string LongestLoadByPlayer( const string & user, const string & loadingtime );
	string YourLoadingTimeWas( const string & loadingtime );
	string HasPlayedDotAGamesWithThisBot( const string & user, const string & totalgames, const string & totalwins, const string & totallosses, const string & totalkills, const string & totaldeaths, const string & totalcreepkills, const string & totalcreepdenies, const string & totalassists, const string & totalneutralkills, const string & totaltowerkills, const string & totalraxkills, const string & totalcourierkills, const string & avgkills, const string & avgdeaths, const string & avgcreepkills, const string & avgcreepdenies, const string & avgassists, const string & avgneutralkills, const string & avgtowerkills, const string & avgraxkills, const string & avgcourierkills );
	string HasntPlayedDotAGamesWithThisBot( const string & user );
	string WasKickedForReservedPlayer( const string & reserved );
	string WasKickedForOwnerPlayer( const string & owner );
	string WasKickedByPlayer( const string & user );
	string HasLostConnectionPlayerError( const string & error );
	string HasLostConnectionSocketError( const string & error );
	string HasLostConnectionClosedByRemoteHost( );
	string HasLeftVoluntarily( );
	string EndingGame( const string & description );
	string HasLostConnectionTimedOut( );
	string GlobalChatMuted( );
	string GlobalChatUnmuted( );
	string ShufflingPlayers( );
	string UnableToLoadConfigFileGameInLobby( );
	string PlayersStillDownloading( const string & stilldownloading );
	string AtLeastOneGameActiveUseForceToShutdown( );
	string CurrentlyLoadedMapCFGIs( const string & mapcfg );
	string LaggedOutDroppedByAdmin( );
	string LaggedOutDroppedByVote( );
	string PlayerVotedToDropLaggers( const string & user );
	string LatencyIs( const string & latency );
	string SyncLimitIs( const string & synclimit );
	string SettingSyncLimitToMinimum( const string & min );
	string SettingSyncLimitToMaximum( const string & max );
	string SettingSyncLimitTo( const string & synclimit );
	string UnableToCreateGameNotLoggedIn( const string & gamename );
	string ConnectingToBNET( const string & server );
	string ConnectedToBNET( const string & server );
	string DisconnectedFromBNET( const string & server );
	string LoggedInToBNET( const string & server );
	string BNETGameHostingSucceeded( const string & server );
	string BNETGameHostingFailed( const string & server, const string & gamename );
	string ConnectingToBNETTimedOut( const string & server );
	string PlayerDownloadedTheMap( const string & user, const string & seconds, const string & rate );
	string UnableToCreateGameNameTooLong( const string & gamename );
	string SettingGameOwnerTo( const string & owner );
	string TheGameIsLocked( );
	string GameLocked( );
	string GameUnlocked( );
	string UnableToStartDownloadNoMatchesFound( const string & victim );
	string UnableToStartDownloadFoundMoreThanOneMatch( const string & victim );
	string UnableToSetGameOwner( const string & owner );
	string UnableToCheckPlayerNoMatchesFound( const string & victim );
	string CheckedPlayer( const string & victim, const string & ping, const string & from, const string & admin, const string & owner, const string & spoofed, const string & spoofedrealm, const string & reserved );
	string UnableToCheckPlayerFoundMoreThanOneMatch( const string & victim );
	string TheGameIsLockedBNET( );
	string UnableToCreateGameDisabled( const string & gamename );
	string BotDisabled( );
	string BotEnabled( );
	string UnableToCreateGameInvalidMap( const string & gamename );
	string DesyncDetected( );
	string UnableToMuteNoMatchesFound( const string & victim );
	string MutedPlayer( const string & victim, const string & user );
	string UnmutedPlayer( const string & victim, const string & user );
	string UnableToMuteFoundMoreThanOneMatch( const string & victim );
	string PlayerIsSavingTheGame( const string & player );
	string UpdatingClanList( );
	string UpdatingFriendsList( );
	string MultipleIPAddressUsageDetected( const string & player, const string & others );
	string UnableToVoteKickAlreadyInProgress( );
	string UnableToVoteKickNotEnoughPlayers( );
	string UnableToVoteKickNoMatchesFound( const string & victim );
	string UnableToVoteKickPlayerIsReserved( const string & victim );
	string StartedVoteKick( const string & victim, const string & user, const string & votesneeded );
	string UnableToVoteKickFoundMoreThanOneMatch( const string & victim );
	string VoteKickPassed( const string & victim );
	string ErrorVoteKickingPlayer( const string & victim );
	string VoteKickAcceptedNeedMoreVotes( const string & victim, const string & user, const string & votes );
	string VoteKickCancelled( const string & victim );
	string VoteKickExpired( const string & victim );
	string WasKickedByVote( );
	string TypeYesToVote( const string & commandtrigger );
	string PlayersNotYetPingedAutoStart( const string & notpinged );
	string WasKickedForNotSpoofChecking( );
	string ErrorListingMaps( );
	string FoundMaps( const string & maps );
	string NoMapsFound( );
	string ErrorListingMapConfigs( );
	string FoundMapConfigs( const string & mapconfigs );
	string NoMapConfigsFound( );
	string PlayerFinishedLoading( const string & user );
	string PleaseWaitPlayersStillLoading( );
	string MapDownloadsDisabled( );
	string MapDownloadsEnabled( );
	string MapDownloadsConditional( );
	string SettingHCL( const string & HCL );
	string UnableToSetHCLInvalid( );
	string UnableToSetHCLTooLong( );
	string TheHCLIs( const string & HCL );
	string TheHCLIsTooLongUseForceToStart( );
	string ClearingHCL( );
	string TryingToRehostAsPrivateGame( const string & gamename );
	string TryingToRehostAsPublicGame( const string & gamename );
	string RehostWasSuccessful( );
	string TryingToJoinTheGameButBannedByName( const string & victim );
	string TryingToJoinTheGameButBannedByIP( const string & victim, const string & ip, const string & bannedname );
	string HasBannedName( const string & victim );
	string HasBannedIP( const string & victim, const string & ip, const string & bannedname );
	string PlayersInGameState( const string & number, const string & players );
	string ValidServers( const string & servers );
	string WasDroppedDesync( );
	string ReloadingConfigurationFiles( );
	string CountDownAbortedSomeoneLeftRecently( );
	string CommandTrigger( const string & trigger );
	string CantEndGameOwnerIsStillPlaying( const string & owner );
	string CantUnhostGameOwnerIsPresent( const string & owner );
	string WasAutomaticallyDroppedAfterSeconds( const string & seconds );
	string HasLostConnectionTimedOutGProxy( );
	string HasLostConnectionSocketErrorGProxy( const string & error );
	string HasLostConnectionClosedByRemoteHostGProxy( );
	string WaitForReconnectSecondsRemain( const string & seconds );
	string WasUnrecoverablyDroppedFromGProxy( );
	string PlayerReconnectedWithGProxy( const string & name );
};

#endif
