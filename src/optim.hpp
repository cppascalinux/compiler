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
void CountUsedVars(koopa::Block *block, std::set<std::string> &used_vars);
void CutDeadVars(koopa::FunBody *ptr);
void BuildStmtCFG(koopa::FunBody *ptr);
void GetLiveVars(koopa::FunBody *ptr);
void AllocRegs(koopa::FunBody *ptr, std::map<std::string, int> &var2reg);
