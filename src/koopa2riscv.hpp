#pragma once

#include <memory>
#include <vector>
#include <string>
#include "koopa.hpp"
#include "types.hpp"

std::string ParseProgram(const std::unique_ptr<koopa::Program> &ptr);