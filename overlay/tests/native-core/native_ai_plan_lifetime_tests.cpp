/*
 * P710 native snapshot update: tests/native-core/native_ai_plan_lifetime_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual soldier reset/teardown and legacy plan factories, without game assets.
#include "Soldier Control.h"
#include "Overhead.h"
#include "Isometric Utils.h"
#include "../../ModularizedTacticalAI/include/LegacyAIPlanFactory.h"
#include "../../ModularizedTacticalAI/include/PlanList.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace {
unsigned checks=0;
void check(bool value,const char* why) {
    ++checks;
    if (!value) { std::cerr<<"FAIL: "<<why<<'\n'; std::abort(); }
}
struct Counts { unsigned destroyed=0, detached=0; };
class CountingPlan final : public AI::tactical::Plan {
    Counts& counts;
public:
    CountingPlan(SOLDIERTYPE* owner,Counts& observed) : Plan(owner),counts(observed) {}
    ~CountingPlan() override {
        ++counts.destroyed;
        if (!get_npc()->ai_masterplan_) ++counts.detached;
    }
    void execute(AI::tactical::PlanInputData&) override {}
    bool done() const override { return false; }
};
std::unique_ptr<SOLDIERTYPE> soldier() {
    auto value=std::make_unique<SOLDIERTYPE>();
    check(!value->ai_masterplan_,"actual soldier constructor starts without an owned plan");
    value->ubID=SoldierID(0);
    value->iFaceIndex=-1;
    value->iLight=-1;
    value->sGridNo=NOWHERE;
    value->sReservedMovementGridNo=NOWHERE;
    value->bActive=TRUE;
    return value;
}
void resetAndDestruction() {
    Counts counts;
    {
        auto owner=soldier();
        owner->ai_masterplan_=new CountingPlan(owner.get(),counts);
        owner->initialize();
        check(!owner->ai_masterplan_ && counts.destroyed==1 && counts.detached==1,
              "actual initialize retires its plan before resetting soldier state");
        owner->initialize();
        check(counts.destroyed==1,"repeated initialization does not retire a plan twice");
        owner->ai_masterplan_=new CountingPlan(owner.get(),counts);
    }
    check(counts.destroyed==2 && counts.detached==2,
          "actual soldier destructor destroys a later owned plan through the virtual interface");
}
void teardownAndSlotReuse() {
    Counts counts;
    {
        auto owner=soldier();
        owner->ai_masterplan_=new CountingPlan(owner.get(),counts);
        check(owner->DeleteSoldier(),"actual soldier resource teardown succeeds without render assets");
        check(!owner->ai_masterplan_ && counts.destroyed==1 && counts.detached==1,
              "explicit teardown retires the plan before soldier slot reuse");
        check(owner->DeleteSoldier() && counts.destroyed==1,
              "repeated resource teardown does not retire the plan twice");
        SOLDIERTYPE replacement;
        *owner=replacement;
        check(!owner->ai_masterplan_,"plan-free replacement assignment leaves no stale plan owner");
        auto copied=std::make_unique<SOLDIERTYPE>(*owner);
        check(!copied->ai_masterplan_,"copy of the retired slot does not inherit its former plan");
        owner->initialize();
    }
    check(counts.destroyed==1,"teardown followed by assignment, reset and destruction retires exactly once");
}
void ownedPlanTree() {
    Counts counts;
    auto owner=soldier();
    auto* tree=new AI::tactical::PlanList(owner.get());
    tree->add_subplan(new CountingPlan(owner.get(),counts));
    tree->add_subplan(new CountingPlan(owner.get(),counts));
    owner->ai_masterplan_=tree;
    owner->initialize();
    check(!owner->ai_masterplan_ && counts.destroyed==2 && counts.detached==2,
          "complete plan type retires the real plan tree and its owned children");
}
void actualLegacyPlans() {
    AI::tactical::LegacyAIPlanFactory factory;
    AI::tactical::AIInputData input;
    auto owner=soldier();
    for (unsigned cycle=0;cycle<8;++cycle) {
        owner->ai_masterplan_=factory.create_plan(owner.get(),input);
        check(owner->ai_masterplan_ && !owner->ai_masterplan_->done(),"actual legacy create path returns an owned live plan");
        owner->initialize();
        check(!owner->ai_masterplan_,"reset retires actual create-path plan");
        factory.update_plan(owner.get(),input);
        auto* existing=owner->ai_masterplan_;
        check(existing!=nullptr,"actual legacy update path creates an absent plan");
        factory.update_plan(owner.get(),input);
        check(owner->ai_masterplan_==existing,"update preserves an existing plan");
        owner->initialize();
        check(!owner->ai_masterplan_,"reset retires actual update-path plan");
    }
}
void leakControl() {
    AI::tactical::LegacyAIPlanFactory factory;
    AI::tactical::AIInputData input;
    auto created=soldier(),updated=soldier();
    created->ai_masterplan_=factory.create_plan(created.get(),input);
    factory.update_plan(updated.get(),input);
    check(created->ai_masterplan_ && updated->ai_masterplan_,"both real allocation paths create plans");
    created->DeleteSoldier();
    updated->DeleteSoldier();
    // Match production slot teardown followed by replacement assignment.
    // The before source loses both allocations here; return normally for LSan.
    SOLDIERTYPE replacement;
    *created=replacement;
    *updated=replacement;
}
}
int main(int argc,char** argv) {
    if (argc==2 && std::string(argv[1])=="--leak-control") {
        leakControl();
        std::cout<<"Completed real legacy-plan slot-reuse control; strict LSan must verify release\n";
        return 0;
    }
    check(argc==1,"expected no arguments or --leak-control");
    resetAndDestruction();
    teardownAndSlotReuse();
    ownedPlanTree();
    actualLegacyPlans();
    std::cout<<"PASS: "<<checks<<" actual soldier/AI-plan lifetime assertions\n";
}
