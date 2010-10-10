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
#include "commandpacket.h"

//
// CCommandPacket
//

CCommandPacket :: CCommandPacket( unsigned char nPacketType, int nID, BYTEARRAY nData ) : m_PacketType( nPacketType ), m_ID( nID ), m_Data( nData )
{

}

CCommandPacket :: ~CCommandPacket( )
{

}
