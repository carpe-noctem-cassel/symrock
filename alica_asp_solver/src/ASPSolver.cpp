/*
 * ASPSolver.cpp
 *
 *  Created on: Sep 8, 2015
 *      Author: Stephan Opfer
 */

#include <alica_asp_solver/ASPVariable.h>
#include "alica_asp_solver/ASPSolver.h"
#include "alica_asp_solver/AnnotatedValVec.h"
#include "engine/model/Plan.h"
#include "engine/model/Variable.h"
#include "alica_asp_solver/ASPTerm.h"
#include "engine/constraintmodul/ConstraintDescriptor.h"
#include "engine/AlicaEngine.h"
#include "engine/PlanBase.h"

namespace alica
{
	namespace reasoner
	{

		const void* const ASPSolver::WILDCARD_POINTER = new int(0);
		const string ASPSolver::WILDCARD_STRING = "wildcard";

		ASPSolver::ASPSolver(AlicaEngine* ae, vector<char const*> args) :
				IConstraintSolver(ae), gen(ASPSolver::WILDCARD_POINTER, ASPSolver::WILDCARD_STRING)
		{
			this->gringoModule = new DefaultGringoModule();
			this->clingo = make_shared<ClingoLib>(*gringoModule, args.size() - 2, args.data());
#ifdef SOLVER_OPTIONS
			traverseOptions(conf, root, "");
#endif

			this->planIntegrator = make_shared<ASPAlicaPlanIntegrator>(clingo, &this->gen);

			this->disableWarnings(true);

			this->sc = supplementary::SystemConfig::getInstance();
			// read alica background knowledge from static file
			string alicaBackGroundKnowledgeFile = (*this->sc)["ASPSolver"]->get<string>("alicaBackgroundKnowledgeFile",
			NULL);
			this->alreadyLoaded.push_back("alicaBackgroundKnowledgeFile");
			alicaBackGroundKnowledgeFile = supplementary::FileSystem::combinePaths((*this->sc).getConfigPath(),
																					alicaBackGroundKnowledgeFile);
#ifdef ASPSolver_DEBUG
			cout << "ASPSolver: " << alicaBackGroundKnowledgeFile << endl;
#endif
			this->clingo->load(alicaBackGroundKnowledgeFile);
			this->queryCounter = 0;
			this->masterPlanLoaded = false;
#ifdef ASPSolver_DEBUG
			this->modelCount = 0;
#endif
			this->conf = &this->clingo->getConf();
			this->root = this->conf->getRootKey();
			this->modelsKey = this->conf->getSubKey(this->root, "solve.models");
			this->conf->setKeyValue(this->modelsKey, "0");
		}

		ASPSolver::~ASPSolver()
		{
			delete this->gringoModule;
		}

		void ASPSolver::loadFile(string absolutFilename)
		{
			this->clingo->load(forward<string>(absolutFilename));
		}

		bool ASPSolver::loadFileFromConfig(string configKey)
		{
			for (auto file : this->alreadyLoaded)
			{
				if (configKey.compare(file) == 0)
				{
					return false;
				}
			}

			string backGroundKnowledgeFile = (*this->sc)["ASPSolver"]->get<string>(configKey.c_str(), NULL);
			this->alreadyLoaded.push_back(configKey.c_str());
			backGroundKnowledgeFile = supplementary::FileSystem::combinePaths((*this->sc).getConfigPath(),
																				backGroundKnowledgeFile);
			this->clingo->load(forward<string>(backGroundKnowledgeFile));
			return true;
		}

		/**
		 * Let the internal solver ground a given program part (context).
		 */
		void ASPSolver::ground(Gringo::Control::GroundVec const &vec, Gringo::Any &&context)
		{
			this->clingo->ground(forward<Gringo::Control::GroundVec const &>(vec), forward<Gringo::Any&&>(context));
		}

		/**
		 * Let the internal solver solve one time.
		 */
		bool ASPSolver::solve()
		{
			this->reduceLifeTime();
			this->currentModels.clear();
#ifdef ASPSolver_DEBUG
			this->modelCount = 0;
#endif
			auto result = this->clingo->solve(bind(&ASPSolver::onModel, this, placeholders::_1), {});
			if (result == Gringo::SolveResult::SAT)
			{
				return true;
			}
			return false;
		}

		/**
		 * Callback for created models during solving.
		 */
		bool ASPSolver::onModel(const Gringo::Model& m)
		{
#ifdef ASPSolver_DEBUG
			this->modelCount++;
			cout << "ASPSolver: Found the following model which is number " << this->modelCount << ":" << endl;
			for (auto &atom : m.atoms(Gringo::Model::SHOWN))
			{
				cout << atom << " ";
			}
			cout << endl;
#endif

			ClingoModel& clingoModel = (ClingoModel&)m;
			this->currentModels.push_back(m.atoms(Gringo::Model::SHOWN));
			bool foundSomething = false;
			for (auto& query : this->registeredQueries)
			{
				query->getCurrentModels()->push_back(m.atoms(Gringo::Model::SHOWN));
				//	cout << "ASPSolver: processing query '" << queryMapPair.first << "'" << endl;

				// determine the domain of the query predicate
				for (auto value : query->getHeadValues())
				{
					cout << "ASPSolver::onModel: " << value.first << endl;
					auto it = clingoModel.out.domains.find(value.first.sig());
					if (it == clingoModel.out.domains.end())
					{
						//cout << "ASPSolver: Didn't find any suitable domain!" << endl;
						continue;
					}

					for (auto& domainPair : it->second.domain)
					{
						//cout << "ASPSolver: Inside domain-loop!" << endl;

						if (&(domainPair.second)
								&& clingoModel.model->isTrue(clingoModel.lp.getLiteral(domainPair.second.uid())))
						{
							//cout << "ASPSolver: Found true literal '" << domainPair.first << "'" << endl;

							if (this->checkMatchValues(&value.first, &domainPair.first))
							{
								//cout << "ASPSolver: Literal '" << domainPair.first << "' matched!" << endl;
								foundSomething = true;
								query->saveHeadValuePair(value.first, domainPair.first);
							}
							else
							{
								//cout << "ASPSolver: Literal '" << domainPair.first << "' didn't match!" << endl;

							}
						}
					}
				}

#ifdef ASP_TEST_RELATED
				for (auto queryValue : query->getQueryValues())
				{
					cout << "ASPSolver::onModel: " << queryValue << endl;
					auto it = clingoModel.out.domains.find(queryValue.sig());
					if (it == clingoModel.out.domains.end())
					{
						//						cout << "ASPSolver: Didn't find any suitable domain!" << endl;
						continue;
					}

					for (auto& domainPair : it->second.domain)
					{
						//							cout << "ASPSolver: Inside domain-loop!" << endl;

						if (&(domainPair.second)
								&& clingoModel.model->isTrue(clingoModel.lp.getLiteral(domainPair.second.uid())))
						{
							//										cout << "ASPSolver: Found true literal '" << domainPair.first << "'" << endl;

							if (this->checkMatchValues(&queryValue, &domainPair.first))
							{
								//										cout << "ASPSolver: Literal '" << domainPair.first << "' matched!" << endl;
								foundSomething = true;
								query->saveStaisfiedPredicate(queryValue, domainPair.first);
							}
							else
							{
								//											cout << "ASPSolver: Literal '" << domainPair.first << "' didn't match!" << endl;

							}
						}
					}
				}

				for (auto rule : query->getRuleModelMap())
				{
					cout << "ASPSolver::onModel: " << rule.first << endl;
					auto it = clingoModel.out.domains.find(rule.first.sig());
					if (it == clingoModel.out.domains.end())
					{
						//						cout << "ASPSolver: Didn't find any suitable domain!" << endl;
						continue;
					}

					for (auto& domainPair : it->second.domain)
					{
						//							cout << "ASPSolver: Inside domain-loop!" << endl;

						if (&(domainPair.second)
								&& clingoModel.model->isTrue(clingoModel.lp.getLiteral(domainPair.second.uid())))
						{
							//										cout << "ASPSolver: Found true literal '" << domainPair.first << "'" << endl;

							if (this->checkMatchValues(&rule.first, &domainPair.first))
							{
								//										cout << "ASPSolver: Literal '" << domainPair.first << "' matched!" << endl;
								foundSomething = true;
								query->saveRuleModelPair(rule.first, domainPair.first);
							}
							else
							{
								//											cout << "ASPSolver: Literal '" << domainPair.first << "' didn't match!" << endl;
							}
						}
					}
				}
#endif
			}

			return foundSomething;
		}

		// Old but keep it for a while
		vector<Gringo::Value> ASPSolver::getAllMatches(Gringo::Value queryValue)
		{
			vector<Gringo::Value> gringoValues;

			this->clingo->solve([queryValue, &gringoValues, this](Gringo::Model const &m)
			{
				//cout << "Inside Lambda!" << endl;
					ClingoModel& clingoModel = (ClingoModel&) m;
					auto it = clingoModel.out.domains.find(queryValue.sig());

					vector<Gringo::AtomState const *> atomStates;
					if (it != clingoModel.out.domains.end())
					{
						//cout << "In Loop" << endl;
						for (auto& domainPair : it->second.domain)
						{
							//cout << domainPair.first << " " << endl;
							if (&(domainPair.second) && clingoModel.model->isTrue(clingoModel.lp.getLiteral(domainPair.second.uid())))
							{
								if (checkMatchValues(&queryValue, &domainPair.first))
								{
									gringoValues.push_back(domainPair.first);
									atomStates.push_back(&(domainPair.second));
								}
							}
						}
					}

					return true;
				},
					{});

			return gringoValues;
		}

		bool ASPSolver::registerQuery(shared_ptr<ASPQuery> query)
		{
			auto entry = find(this->registeredQueries.begin(), this->registeredQueries.end(), query);
			if (entry == this->registeredQueries.end())
			{
				this->registeredQueries.push_back(query);
				return true;
			}
			return false;
		}

		bool ASPSolver::unregisterQuery(shared_ptr<ASPQuery> query)
		{
			auto entry = find(this->registeredQueries.begin(), this->registeredQueries.end(), query);
			if (entry != this->registeredQueries.end())
			{
				this->registeredQueries.erase(entry);
				return true;
			}
			return false;
		}

		bool ASPSolver::checkMatchValues(const Gringo::Value* value1, const Gringo::Value* value2)
		{
			if (value2->type() != Gringo::Value::Type::FUNC)
				return false;

			if (value1->name() != value2->name())
				return false;

			if (value1->args().size() != value2->args().size())
				return false;

			for (uint i = 0; i < value1->args().size(); ++i)
			{
				Gringo::Value arg = value1->args()[i];

				if (arg.type() == Gringo::Value::Type::ID && arg.name() == ASPSolver::WILDCARD_STRING)
					continue;

				if (arg.type() == Gringo::Value::Type::FUNC && value2->args()[i].type() == Gringo::Value::Type::FUNC)
				{
					if (false == checkMatchValues(&arg, &value2->args()[i]))
						return false;
				}
				else if (arg != value2->args()[i])
					return false;
			}

			return true;
		}

		void ASPSolver::disableWarnings(bool disable)
		{
			if (disable)
			{
				Gringo::message_printer()->disable(Gringo::Warnings::W_ATOM_UNDEFINED);
				Gringo::message_printer()->disable(Gringo::Warnings::W_FILE_INCLUDED);
				Gringo::message_printer()->disable(Gringo::Warnings::W_GLOBAL_VARIABLE);
				Gringo::message_printer()->disable(Gringo::Warnings::W_OPERATION_UNDEFINED);
				Gringo::message_printer()->disable(Gringo::Warnings::W_TOTAL);
				Gringo::message_printer()->disable(Gringo::Warnings::W_VARIABLE_UNBOUNDED);
			}
			else
			{
				Gringo::message_printer()->enable(Gringo::Warnings::W_ATOM_UNDEFINED);
				Gringo::message_printer()->enable(Gringo::Warnings::W_FILE_INCLUDED);
				Gringo::message_printer()->enable(Gringo::Warnings::W_GLOBAL_VARIABLE);
				Gringo::message_printer()->enable(Gringo::Warnings::W_OPERATION_UNDEFINED);
				Gringo::message_printer()->enable(Gringo::Warnings::W_TOTAL);
				Gringo::message_printer()->enable(Gringo::Warnings::W_VARIABLE_UNBOUNDED);
			}
		}

		bool ASPSolver::existsSolution(vector<alica::Variable*>& vars, vector<shared_ptr<ConstraintDescriptor> >& calls)
		{

			this->conf->setKeyValue(this->modelsKey, "1");
			int dim = prepareSolution(vars, calls);
			auto satisfied = this->solve();
			this->removeDeadQueries();
			return satisfied;
		}

		bool ASPSolver::getSolution(vector<alica::Variable*>& vars, vector<shared_ptr<ConstraintDescriptor> >& calls,
									vector<void*>& results)
		{

			this->conf->setKeyValue(this->modelsKey, "0");
			int dim = prepareSolution(vars, calls);
			auto satisfied = this->solve();
			if (!satisfied)
			{
				this->removeDeadQueries();
				return false;
			}
			shared_ptr<vector<Gringo::ValVec>> gresults = make_shared<vector<Gringo::ValVec>>();
			for (auto query : this->registeredQueries)
			{
				for (auto pair : query->getHeadValues())
				{
					results.push_back(new AnnotatedValVec(query->getTerm()->getId(), pair.second, query));
				}
			}
			if (gresults->size() > 0)
			{
				this->removeDeadQueries();
				return true;
			}
//			if (gresults->size() > 0)
//			{
//				for (int i = 0; i < gresults->size(); ++i)
//				{
//					Gringo::ValVec *rVal = new Gringo::ValVec {gresults->at(i)};
//					results.push_back(rVal);
//				}
//				this->removeDeadQueries();
//				return true;
//			}
			else
			{
				this->removeDeadQueries();
				return false;
			}
		}

		int ASPSolver::prepareSolution(vector<alica::Variable*>& vars, vector<shared_ptr<ConstraintDescriptor> >& calls)
		{
			if (!this->masterPlanLoaded)
			{
				this->planIntegrator->loadPlanTree(this->ae->getPlanBase()->getMasterPlan());
				this->masterPlanLoaded = true;
			}
			auto cVars = make_shared<vector<shared_ptr<alica::reasoner::ASPVariable> > >(vars.size());
			for (int i = 0; i < vars.size(); ++i)
			{
				cVars->at(i) = dynamic_pointer_cast<alica::reasoner::ASPVariable>(vars.at(i)->getSolverVar());
			}
			vector<shared_ptr<alica::reasoner::ASPTerm> > constraint;
			for (auto& c : calls)
			{
				if (!(dynamic_pointer_cast<alica::reasoner::ASPTerm>(c->getConstraint()) != 0))
				{
					cerr << "ASPSolver: Type of constraint not compatible with selected solver." << endl;
					continue;
				}
				constraint.push_back(dynamic_pointer_cast<alica::reasoner::ASPTerm>(c->getConstraint()));
			}
			for (auto term : constraint)
			{
				if (term->getNumberOfModels().empty())
				{
					this->conf->setKeyValue(this->modelsKey, term->getNumberOfModels().c_str());
				}
				if (term->getType() == ASPQueryType::Variable)
				{
					this->registerQuery(make_shared<ASPVariableQuery>(this, term));
				}
				else if (term->getType() == ASPQueryType::Facts)
				{
					this->registerQuery(make_shared<ASPFactsQuery>(this, term));
				}
				else
				{
					//TODO
				}
				if (term->getExternals() != nullptr)
				{
					for (auto p : *term->getExternals())
					{
						auto it = find_if(this->assignedExternals.begin(), this->assignedExternals.end(),
											[p](shared_ptr<AnnotatedExternal> element)
											{return element->getAspPredicate() == p.first;});
						if (it == this->assignedExternals.end())
						{

							shared_ptr<Gringo::Value> val = make_shared<Gringo::Value>(
									this->gringoModule->parseValue(p.first));
							this->clingo->assignExternal(
									*val, p.second ? Gringo::TruthValue::True : Gringo::TruthValue::False);
							this->assignedExternals.push_back(make_shared<AnnotatedExternal>(p.first, val, p.second));
						}
						else
						{
							if (p.second != (*it)->getValue())
							{
								this->clingo->assignExternal(
										*((*it)->getGringoValue()),
										p.second ? Gringo::TruthValue::True : Gringo::TruthValue::False);
								(*it)->setValue(p.second);
							}
						}
					}
				}
			}
			return vars.size();
		}

		shared_ptr<SolverVariable> ASPSolver::createVariable(long id)
		{
			return make_shared<alica::reasoner::ASPVariable>();
		}

		void ASPSolver::integrateRules()
		{
			for (auto query : this->registeredQueries)
			{
				for (auto rule : query->getRules())
				{
					this->clingo->add("planBase", {}, rule);
				}
			}
			this->clingo->ground( { {"planBase", {}}}, nullptr);
		}

		/**
		 * Validates the well-formedness of a given plan.
		 *
		 * @returns False, if the plan is not valid. True, otherwise.
		 */
		bool ASPSolver::validatePlan(Plan* plan)
		{
			if (!this->masterPlanLoaded)
			{
				this->planIntegrator->loadPlanTree(plan);
				this->masterPlanLoaded = true;
			}
			this->integrateRules();
			this->reduceLifeTime();
			this->currentModels.clear();
			auto result = this->clingo->solve(bind(&ASPSolver::onModel, this, placeholders::_1), {});
			this->removeDeadQueries();
			if (result == Gringo::SolveResult::SAT)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		void ASPSolver::removeDeadQueries()
		{
			vector<shared_ptr<ASPQuery>> toRemove;
			for (auto query : this->registeredQueries)
			{
				if (query->getLifeTime() == 0)
				{
					toRemove.push_back(query);
				}
			}
			for (auto query : toRemove)
			{
				this->unregisterQuery(query);
			}
		}

		DefaultGringoModule* ASPSolver::getGringoModule()
		{
			return this->gringoModule;
		}

		const void* ASPSolver::getWildcardPointer()
		{
			return WILDCARD_POINTER;
		}

		const string& ASPSolver::getWildcardString()
		{
			return WILDCARD_STRING;
		}

		int ASPSolver::getRegisteredQueriesCount()
		{
			return this->registeredQueries.size();
		}

		shared_ptr<ClingoLib> ASPSolver::getClingo()
		{
			return this->clingo;
		}

		void ASPSolver::reduceLifeTime()
		{
			for (auto query : this->registeredQueries)
			{
				if (query->getLifeTime() > 0)
				{
					query->reduceLifeTime();
				}
			}
		}

		const long long ASPSolver::getSolvingTime()
		{
			auto claspFacade = this->clingo->clasp;

			if (claspFacade == nullptr)
				return -1;

			// time in seconds
			return claspFacade->summary().solveTime * 1000;
		}

		const long long ASPSolver::getSatTime()
		{
			auto claspFacade = this->clingo->clasp;

			if (claspFacade == nullptr)
				return -1;

			// time in seconds
			return claspFacade->summary().satTime * 1000;
		}

		const long long ASPSolver::getUnsatTime()
		{
			auto claspFacade = this->clingo->clasp;

			if (claspFacade == nullptr)
				return -1;

			// time in seconds
			return claspFacade->summary().unsatTime * 1000;
		}

		const long ASPSolver::getModelCount()
		{
			auto claspFacade = this->clingo->clasp;

			if (claspFacade == nullptr)
				return -1;

			return claspFacade->summary().enumerated();
		}

		int ASPSolver::getQueryCounter()
		{
			this->queryCounter++;
			return this->queryCounter;
		}
		const long ASPSolver::getAtomCount()
		{
			auto claspFacade = this->clingo->clasp;

			if (claspFacade == nullptr)
				return -1;

			return claspFacade->summary().lpStats()->atoms;
		}

		const long ASPSolver::getBodiesCount()
		{
			auto claspFacade = this->clingo->clasp;

			if (claspFacade == nullptr)
				return -1;

			return claspFacade->summary().lpStats()->bodies;
		}

		const long ASPSolver::getAuxAtomsCount()
		{
			auto claspFacade = this->clingo->clasp;

			if (claspFacade == nullptr)
				return -1;

			return claspFacade->summary().lpStats()->auxAtoms;
		}

		void ASPSolver::printStats()
		{
			auto claspFacade = this->clingo->clasp;

			if (claspFacade == nullptr)
				return;

			stringstream ss;
			ss << "Solve Statistics:" << endl;
			ss << "TOTAL Time: " << claspFacade->summary().totalTime << "s" << endl;
			ss << "CPU Time: " << claspFacade->summary().cpuTime << "s" << endl;
			ss << "SAT Time: " << (claspFacade->summary().satTime * 1000.0) << "ms" << endl;
			ss << "UNSAT Time: " << (claspFacade->summary().unsatTime * 1000.0) << "ms" << endl;
			ss << "SOLVE Time: " << (claspFacade->summary().solveTime * 1000.0) << "ms" << endl;

			cout << ss.str() << flush;
		}

	} /* namespace reasoner */
} /* namespace alica */

