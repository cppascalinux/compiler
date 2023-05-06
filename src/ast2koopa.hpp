#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include "ast.hpp"
#include "koopa.hpp"

std::unique_ptr<koopa::Program> GetProgram(const std::unique_ptr<sysy::CompUnitAST> &ast);