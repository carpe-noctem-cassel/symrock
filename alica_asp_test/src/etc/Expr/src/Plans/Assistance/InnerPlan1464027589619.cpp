#include "Plans/Assistance/InnerPlan1464027589619.h"
using namespace alica;
/*PROTECTED REGION ID(eph1464027589619) ENABLED START*/ //Add additional using directives here
/*PROTECTED REGION END*/
namespace alicaAutogenerated
{
    //Plan:InnerPlan

    //Check of PreCondition - (Name): NewPreCondition, (ConditionString): agent(A), in(A,p1464027589619, tsk1225112227903, S). , (Comment) :  

    /*
     * Available Vars:
     */
    bool PreCondition1464079686712::evaluate(shared_ptr<RunningPlan> rp)
    {
        /*PROTECTED REGION ID(1464079686712) ENABLED START*/
        //--> "PreCondition:1464079686712  not implemented";
        return true;
        /*PROTECTED REGION END*/
    }

    /* generated comment
     
     Task: DefaultTask  -> EntryPoint-ID: 1464027605608

     */
    shared_ptr<UtilityFunction> UtilityFunction1464027589619::getUtilityFunction(Plan* plan)
    {
        /*PROTECTED REGION ID(1464027589619) ENABLED START*/

        shared_ptr < UtilityFunction > defaultFunction = make_shared < DefaultUtilityFunction > (plan);
        return defaultFunction;

        /*PROTECTED REGION END*/
    }

//State: State in Plan: InnerPlan

}
