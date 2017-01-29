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

  inline int32_t Step(void* Statement) { return sqlite3_step((sqlite3_stmt*)Statement); }
  inline int32_t Prepare(const std::string& query, void** Statement) { return sqlite3_prepare_v2((sqlite3*)m_DB, query.c_str(), -1, (sqlite3_stmt**)Statement, nullptr); }
  inline int32_t Finalize(void* Statement) { return sqlite3_finalize((sqlite3_stmt*)Statement); }
  inline int32_t Reset(void* Statement) { return sqlite3_reset((sqlite3_stmt*)Statement); }
  inline int32_t Exec(const std::string& query) { return sqlite3_exec((sqlite3*)m_DB, query.c_str(), nullptr, nullptr, nullptr); }
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

  std::string FromCheck(uint32_t ip);
  bool FromAdd(uint32_t ip1, uint32_t ip2, const std::string& country);
  uint32_t AdminCount(const std::string& server);
  bool AdminCheck(const std::string& server, std::string user);
  bool AdminCheck(std::string user);
  bool RootAdminCheck(const std::string& server, std::string user);
  bool RootAdminCheck(std::string user);
  bool AdminAdd(const std::string& server, std::string user);
  bool RootAdminAdd(const std::string& server, std::string user);
  bool AdminRemove(const std::string& server, std::string user);
  uint32_t BanCount(const std::string& server);
  CDBBan* BanCheck(const std::string& server, std::string user);
  bool BanAdd(const std::string& server, std::string user, const std::string& admin, const std::string& reason);
  bool BanRemove(const std::string& server, std::string user);
  bool BanRemove(std::string user);
  void GamePlayerAdd(std::string name, uint64_t loadingtime, uint64_t duration, uint64_t left);
  CDBGamePlayerSummary* GamePlayerSummaryCheck(std::string name);
  void DotAPlayerAdd(std::string name, uint32_t winner, uint32_t kills, uint32_t deaths, uint32_t creepkills, uint32_t creepdenies, uint32_t assists, uint32_t neutralkills, uint32_t towerkills, uint32_t raxkills, uint32_t courierkills);
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
  std::string m_Name;
  uint64_t    m_LoadingTime;
  uint64_t    m_Left;
  uint32_t    m_Colour;

public:
  CDBGamePlayer(std::string name, uint64_t nLoadingTime, uint64_t nLeft, uint32_t nColour);
  ~CDBGamePlayer();

  inline std::string GetName() const { return m_Name; }
  inline uint64_t    GetLoadingTime() const { return m_LoadingTime; }
  inline uint64_t    GetLeft() const { return m_Left; }
  inline uint32_t    GetColour() const { return m_Colour; }

  inline void SetLoadingTime(uint64_t nLoadingTime) { m_LoadingTime = nLoadingTime; }
  inline void SetLeft(uint64_t nLeft) { m_Left = nLeft; }
};

//
// CDBGamePlayerSummary
//

class CDBGamePlayerSummary
{
private:
  uint32_t m_TotalGames;     // total number of games played
  float    m_AvgLoadingTime; // average loading time in milliseconds (this could be skewed because different maps have different load times)
  uint32_t m_AvgLeftPercent; // average time at which the player left the game expressed as a percentage of the game duration (0-100)

public:
  CDBGamePlayerSummary(uint32_t nTotalGames, float nAvgLoadingTime, uint32_t nAvgLeftPercent);
  ~CDBGamePlayerSummary();

  inline uint32_t GetTotalGames() const { return m_TotalGames; }
  inline float    GetAvgLoadingTime() const { return m_AvgLoadingTime; }
  inline uint32_t GetAvgLeftPercent() const { return m_AvgLeftPercent; }
};

//
// CDBDotAPlayer
//

class CDBDotAPlayer
{
private:
  uint32_t m_Colour;
  uint32_t m_NewColour;
  uint32_t m_Kills;
  uint32_t m_Deaths;
  uint32_t m_CreepKills;
  uint32_t m_CreepDenies;
  uint32_t m_Assists;
  uint32_t m_NeutralKills;
  uint32_t m_TowerKills;
  uint32_t m_RaxKills;
  uint32_t m_CourierKills;

public:
  CDBDotAPlayer();
  CDBDotAPlayer(uint32_t nKills, uint32_t nDeaths, uint32_t nCreepKills, uint32_t nCreepDenies, uint32_t nAssists, uint32_t nNeutralKills, uint32_t nTowerKills, uint32_t nRaxKills, uint32_t nCourierKills);
  ~CDBDotAPlayer();

  inline uint32_t GetColour() const { return m_Colour; }
  inline uint32_t GetNewColour() const { return m_NewColour; }
  inline uint32_t GetKills() const { return m_Kills; }
  inline uint32_t GetDeaths() const { return m_Deaths; }
  inline uint32_t GetCreepKills() const { return m_CreepKills; }
  inline uint32_t GetCreepDenies() const { return m_CreepDenies; }
  inline uint32_t GetAssists() const { return m_Assists; }
  inline uint32_t GetNeutralKills() const { return m_NeutralKills; }
  inline uint32_t GetTowerKills() const { return m_TowerKills; }
  inline uint32_t GetRaxKills() const { return m_RaxKills; }
  inline uint32_t GetCourierKills() const { return m_CourierKills; }

  inline void IncKills() { ++m_Kills; }
  inline void IncDeaths() { ++m_Deaths; }
  inline void IncAssists() { ++m_Assists; }
  inline void IncTowerKills() { ++m_TowerKills; }
  inline void IncRaxKills() { ++m_RaxKills; }
  inline void IncCourierKills() { ++m_CourierKills; }

  inline void SetColour(uint32_t nColour) { m_Colour = nColour; }
  inline void SetNewColour(uint32_t nNewColour) { m_NewColour = nNewColour; }
  inline void SetCreepKills(uint32_t nCreepKills) { m_CreepKills = nCreepKills; }
  inline void SetCreepDenies(uint32_t nCreepDenies) { m_CreepDenies = nCreepDenies; }
  inline void SetNeutralKills(uint32_t nNeutralKills) { m_NeutralKills = nNeutralKills; }
};

//
// CDBDotAPlayerSummary
//

class CDBDotAPlayerSummary
{
private:
  uint32_t m_TotalGames;        // total number of dota games played
  uint32_t m_TotalWins;         // total number of dota games won
  uint32_t m_TotalLosses;       // total number of dota games lost
  uint32_t m_TotalKills;        // total number of hero kills
  uint32_t m_TotalDeaths;       // total number of deaths
  uint32_t m_TotalCreepKills;   // total number of creep kills
  uint32_t m_TotalCreepDenies;  // total number of creep denies
  uint32_t m_TotalAssists;      // total number of assists
  uint32_t m_TotalNeutralKills; // total number of neutral kills
  uint32_t m_TotalTowerKills;   // total number of tower kills
  uint32_t m_TotalRaxKills;     // total number of rax kills
  uint32_t m_TotalCourierKills; // total number of courier kills

public:
  CDBDotAPlayerSummary(uint32_t nTotalGames, uint32_t nTotalWins, uint32_t nTotalLosses, uint32_t nTotalKills, uint32_t nTotalDeaths, uint32_t nTotalCreepKills, uint32_t nTotalCreepDenies, uint32_t nTotalAssists, uint32_t nTotalNeutralKills, uint32_t nTotalTowerKills, uint32_t nTotalRaxKills, uint32_t nTotalCourierKills);
  ~CDBDotAPlayerSummary();

  inline uint32_t GetTotalGames() const { return m_TotalGames; }
  inline uint32_t GetTotalWins() const { return m_TotalWins; }
  inline uint32_t GetTotalLosses() const { return m_TotalLosses; }
  inline uint32_t GetTotalKills() const { return m_TotalKills; }
  inline uint32_t GetTotalDeaths() const { return m_TotalDeaths; }
  inline uint32_t GetTotalCreepKills() const { return m_TotalCreepKills; }
  inline uint32_t GetTotalCreepDenies() const { return m_TotalCreepDenies; }
  inline uint32_t GetTotalAssists() const { return m_TotalAssists; }
  inline uint32_t GetTotalNeutralKills() const { return m_TotalNeutralKills; }
  inline uint32_t GetTotalTowerKills() const { return m_TotalTowerKills; }
  inline uint32_t GetTotalRaxKills() const { return m_TotalRaxKills; }
  inline uint32_t GetTotalCourierKills() const { return m_TotalCourierKills; }
  inline float    GetAvgKills() const { return m_TotalGames > 0 ? (float)m_TotalKills / m_TotalGames : 0.f; }
  inline float    GetAvgDeaths() const { return m_TotalGames > 0 ? (float)m_TotalDeaths / m_TotalGames : 0.f; }
  inline float    GetAvgCreepKills() const { return m_TotalGames > 0 ? (float)m_TotalCreepKills / m_TotalGames : 0.f; }
  inline float    GetAvgCreepDenies() const { return m_TotalGames > 0 ? (float)m_TotalCreepDenies / m_TotalGames : 0.f; }
  inline float    GetAvgAssists() const { return m_TotalGames > 0 ? (float)m_TotalAssists / m_TotalGames : 0.f; }
  inline float    GetAvgNeutralKills() const { return m_TotalGames > 0 ? (float)m_TotalNeutralKills / m_TotalGames : 0.f; }
  inline float    GetAvgTowerKills() const { return m_TotalGames > 0 ? (float)m_TotalTowerKills / m_TotalGames : 0.f; }
  inline float    GetAvgRaxKills() const { return m_TotalGames > 0 ? (float)m_TotalRaxKills / m_TotalGames : 0.f; }
  inline float    GetAvgCourierKills() const { return m_TotalGames > 0 ? (float)m_TotalCourierKills / m_TotalGames : 0.f; }
};

#endif // AURA_AURADB_H_
