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
	CLanguage( string nCFGFile );
	~CLanguage( );

	string UnableToCreateGameTryAnotherName( string server, string gamename );
	string UserIsAlreadyAnAdmin( string server, string user );
	string AddedUserToAdminDatabase( string server, string user );
	string ErrorAddingUserToAdminDatabase( string server, string user );
	string YouDontHaveAccessToThatCommand( );
	string UserIsAlreadyBanned( string server, string victim );
	string BannedUser( string server, string victim );
	string ErrorBanningUser( string server, string victim );
	string UserIsAnAdmin( string server, string user );
	string UserIsNotAnAdmin( string server, string user );
	string UserWasBannedOnByBecause( string server, string victim, string date, string admin, string reason );
	string UserIsNotBanned( string server, string victim );
	string ThereAreNoAdmins( string server );
	string ThereIsAdmin( string server );
	string ThereAreAdmins( string server, string count );
	string ThereAreNoBannedUsers( string server );
	string ThereIsBannedUser( string server );
	string ThereAreBannedUsers( string server, string count );
	string YouCantDeleteTheRootAdmin( );
	string DeletedUserFromAdminDatabase( string server, string user );
	string ErrorDeletingUserFromAdminDatabase( string server, string user );
	string UnbannedUser( string victim );
	string ErrorUnbanningUser( string victim );
	string GameNumberIs( string number, string description );
	string GameNumberDoesntExist( string number );
	string GameIsInTheLobby( string description, string current, string max );
	string ThereIsNoGameInTheLobby( string current, string max );
	string UnableToLoadConfigFilesOutside( );
	string LoadingConfigFile( string file );
	string UnableToLoadConfigFileDoesntExist( string file );
	string CreatingPrivateGame( string gamename, string user );
	string CreatingPublicGame( string gamename, string user );
	string UnableToUnhostGameCountdownStarted( string description );
	string UnhostingGame( string description );
	string UnableToUnhostGameNoGameInLobby( );
	string VersionAdmin( string version );
	string VersionNotAdmin( string version );
	string UnableToCreateGameAnotherGameInLobby( string gamename, string description );
	string UnableToCreateGameMaxGamesReached( string gamename, string max );
	string GameIsOver( string description );
	string SpoofCheckByReplying( );
	string GameRefreshed( );
	string SpoofPossibleIsAway( string user );
	string SpoofPossibleIsUnavailable( string user );
	string SpoofPossibleIsRefusingMessages( string user );
	string SpoofDetectedIsNotInGame( string user );
	string SpoofDetectedIsInPrivateChannel( string user );
	string SpoofDetectedIsInAnotherGame( string user );
	string CountDownAborted( );
	string TryingToJoinTheGameButBanned( string victim );
	string UnableToBanNoMatchesFound( string victim );
	string PlayerWasBannedByPlayer( string server, string victim, string user );
	string UnableToBanFoundMoreThanOneMatch( string victim );
	string AddedPlayerToTheHoldList( string user );
	string UnableToKickNoMatchesFound( string victim );
	string UnableToKickFoundMoreThanOneMatch( string victim );
	string SettingLatencyToMinimum( string min );
	string SettingLatencyToMaximum( string max );
	string SettingLatencyTo( string latency );
	string KickingPlayersWithPingsGreaterThan( string total, string ping );
	string HasPlayedGamesWithThisBot( string user, string firstgame, string lastgame, string totalgames, string avgloadingtime, string avgstay );
	string HasntPlayedGamesWithThisBot( string user );
	string AutokickingPlayerForExcessivePing( string victim, string ping );
	string SpoofCheckAcceptedFor( string server, string user );
	string PlayersNotYetSpoofChecked( string notspoofchecked );
	string ManuallySpoofCheckByWhispering( string hostname );
	string SpoofCheckByWhispering( string hostname );
	string EveryoneHasBeenSpoofChecked( );
	string PlayersNotYetPinged( string notpinged );
	string EveryoneHasBeenPinged( );
	string ShortestLoadByPlayer( string user, string loadingtime );
	string LongestLoadByPlayer( string user, string loadingtime );
	string YourLoadingTimeWas( string loadingtime );
	string HasPlayedDotAGamesWithThisBot( string user, string totalgames, string totalwins, string totallosses, string totalkills, string totaldeaths, string totalcreepkills, string totalcreepdenies, string totalassists, string totalneutralkills, string totaltowerkills, string totalraxkills, string totalcourierkills, string avgkills, string avgdeaths, string avgcreepkills, string avgcreepdenies, string avgassists, string avgneutralkills, string avgtowerkills, string avgraxkills, string avgcourierkills );
	string HasntPlayedDotAGamesWithThisBot( string user );
	string WasKickedForReservedPlayer( string reserved );
	string WasKickedForOwnerPlayer( string owner );
	string WasKickedByPlayer( string user );
	string HasLostConnectionPlayerError( string error );
	string HasLostConnectionSocketError( string error );
	string HasLostConnectionClosedByRemoteHost( );
	string HasLeftVoluntarily( );
	string EndingGame( string description );
	string HasLostConnectionTimedOut( );
	string GlobalChatMuted( );
	string GlobalChatUnmuted( );
	string ShufflingPlayers( );
	string UnableToLoadConfigFileGameInLobby( );
	string PlayersStillDownloading( string stilldownloading );
	string RefreshMessagesEnabled( );
	string RefreshMessagesDisabled( );
	string AtLeastOneGameActiveUseForceToShutdown( );
	string CurrentlyLoadedMapCFGIs( string mapcfg );
	string LaggedOutDroppedByAdmin( );
	string LaggedOutDroppedByVote( );
	string PlayerVotedToDropLaggers( string user );
	string LatencyIs( string latency );
	string SyncLimitIs( string synclimit );
	string SettingSyncLimitToMinimum( string min );
	string SettingSyncLimitToMaximum( string max );
	string SettingSyncLimitTo( string synclimit );
	string UnableToCreateGameNotLoggedIn( string gamename );
	string AdminLoggedIn( );
	string AdminInvalidPassword( string attempt );
	string ConnectingToBNET( string server );
	string ConnectedToBNET( string server );
	string DisconnectedFromBNET( string server );
	string LoggedInToBNET( string server );
	string BNETGameHostingSucceeded( string server );
	string BNETGameHostingFailed( string server, string gamename );
	string ConnectingToBNETTimedOut( string server );
	string PlayerDownloadedTheMap( string user, string seconds, string rate );
	string UnableToCreateGameNameTooLong( string gamename );
	string SettingGameOwnerTo( string owner );
	string TheGameIsLocked( );
	string GameLocked( );
	string GameUnlocked( );
	string UnableToStartDownloadNoMatchesFound( string victim );
	string UnableToStartDownloadFoundMoreThanOneMatch( string victim );
	string UnableToSetGameOwner( string owner );
	string UnableToCheckPlayerNoMatchesFound( string victim );
	string CheckedPlayer( string victim, string ping, string from, string admin, string owner, string spoofed, string spoofedrealm, string reserved );
	string UnableToCheckPlayerFoundMoreThanOneMatch( string victim );
	string TheGameIsLockedBNET( );
	string UnableToCreateGameDisabled( string gamename );
	string BotDisabled( );
	string BotEnabled( );
	string UnableToCreateGameInvalidMap( string gamename );
	string WaitingForPlayersBeforeAutoStart( string players, string playersleft );
	string AutoStartDisabled( );
	string AutoStartEnabled( string players );
	string AnnounceMessageEnabled( );
	string AnnounceMessageDisabled( );
	string AutoHostEnabled( );
	string AutoHostDisabled( );
	string UnableToLoadSaveGamesOutside( );
	string UnableToLoadSaveGameGameInLobby( );
	string LoadingSaveGame( string file );
	string UnableToLoadSaveGameDoesntExist( string file );
	string UnableToCreateGameInvalidSaveGame( string gamename );
	string UnableToCreateGameSaveGameMapMismatch( string gamename );
	string AutoSaveEnabled( );
	string AutoSaveDisabled( );
	string DesyncDetected( );
	string UnableToMuteNoMatchesFound( string victim );
	string MutedPlayer( string victim, string user );
	string UnmutedPlayer( string victim, string user );
	string UnableToMuteFoundMoreThanOneMatch( string victim );
	string PlayerIsSavingTheGame( string player );
	string UpdatingClanList( );
	string UpdatingFriendsList( );
	string MultipleIPAddressUsageDetected( string player, string others );
	string UnableToVoteKickAlreadyInProgress( );
	string UnableToVoteKickNotEnoughPlayers( );
	string UnableToVoteKickNoMatchesFound( string victim );
	string UnableToVoteKickPlayerIsReserved( string victim );
	string StartedVoteKick( string victim, string user, string votesneeded );
	string UnableToVoteKickFoundMoreThanOneMatch( string victim );
	string VoteKickPassed( string victim );
	string ErrorVoteKickingPlayer( string victim );
	string VoteKickAcceptedNeedMoreVotes( string victim, string user, string votes );
	string VoteKickCancelled( string victim );
	string VoteKickExpired( string victim );
	string WasKickedByVote( );
	string TypeYesToVote( string commandtrigger );
	string PlayersNotYetPingedAutoStart( string notpinged );
	string WasKickedForNotSpoofChecking( );
	string WasKickedForHavingFurthestScore( string score, string average );
	string PlayerHasScore( string player, string score );
	string RatedPlayersSpread( string rated, string total, string spread );
	string ErrorListingMaps( );
	string FoundMaps( string maps );
	string NoMapsFound( );
	string ErrorListingMapConfigs( );
	string FoundMapConfigs( string mapconfigs );
	string NoMapConfigsFound( );
	string PlayerFinishedLoading( string user );
	string PleaseWaitPlayersStillLoading( );
	string MapDownloadsDisabled( );
	string MapDownloadsEnabled( );
	string MapDownloadsConditional( );
	string SettingHCL( string HCL );
	string UnableToSetHCLInvalid( );
	string UnableToSetHCLTooLong( );
	string TheHCLIs( string HCL );
	string TheHCLIsTooLongUseForceToStart( );
	string ClearingHCL( );
	string TryingToRehostAsPrivateGame( string gamename );
	string TryingToRehostAsPublicGame( string gamename );
	string RehostWasSuccessful( );
	string TryingToJoinTheGameButBannedByName( string victim );
	string TryingToJoinTheGameButBannedByIP( string victim, string ip, string bannedname );
	string HasBannedName( string victim );
	string HasBannedIP( string victim, string ip, string bannedname );
	string PlayersInGameState( string number, string players );
	string ValidServers( string servers );
	string TeamCombinedScore( string team, string score );
	string BalancingSlotsCompleted( );
	string PlayerWasKickedForFurthestScore( string name, string score, string average );
	string LocalAdminMessagesEnabled( );
	string LocalAdminMessagesDisabled( );
	string WasDroppedDesync( );
	string WasKickedForHavingLowestScore( string score );
	string PlayerWasKickedForLowestScore( string name, string score );
	string ReloadingConfigurationFiles( );
	string CountDownAbortedSomeoneLeftRecently( );
	string UnableToCreateGameMustEnforceFirst( string gamename );
	string UnableToLoadReplaysOutside( );
	string LoadingReplay( string file );
	string UnableToLoadReplayDoesntExist( string file );
	string CommandTrigger( string trigger );
	string CantEndGameOwnerIsStillPlaying( string owner );
	string CantUnhostGameOwnerIsPresent( string owner );
	string WasAutomaticallyDroppedAfterSeconds( string seconds );
	string HasLostConnectionTimedOutGProxy( );
	string HasLostConnectionSocketErrorGProxy( string error );
	string HasLostConnectionClosedByRemoteHostGProxy( );
	string WaitForReconnectSecondsRemain( string seconds );
	string WasUnrecoverablyDroppedFromGProxy( );
	string PlayerReconnectedWithGProxy( string name );
};

#endif
