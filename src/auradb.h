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

#ifndef AURA_AURADB_H_
#define AURA_AURADB_H_

#include "sqlite3.h"

/**************
 *** SCHEMA ***
 **************

CREATE TABLE admins (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    server TEXT NOT NULL DEFAULT ""
)

CREATE TABLE bans (
    id INTEGER PRIMARY KEY,
    server TEXT NOT NULL,
    name TEXT NOT NULL,
    date TEXT NOT NULL,
    admin TEXT NOT NULL,
    reason TEXT
)

CREATE TABLE players (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    games INTEGER,
    dotas INTEGER,
    loadingtime INTEGER,
    duration INTEGER,
    left INTEGER,
    wins INTEGER,
    losses INTEGER,
    kills INTEGER,
    deaths INTEGER,
    creepkills INTEGER,
    creepdenies INTEGER,
    assists INTEGER,
    neutralkills INTEGER,
    towerkills INTEGER,
    raxkills INTEGER,
    courierkills INTEGER
)

CREATE TABLE config (
    name TEXT NOT NULL PRIMARY KEY,
    value TEXT NOT NULL
)

CREATE TEMPORARY TABLE iptocountry (
    ip1 INTEGER NOT NULL,
    ip2 INTEGER NOT NULL,
    country TEXT NOT NULL,
    PRIMARY KEY ( ip1, ip2 )
)

CREATE TEMPORARY TABLE rootadmins (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    server TEXT NOT NULL DEFAULT ""
)

 **************
 *** SCHEMA ***
 **************/

//
// CSQLITE3 (wrapper class)
//

#include "includes.h"

struct sqlite3;
struct sqlite3_stmt;

class CSQLITE3
{
private:
  void* m_DB;
  bool  m_Ready;

public:
  explicit CSQLITE3(const std::string& filename);
  ~CSQLITE3();
  CSQLITE3(CSQLITE3&) = delete;

  inline bool        GetReady() const { return m_Ready; }
  inline std::string GetError() const { return sqlite3_errmsg((sqlite3*)m_DB); }

  inline int_fast32_t Step(void* Statement) { return sqlite3_step((sqlite3_stmt*)Statement); }
  inline int_fast32_t Prepare(const std::string& query, void** Statement) { return sqlite3_prepare_v2((sqlite3*)m_DB, query.c_str(), -1, (sqlite3_stmt**)Statement, nullptr); }
  inline int_fast32_t Finalize(void* Statement) { return sqlite3_finalize((sqlite3_stmt*)Statement); }
  inline int_fast32_t Reset(void* Statement) { return sqlite3_reset((sqlite3_stmt*)Statement); }
  inline int_fast32_t Exec(const std::string& query) { return sqlite3_exec((sqlite3*)m_DB, query.c_str(), nullptr, nullptr, nullptr); }
};

//
// CAuraDB
//

class CDBDotAPlayerSummary;
class CDBGamePlayerSummary;
class CConfig;
class CDBBan;

class CAuraDB
{
private:
  CSQLITE3*   m_DB;
  std::string m_File;
  std::string m_Error;

  // we keep some prepared statements in memory rather than recreating them each function call
  // this is an optimization because preparing statements takes time
  // however it only pays off if you're going to be using the statement extremely often

  void* FromAddStmt;        // for faster startup time
  void* FromCheckStmt;      // frequently used
  void* BanCheckStmt;       // frequently used
  void* AdminCheckStmt;     // frequently used
  void* RootAdminCheckStmt; // frequently used

  bool m_HasError;

public:
  explicit CAuraDB(CConfig* CFG);
  ~CAuraDB();
  CAuraDB(CAuraDB&) = delete;

  inline bool        HasError() const { return m_HasError; }
  inline std::string GetError() const { return m_Error; }

  inline bool Begin() const { return m_DB->Exec("BEGIN TRANSACTION") == SQLITE_OK; }
  inline bool Commit() const { return m_DB->Exec("COMMIT TRANSACTION") == SQLITE_OK; }

  std::string FromCheck(uint_fast32_t ip);
  bool FromAdd(uint_fast32_t ip1, uint_fast32_t ip2, const std::string& country);
  uint_fast32_t AdminCount(const std::string& server);
  bool AdminCheck(const std::string& server, std::string user);
  bool AdminCheck(std::string user);
  bool RootAdminCheck(const std::string& server, std::string user);
  bool RootAdminCheck(std::string user);
  bool AdminAdd(const std::string& server, std::string user);
  bool RootAdminAdd(const std::string& server, std::string user);
  bool AdminRemove(const std::string& server, std::string user);
  uint_fast32_t BanCount(const std::string& server);
  CDBBan* BanCheck(const std::string& server, std::string user);
  bool BanAdd(const std::string& server, std::string user, const std::string& admin, const std::string& reason);
  bool BanRemove(const std::string& server, std::string user);
  bool BanRemove(std::string user);
  void GamePlayerAdd(std::string name, uint_fast64_t loadingtime, uint_fast64_t duration, uint_fast64_t left);
  CDBGamePlayerSummary* GamePlayerSummaryCheck(std::string name);
  void DotAPlayerAdd(std::string name, uint_fast32_t winner, uint_fast32_t kills, uint_fast32_t deaths, uint_fast32_t creepkills, uint_fast32_t creepdenies, uint_fast32_t assists, uint_fast32_t neutralkills, uint_fast32_t towerkills, uint_fast32_t raxkills, uint_fast32_t courierkills);
  CDBDotAPlayerSummary* DotAPlayerSummaryCheck(std::string name);
};

//
// CDBBan
//

class CDBBan
{
private:
  std::string m_Server;
  std::string m_Name;
  std::string m_Date;
  std::string m_Admin;
  std::string m_Reason;

public:
  CDBBan(std::string nServer, std::string nName, std::string nDate, std::string nAdmin, std::string nReason);
  ~CDBBan();

  inline std::string GetServer() const { return m_Server; }
  inline std::string GetName() const { return m_Name; }
  inline std::string GetDate() const { return m_Date; }
  inline std::string GetAdmin() const { return m_Admin; }
  inline std::string GetReason() const { return m_Reason; }
};

//
// CDBGamePlayer
//

class CDBGamePlayer
{
private:
  std::string   m_Name;
  uint_fast64_t m_LoadingTime;
  uint_fast64_t m_Left;
  uint_fast32_t m_Colour;

public:
  CDBGamePlayer(std::string name, uint_fast64_t nLoadingTime, uint_fast64_t nLeft, uint_fast32_t nColour);
  ~CDBGamePlayer();

  inline std::string   GetName() const { return m_Name; }
  inline uint_fast64_t GetLoadingTime() const { return m_LoadingTime; }
  inline uint_fast64_t GetLeft() const { return m_Left; }
  inline uint_fast32_t GetColour() const { return m_Colour; }

  inline void SetLoadingTime(uint_fast64_t nLoadingTime) { m_LoadingTime = nLoadingTime; }
  inline void SetLeft(uint_fast64_t nLeft) { m_Left = nLeft; }
};

//
// CDBGamePlayerSummary
//

class CDBGamePlayerSummary
{
private:
  uint_fast32_t m_TotalGames;     // total number of games played
  float         m_AvgLoadingTime; // average loading time in milliseconds (this could be skewed because different maps have different load times)
  uint_fast32_t m_AvgLeftPercent; // average time at which the player left the game expressed as a percentage of the game duration (0-100)

public:
  CDBGamePlayerSummary(uint_fast32_t nTotalGames, float nAvgLoadingTime, uint_fast32_t nAvgLeftPercent);
  ~CDBGamePlayerSummary();

  inline uint_fast32_t GetTotalGames() const { return m_TotalGames; }
  inline float         GetAvgLoadingTime() const { return m_AvgLoadingTime; }
  inline uint_fast32_t GetAvgLeftPercent() const { return m_AvgLeftPercent; }
};

//
// CDBDotAPlayer
//

class CDBDotAPlayer
{
private:
  uint_fast32_t m_Colour;
  uint_fast32_t m_NewColour;
  uint_fast32_t m_Kills;
  uint_fast32_t m_Deaths;
  uint_fast32_t m_CreepKills;
  uint_fast32_t m_CreepDenies;
  uint_fast32_t m_Assists;
  uint_fast32_t m_NeutralKills;
  uint_fast32_t m_TowerKills;
  uint_fast32_t m_RaxKills;
  uint_fast32_t m_CourierKills;

public:
  CDBDotAPlayer();
  CDBDotAPlayer(uint_fast32_t nKills, uint_fast32_t nDeaths, uint_fast32_t nCreepKills, uint_fast32_t nCreepDenies, uint_fast32_t nAssists, uint_fast32_t nNeutralKills, uint_fast32_t nTowerKills, uint_fast32_t nRaxKills, uint_fast32_t nCourierKills);
  ~CDBDotAPlayer();

  inline uint_fast32_t GetColour() const { return m_Colour; }
  inline uint_fast32_t GetNewColour() const { return m_NewColour; }
  inline uint_fast32_t GetKills() const { return m_Kills; }
  inline uint_fast32_t GetDeaths() const { return m_Deaths; }
  inline uint_fast32_t GetCreepKills() const { return m_CreepKills; }
  inline uint_fast32_t GetCreepDenies() const { return m_CreepDenies; }
  inline uint_fast32_t GetAssists() const { return m_Assists; }
  inline uint_fast32_t GetNeutralKills() const { return m_NeutralKills; }
  inline uint_fast32_t GetTowerKills() const { return m_TowerKills; }
  inline uint_fast32_t GetRaxKills() const { return m_RaxKills; }
  inline uint_fast32_t GetCourierKills() const { return m_CourierKills; }

  inline void IncKills() { ++m_Kills; }
  inline void IncDeaths() { ++m_Deaths; }
  inline void IncAssists() { ++m_Assists; }
  inline void IncTowerKills() { ++m_TowerKills; }
  inline void IncRaxKills() { ++m_RaxKills; }
  inline void IncCourierKills() { ++m_CourierKills; }

  inline void SetColour(uint_fast32_t nColour) { m_Colour = nColour; }
  inline void SetNewColour(uint_fast32_t nNewColour) { m_NewColour = nNewColour; }
  inline void SetCreepKills(uint_fast32_t nCreepKills) { m_CreepKills = nCreepKills; }
  inline void SetCreepDenies(uint_fast32_t nCreepDenies) { m_CreepDenies = nCreepDenies; }
  inline void SetNeutralKills(uint_fast32_t nNeutralKills) { m_NeutralKills = nNeutralKills; }
};

//
// CDBDotAPlayerSummary
//

class CDBDotAPlayerSummary
{
private:
  uint_fast32_t m_TotalGames;        // total number of dota games played
  uint_fast32_t m_TotalWins;         // total number of dota games won
  uint_fast32_t m_TotalLosses;       // total number of dota games lost
  uint_fast32_t m_TotalKills;        // total number of hero kills
  uint_fast32_t m_TotalDeaths;       // total number of deaths
  uint_fast32_t m_TotalCreepKills;   // total number of creep kills
  uint_fast32_t m_TotalCreepDenies;  // total number of creep denies
  uint_fast32_t m_TotalAssists;      // total number of assists
  uint_fast32_t m_TotalNeutralKills; // total number of neutral kills
  uint_fast32_t m_TotalTowerKills;   // total number of tower kills
  uint_fast32_t m_TotalRaxKills;     // total number of rax kills
  uint_fast32_t m_TotalCourierKills; // total number of courier kills

public:
  CDBDotAPlayerSummary(uint_fast32_t nTotalGames, uint_fast32_t nTotalWins, uint_fast32_t nTotalLosses, uint_fast32_t nTotalKills, uint_fast32_t nTotalDeaths, uint_fast32_t nTotalCreepKills, uint_fast32_t nTotalCreepDenies, uint_fast32_t nTotalAssists, uint_fast32_t nTotalNeutralKills, uint_fast32_t nTotalTowerKills, uint_fast32_t nTotalRaxKills, uint_fast32_t nTotalCourierKills);
  ~CDBDotAPlayerSummary();

  inline uint_fast32_t GetTotalGames() const { return m_TotalGames; }
  inline uint_fast32_t GetTotalWins() const { return m_TotalWins; }
  inline uint_fast32_t GetTotalLosses() const { return m_TotalLosses; }
  inline uint_fast32_t GetTotalKills() const { return m_TotalKills; }
  inline uint_fast32_t GetTotalDeaths() const { return m_TotalDeaths; }
  inline uint_fast32_t GetTotalCreepKills() const { return m_TotalCreepKills; }
  inline uint_fast32_t GetTotalCreepDenies() const { return m_TotalCreepDenies; }
  inline uint_fast32_t GetTotalAssists() const { return m_TotalAssists; }
  inline uint_fast32_t GetTotalNeutralKills() const { return m_TotalNeutralKills; }
  inline uint_fast32_t GetTotalTowerKills() const { return m_TotalTowerKills; }
  inline uint_fast32_t GetTotalRaxKills() const { return m_TotalRaxKills; }
  inline uint_fast32_t GetTotalCourierKills() const { return m_TotalCourierKills; }
  inline float         GetAvgKills() const { return m_TotalGames > 0 ? (float)m_TotalKills / m_TotalGames : 0.f; }
  inline float         GetAvgDeaths() const { return m_TotalGames > 0 ? (float)m_TotalDeaths / m_TotalGames : 0.f; }
  inline float         GetAvgCreepKills() const { return m_TotalGames > 0 ? (float)m_TotalCreepKills / m_TotalGames : 0.f; }
  inline float         GetAvgCreepDenies() const { return m_TotalGames > 0 ? (float)m_TotalCreepDenies / m_TotalGames : 0.f; }
  inline float         GetAvgAssists() const { return m_TotalGames > 0 ? (float)m_TotalAssists / m_TotalGames : 0.f; }
  inline float         GetAvgNeutralKills() const { return m_TotalGames > 0 ? (float)m_TotalNeutralKills / m_TotalGames : 0.f; }
  inline float         GetAvgTowerKills() const { return m_TotalGames > 0 ? (float)m_TotalTowerKills / m_TotalGames : 0.f; }
  inline float         GetAvgRaxKills() const { return m_TotalGames > 0 ? (float)m_TotalRaxKills / m_TotalGames : 0.f; }
  inline float         GetAvgCourierKills() const { return m_TotalGames > 0 ? (float)m_TotalCourierKills / m_TotalGames : 0.f; }
};

#endif // AURA_AURADB_H_
