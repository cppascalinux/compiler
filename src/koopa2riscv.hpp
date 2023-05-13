#pragma once

#include <memory>
#include <vector>
#include <string>
#include "koopa.hpp"
#include "types.hpp"

std::string ParseProgram(koopa::Program *ptr);

enum VarDefType {
	LOCALDEF,
	ALLOCDEF,
	GLOBALDEF,
	PARAMDEF
};

class VarInfo {
	public:
		std::string name;
		VarDefType var_def;
		std::shared_ptr<koopa::Type> type;
		int reg;
		int offset;
		VarInfo(): reg(-1), offset(-1) {}
		VarInfo(std::string s, VarDefType v):
			name(s), var_def(v), reg(-1), offset(-1){}
		VarInfo(std::string s, VarDefType v, std::shared_ptr<koopa::Type> t):
			name(s), var_def(v), type(t), reg(-1), offset(-1) {}
		VarInfo(std::string s, VarDefType v, std::shared_ptr<koopa::Type> t, int r):
			name(s), var_def(v), type(t), reg(r), offset(-1) {}
		VarInfo(std::string s, VarDefType v, std::shared_ptr<koopa::Type> t, int r, int o):
			name(s), var_def(v), type(t), reg(r), offset(o) {}
};

const int max_regs = 25;
const int callee_regs = 12;