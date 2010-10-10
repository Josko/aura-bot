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

//
// CLanguage
//

CLanguage :: CLanguage( string nCFGFile )
{
	m_CFG = new CConfig( );
	m_CFG->Read( nCFGFile );
}

CLanguage :: ~CLanguage( )
{
	delete m_CFG;
}

string CLanguage :: UnableToCreateGameTryAnotherName( string server, string gamename )
{
	string Out = m_CFG->GetString( "lang_0001", "lang_0001" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: UserIsAlreadyAnAdmin( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0002", "lang_0002" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: AddedUserToAdminDatabase( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0003", "lang_0003" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: ErrorAddingUserToAdminDatabase( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0004", "lang_0004" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: YouDontHaveAccessToThatCommand( )
{
	return m_CFG->GetString( "lang_0005", "lang_0005" );
}

string CLanguage :: UserIsAlreadyBanned( string server, string victim )
{
	string Out = m_CFG->GetString( "lang_0006", "lang_0006" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: BannedUser( string server, string victim )
{
	string Out = m_CFG->GetString( "lang_0007", "lang_0007" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: ErrorBanningUser( string server, string victim )
{
	string Out = m_CFG->GetString( "lang_0008", "lang_0008" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UserIsAnAdmin( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0009", "lang_0009" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UserIsNotAnAdmin( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0010", "lang_0010" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UserWasBannedOnByBecause( string server, string victim, string date, string admin, string reason )
{
	string Out = m_CFG->GetString( "lang_0011", "lang_0011" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$DATE$", date );
	UTIL_Replace( Out, "$ADMIN$", admin );
	UTIL_Replace( Out, "$REASON$", reason );
	return Out;
}

string CLanguage :: UserIsNotBanned( string server, string victim )
{
	string Out = m_CFG->GetString( "lang_0012", "lang_0012" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: ThereAreNoAdmins( string server )
{
	string Out = m_CFG->GetString( "lang_0013", "lang_0013" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ThereIsAdmin( string server )
{
	string Out = m_CFG->GetString( "lang_0014", "lang_0014" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ThereAreAdmins( string server, string count )
{
	string Out = m_CFG->GetString( "lang_0015", "lang_0015" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$COUNT$", count );
	return Out;
}

string CLanguage :: ThereAreNoBannedUsers( string server )
{
	string Out = m_CFG->GetString( "lang_0016", "lang_0016" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ThereIsBannedUser( string server )
{
	string Out = m_CFG->GetString( "lang_0017", "lang_0017" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ThereAreBannedUsers( string server, string count )
{
	string Out = m_CFG->GetString( "lang_0018", "lang_0018" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$COUNT$", count );
	return Out;
}

string CLanguage :: YouCantDeleteTheRootAdmin( )
{
	return m_CFG->GetString( "lang_0019", "lang_0019" );
}

string CLanguage :: DeletedUserFromAdminDatabase( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0020", "lang_0020" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: ErrorDeletingUserFromAdminDatabase( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0021", "lang_0021" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnbannedUser( string victim )
{
	string Out = m_CFG->GetString( "lang_0022", "lang_0022" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: ErrorUnbanningUser( string victim )
{
	string Out = m_CFG->GetString( "lang_0023", "lang_0023" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: GameNumberIs( string number, string description )
{
	string Out = m_CFG->GetString( "lang_0024", "lang_0024" );
	UTIL_Replace( Out, "$NUMBER$", number );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: GameNumberDoesntExist( string number )
{
	string Out = m_CFG->GetString( "lang_0025", "lang_0025" );
	UTIL_Replace( Out, "$NUMBER$", number );
	return Out;
}

string CLanguage :: GameIsInTheLobby( string description, string current, string max )
{
	string Out = m_CFG->GetString( "lang_0026", "lang_0026" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	UTIL_Replace( Out, "$CURRENT$", current );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: ThereIsNoGameInTheLobby( string current, string max )
{
	string Out = m_CFG->GetString( "lang_0027", "lang_0027" );
	UTIL_Replace( Out, "$CURRENT$", current );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: UnableToLoadConfigFilesOutside( )
{
	return m_CFG->GetString( "lang_0028", "lang_0028" );
}

string CLanguage :: LoadingConfigFile( string file )
{
	string Out = m_CFG->GetString( "lang_0029", "lang_0029" );
	UTIL_Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage :: UnableToLoadConfigFileDoesntExist( string file )
{
	string Out = m_CFG->GetString( "lang_0030", "lang_0030" );
	UTIL_Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage :: CreatingPrivateGame( string gamename, string user )
{
	string Out = m_CFG->GetString( "lang_0031", "lang_0031" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: CreatingPublicGame( string gamename, string user )
{
	string Out = m_CFG->GetString( "lang_0032", "lang_0032" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnableToUnhostGameCountdownStarted( string description )
{
	string Out = m_CFG->GetString( "lang_0033", "lang_0033" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: UnhostingGame( string description )
{
	string Out = m_CFG->GetString( "lang_0034", "lang_0034" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: UnableToUnhostGameNoGameInLobby( )
{
	return m_CFG->GetString( "lang_0035", "lang_0035" );
}

string CLanguage :: VersionAdmin( string version )
{
	string Out = m_CFG->GetString( "lang_0036", "lang_0036" );
	UTIL_Replace( Out, "$VERSION$", version );
	return Out;
}

string CLanguage :: VersionNotAdmin( string version )
{
	string Out = m_CFG->GetString( "lang_0037", "lang_0037" );
	UTIL_Replace( Out, "$VERSION$", version );
	return Out;
}

string CLanguage :: UnableToCreateGameAnotherGameInLobby( string gamename, string description )
{
	string Out = m_CFG->GetString( "lang_0038", "lang_0038" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: UnableToCreateGameMaxGamesReached( string gamename, string max )
{
	string Out = m_CFG->GetString( "lang_0039", "lang_0039" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: GameIsOver( string description )
{
	string Out = m_CFG->GetString( "lang_0040", "lang_0040" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: SpoofCheckByReplying( )
{
	return m_CFG->GetString( "lang_0041", "lang_0041" );
}

string CLanguage :: GameRefreshed( )
{
	return m_CFG->GetString( "lang_0042", "lang_0042" );
}

string CLanguage :: SpoofPossibleIsAway( string user )
{
	string Out = m_CFG->GetString( "lang_0043", "lang_0043" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofPossibleIsUnavailable( string user )
{
	string Out = m_CFG->GetString( "lang_0044", "lang_0044" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofPossibleIsRefusingMessages( string user )
{
	string Out = m_CFG->GetString( "lang_0045", "lang_0045" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofDetectedIsNotInGame( string user )
{
	string Out = m_CFG->GetString( "lang_0046", "lang_0046" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofDetectedIsInPrivateChannel( string user )
{
	string Out = m_CFG->GetString( "lang_0047", "lang_0047" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofDetectedIsInAnotherGame( string user )
{
	string Out = m_CFG->GetString( "lang_0048", "lang_0048" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: CountDownAborted( )
{
	return m_CFG->GetString( "lang_0049", "lang_0049" );
}

string CLanguage :: TryingToJoinTheGameButBanned( string victim )
{
	string Out = m_CFG->GetString( "lang_0050", "lang_0050" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToBanNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0051", "lang_0051" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: PlayerWasBannedByPlayer( string server, string victim, string user )
{
	string Out = m_CFG->GetString( "lang_0052", "lang_0052" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnableToBanFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0053", "lang_0053" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: AddedPlayerToTheHoldList( string user )
{
	string Out = m_CFG->GetString( "lang_0054", "lang_0054" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnableToKickNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0055", "lang_0055" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToKickFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0056", "lang_0056" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: SettingLatencyToMinimum( string min )
{
	string Out = m_CFG->GetString( "lang_0057", "lang_0057" );
	UTIL_Replace( Out, "$MIN$", min );
	return Out;
}

string CLanguage :: SettingLatencyToMaximum( string max )
{
	string Out = m_CFG->GetString( "lang_0058", "lang_0058" );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: SettingLatencyTo( string latency )
{
	string Out = m_CFG->GetString( "lang_0059", "lang_0059" );
	UTIL_Replace( Out, "$LATENCY$", latency );
	return Out;
}

string CLanguage :: KickingPlayersWithPingsGreaterThan( string total, string ping )
{
	string Out = m_CFG->GetString( "lang_0060", "lang_0060" );
	UTIL_Replace( Out, "$TOTAL$", total );
	UTIL_Replace( Out, "$PING$", ping );
	return Out;
}

string CLanguage :: HasPlayedGamesWithThisBot( string user, string firstgame, string lastgame, string totalgames, string avgloadingtime, string avgstay )
{
	string Out = m_CFG->GetString( "lang_0061", "lang_0061" );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$FIRSTGAME$", firstgame );
	UTIL_Replace( Out, "$LASTGAME$", lastgame );
	UTIL_Replace( Out, "$TOTALGAMES$", totalgames );
	UTIL_Replace( Out, "$AVGLOADINGTIME$", avgloadingtime );
	UTIL_Replace( Out, "$AVGSTAY$", avgstay );
	return Out;
}

string CLanguage :: HasntPlayedGamesWithThisBot( string user )
{
	string Out = m_CFG->GetString( "lang_0062", "lang_0062" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: AutokickingPlayerForExcessivePing( string victim, string ping )
{
	string Out = m_CFG->GetString( "lang_0063", "lang_0063" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$PING$", ping );
	return Out;
}

string CLanguage :: SpoofCheckAcceptedFor( string server, string user )
{
	string Out = m_CFG->GetString( "lang_0064", "lang_0064" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: PlayersNotYetSpoofChecked( string notspoofchecked )
{
	string Out = m_CFG->GetString( "lang_0065", "lang_0065" );
	UTIL_Replace( Out, "$NOTSPOOFCHECKED$", notspoofchecked );
	return Out;
}

string CLanguage :: ManuallySpoofCheckByWhispering( string hostname )
{
	string Out = m_CFG->GetString( "lang_0066", "lang_0066" );
	UTIL_Replace( Out, "$HOSTNAME$", hostname );
	return Out;
}

string CLanguage :: SpoofCheckByWhispering( string hostname )
{
	string Out = m_CFG->GetString( "lang_0067", "lang_0067" );
	UTIL_Replace( Out, "$HOSTNAME$", hostname );
	return Out;
}

string CLanguage :: EveryoneHasBeenSpoofChecked( )
{
	return m_CFG->GetString( "lang_0068", "lang_0068" );
}

string CLanguage :: PlayersNotYetPinged( string notpinged )
{
	string Out = m_CFG->GetString( "lang_0069", "lang_0069" );
	UTIL_Replace( Out, "$NOTPINGED$", notpinged );
	return Out;
}

string CLanguage :: EveryoneHasBeenPinged( )
{
	return m_CFG->GetString( "lang_0070", "lang_0070" );
}

string CLanguage :: ShortestLoadByPlayer( string user, string loadingtime )
{
	string Out = m_CFG->GetString( "lang_0071", "lang_0071" );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage :: LongestLoadByPlayer( string user, string loadingtime )
{
	string Out = m_CFG->GetString( "lang_0072", "lang_0072" );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage :: YourLoadingTimeWas( string loadingtime )
{
	string Out = m_CFG->GetString( "lang_0073", "lang_0073" );
	UTIL_Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage :: HasPlayedDotAGamesWithThisBot( string user, string totalgames, string totalwins, string totallosses, string totalkills, string totaldeaths, string totalcreepkills, string totalcreepdenies, string totalassists, string totalneutralkills, string totaltowerkills, string totalraxkills, string totalcourierkills, string avgkills, string avgdeaths, string avgcreepkills, string avgcreepdenies, string avgassists, string avgneutralkills, string avgtowerkills, string avgraxkills, string avgcourierkills )
{
	string Out = m_CFG->GetString( "lang_0074", "lang_0074" );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$TOTALGAMES$", totalgames );
	UTIL_Replace( Out, "$TOTALWINS$", totalwins );
	UTIL_Replace( Out, "$TOTALLOSSES$", totallosses );
	UTIL_Replace( Out, "$TOTALKILLS$", totalkills );
	UTIL_Replace( Out, "$TOTALDEATHS$", totaldeaths );
	UTIL_Replace( Out, "$TOTALCREEPKILLS$", totalcreepkills );
	UTIL_Replace( Out, "$TOTALCREEPDENIES$", totalcreepdenies );
	UTIL_Replace( Out, "$TOTALASSISTS$", totalassists );
	UTIL_Replace( Out, "$TOTALNEUTRALKILLS$", totalneutralkills );
	UTIL_Replace( Out, "$TOTALTOWERKILLS$", totaltowerkills );
	UTIL_Replace( Out, "$TOTALRAXKILLS$", totalraxkills );
	UTIL_Replace( Out, "$TOTALCOURIERKILLS$", totalcourierkills );
	UTIL_Replace( Out, "$AVGKILLS$", avgkills );
	UTIL_Replace( Out, "$AVGDEATHS$", avgdeaths );
	UTIL_Replace( Out, "$AVGCREEPKILLS$", avgcreepkills );
	UTIL_Replace( Out, "$AVGCREEPDENIES$", avgcreepdenies );
	UTIL_Replace( Out, "$AVGASSISTS$", avgassists );
	UTIL_Replace( Out, "$AVGNEUTRALKILLS$", avgneutralkills );
	UTIL_Replace( Out, "$AVGTOWERKILLS$", avgtowerkills );
	UTIL_Replace( Out, "$AVGRAXKILLS$", avgraxkills );
	UTIL_Replace( Out, "$AVGCOURIERKILLS$", avgcourierkills );
	return Out;
}

string CLanguage :: HasntPlayedDotAGamesWithThisBot( string user )
{
	string Out = m_CFG->GetString( "lang_0075", "lang_0075" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: WasKickedForReservedPlayer( string reserved )
{
	string Out = m_CFG->GetString( "lang_0076", "lang_0076" );
	UTIL_Replace( Out, "$RESERVED$", reserved );
	return Out;
}

string CLanguage :: WasKickedForOwnerPlayer( string owner )
{
	string Out = m_CFG->GetString( "lang_0077", "lang_0077" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: WasKickedByPlayer( string user )
{
	string Out = m_CFG->GetString( "lang_0078", "lang_0078" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: HasLostConnectionPlayerError( string error )
{
	string Out = m_CFG->GetString( "lang_0079", "lang_0079" );
	UTIL_Replace( Out, "$ERROR$", error );
	return Out;
}

string CLanguage :: HasLostConnectionSocketError( string error )
{
	string Out = m_CFG->GetString( "lang_0080", "lang_0080" );
	UTIL_Replace( Out, "$ERROR$", error );
	return Out;
}

string CLanguage :: HasLostConnectionClosedByRemoteHost( )
{
	return m_CFG->GetString( "lang_0081", "lang_0081" );
}

string CLanguage :: HasLeftVoluntarily( )
{
	return m_CFG->GetString( "lang_0082", "lang_0082" );
}

string CLanguage :: EndingGame( string description )
{
	string Out = m_CFG->GetString( "lang_0083", "lang_0083" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: HasLostConnectionTimedOut( )
{
	return m_CFG->GetString( "lang_0084", "lang_0084" );
}

string CLanguage :: GlobalChatMuted( )
{
	return m_CFG->GetString( "lang_0085", "lang_0085" );
}

string CLanguage :: GlobalChatUnmuted( )
{
	return m_CFG->GetString( "lang_0086", "lang_0086" );
}

string CLanguage :: ShufflingPlayers( )
{
	return m_CFG->GetString( "lang_0087", "lang_0087" );
}

string CLanguage :: UnableToLoadConfigFileGameInLobby( )
{
	return m_CFG->GetString( "lang_0088", "lang_0088" );
}

string CLanguage :: PlayersStillDownloading( string stilldownloading )
{
	string Out = m_CFG->GetString( "lang_0089", "lang_0089" );
	UTIL_Replace( Out, "$STILLDOWNLOADING$", stilldownloading );
	return Out;
}

string CLanguage :: RefreshMessagesEnabled( )
{
	return m_CFG->GetString( "lang_0090", "lang_0090" );
}

string CLanguage :: RefreshMessagesDisabled( )
{
	return m_CFG->GetString( "lang_0091", "lang_0091" );
}

string CLanguage :: AtLeastOneGameActiveUseForceToShutdown( )
{
	return m_CFG->GetString( "lang_0092", "lang_0092" );
}

string CLanguage :: CurrentlyLoadedMapCFGIs( string mapcfg )
{
	string Out = m_CFG->GetString( "lang_0093", "lang_0093" );
	UTIL_Replace( Out, "$MAPCFG$", mapcfg );
	return Out;
}

string CLanguage :: LaggedOutDroppedByAdmin( )
{
	return m_CFG->GetString( "lang_0094", "lang_0094" );
}

string CLanguage :: LaggedOutDroppedByVote( )
{
	return m_CFG->GetString( "lang_0095", "lang_0095" );
}

string CLanguage :: PlayerVotedToDropLaggers( string user )
{
	string Out = m_CFG->GetString( "lang_0096", "lang_0096" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: LatencyIs( string latency )
{
	string Out = m_CFG->GetString( "lang_0097", "lang_0097" );
	UTIL_Replace( Out, "$LATENCY$", latency );
	return Out;
}

string CLanguage :: SyncLimitIs( string synclimit )
{
	string Out = m_CFG->GetString( "lang_0098", "lang_0098" );
	UTIL_Replace( Out, "$SYNCLIMIT$", synclimit );
	return Out;
}

string CLanguage :: SettingSyncLimitToMinimum( string min )
{
	string Out = m_CFG->GetString( "lang_0099", "lang_0099" );
	UTIL_Replace( Out, "$MIN$", min );
	return Out;
}

string CLanguage :: SettingSyncLimitToMaximum( string max )
{
	string Out = m_CFG->GetString( "lang_0100", "lang_0100" );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: SettingSyncLimitTo( string synclimit )
{
	string Out = m_CFG->GetString( "lang_0101", "lang_0101" );
	UTIL_Replace( Out, "$SYNCLIMIT$", synclimit );
	return Out;
}

string CLanguage :: UnableToCreateGameNotLoggedIn( string gamename )
{
	string Out = m_CFG->GetString( "lang_0102", "lang_0102" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: AdminLoggedIn( )
{
	return m_CFG->GetString( "lang_0103", "lang_0103" );
}

string CLanguage :: AdminInvalidPassword( string attempt )
{
	string Out = m_CFG->GetString( "lang_0104", "lang_0104" );
	UTIL_Replace( Out, "$ATTEMPT$", attempt );
	return Out;
}

string CLanguage :: ConnectingToBNET( string server )
{
	string Out = m_CFG->GetString( "lang_0105", "lang_0105" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ConnectedToBNET( string server )
{
	string Out = m_CFG->GetString( "lang_0106", "lang_0106" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: DisconnectedFromBNET( string server )
{
	string Out = m_CFG->GetString( "lang_0107", "lang_0107" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: LoggedInToBNET( string server )
{
	string Out = m_CFG->GetString( "lang_0108", "lang_0108" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: BNETGameHostingSucceeded( string server )
{
	string Out = m_CFG->GetString( "lang_0109", "lang_0109" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: BNETGameHostingFailed( string server, string gamename )
{
	string Out = m_CFG->GetString( "lang_0110", "lang_0110" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: ConnectingToBNETTimedOut( string server )
{
	string Out = m_CFG->GetString( "lang_0111", "lang_0111" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: PlayerDownloadedTheMap( string user, string seconds, string rate )
{
	string Out = m_CFG->GetString( "lang_0112", "lang_0112" );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$SECONDS$", seconds );
	UTIL_Replace( Out, "$RATE$", rate );
	return Out;
}

string CLanguage :: UnableToCreateGameNameTooLong( string gamename )
{
	string Out = m_CFG->GetString( "lang_0113", "lang_0113" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: SettingGameOwnerTo( string owner )
{
	string Out = m_CFG->GetString( "lang_0114", "lang_0114" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: TheGameIsLocked( )
{
	return m_CFG->GetString( "lang_0115", "lang_0115" );
}

string CLanguage :: GameLocked( )
{
	return m_CFG->GetString( "lang_0116", "lang_0116" );
}

string CLanguage :: GameUnlocked( )
{
	return m_CFG->GetString( "lang_0117", "lang_0117" );
}

string CLanguage :: UnableToStartDownloadNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0118", "lang_0118" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToStartDownloadFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0119", "lang_0119" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToSetGameOwner( string owner )
{
	string Out = m_CFG->GetString( "lang_0120", "lang_0120" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: UnableToCheckPlayerNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0121", "lang_0121" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: CheckedPlayer( string victim, string ping, string from, string admin, string owner, string spoofed, string spoofedrealm, string reserved )
{
	string Out = m_CFG->GetString( "lang_0122", "lang_0122" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$PING$", ping );
	UTIL_Replace( Out, "$FROM$", from );
	UTIL_Replace( Out, "$ADMIN$", admin );
	UTIL_Replace( Out, "$OWNER$", owner );
	UTIL_Replace( Out, "$SPOOFED$", spoofed );
	UTIL_Replace( Out, "$SPOOFEDREALM$", spoofedrealm );
	UTIL_Replace( Out, "$RESERVED$", reserved );
	return Out;
}

string CLanguage :: UnableToCheckPlayerFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0123", "lang_0123" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: TheGameIsLockedBNET( )
{
	return m_CFG->GetString( "lang_0124", "lang_0124" );
}

string CLanguage :: UnableToCreateGameDisabled( string gamename )
{
	string Out = m_CFG->GetString( "lang_0125", "lang_0125" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: BotDisabled( )
{
	return m_CFG->GetString( "lang_0126", "lang_0126" );
}

string CLanguage :: BotEnabled( )
{
	return m_CFG->GetString( "lang_0127", "lang_0127" );
}

string CLanguage :: UnableToCreateGameInvalidMap( string gamename )
{
	string Out = m_CFG->GetString( "lang_0128", "lang_0128" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: WaitingForPlayersBeforeAutoStart( string players, string playersleft )
{
	string Out = m_CFG->GetString( "lang_0129", "lang_0129" );
	UTIL_Replace( Out, "$PLAYERS$", players );
	UTIL_Replace( Out, "$PLAYERSLEFT$", playersleft );
	return Out;
}

string CLanguage :: AutoStartDisabled( )
{
	return m_CFG->GetString( "lang_0130", "lang_0130" );
}

string CLanguage :: AutoStartEnabled( string players )
{
	string Out = m_CFG->GetString( "lang_0131", "lang_0131" );
	UTIL_Replace( Out, "$PLAYERS$", players );
	return Out;
}

string CLanguage :: AnnounceMessageEnabled( )
{
	return m_CFG->GetString( "lang_0132", "lang_0132" );
}

string CLanguage :: AnnounceMessageDisabled( )
{
	return m_CFG->GetString( "lang_0133", "lang_0133" );
}

string CLanguage :: AutoHostEnabled( )
{
	return m_CFG->GetString( "lang_0134", "lang_0134" );
}

string CLanguage :: AutoHostDisabled( )
{
	return m_CFG->GetString( "lang_0135", "lang_0135" );
}

string CLanguage :: UnableToLoadSaveGamesOutside( )
{
	return m_CFG->GetString( "lang_0136", "lang_0136" );
}

string CLanguage :: UnableToLoadSaveGameGameInLobby( )
{
	return m_CFG->GetString( "lang_0137", "lang_0137" );
}

string CLanguage :: LoadingSaveGame( string file )
{
	string Out = m_CFG->GetString( "lang_0138", "lang_0138" );
	UTIL_Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage :: UnableToLoadSaveGameDoesntExist( string file )
{
	string Out = m_CFG->GetString( "lang_0139", "lang_0139" );
	UTIL_Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage :: UnableToCreateGameInvalidSaveGame( string gamename )
{
	string Out = m_CFG->GetString( "lang_0140", "lang_0140" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: UnableToCreateGameSaveGameMapMismatch( string gamename )
{
	string Out = m_CFG->GetString( "lang_0141", "lang_0141" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: AutoSaveEnabled( )
{
	return m_CFG->GetString( "lang_0142", "lang_0142" );
}

string CLanguage :: AutoSaveDisabled( )
{
	return m_CFG->GetString( "lang_0143", "lang_0143" );
}

string CLanguage :: DesyncDetected( )
{
	return m_CFG->GetString( "lang_0144", "lang_0144" );
}

string CLanguage :: UnableToMuteNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0145", "lang_0145" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: MutedPlayer( string victim, string user )
{
	string Out = m_CFG->GetString( "lang_0146", "lang_0146" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnmutedPlayer( string victim, string user )
{
	string Out = m_CFG->GetString( "lang_0147", "lang_0147" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnableToMuteFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0148", "lang_0148" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: PlayerIsSavingTheGame( string player )
{
	string Out = m_CFG->GetString( "lang_0149", "lang_0149" );
	UTIL_Replace( Out, "$PLAYER$", player );
	return Out;
}

string CLanguage :: UpdatingClanList( )
{
	return m_CFG->GetString( "lang_0150", "lang_0150" );
}

string CLanguage :: UpdatingFriendsList( )
{
	return m_CFG->GetString( "lang_0151", "lang_0151" );
}

string CLanguage :: MultipleIPAddressUsageDetected( string player, string others )
{
	string Out = m_CFG->GetString( "lang_0152", "lang_0152" );
	UTIL_Replace( Out, "$PLAYER$", player );
	UTIL_Replace( Out, "$OTHERS$", others );
	return Out;
}

string CLanguage :: UnableToVoteKickAlreadyInProgress( )
{
	return m_CFG->GetString( "lang_0153", "lang_0153" );
}

string CLanguage :: UnableToVoteKickNotEnoughPlayers( )
{
	return m_CFG->GetString( "lang_0154", "lang_0154" );
}

string CLanguage :: UnableToVoteKickNoMatchesFound( string victim )
{
	string Out = m_CFG->GetString( "lang_0155", "lang_0155" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToVoteKickPlayerIsReserved( string victim )
{
	string Out = m_CFG->GetString( "lang_0156", "lang_0156" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: StartedVoteKick( string victim, string user, string votesneeded )
{
	string Out = m_CFG->GetString( "lang_0157", "lang_0157" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$VOTESNEEDED$", votesneeded );
	return Out;
}

string CLanguage :: UnableToVoteKickFoundMoreThanOneMatch( string victim )
{
	string Out = m_CFG->GetString( "lang_0158", "lang_0158" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: VoteKickPassed( string victim )
{
	string Out = m_CFG->GetString( "lang_0159", "lang_0159" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: ErrorVoteKickingPlayer( string victim )
{
	string Out = m_CFG->GetString( "lang_0160", "lang_0160" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: VoteKickAcceptedNeedMoreVotes( string victim, string user, string votes )
{
	string Out = m_CFG->GetString( "lang_0161", "lang_0161" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$VOTES$", votes );
	return Out;
}

string CLanguage :: VoteKickCancelled( string victim )
{
	string Out = m_CFG->GetString( "lang_0162", "lang_0162" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: VoteKickExpired( string victim )
{
	string Out = m_CFG->GetString( "lang_0163", "lang_0163" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: WasKickedByVote( )
{
	return m_CFG->GetString( "lang_0164", "lang_0164" );
}

string CLanguage :: TypeYesToVote( string commandtrigger )
{
	string Out = m_CFG->GetString( "lang_0165", "lang_0165" );
	UTIL_Replace( Out, "$COMMANDTRIGGER$", commandtrigger );
	return Out;
}

string CLanguage :: PlayersNotYetPingedAutoStart( string notpinged )
{
	string Out = m_CFG->GetString( "lang_0166", "lang_0166" );
	UTIL_Replace( Out, "$NOTPINGED$", notpinged );
	return Out;
}

string CLanguage :: WasKickedForNotSpoofChecking( )
{
	return m_CFG->GetString( "lang_0167", "lang_0167" );
}

string CLanguage :: WasKickedForHavingFurthestScore( string score, string average )
{
	string Out = m_CFG->GetString( "lang_0168", "lang_0168" );
	UTIL_Replace( Out, "$SCORE$", score );
	UTIL_Replace( Out, "$AVERAGE$", average );
	return Out;
}

string CLanguage :: PlayerHasScore( string player, string score )
{
	string Out = m_CFG->GetString( "lang_0169", "lang_0169" );
	UTIL_Replace( Out, "$PLAYER$", player );
	UTIL_Replace( Out, "$SCORE$", score );
	return Out;
}

string CLanguage :: RatedPlayersSpread( string rated, string total, string spread )
{
	string Out = m_CFG->GetString( "lang_0170", "lang_0170" );
	UTIL_Replace( Out, "$RATED$", rated );
	UTIL_Replace( Out, "$TOTAL$", total );
	UTIL_Replace( Out, "$SPREAD$", spread );
	return Out;
}

string CLanguage :: ErrorListingMaps( )
{
	return m_CFG->GetString( "lang_0171", "lang_0171" );
}

string CLanguage :: FoundMaps( string maps )
{
	string Out = m_CFG->GetString( "lang_0172", "lang_0172" );
	UTIL_Replace( Out, "$MAPS$", maps );
	return Out;
}

string CLanguage :: NoMapsFound( )
{
	return m_CFG->GetString( "lang_0173", "lang_0173" );
}

string CLanguage :: ErrorListingMapConfigs( )
{
	return m_CFG->GetString( "lang_0174", "lang_0174" );
}

string CLanguage :: FoundMapConfigs( string mapconfigs )
{
	string Out = m_CFG->GetString( "lang_0175", "lang_0175" );
	UTIL_Replace( Out, "$MAPCONFIGS$", mapconfigs );
	return Out;
}

string CLanguage :: NoMapConfigsFound( )
{
	return m_CFG->GetString( "lang_0176", "lang_0176" );
}

string CLanguage :: PlayerFinishedLoading( string user )
{
	string Out = m_CFG->GetString( "lang_0177", "lang_0177" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: PleaseWaitPlayersStillLoading( )
{
	return m_CFG->GetString( "lang_0178", "lang_0178" );
}

string CLanguage :: MapDownloadsDisabled( )
{
	return m_CFG->GetString( "lang_0179", "lang_0179" );
}

string CLanguage :: MapDownloadsEnabled( )
{
	return m_CFG->GetString( "lang_0180", "lang_0180" );
}

string CLanguage :: MapDownloadsConditional( )
{
	return m_CFG->GetString( "lang_0181", "lang_0181" );
}

string CLanguage :: SettingHCL( string HCL )
{
	string Out = m_CFG->GetString( "lang_0182", "lang_0182" );
	UTIL_Replace( Out, "$HCL$", HCL );
	return Out;
}

string CLanguage :: UnableToSetHCLInvalid( )
{
	return m_CFG->GetString( "lang_0183", "lang_0183" );
}

string CLanguage :: UnableToSetHCLTooLong( )
{
	return m_CFG->GetString( "lang_0184", "lang_0184" );
}

string CLanguage :: TheHCLIs( string HCL )
{
	string Out = m_CFG->GetString( "lang_0185", "lang_0185" );
	UTIL_Replace( Out, "$HCL$", HCL );
	return Out;
}

string CLanguage :: TheHCLIsTooLongUseForceToStart( )
{
	return m_CFG->GetString( "lang_0186", "lang_0186" );
}

string CLanguage :: ClearingHCL( )
{
	return m_CFG->GetString( "lang_0187", "lang_0187" );
}

string CLanguage :: TryingToRehostAsPrivateGame( string gamename )
{
	string Out = m_CFG->GetString( "lang_0188", "lang_0188" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: TryingToRehostAsPublicGame( string gamename )
{
	string Out = m_CFG->GetString( "lang_0189", "lang_0189" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: RehostWasSuccessful( )
{
	return m_CFG->GetString( "lang_0190", "lang_0190" );
}

string CLanguage :: TryingToJoinTheGameButBannedByName( string victim )
{
	string Out = m_CFG->GetString( "lang_0191", "lang_0191" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: TryingToJoinTheGameButBannedByIP( string victim, string ip, string bannedname )
{
	string Out = m_CFG->GetString( "lang_0192", "lang_0192" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$IP$", ip );
	UTIL_Replace( Out, "$BANNEDNAME$", bannedname );
	return Out;
}

string CLanguage :: HasBannedName( string victim )
{
	string Out = m_CFG->GetString( "lang_0193", "lang_0193" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: HasBannedIP( string victim, string ip, string bannedname )
{
	string Out = m_CFG->GetString( "lang_0194", "lang_0194" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$IP$", ip );
	UTIL_Replace( Out, "$BANNEDNAME$", bannedname );
	return Out;
}

string CLanguage :: PlayersInGameState( string number, string players )
{
	string Out = m_CFG->GetString( "lang_0195", "lang_0195" );
	UTIL_Replace( Out, "$NUMBER$", number );
	UTIL_Replace( Out, "$PLAYERS$", players );
	return Out;
}

string CLanguage :: ValidServers( string servers )
{
	string Out = m_CFG->GetString( "lang_0196", "lang_0196" );
	UTIL_Replace( Out, "$SERVERS$", servers );
	return Out;
}

string CLanguage :: TeamCombinedScore( string team, string score )
{
	string Out = m_CFG->GetString( "lang_0197", "lang_0197" );
	UTIL_Replace( Out, "$TEAM$", team );
	UTIL_Replace( Out, "$SCORE$", score );
	return Out;
}

string CLanguage :: BalancingSlotsCompleted( )
{
	return m_CFG->GetString( "lang_0198", "lang_0198" );
}

string CLanguage :: PlayerWasKickedForFurthestScore( string name, string score, string average )
{
	string Out = m_CFG->GetString( "lang_0199", "lang_0199" );
	UTIL_Replace( Out, "$NAME$", name );
	UTIL_Replace( Out, "$SCORE$", score );
	UTIL_Replace( Out, "$AVERAGE$", average );
	return Out;
}

string CLanguage :: LocalAdminMessagesEnabled( )
{
	return m_CFG->GetString( "lang_0200", "lang_0200" );
}

string CLanguage :: LocalAdminMessagesDisabled( )
{
	return m_CFG->GetString( "lang_0201", "lang_0201" );
}

string CLanguage :: WasDroppedDesync( )
{
	return m_CFG->GetString( "lang_0202", "lang_0202" );
}

string CLanguage :: WasKickedForHavingLowestScore( string score )
{
	string Out = m_CFG->GetString( "lang_0203", "lang_0203" );
	UTIL_Replace( Out, "$SCORE$", score );
	return Out;
}

string CLanguage :: PlayerWasKickedForLowestScore( string name, string score )
{
	string Out = m_CFG->GetString( "lang_0204", "lang_0204" );
	UTIL_Replace( Out, "$NAME$", name );
	UTIL_Replace( Out, "$SCORE$", score );
	return Out;
}

string CLanguage :: ReloadingConfigurationFiles( )
{
	return m_CFG->GetString( "lang_0205", "lang_0205" );
}

string CLanguage :: CountDownAbortedSomeoneLeftRecently( )
{
	return m_CFG->GetString( "lang_0206", "lang_0206" );
}

string CLanguage :: UnableToCreateGameMustEnforceFirst( string gamename )
{
	string Out = m_CFG->GetString( "lang_0207", "lang_0207" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: UnableToLoadReplaysOutside( )
{
	return m_CFG->GetString( "lang_0208", "lang_0208" );
}

string CLanguage :: LoadingReplay( string file )
{
	string Out = m_CFG->GetString( "lang_0209", "lang_0209" );
	UTIL_Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage :: UnableToLoadReplayDoesntExist( string file )
{
	string Out = m_CFG->GetString( "lang_0210", "lang_0210" );
	UTIL_Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage :: CommandTrigger( string trigger )
{
	string Out = m_CFG->GetString( "lang_0211", "lang_0211" );
	UTIL_Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage :: CantEndGameOwnerIsStillPlaying( string owner )
{
	string Out = m_CFG->GetString( "lang_0212", "lang_0212" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: CantUnhostGameOwnerIsPresent( string owner )
{
	string Out = m_CFG->GetString( "lang_0213", "lang_0213" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: WasAutomaticallyDroppedAfterSeconds( string seconds )
{
	string Out = m_CFG->GetString( "lang_0214", "lang_0214" );
	UTIL_Replace( Out, "$SECONDS$", seconds );
	return Out;
}

string CLanguage :: HasLostConnectionTimedOutGProxy( )
{
	return m_CFG->GetString( "lang_0215", "lang_0215" );
}

string CLanguage :: HasLostConnectionSocketErrorGProxy( string error )
{
	string Out = m_CFG->GetString( "lang_0216", "lang_0216" );
	UTIL_Replace( Out, "$ERROR$", error );
	return Out;
}

string CLanguage :: HasLostConnectionClosedByRemoteHostGProxy( )
{
	return m_CFG->GetString( "lang_0217", "lang_0217" );
}

string CLanguage :: WaitForReconnectSecondsRemain( string seconds )
{
	string Out = m_CFG->GetString( "lang_0218", "lang_0218" );
	UTIL_Replace( Out, "$SECONDS$", seconds );
	return Out;
}

string CLanguage :: WasUnrecoverablyDroppedFromGProxy( )
{
	return m_CFG->GetString( "lang_0219", "lang_0219" );
}

string CLanguage :: PlayerReconnectedWithGProxy( string name )
{
	string Out = m_CFG->GetString( "lang_0220", "lang_0220" );
	UTIL_Replace( Out, "$NAME$", name );
	return Out;
}
