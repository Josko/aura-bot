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

CLanguage :: CLanguage( const string & nCFGFile )
{
	m_CFG = new CConfig( );
	m_CFG->Read( nCFGFile );
}

CLanguage :: ~CLanguage( )
{
	delete m_CFG;
}

string CLanguage :: UnableToCreateGameTryAnotherName( const string &server, const string &gamename )
{
	string Out = m_CFG->GetString( "lang_0001", "lang_0001" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: UserIsAlreadyAnAdmin( const string &server, const string &user )
{
	string Out = m_CFG->GetString( "lang_0002", "lang_0002" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: AddedUserToAdminDatabase( const string &server, const string &user )
{
	string Out = m_CFG->GetString( "lang_0003", "lang_0003" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: ErrorAddingUserToAdminDatabase( const string &server, const string &user )
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

string CLanguage :: UserIsAlreadyBanned( const string &server, const string &victim )
{
	string Out = m_CFG->GetString( "lang_0006", "lang_0006" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: BannedUser( const string & server, const string & victim )
{
	string Out = m_CFG->GetString( "lang_0007", "lang_0007" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: ErrorBanningUser( const string & server, const string & victim )
{
	string Out = m_CFG->GetString( "lang_0008", "lang_0008" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UserIsAnAdmin( const string & server, const string & user )
{
	string Out = m_CFG->GetString( "lang_0009", "lang_0009" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UserIsNotAnAdmin( const string & server, const string & user )
{
	string Out = m_CFG->GetString( "lang_0010", "lang_0010" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UserWasBannedOnByBecause( const string & server, const string & victim, const string & date, const string & admin, const string & reason )
{
	string Out = m_CFG->GetString( "lang_0011", "lang_0011" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$DATE$", date );
	UTIL_Replace( Out, "$ADMIN$", admin );
	UTIL_Replace( Out, "$REASON$", reason );
	return Out;
}

string CLanguage :: UserIsNotBanned( const string & server, const string & victim )
{
	string Out = m_CFG->GetString( "lang_0012", "lang_0012" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: ThereAreNoAdmins( const string & server )
{
	string Out = m_CFG->GetString( "lang_0013", "lang_0013" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ThereIsAdmin( const string & server )
{
	string Out = m_CFG->GetString( "lang_0014", "lang_0014" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ThereAreAdmins( const string & server, const string & count )
{
	string Out = m_CFG->GetString( "lang_0015", "lang_0015" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$COUNT$", count );
	return Out;
}

string CLanguage :: ThereAreNoBannedUsers( const string & server )
{
	string Out = m_CFG->GetString( "lang_0016", "lang_0016" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ThereIsBannedUser( const string & server )
{
	string Out = m_CFG->GetString( "lang_0017", "lang_0017" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ThereAreBannedUsers( const string & server, const string & count )
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

string CLanguage :: DeletedUserFromAdminDatabase( const string & server, const string & user )
{
	string Out = m_CFG->GetString( "lang_0020", "lang_0020" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: ErrorDeletingUserFromAdminDatabase( const string & server, const string & user )
{
	string Out = m_CFG->GetString( "lang_0021", "lang_0021" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnbannedUser( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0022", "lang_0022" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: ErrorUnbanningUser( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0023", "lang_0023" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: GameNumberIs( const string & number, const string & description )
{
	string Out = m_CFG->GetString( "lang_0024", "lang_0024" );
	UTIL_Replace( Out, "$NUMBER$", number );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: GameNumberDoesntExist( const string & number )
{
	string Out = m_CFG->GetString( "lang_0025", "lang_0025" );
	UTIL_Replace( Out, "$NUMBER$", number );
	return Out;
}

string CLanguage :: GameIsInTheLobby( const string & description, const string & current, const string & max )
{
	string Out = m_CFG->GetString( "lang_0026", "lang_0026" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	UTIL_Replace( Out, "$CURRENT$", current );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: ThereIsNoGameInTheLobby( const string & current, const string & max )
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

string CLanguage :: LoadingConfigFile( const string & file )
{
	string Out = m_CFG->GetString( "lang_0029", "lang_0029" );
	UTIL_Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage :: UnableToLoadConfigFileDoesntExist( const string & file )
{
	string Out = m_CFG->GetString( "lang_0030", "lang_0030" );
	UTIL_Replace( Out, "$FILE$", file );
	return Out;
}

string CLanguage :: CreatingPrivateGame( const string & gamename, const string & user )
{
	string Out = m_CFG->GetString( "lang_0031", "lang_0031" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: CreatingPublicGame( const string & gamename, const string & user )
{
	string Out = m_CFG->GetString( "lang_0032", "lang_0032" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnableToUnhostGameCountdownStarted( const string & description )
{
	string Out = m_CFG->GetString( "lang_0033", "lang_0033" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: UnhostingGame( const string & description )
{
	string Out = m_CFG->GetString( "lang_0034", "lang_0034" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: UnableToUnhostGameNoGameInLobby( )
{
	return m_CFG->GetString( "lang_0035", "lang_0035" );
}

string CLanguage :: VersionAdmin( const string & version )
{
	string Out = m_CFG->GetString( "lang_0036", "lang_0036" );
	UTIL_Replace( Out, "$VERSION$", version );
	return Out;
}

string CLanguage :: VersionNotAdmin( const string & version )
{
	string Out = m_CFG->GetString( "lang_0037", "lang_0037" );
	UTIL_Replace( Out, "$VERSION$", version );
	return Out;
}

string CLanguage :: UnableToCreateGameAnotherGameInLobby( const string & gamename, const string & description )
{
	string Out = m_CFG->GetString( "lang_0038", "lang_0038" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: UnableToCreateGameMaxGamesReached( const string & gamename, const string & max )
{
	string Out = m_CFG->GetString( "lang_0039", "lang_0039" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: GameIsOver( const string & description )
{
	string Out = m_CFG->GetString( "lang_0040", "lang_0040" );
	UTIL_Replace( Out, "$DESCRIPTION$", description );
	return Out;
}

string CLanguage :: SpoofCheckByReplying( )
{
	return m_CFG->GetString( "lang_0041", "lang_0041" );
}

string CLanguage :: SpoofPossibleIsAway( const string & user )
{
	string Out = m_CFG->GetString( "lang_0043", "lang_0043" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofPossibleIsUnavailable( const string & user )
{
	string Out = m_CFG->GetString( "lang_0044", "lang_0044" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofPossibleIsRefusingMessages( const string & user )
{
	string Out = m_CFG->GetString( "lang_0045", "lang_0045" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofDetectedIsNotInGame( const string & user )
{
	string Out = m_CFG->GetString( "lang_0046", "lang_0046" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofDetectedIsInPrivateChannel( const string & user )
{
	string Out = m_CFG->GetString( "lang_0047", "lang_0047" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: SpoofDetectedIsInAnotherGame( const string & user )
{
	string Out = m_CFG->GetString( "lang_0048", "lang_0048" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: CountDownAborted( )
{
	return m_CFG->GetString( "lang_0049", "lang_0049" );
}

string CLanguage :: TryingToJoinTheGameButBanned( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0050", "lang_0050" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToBanNoMatchesFound( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0051", "lang_0051" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: PlayerWasBannedByPlayer( const string & server, const string & victim, const string & user )
{
	string Out = m_CFG->GetString( "lang_0052", "lang_0052" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnableToBanFoundMoreThanOneMatch( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0053", "lang_0053" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: AddedPlayerToTheHoldList( const string & user )
{
	string Out = m_CFG->GetString( "lang_0054", "lang_0054" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnableToKickNoMatchesFound( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0055", "lang_0055" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToKickFoundMoreThanOneMatch( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0056", "lang_0056" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: SettingLatencyToMinimum( const string & min )
{
	string Out = m_CFG->GetString( "lang_0057", "lang_0057" );
	UTIL_Replace( Out, "$MIN$", min );
	return Out;
}

string CLanguage :: SettingLatencyToMaximum( const string & max )
{
	string Out = m_CFG->GetString( "lang_0058", "lang_0058" );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: SettingLatencyTo( const string & latency )
{
	string Out = m_CFG->GetString( "lang_0059", "lang_0059" );
	UTIL_Replace( Out, "$LATENCY$", latency );
	return Out;
}

string CLanguage :: KickingPlayersWithPingsGreaterThan( const string & total, const string & ping )
{
	string Out = m_CFG->GetString( "lang_0060", "lang_0060" );
	UTIL_Replace( Out, "$TOTAL$", total );
	UTIL_Replace( Out, "$PING$", ping );
	return Out;
}

string CLanguage :: HasPlayedGamesWithThisBot( const string & user, const string & firstgame, const string & lastgame, const string & totalgames, const string & avgloadingtime, const string & avgstay )
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

string CLanguage :: HasntPlayedGamesWithThisBot( const string & user )
{
	string Out = m_CFG->GetString( "lang_0062", "lang_0062" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: AutokickingPlayerForExcessivePing( const string & victim, const string & ping )
{
	string Out = m_CFG->GetString( "lang_0063", "lang_0063" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$PING$", ping );
	return Out;
}

string CLanguage :: SpoofCheckAcceptedFor( const string & server, const string & user )
{
	string Out = m_CFG->GetString( "lang_0064", "lang_0064" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: PlayersNotYetSpoofChecked( const string & notspoofchecked )
{
	string Out = m_CFG->GetString( "lang_0065", "lang_0065" );
	UTIL_Replace( Out, "$NOTSPOOFCHECKED$", notspoofchecked );
	return Out;
}

string CLanguage :: ManuallySpoofCheckByWhispering( const string & hostname )
{
	string Out = m_CFG->GetString( "lang_0066", "lang_0066" );
	UTIL_Replace( Out, "$HOSTNAME$", hostname );
	return Out;
}

string CLanguage :: SpoofCheckByWhispering( const string & hostname )
{
	string Out = m_CFG->GetString( "lang_0067", "lang_0067" );
	UTIL_Replace( Out, "$HOSTNAME$", hostname );
	return Out;
}

string CLanguage :: EveryoneHasBeenSpoofChecked( )
{
	return m_CFG->GetString( "lang_0068", "lang_0068" );
}

string CLanguage :: PlayersNotYetPinged( const string & notpinged )
{
	string Out = m_CFG->GetString( "lang_0069", "lang_0069" );
	UTIL_Replace( Out, "$NOTPINGED$", notpinged );
	return Out;
}

string CLanguage :: EveryoneHasBeenPinged( )
{
	return m_CFG->GetString( "lang_0070", "lang_0070" );
}

string CLanguage :: ShortestLoadByPlayer( const string & user, const string & loadingtime )
{
	string Out = m_CFG->GetString( "lang_0071", "lang_0071" );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage :: LongestLoadByPlayer( const string & user, const string & loadingtime )
{
	string Out = m_CFG->GetString( "lang_0072", "lang_0072" );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage :: YourLoadingTimeWas( const string & loadingtime )
{
	string Out = m_CFG->GetString( "lang_0073", "lang_0073" );
	UTIL_Replace( Out, "$LOADINGTIME$", loadingtime );
	return Out;
}

string CLanguage :: HasPlayedDotAGamesWithThisBot( const string & user, const string & totalgames, const string & totalwins, const string & totallosses, const string & totalkills, const string & totaldeaths, const string & totalcreepkills, const string & totalcreepdenies, const string & totalassists, const string & totalneutralkills, const string & totaltowerkills, const string & totalraxkills, const string & totalcourierkills, const string & avgkills, const string & avgdeaths, const string & avgcreepkills, const string & avgcreepdenies, const string & avgassists, const string & avgneutralkills, const string & avgtowerkills, const string & avgraxkills, const string & avgcourierkills )
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

string CLanguage :: HasntPlayedDotAGamesWithThisBot( const string & user )
{
	string Out = m_CFG->GetString( "lang_0075", "lang_0075" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: WasKickedForReservedPlayer( const string & reserved )
{
	string Out = m_CFG->GetString( "lang_0076", "lang_0076" );
	UTIL_Replace( Out, "$RESERVED$", reserved );
	return Out;
}

string CLanguage :: WasKickedForOwnerPlayer( const string & owner )
{
	string Out = m_CFG->GetString( "lang_0077", "lang_0077" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: WasKickedByPlayer( const string & user )
{
	string Out = m_CFG->GetString( "lang_0078", "lang_0078" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: HasLostConnectionPlayerError( const string & error )
{
	string Out = m_CFG->GetString( "lang_0079", "lang_0079" );
	UTIL_Replace( Out, "$ERROR$", error );
	return Out;
}

string CLanguage :: HasLostConnectionSocketError( const string & error )
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

string CLanguage :: EndingGame( const string & description )
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

string CLanguage :: PlayersStillDownloading( const string & stilldownloading )
{
	string Out = m_CFG->GetString( "lang_0089", "lang_0089" );
	UTIL_Replace( Out, "$STILLDOWNLOADING$", stilldownloading );
	return Out;
}

string CLanguage :: AtLeastOneGameActiveUseForceToShutdown( )
{
	return m_CFG->GetString( "lang_0092", "lang_0092" );
}

string CLanguage :: CurrentlyLoadedMapCFGIs( const string & mapcfg )
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

string CLanguage :: PlayerVotedToDropLaggers( const string & user )
{
	string Out = m_CFG->GetString( "lang_0096", "lang_0096" );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: LatencyIs( const string & latency )
{
	string Out = m_CFG->GetString( "lang_0097", "lang_0097" );
	UTIL_Replace( Out, "$LATENCY$", latency );
	return Out;
}

string CLanguage :: SyncLimitIs( const string & synclimit )
{
	string Out = m_CFG->GetString( "lang_0098", "lang_0098" );
	UTIL_Replace( Out, "$SYNCLIMIT$", synclimit );
	return Out;
}

string CLanguage :: SettingSyncLimitToMinimum( const string & min )
{
	string Out = m_CFG->GetString( "lang_0099", "lang_0099" );
	UTIL_Replace( Out, "$MIN$", min );
	return Out;
}

string CLanguage :: SettingSyncLimitToMaximum( const string & max )
{
	string Out = m_CFG->GetString( "lang_0100", "lang_0100" );
	UTIL_Replace( Out, "$MAX$", max );
	return Out;
}

string CLanguage :: SettingSyncLimitTo( const string & synclimit )
{
	string Out = m_CFG->GetString( "lang_0101", "lang_0101" );
	UTIL_Replace( Out, "$SYNCLIMIT$", synclimit );
	return Out;
}

string CLanguage :: UnableToCreateGameNotLoggedIn( const string & gamename )
{
	string Out = m_CFG->GetString( "lang_0102", "lang_0102" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: ConnectingToBNET( const string & server )
{
	string Out = m_CFG->GetString( "lang_0105", "lang_0105" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: ConnectedToBNET( const string & server )
{
	string Out = m_CFG->GetString( "lang_0106", "lang_0106" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: DisconnectedFromBNET( const string & server )
{
	string Out = m_CFG->GetString( "lang_0107", "lang_0107" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: LoggedInToBNET( const string & server )
{
	string Out = m_CFG->GetString( "lang_0108", "lang_0108" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: BNETGameHostingSucceeded( const string & server )
{
	string Out = m_CFG->GetString( "lang_0109", "lang_0109" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: BNETGameHostingFailed( const string & server, const string & gamename )
{
	string Out = m_CFG->GetString( "lang_0110", "lang_0110" );
	UTIL_Replace( Out, "$SERVER$", server );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: ConnectingToBNETTimedOut( const string & server )
{
	string Out = m_CFG->GetString( "lang_0111", "lang_0111" );
	UTIL_Replace( Out, "$SERVER$", server );
	return Out;
}

string CLanguage :: PlayerDownloadedTheMap( const string & user, const string & seconds, const string & rate )
{
	string Out = m_CFG->GetString( "lang_0112", "lang_0112" );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$SECONDS$", seconds );
	UTIL_Replace( Out, "$RATE$", rate );
	return Out;
}

string CLanguage :: UnableToCreateGameNameTooLong( const string & gamename )
{
	string Out = m_CFG->GetString( "lang_0113", "lang_0113" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: SettingGameOwnerTo( const string & owner )
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

string CLanguage :: UnableToStartDownloadNoMatchesFound( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0118", "lang_0118" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToStartDownloadFoundMoreThanOneMatch( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0119", "lang_0119" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToSetGameOwner( const string & owner )
{
	string Out = m_CFG->GetString( "lang_0120", "lang_0120" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: UnableToCheckPlayerNoMatchesFound( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0121", "lang_0121" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: CheckedPlayer( const string & victim, const string & ping, const string & from, const string & admin, const string & owner, const string & spoofed, const string & spoofedrealm, const string & reserved )
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

string CLanguage :: UnableToCheckPlayerFoundMoreThanOneMatch( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0123", "lang_0123" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: TheGameIsLockedBNET( )
{
	return m_CFG->GetString( "lang_0124", "lang_0124" );
}

string CLanguage :: UnableToCreateGameDisabled( const string & gamename )
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

string CLanguage :: UnableToCreateGameInvalidMap( const string & gamename )
{
	string Out = m_CFG->GetString( "lang_0128", "lang_0128" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: DesyncDetected( )
{
	return m_CFG->GetString( "lang_0144", "lang_0144" );
}

string CLanguage :: UnableToMuteNoMatchesFound( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0145", "lang_0145" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: MutedPlayer( const string & victim, const string & user )
{
	string Out = m_CFG->GetString( "lang_0146", "lang_0146" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnmutedPlayer( const string & victim, const string & user )
{
	string Out = m_CFG->GetString( "lang_0147", "lang_0147" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	return Out;
}

string CLanguage :: UnableToMuteFoundMoreThanOneMatch( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0148", "lang_0148" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: PlayerIsSavingTheGame( const string & player )
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

string CLanguage :: MultipleIPAddressUsageDetected( const string & player, const string & others )
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

string CLanguage :: UnableToVoteKickNoMatchesFound( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0155", "lang_0155" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: UnableToVoteKickPlayerIsReserved( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0156", "lang_0156" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: StartedVoteKick( const string & victim, const string & user, const string & votesneeded )
{
	string Out = m_CFG->GetString( "lang_0157", "lang_0157" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$VOTESNEEDED$", votesneeded );
	return Out;
}

string CLanguage :: UnableToVoteKickFoundMoreThanOneMatch( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0158", "lang_0158" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: VoteKickPassed( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0159", "lang_0159" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: ErrorVoteKickingPlayer( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0160", "lang_0160" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: VoteKickAcceptedNeedMoreVotes( const string & victim, const string & user, const string & votes )
{
	string Out = m_CFG->GetString( "lang_0161", "lang_0161" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$USER$", user );
	UTIL_Replace( Out, "$VOTES$", votes );
	return Out;
}

string CLanguage :: VoteKickCancelled( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0162", "lang_0162" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: VoteKickExpired( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0163", "lang_0163" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: WasKickedByVote( )
{
	return m_CFG->GetString( "lang_0164", "lang_0164" );
}

string CLanguage :: TypeYesToVote( const string & commandtrigger )
{
	string Out = m_CFG->GetString( "lang_0165", "lang_0165" );
	UTIL_Replace( Out, "$COMMANDTRIGGER$", commandtrigger );
	return Out;
}

string CLanguage :: WasKickedForNotSpoofChecking( )
{
	return m_CFG->GetString( "lang_0167", "lang_0167" );
}

string CLanguage :: ErrorListingMaps( )
{
	return m_CFG->GetString( "lang_0171", "lang_0171" );
}

string CLanguage :: FoundMaps( const string & maps )
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

string CLanguage :: FoundMapConfigs( const string & mapconfigs )
{
	string Out = m_CFG->GetString( "lang_0175", "lang_0175" );
	UTIL_Replace( Out, "$MAPCONFIGS$", mapconfigs );
	return Out;
}

string CLanguage :: NoMapConfigsFound( )
{
	return m_CFG->GetString( "lang_0176", "lang_0176" );
}

string CLanguage :: PlayerFinishedLoading( const string & user )
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

string CLanguage :: SettingHCL( const string & HCL )
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

string CLanguage :: TheHCLIs( const string & HCL )
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

string CLanguage :: TryingToRehostAsPrivateGame( const string & gamename )
{
	string Out = m_CFG->GetString( "lang_0188", "lang_0188" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: TryingToRehostAsPublicGame( const string & gamename )
{
	string Out = m_CFG->GetString( "lang_0189", "lang_0189" );
	UTIL_Replace( Out, "$GAMENAME$", gamename );
	return Out;
}

string CLanguage :: RehostWasSuccessful( )
{
	return m_CFG->GetString( "lang_0190", "lang_0190" );
}

string CLanguage :: TryingToJoinTheGameButBannedByName( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0191", "lang_0191" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: TryingToJoinTheGameButBannedByIP( const string & victim, const string & ip, const string & bannedname )
{
	string Out = m_CFG->GetString( "lang_0192", "lang_0192" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$IP$", ip );
	UTIL_Replace( Out, "$BANNEDNAME$", bannedname );
	return Out;
}

string CLanguage :: HasBannedName( const string & victim )
{
	string Out = m_CFG->GetString( "lang_0193", "lang_0193" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	return Out;
}

string CLanguage :: HasBannedIP( const string & victim, const string & ip, const string & bannedname )
{
	string Out = m_CFG->GetString( "lang_0194", "lang_0194" );
	UTIL_Replace( Out, "$VICTIM$", victim );
	UTIL_Replace( Out, "$IP$", ip );
	UTIL_Replace( Out, "$BANNEDNAME$", bannedname );
	return Out;
}

string CLanguage :: PlayersInGameState( const string & number, const string & players )
{
	string Out = m_CFG->GetString( "lang_0195", "lang_0195" );
	UTIL_Replace( Out, "$NUMBER$", number );
	UTIL_Replace( Out, "$PLAYERS$", players );
	return Out;
}

string CLanguage :: ValidServers( const string & servers )
{
	string Out = m_CFG->GetString( "lang_0196", "lang_0196" );
	UTIL_Replace( Out, "$SERVERS$", servers );
	return Out;
}

string CLanguage :: WasDroppedDesync( )
{
	return m_CFG->GetString( "lang_0202", "lang_0202" );
}

string CLanguage :: ReloadingConfigurationFiles( )
{
	return m_CFG->GetString( "lang_0205", "lang_0205" );
}

string CLanguage :: CountDownAbortedSomeoneLeftRecently( )
{
	return m_CFG->GetString( "lang_0206", "lang_0206" );
}

string CLanguage :: CommandTrigger( const string & trigger )
{
	string Out = m_CFG->GetString( "lang_0211", "lang_0211" );
	UTIL_Replace( Out, "$TRIGGER$", trigger );
	return Out;
}

string CLanguage :: CantEndGameOwnerIsStillPlaying( const string & owner )
{
	string Out = m_CFG->GetString( "lang_0212", "lang_0212" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: CantUnhostGameOwnerIsPresent( const string & owner )
{
	string Out = m_CFG->GetString( "lang_0213", "lang_0213" );
	UTIL_Replace( Out, "$OWNER$", owner );
	return Out;
}

string CLanguage :: WasAutomaticallyDroppedAfterSeconds( const string & seconds )
{
	string Out = m_CFG->GetString( "lang_0214", "lang_0214" );
	UTIL_Replace( Out, "$SECONDS$", seconds );
	return Out;
}

string CLanguage :: HasLostConnectionTimedOutGProxy( )
{
	return m_CFG->GetString( "lang_0215", "lang_0215" );
}

string CLanguage :: HasLostConnectionSocketErrorGProxy( const string & error )
{
	string Out = m_CFG->GetString( "lang_0216", "lang_0216" );
	UTIL_Replace( Out, "$ERROR$", error );
	return Out;
}

string CLanguage :: HasLostConnectionClosedByRemoteHostGProxy( )
{
	return m_CFG->GetString( "lang_0217", "lang_0217" );
}

string CLanguage :: WaitForReconnectSecondsRemain( const string & seconds )
{
	string Out = m_CFG->GetString( "lang_0218", "lang_0218" );
	UTIL_Replace( Out, "$SECONDS$", seconds );
	return Out;
}

string CLanguage :: WasUnrecoverablyDroppedFromGProxy( )
{
	return m_CFG->GetString( "lang_0219", "lang_0219" );
}

string CLanguage :: PlayerReconnectedWithGProxy( const string & name )
{
	string Out = m_CFG->GetString( "lang_0220", "lang_0220" );
	UTIL_Replace( Out, "$NAME$", name );
	return Out;
}
