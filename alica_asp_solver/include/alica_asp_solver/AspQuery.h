/*
 * AspQuery.h
 *
 *  Created on: Jun 8, 2016
 *      Author: Stefan Jakob
 */

#ifndef SYMROCK_ALICA_ASP_SOLVER_INCLUDE_ALICA_ASP_SOLVER_ASPQUERY_H_
#define SYMROCK_ALICA_ASP_SOLVER_INCLUDE_ALICA_ASP_SOLVER_ASPQUERY_H_

#include <string>
#include <clingo/clingocontrol.hh>

using namespace std;

namespace alica
{
	namespace reasoner
	{

		class ASPSolver;
		class AspQuery
		{
		public:
			AspQuery(ASPSolver* solver);
			AspQuery(ASPSolver* solver, string queryString);
			AspQuery(ASPSolver* solver, string queryString, int lifeTime);
			virtual ~AspQuery();
			shared_ptr<vector<Gringo::ValVec>> getCurrentModels();
			int getLifeTime();
			void setLifeTime(int lifeTime);
			string getQueryString();
			bool setQueryString(string queryString);
			void reduceLifeTime();
			map<Gringo::Value, vector<Gringo::ValVec>> getPredicateModelMap();
			map<Gringo::Value, vector<Gringo::ValVec>> getRuleModelMap();
			void savePredicateModelPair(Gringo::Value key, Gringo::ValVec valueVector);
			void saveRuleModelPair(Gringo::Value key, Gringo::ValVec valueVector);
			bool isDisjunction();
			vector<Gringo::Value> getQueryValues();
			shared_ptr<map<Gringo::Value, vector<Gringo::ValVec>>> getSattisfiedPredicates();
			shared_ptr<map<Gringo::Value, vector<Gringo::ValVec>>> getSattisfiedRules();
			string toString();
			void createRules(string domainName);
			vector<string> getRules();
			void addRuleAndGround(string domainName, string rule);

		private:
			void addRule(string domainName, string rule, string ruleIdentifier);
			string queryString;
			ASPSolver* solver;
			shared_ptr<vector<Gringo::ValVec>> currentModels;
			vector<Gringo::Value> queryValues;
			vector<string> rules;
			// key := query value, value := model which satisfies query
			map<Gringo::Value, vector<Gringo::ValVec>> predicateModelMap;
			// key := rule , value := model which satisfies query
			map<Gringo::Value, vector<Gringo::ValVec>> ruleModelMap;
			// lifeTime == 1 => query is used once
			// lifeTime == x => query is used x times
			// LifeTime == -1 => query is used util unregistered
			int lifeTime;
			bool disjunction;
			int counter;
			vector<Gringo::Value> createQueryValues(std::string const &queryString);

		};

		} /* namespace reasoner */
	} /* namespace alica */

#endif /* SYMROCK_ALICA_ASP_SOLVER_INCLUDE_ALICA_ASP_SOLVER_ASPQUERY_H_ */
