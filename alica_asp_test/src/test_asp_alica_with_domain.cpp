#include <gtest/gtest.h>

#include <SystemConfig.h>

// ALICA Additional Modules
#include <clock/AlicaSystemClock.h>
#include <communication/AlicaDummyCommunication.h>

// ALICA Model and Engine Stuff
#include "BehaviourCreator.h"
#include "ConditionCreator.h"
#include "ConstraintCreator.h"
#include "UtilityFunctionCreator.h"
#include <engine/AlicaEngine.h>
#include <engine/PlanBase.h>
#include <engine/model/Plan.h>
#include <engine/model/PlanType.h>
#include <engine/model/State.h>
#include <engine/parser/PlanParser.h>

// ALICA ASP Solver
#include <asp_commons/ASPCommonsTerm.h>
#include <asp_commons/ASPQueryType.h>
#include <asp_commons/IASPSolver.h>
#include <asp_solver/ASPFactsQuery.h>
#include <asp_solver/ASPSolver.h>
#include <asp_solver_wrapper/ASPGenerator.h>
#include <asp_solver_wrapper/ASPSolverWrapper.h>

class AspAlicaEngineWithDomain : public ::testing::Test
{

  protected:
    supplementary::SystemConfig *sc;
    alica::AlicaEngine *ae;
    alica::BehaviourCreator *bc;
    alica::ConditionCreator *cc;
    alica::UtilityFunctionCreator *uc;
    alica::ConstraintCreator *crc;
    std::chrono::_V2::system_clock::time_point start;

    virtual void SetUp()
    {
        // determine the path to the test config
        string path = supplementary::FileSystem::getSelfPath();
        int place = path.rfind("devel");
        path = path.substr(0, place);
        path = path + "src/symrock/alica_asp_test/src";

        // bring up the SystemConfig with the corresponding path
        sc = supplementary::SystemConfig::getInstance();
        sc->setRootPath(path);
        sc->setConfigPath(path + "/etc");
        sc->setHostname("nase");

        // setup the engine
        bc = new alica::BehaviourCreator();
        cc = new alica::ConditionCreator();
        uc = new alica::UtilityFunctionCreator();
        crc = new alica::ConstraintCreator();

        start = std::chrono::high_resolution_clock::now(); // start time measurement
    }

    virtual void TearDown()
    {
        // stop time measurement and report
        std::chrono::_V2::system_clock::time_point end = std::chrono::high_resolution_clock::now();
        cout << "Measured Time: " << std::chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms"
             << endl;
        ae->shutdown();
        sc->shutdown();
        delete ae->getIAlicaClock();
        delete ae->getCommunicator();
        delete cc;
        delete bc;
        delete uc;
        delete crc;
    }
};

/**
 * Tests the validation of ALICA plans with domain specific knowledge
 */
TEST_F(AspAlicaEngineWithDomain, AgentInTwoStatesOfSamePlan)
{
    ae = new alica::AlicaEngine(new supplementary::AgentIDManager(new supplementary::AgentIDFactory()),
                                "ReusePlanWithoutCycle", "CarryBookMaster", ".", false);
    ae->setIAlicaClock(new alica_dummy_proxy::AlicaSystemClock());
    ae->setCommunicator(new alica_dummy_proxy::AlicaDummyCommunication(ae));
    // "1" stands for the ASPSolver in this test suite only!
    std::vector<char const *> args{"clingo", "-W", "no-atom-undefined", nullptr};
    auto solver = new ::reasoner::ASPSolver(args);
    auto solverWrapper = new alica::reasoner::ASPSolverWrapper(ae, args);
    solverWrapper->init(solver);

    solverWrapper->getSolver()->loadFileFromConfig("assistanceBackgroundKnowledgeFile");
    ae->addSolver(1, solverWrapper);
    EXPECT_TRUE(ae->init(bc, cc, uc, crc)) << "Unable to initialise the ALICA Engine!";

    alica::reasoner::ASPSolverWrapper *aspSolver =
        dynamic_cast<alica::reasoner::ASPSolverWrapper *>(ae->getSolver(1)); // "1" for ASPSolver

    alica::Plan *plan = ae->getPlanBase()->getMasterPlan();

    string queryString1 = "brokenPlanBase(donatello)";
    string queryString2 = "brokenPlanBase(leonardo)";
    string queryString3 = "brokenPlanBase(raphael)";
    string queryString4 = "brokenPlanBase(michelangelo)";
    auto constraint1 = make_shared<::reasoner::ASPCommonsTerm>();
    constraint1->addRule(queryString1);
    constraint1->setProgramSection("assistanceTestFacts");
    constraint1->setType(::reasoner::ASPQueryType::Facts);
    auto constraint2 = make_shared<::reasoner::ASPCommonsTerm>();
    constraint2->addRule(queryString2);
    constraint2->setProgramSection("assistanceTestFacts");
    constraint2->setType(::reasoner::ASPQueryType::Facts);
    auto constraint3 = make_shared<::reasoner::ASPCommonsTerm>();
    constraint3->addRule(queryString3);
    constraint3->setProgramSection("assistanceTestFacts");
    constraint3->setType(::reasoner::ASPQueryType::Facts);
    auto constraint4 = make_shared<::reasoner::ASPCommonsTerm>();
    constraint4->addRule(queryString4);
    constraint4->setProgramSection("assistanceTestFacts");
    constraint4->setType(::reasoner::ASPQueryType::Facts);
    shared_ptr<::reasoner::ASPFactsQuery> queryObject1 =
        make_shared<::reasoner::ASPFactsQuery>(((::reasoner::ASPSolver *)(aspSolver->getSolver())), constraint1);
    shared_ptr<::reasoner::ASPFactsQuery> queryObject2 =
        make_shared<::reasoner::ASPFactsQuery>(((::reasoner::ASPSolver *)(aspSolver->getSolver())), constraint2);
    shared_ptr<::reasoner::ASPFactsQuery> queryObject3 =
        make_shared<::reasoner::ASPFactsQuery>(((::reasoner::ASPSolver *)(aspSolver->getSolver())), constraint3);
    shared_ptr<::reasoner::ASPFactsQuery> queryObject4 =
        make_shared<::reasoner::ASPFactsQuery>(((::reasoner::ASPSolver *)(aspSolver->getSolver())), constraint4);
    ((::reasoner::ASPSolver *)(aspSolver->getSolver()))->registerQuery(queryObject1);
    ((::reasoner::ASPSolver *)(aspSolver->getSolver()))->registerQuery(queryObject2);
    ((::reasoner::ASPSolver *)(aspSolver->getSolver()))->registerQuery(queryObject3);
    ((::reasoner::ASPSolver *)(aspSolver->getSolver()))->registerQuery(queryObject4);

    ((::reasoner::ASPSolver *)(aspSolver->getSolver()))->ground({{"assistanceTestFacts", {}}}, nullptr);
    ((::reasoner::ASPSolver *)(aspSolver->getSolver()))->ground({{"assistanceBackground", {}}}, nullptr);
    // start time measurement for grounding
    std::chrono::_V2::system_clock::time_point groundingStart = std::chrono::high_resolution_clock::now();
    if (!((alica::reasoner::ASPSolverWrapper *)(aspSolver->getSolver()))->validatePlan(plan))
    {
        cout << "ASPAlicaTest: No Model found!" << endl;
    }
    else
    {
        ((alica::reasoner::ASPSolverWrapper *)(aspSolver->getSolver()))->printStats();
    }
    std::chrono::_V2::system_clock::time_point end = std::chrono::high_resolution_clock::now();
    cout << "Measured Grounding Time: "
         << std::chrono::duration_cast<chrono::milliseconds>(end - groundingStart).count() << " ms" << endl;

    EXPECT_TRUE(queryObject1->factsExistForAllModels()) << "The planbase of agent donatello should be broken.";
    EXPECT_FALSE(queryObject2->factsExistForAllModels()) << "The planbase of agent leonardo should not be broken.";
    EXPECT_FALSE(queryObject3->factsExistForAllModels()) << "The planbase of agent raphael should not be broken.";
    EXPECT_FALSE(queryObject4->factsExistForAllModels()) << "The planbase of agent michelangelo should not be broken.";
    cout << queryObject1->toString() << endl;
    cout << queryObject2->toString() << endl;
    cout << queryObject3->toString() << endl;
    cout << queryObject4->toString() << endl;
}

TEST_F(AspAlicaEngineWithDomain, ReusePlanFromPlantypeWithoutCycle_PlanBase)
{
    ae = new alica::AlicaEngine(new supplementary::AgentIDManager(new supplementary::AgentIDFactory()),
                                "ReusePlanWithoutCycle", "ReusePlanFromPlantypeWithoutCycle", ".", false);
    ae->setIAlicaClock(new alica_dummy_proxy::AlicaSystemClock());
    ae->setCommunicator(new alica_dummy_proxy::AlicaDummyCommunication(ae));
    // "1" stands for the ASPSolver in this test suite only!
    std::vector<char const *> args{"clingo", "-W", "no-atom-undefined", nullptr};
    auto solver = new ::reasoner::ASPSolver(args);
    auto solverWrapper = new alica::reasoner::ASPSolverWrapper(ae, args);
    solverWrapper->init(solver);

    solverWrapper->getSolver()->loadFileFromConfig("assistanceBackgroundKnowledgeFile");
    ae->addSolver(1, solverWrapper);
    EXPECT_TRUE(ae->init(bc, cc, uc, crc)) << "Unable to initialise the ALICA Engine!";

    alica::reasoner::ASPSolverWrapper *aspSolver =
        dynamic_cast<alica::reasoner::ASPSolverWrapper *>(ae->getSolver(1)); // "1" for ASPSolver

    alica::Plan *plan = ae->getPlanBase()->getMasterPlan();

    string queryString1 = "brokenPlanBase(donatello)";
    auto constraint = make_shared<::reasoner::ASPCommonsTerm>();
    constraint->addRule(queryString1);
    constraint->setType(::reasoner::ASPQueryType::Facts);
    constraint->setProgramSection("assistanceTestFacts");

    ((::reasoner::ASPSolver *)(aspSolver->getSolver()))->ground({{"assistanceBackground", {}}}, nullptr);

    shared_ptr<::reasoner::ASPFactsQuery> queryObject1 =
        make_shared<::reasoner::ASPFactsQuery>(((::reasoner::ASPSolver *)(aspSolver->getSolver())), constraint);
    ((alica::reasoner::ASPSolverWrapper *)(aspSolver->getSolver()))->registerQuery(queryObject1);

    // start time measurement for grounding
    std::chrono::_V2::system_clock::time_point groundingStart = std::chrono::high_resolution_clock::now();
    if (!((alica::reasoner::ASPSolverWrapper *)(aspSolver->getSolver()))->validatePlan(plan))
    {
        cout << "ASPAlicaTest: No Model found!" << endl;
    }
    else
    {
        ((alica::reasoner::ASPSolverWrapper *)(aspSolver->getSolver()))->printStats();
    }
    std::chrono::_V2::system_clock::time_point end = std::chrono::high_resolution_clock::now();
    cout << "Measured Grounding Time: "
         << std::chrono::duration_cast<chrono::milliseconds>(end - groundingStart).count() << " ms" << endl;

    EXPECT_FALSE(queryObject1->factsExistForAllModels()) << "The plan base of agent donatello should not be broken.";
    cout << queryObject1->toString() << endl;
}
