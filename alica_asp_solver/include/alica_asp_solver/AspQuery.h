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
			AspQuery(ASPSolver* solver, string queryString, string domainName);
			AspQuery(ASPSolver* solver, string queryString, string domainName, int lifeTime);
			virtual ~AspQuery();

			shared_ptr<vector<Gringo::ValVec>> getCurrentModels();

			int getLifeTime();
			void setLifeTime(int lifeTime);
			void reduceLifeTime();

			ASPSolver* getSolver();
			string getDomainName();
			void setDomainName(string domainName);
			string getQueryString();
			bool setQueryString(string queryString);
			vector<Gringo::Value> getQueryValues();
			bool isDisjunction();

			map<Gringo::Value, vector<Gringo::Value>> getPredicateModelMap();
			map<Gringo::Value, vector<Gringo::Value>> getRuleModelMap();
			shared_ptr<map<Gringo::Value, vector<Gringo::Value>>> getSattisfiedRules();
			shared_ptr<map<Gringo::Value, vector<Gringo::Value>>> getSattisfiedPredicates();

			void saveRuleModelPair(Gringo::Value key, Gringo::Value valueVector);
			void saveStaisfiedPredicate(Gringo::Value key, Gringo::Value value);

			vector<string> getRules();
			void addRule(string domainName, string rule, bool ground);

			string toString();

		private:
			/**
			 * queryString is used to ask the solver if specific predicates are true.
			 * predicates are seperated by "," meaning all of them will be in the same rule and ";"
			 * meaning that there is arule for every predicate
			 */
			string queryString;
			ASPSolver* solver;
			shared_ptr<vector<Gringo::ValVec>> currentModels;
			vector<Gringo::Value> queryValues;
			vector<string> rules;
			// key := query value, value := predicates that satisfy the query value
			map<Gringo::Value, vector<Gringo::Value>> predicateModelMap;
			// key := rule , value := model which satisfies query
			map<Gringo::Value, vector<Gringo::Value>> ruleModelMap;
			// lifeTime == 1 => query is used once
			// lifeTime == x => query is used x times
			// LifeTime == -1 => query is used util unregistered
			int lifeTime;
			bool disjunction;
			string domainName;
			vector<Gringo::Value> createQueryValues(std::string queryString);
			void generateRules(string queryString);

		};

		} /* namespace reasoner */
	} /* namespace alica */

#endif /* SYMROCK_ALICA_ASP_SOLVER_INCLUDE_ALICA_ASP_SOLVER_ASPQUERY_H_ */
