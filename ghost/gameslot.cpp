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
#include "gameslot.h"

//
// CGameSlot
//

CGameSlot :: CGameSlot( BYTEARRAY &n ) : m_PID( 0 ), m_DownloadStatus( 255 ), m_SlotStatus( SLOTSTATUS_OPEN ), m_Computer( 0 ), m_Team( 0 ), m_Colour( 1 ), m_Race( SLOTRACE_RANDOM ), m_ComputerType( SLOTCOMP_NORMAL ), m_Handicap( 100 )
{
	if( n.size( ) >= 7 )
	{
		m_PID = n[0];
		m_DownloadStatus = n[1];
		m_SlotStatus = n[2];
		m_Computer = n[3];
		m_Team = n[4];
		m_Colour = n[5];
		m_Race = n[6];

		if( n.size( ) >= 8 )
			m_ComputerType = n[7];

		if( n.size( ) >= 9 )
			m_Handicap = n[8];
	}
}

CGameSlot :: CGameSlot( unsigned char nPID, unsigned char nDownloadStatus, unsigned char nSlotStatus, unsigned char nComputer, unsigned char nTeam, unsigned char nColour, unsigned char nRace, unsigned char nComputerType, unsigned char nHandicap ) : m_PID( nPID ), m_DownloadStatus( nDownloadStatus ), m_SlotStatus( nSlotStatus ), m_Computer( nComputer ), m_Team( nTeam ), m_Colour( nColour ), m_Race( nRace ), m_ComputerType( nComputerType ), m_Handicap( nHandicap )
{

}

CGameSlot :: ~CGameSlot( )
{

}

BYTEARRAY CGameSlot :: GetByteArray( ) const
{
	BYTEARRAY b;
	b.push_back( m_PID );
	b.push_back( m_DownloadStatus );
	b.push_back( m_SlotStatus );
	b.push_back( m_Computer );
	b.push_back( m_Team );
	b.push_back( m_Colour );
	b.push_back( m_Race );
	b.push_back( m_ComputerType );
	b.push_back( m_Handicap );
	return b;
}
