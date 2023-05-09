#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include "sysy.hpp"
#include "koopa.hpp"
#include "symtab.hpp"

std::unique_ptr<koopa::Program> GetProgram(const std::unique_ptr<sysy::CompUnit> &ast);
std::unique_ptr<koopa::Value> GetExp(const std::unique_ptr<sysy::Exp> &ast,
			std::vector<std::unique_ptr<koopa::Statement> > &states);

void GetBlock(const std::unique_ptr<sysy::Block> &ast,
std::vector<std::unique_ptr<koopa::Block> > &blocks,
std::vector<std::unique_ptr<koopa::Statement> > &stmts);