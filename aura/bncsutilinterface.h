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

#ifndef BNCSUTIL_INTERFACE_H_
#define BNCSUTIL_INTERFACE_H_

//
// CBNCSUtilInterface
//

class CBNCSUtilInterface
{
private:
  void *m_NLS;
  BYTEARRAY m_EXEVersion;         // set in HELP_SID_AUTH_CHECK
  BYTEARRAY m_EXEVersionHash;     // set in HELP_SID_AUTH_CHECK  
  BYTEARRAY m_KeyInfoROC;         // set in HELP_SID_AUTH_CHECK
  BYTEARRAY m_KeyInfoTFT;         // set in HELP_SID_AUTH_CHECK
  BYTEARRAY m_ClientKey;          // set in HELP_SID_AUTH_ACCOUNTLOGON
  BYTEARRAY m_M1;                 // set in HELP_SID_AUTH_ACCOUNTLOGONPROOF
  BYTEARRAY m_PvPGNPasswordHash;  // set in HELP_PvPGNPasswordHash
  string m_EXEInfo;               // set in HELP_SID_AUTH_CHECK

public:
  CBNCSUtilInterface( const string &userName, const string &userPassword );
  ~CBNCSUtilInterface( );

  inline BYTEARRAY GetEXEVersion( ) const               { return m_EXEVersion; }
  inline BYTEARRAY GetEXEVersionHash( ) const           { return m_EXEVersionHash; }
  inline string GetEXEInfo( ) const                     { return m_EXEInfo; }
  inline BYTEARRAY GetKeyInfoROC( ) const               { return m_KeyInfoROC; }
  inline BYTEARRAY GetKeyInfoTFT( ) const               { return m_KeyInfoTFT; }
  inline BYTEARRAY GetClientKey( ) const                { return m_ClientKey; }
  inline BYTEARRAY GetM1( ) const                       { return m_M1; }
  inline BYTEARRAY GetPvPGNPasswordHash( ) const        { return m_PvPGNPasswordHash; }
  
  inline void SetEXEVersion( BYTEARRAY &nEXEVersion )         { m_EXEVersion = nEXEVersion; }
  inline void SetEXEVersionHash( BYTEARRAY &nEXEVersionHash ) { m_EXEVersionHash = nEXEVersionHash; }

  void Reset( string &userName, string &userPassword );

  bool HELP_SID_AUTH_CHECK( string &war3Path, string &keyROC, string &keyTFT, const string &valueStringFormula, const string &mpqFileName, const BYTEARRAY &clientToken, const BYTEARRAY &serverToken );
  bool HELP_SID_AUTH_ACCOUNTLOGON( );
  bool HELP_SID_AUTH_ACCOUNTLOGONPROOF( const BYTEARRAY &salt, const BYTEARRAY &serverKey );
  bool HELP_PvPGNPasswordHash( string &userPassword );

private:
  BYTEARRAY CreateKeyInfo( string &key, uint32_t clientToken, uint32_t serverToken );
};

#endif
