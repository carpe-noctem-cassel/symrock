#pragma once

#include <clingo.hh>

#include <mutex>
#include <map>
#include <vector>

namespace reasoner{
    namespace asp {
        class Solver;
    }
}

namespace knowledge_manager
{
class ASPKnowledgeManager
{
public:
    ASPKnowledgeManager();
    virtual ~ASPKnowledgeManager();

    std::vector<std::string> filterModel(const std::string& queryValue);
    int addInformation(std::vector<std::string>& information, int lifetime = -1);
    void updateExternals(std::shared_ptr<std::map<std::string, bool>> externals);
    void addRulesPermanent(const std::string& programSection, std::vector<std::string>& rules);

    void revoke(int queryID);
    virtual void setSolver(reasoner::asp::Solver* solver);

//    reasoner::asp::Solver* getSolver();

    // TODO These 4 four methods are used by the ASPTranslator in the SRGWorldModel.
    // FIXME The ASPTranlsator should use some other more conveniant methods of the ASPKnowledgeManager.
    // Maybe these methods need to be implemented. :P
    bool solve();
    void add(char const* name, Clingo::StringSpan const& params, char const* part);
    void ground(Clingo::PartSpan vec, Clingo::GroundCallback callBack);
    Clingo::Symbol parseValue(std::string const& str);
private:
    ::reasoner::asp::Solver* solver;
    std::vector<std::string> currentSolution;
    std::mutex mtx;
    bool isDirty;
};

} /* namespace knowledge_manager */