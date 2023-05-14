#pragma once

#include <string>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <queue>
#include <cassert>
#include "koopa.hpp"

void BuildBlockCFG(koopa::FunBody *ptr);
void CutDeadBlocks(koopa::FunBody *ptr);
void CountUsedVars(koopa::Block *block, std::map<std::string, int> &used_vars);
void CutDeadVars(koopa::FunBody *ptr, std::map<std::string, int> &used_vars);
void BuildStmtCFG(koopa::FunBody *ptr);
void GetLiveVars(koopa::FunBody *ptr);
void AllocRegs(koopa::FunBody *ptr,
std::map<std::string, int> &var2reg, std::map<std::string, int> &used_vars);
