
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "types.hpp"
#include "koopa.hpp"


namespace koopa {
	AggregateInit::AggregateInit(std::unique_ptr<Aggregate> a):
		Initializer(AGGREGATEINIT), aggr(std::move(a)){}
	std::string AggregateInit::Str() const {return aggr->Str();}
}