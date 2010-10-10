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

#ifndef STATS_H
#define STATS_H

//
// CStats
//

// the stats class is passed a copy of every player action in ProcessAction when it's received
// then when the game is over the Save function is called
// so the idea is that you parse the actions to gather data about the game, storing the results in any member variables you need in your subclass
// and in the Save function you write the results to the database
// e.g. for dota the number of kills/deaths/assists, etc...
// the base class is almost completely empty

class CIncomingAction;
class CGHostDB;

class CStats
{
protected:
	CBaseGame *m_Game;

public:
	CStats( CBaseGame *nGame );
	virtual ~CStats( );

	virtual bool ProcessAction( CIncomingAction *Action );
	virtual void Save( CGHost *GHost, CGHostDB *DB, uint32_t GameID );
};

#endif
