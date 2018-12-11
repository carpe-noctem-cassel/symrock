#include "reasoner/asp/FilterQuery.h"

#include "reasoner/asp/Solver.h"
#include "reasoner/asp/Enums.h"

namespace reasoner
{
namespace asp
{

FilterQuery::FilterQuery(Solver* solver, Term* term)
        : Query(solver, term)
{
    this->type = QueryType ::Filter;
    this->addQueryValues(term->getRuleHeads());
    this->currentModels = std::make_shared<std::vector<Clingo::SymbolVector>>();

#ifdef Solver_DEBUG
    std::cout << "Solver: Query contains rule: " << std::endl;
    for (auto rule : this->term->getRules()) {
        std::cout << rule << std::endl;
    }

    for (auto fact : this->term->getFacts()) {
        std::cout << "Solver: Query contains fact: " << fact << std::endl;
    }
#endif
}

FilterQuery::~FilterQuery() {}

void FilterQuery::addQueryValues(std::vector<std::string> queryVec)
{
    for (auto queryString : queryVec) {
        if (queryString.compare("") == 0) {
            return;
        }
        // TODO: Fix nested braces and move funtionality to central accessable helper class
        if (queryString.find(",") != std::string::npos) {
            size_t start = 0;
            size_t end = std::string::npos;
            std::string currentQuery = "";
            while (start != std::string::npos) {
                end = queryString.find(")", start);
                if (end == std::string::npos || end == queryString.size()) {
                    break;
                }
                currentQuery = queryString.substr(start, end - start + 1);
                currentQuery = supplementary::Configuration::trim(currentQuery);
                this->headValues.emplace(this->solver->parseValue(currentQuery), std::vector<Clingo::Symbol>());
                start = queryString.find(",", end);
                if (start != std::string::npos) {
                    start += 1;
                }
            }
        } else {
            this->headValues.emplace(this->solver->parseValue(queryString), std::vector<Clingo::Symbol>());
        }
    }
}

bool FilterQuery::factsExistForAtLeastOneModel()
{
    for (auto queryValue : this->headValues) {
        if (queryValue.second.size() > 0) {
            return true;
        }
    }
    return false;
}

bool FilterQuery::factsExistForAllModels()
{
    for (auto queryValue : this->headValues) {
        if (queryValue.second.size() == 0) {
            return false;
        }
    }
    return true;
}

void FilterQuery::removeExternal()
{ // NOOP in case of FilterQuery
}

std::vector<std::pair<Clingo::Symbol, TruthValue>> FilterQuery::getTruthValues()
{
    std::vector<std::pair<Clingo::Symbol, TruthValue>> ret;
    for (auto iter : this->getHeadValues()) {
        if (iter.second.size() == 0) {
            ret.push_back(std::pair<Clingo::Symbol, TruthValue>(iter.first, TruthValue::Unknown));
        } else {
            if (iter.second.at(0).is_positive()) {
                ret.push_back(std::pair<Clingo::Symbol, TruthValue>(iter.first, TruthValue::True));
            } else {
                ret.push_back(std::pair<Clingo::Symbol, TruthValue>(iter.first, TruthValue::False));
            }
        }
    }
    return ret;
}

void FilterQuery::onModel(Clingo::Model& clingoModel)
{
    // Remember model
    Clingo::SymbolVector vec;
    auto tmp = clingoModel.symbols(clingo_show_type_shown);
    for (int i = 0; i < tmp.size(); i++) {
        vec.push_back(tmp[i]);
    }
    this->getCurrentModels()->push_back(vec);

    // Fill mapping from query fact towards model fact
    for (auto value : this->getHeadValues()) {
#ifdef QUERY_DEBUG
        cout << "FilterQuery::onModel: " << value.first << endl;
#endif
        auto it = ((Solver*) this->solver)
                          ->clingo->symbolic_atoms()
                          .begin(Clingo::Signature(value.first.name(), value.first.arguments().size(), value.first.is_positive())); // value.first.signature();
        if (it == ((Solver*) this->solver)->clingo->symbolic_atoms().end()) {
            std::cout << "FilterQuery: Didn't find any suitable domain!" << std::endl;
            continue;
        }

        while (it) {
            if (clingoModel.contains((*it).symbol()) && this->checkMatchValues(Clingo::Symbol(value.first), (*it).symbol())) {
                this->saveHeadValuePair(Clingo::Symbol(value.first), (*it).symbol());
            }
            it++;
        }
    }
}
} /* namespace asp */
} /* namespace reasoner */