#program alicaBackground.

%%%%% General relations between ALICA program elements

% A plan without parents is a root plan.
rootPlan(P) :- plan(P), not hasParentPlan(P, X) : plan(X), X!=P.

%Transitivity of hasParentPlan
hasParentPlan(Child, Parent) :- plan(Parent), plan(Child), hasState(Parent, S), hasPlan(S, Child).
hasParentPlan(Child, Parent) :- plan(Parent), plan(Child), planType(PT), hasState(Parent, S), hasPlanType(S, PT), hasRealisation(PT, Child).
%hasParentPlan(Child, Parent) :- hasParentPlan(Child, MidPlan), hasParentPlan(MidPlan, Parent), Child!=Parent.

% Success and Failure States are subsets of Terminal States, which are subsets of States.
terminalState(S) :- successState(S).
terminalState(S) :- failureState(S).
state(S) :- terminalState(S).

% Inverse relations between predicates.
hasState(P,S) :- isStateOf(S,P).
isStateOf(S,P) :- hasState(P,S).
hasTransition(P,T) :- isTransitionOf(T,P).
hasInTransition(S,T) :- hasOutState(T,S).
hasOutState(T,S) :- hasInTransition(S,T).
hasOutTransition(S,T) :- hasInState(T,S).
hasInState(T,S) :- hasOutTransition(S,T).

% Reachability of states within plans
isReachableFrom(S,EP) :- state(S), entryPoint(EP), hasInitialState(EP,S).
isReachableFrom(S2,S1) :- state(S1), state(S2), transition(T), hasInState(T,S1), hasOutState(T,S2).
isReachableFrom(S2,EP) :- state(S1), state(S2), entryPoint(EP), hasInitialState(EP,S1), isReachableFrom(S2,S1).
isReachableFrom(S3,S1) :- state(S1), state(S2), state(S3), isReachableFrom(S2,S1), isReachableFrom(S3,S2).

%%%%% General realtions for plan base 

in(A,P,T,S) :- agent(A), plan(P), task(T), state(S), hasTask(P,T), hasState(P,S), entryPoint(E), hasEntryPoint(P,T,E), isReachableFrom(S,E), isIn(A,S).

%%%%% Conditions for well-formed ALICA programs

% States belong to at most on plan.
brokenState(S) :- state(S), ParentPlansCnt = #count {P : isStateOf(S,P), plan(P)}, ParentPlansCnt > 1.

% No transition connects states in different plans.
brokenTransition(T) :- transition(T), state(SIN), state (SOut), plan(PIn) ,plan(POut), hasInState(T,SIn), hasOutState(T,SOut), isStateOf(SIn, PIn), isStateOf(SOut, POut), PIn != POut.

% Synchronisations only happen within a plan.
brokenSynchronisation(Synch) :- synchronisation(Synch), hasSynchedTransition(Synch, T1), hasSynchedTransition(Synch, T2), hasTransition(P2, T2), hasTransition(P1,T1), P1 != P2.

% Synchronsations must sync at least two transitions.
brokenSynchronisation(Synch) :- synchronisation(Synch), SynchedTransitionCnt = #count{T : hasSynchedTransition(Synch, T)}, SynchedTransitionCnt < 2.

% The failure and success states of a plan are disjoint (subsets of its states).
brokenTerminalState(S) :- successState(S), failureState(S).
brokenTerminalState(S) :- hasTerminalState(P, S), not hasState(P,S).

% Terminal states do not include sub-plans or behaviours
brokenTerminalState(S) :- terminalState(S), hasPlan(S,P).
brokenTerminalState(S) :- terminalState(S), hasBehaviour(S,B).

% There is a postcondition associated with each success and failure state. 
brokenTerminalState(S) :- terminalState(S), PostConditionCnt = #count{PostC : hasPostCondition(S,PostC), postCondition(PostC)}, PostConditionCnt != 1.

% A task associated with a plan identifies an initial state within that plan
brokenPlanTaskPair(P, T) :- plan(P), task(T), hasTask(P,T), EntryPointCnt = #count{E : hasEntryPoint(P,T,E), entryPoint(E)}, EntryPointCnt != 1.
brokenEntryPoint(E) :- entryPoint(E), InitialStateCnt = #count{S : hasInitialState(E,S)}, InitialStateCnt > 1.

%% CARDINALITIES

% All plan-task pairs have a valid cardinality interval associated. 
brokenEntryPoint(E) :- entryPoint(E), MinCardinalityCnt = #count{Min : hasMinCardinality(E,Min)}, MinCardinalityCnt != 1.
brokenEntryPoint(E) :- entryPoint(E), MaxCardinalityCnt = #count{Max : hasMaxCardinality(E,Max)}, MaxCardinalityCnt != 1.
brokenEntryPoint(E) :- entryPoint(E), hasMinCardinality(E, Min), hasMaxCardinality(E, Max), Min > Max.

% The sum of the minimum cardinalities of a child plan is large than the maximum cardinality of the parents entry point		 
brokenPlan(ChildPlan) :- plan(ChildPlan), SumOfChildPlanMinCard = #sum { ChildMin : entryPoint(ChildEntryPoint), hasMinCardinality(ChildEntryPoint,ChildMin), hasEntryPoint(ChildPlan, ChildTask, ChildEntryPoint)}, plan(ParentPlan), entryPoint(ParentEntrypoint), hasMaxCardinality(ParentEntrypoint, ParentMax), hasEntryPoint(ParentPlan, ParentTask, ParentEntryPoint), state(ParentState), isReachableFrom(ParentState, ParentEntrypoint), hasPlan(ParentState, ChildPlan), SumOfChildPlanMinCard > ParentMax. 						 

%% CYCLES

% Cycles are not allowed in the plan hierarchy
cycleFree(P) :- plan(P), cycleFree(Parent):hasParentPlan(P,Parent).
cyclicPlan(P) :- plan(P), not cycleFree(P).

% There is at most one root plan.
brokenPlan(P1) :- rootPlan(P1), rootPlan(P2), P1 != P2.


% The transitive closure of the parent-child relationship is asymmetric.
%brokenPlan(P) :- plan(P), hasParentPlan(P,P).

% Each state is only reachable from exactly one entry point.
brokenState(S) :- state(S), ReachableEntryPointCnt = #count{EP: isReachableFrom(S,EP), entryPoint(EP)}, ReachableEntryPointCnt != 1.
						 


%%%%% Plan base axioms

% An agent can only occupy one state per plan
brokenPlanBase(A) :- agent(A), plan(P), IsInStateCnt = #count{S : isIn(A,S), state(S), hasState(P,S)}, IsInStateCnt != 1.  
 
% An agent can only execute one plan per plantype
brokenPlanBase(A) :- agent(A), planType(PT), InPlanTypePlanCnt = #count{P : hasRealisation(PT,P), plan(P), state(S), hasState(P,S), isIn(A,S)}, InPlanTypePlanCnt > 1.  

% The state occupied by an agent must be reachable (isReachableFrom) by its corresponding plan-task pair
brokenPlanBase(A) :- agent(A), plan(P), task(T), state(S), entryPoint(E), hasEntryPoint(P,T,E), isIn(A,S), hasTask(P,T), hasState(P,S), not isReachableFrom(S,E).

%%%%% Locality of In-Predicate

brokenPlanBase(A) :- agent(A), plan(P), task(T), state(S), in(A,P,T,S), not hasState(P,S).
brokenPlanBase(A) :- agent(A), plan(P), task(T), state(S), in(A,P,T,S), not hasTask(P,T).
brokenPlanBase(A) :- agent(A), plan(P), task(T), state(S), in(A,P,T,S), hasEntryPoint(P,T,E), not isReachableFrom(S,E).