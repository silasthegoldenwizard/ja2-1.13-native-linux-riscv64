/*
 * P710 native snapshot update: tests/native-core/native_popup_lifetime_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual popup ownership and callback lifetimes; no render assets or game data.
// Strict LeakSanitizer must also verify option/callback storage deallocation:
// destructor counts alone cannot detect explicit destructor calls without delete.
#include <initializer_list>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "mousesystem.h"
#include "popup_class.h"

namespace {
unsigned checks=0;
void check(bool value,const char* why) {
    ++checks;
    if (!value) throw std::runtime_error(why);
}
struct Counts {
    unsigned created=0, destroyed=0, called=0;
};
class CountingCallback final : public popupCallback {
    Counts& counts;
    bool result;
public:
    explicit CountingCallback(Counts& value,bool available=true)
        : counts(value),result(available) { ++counts.created; }
    ~CountingCallback() override { ++counts.destroyed; }
    bool call() override { ++counts.called; return result; }
};
std::unique_ptr<POPUP> makePopup() {
    CHAR8 name[]="Authored lifetime test";
    return std::make_unique<POPUP>(name);
}
void ownedOptions() {
    const auto baseline=gPopupIndex.size();
    Counts action,availability,hover;
    {
        auto popup=makePopup();
        for (unsigned i=0;i<4;++i) {
            // Both production insertion paths transfer heap-allocated options.
            POPUP_OPTION* option=nullptr;
            if (i%2==0) {
                option=popup->addOption(L"Authored owned option with a long allocated label",
                                       new CountingCallback(action));
                check(option!=nullptr,"name overload accepts owned option");
            } else {
                option=new POPUP_OPTION(L"Authored external heap option with a long label",
                                        new CountingCallback(action));
                check(popup->addOption(*option),"reference overload accepts owned heap option");
            }
            check(option->setAvail(new CountingCallback(availability)),"owned availability callback");
            check(option->setHover(new CountingCallback(hover)),"owned hover callback");
        }
        check(popup->callOption(0),"real popup invokes owned action");
        check(action.called==1 && availability.called==1,"action checks real availability callback");
    }
    check(action.created==4 && action.destroyed==4,"popup destroys each owned option action once");
    check(availability.destroyed==4 && hover.destroyed==4,"popup destroys owned availability and hover callbacks");
    check(gPopupIndex.size()==baseline,"owned-option popup leaves registry baseline");
}
void optionReplacement() {
    Counts action,availability,hover;
    {
        auto* initialAction=new CountingCallback(action);
        POPUP_OPTION option(L"Authored replacement option",initialAction);
        check(option.setAction(initialAction) && !action.destroyed,"same action reassignment preserves ownership");
        check(option.setAction(new CountingCallback(action)),"replace option action");
        check(action.destroyed==1,"action replacement releases preceding callback");
        auto* initialAvailability=new CountingCallback(availability,false);
        check(option.setAvail(initialAvailability),"initial unavailable predicate");
        check(option.setAvail(initialAvailability) && !availability.destroyed,
              "same availability reassignment preserves ownership");
        check(!option.run() && !action.called,"unavailable option does not invoke action");
        check(option.setAvail(new CountingCallback(availability)),"replace option availability");
        check(availability.destroyed==1,"availability replacement releases preceding callback");
        auto* initialHover=new CountingCallback(hover);
        check(option.setHover(initialHover),"initial hover callback");
        check(option.setHover(initialHover) && !hover.destroyed,"same hover reassignment preserves ownership");
        check(option.setHover(new CountingCallback(hover)),"replace option hover");
        check(hover.destroyed==1,"hover replacement releases preceding callback");
        check(option.run() && option.runHoverCallback(nullptr),"replacement callbacks remain callable");
        check(action.called==1 && hover.called==1,"replacement action and hover run once");
    }
    check(action.destroyed==2 && availability.destroyed==2 && hover.destroyed==2,
          "option destruction releases remaining replacement callbacks once");
}
void popupCallbackReplacement() {
    Counts counts;
    {
        auto popup=makePopup();
        for (UINT8 type:{POPUP_CALLBACK_INIT,POPUP_CALLBACK_SHOW,POPUP_CALLBACK_HIDE,POPUP_CALLBACK_END}) {
            auto* initial=new CountingCallback(counts);
            check(popup->setCallback(type,initial),"initial popup callback");
            const auto before=counts.destroyed;
            check(popup->setCallback(type,initial) && counts.destroyed==before,
                  "same popup callback reassignment preserves ownership");
            check(popup->setCallback(type,new CountingCallback(counts)),"replace popup callback");
            check(counts.destroyed==before+1,"popup replacement releases preceding callback");
            check(popup->isCallbackSet(type),"replacement popup callback remains bound");
        }
        check(counts.created==8 && counts.destroyed==4,"four popup replacements release four callbacks");
    }
    check(counts.destroyed==8,"popup destruction releases all replacement callbacks once");
    check(counts.called==1,"only final end callback runs for an undisplayed popup");
}
void childOwnership() {
    const auto baseline=gPopupIndex.size();
    Counts action,end;
    {
        auto parent=makePopup();
        POPUP* child=parent->addSubMenuOption(L"Authored child submenu");
        check(child!=nullptr,"real submenu creates child popup");
        POPUP* grandchild=child->addSubMenuOption(L"Authored grandchild submenu");
        check(grandchild!=nullptr,"real submenu creates grandchild popup");
        check(gPopupIndex.size()==baseline+3,"nested popups enter actual registry");
        check(child->addOption(L"Owned child option",new CountingCallback(action))!=nullptr,
              "child owns option callback");
        check(grandchild->addOption(L"Owned grandchild option",new CountingCallback(action))!=nullptr,
              "grandchild owns option callback");
        check(child->setCallback(POPUP_CALLBACK_END,new CountingCallback(end)),"child end callback");
        check(grandchild->setCallback(POPUP_CALLBACK_END,new CountingCallback(end)),"grandchild end callback");
    }
    check(gPopupIndex.size()==baseline,"parent destruction removes complete submenu tree");
    check(action.destroyed==2,"parent destruction releases nested option callbacks");
    check(end.destroyed==2 && end.called==2,"nested end callbacks run and release once");
}
void regionIndexLifetime() {
    const auto baseline=gPopupRegionIndex.size();
    auto popup=makePopup();
    for (unsigned i=0;i<16;++i) {
        MOUSE_REGION region{};
        MSYS_DefineRegion(&region,1,1,9,9,MSYS_PRIORITY_NORMAL,MSYS_NO_CURSOR,nullptr,nullptr);
        check(region.IDNumber!=0,"production mouse region receives identity");
        check(registerPopupRegion(region.IDNumber,popup->id),"register actual popup region");
        check(gPopupRegionIndex.size()==baseline+1,"popup region index gains one value");
        const auto* entry=findMouseRegionInIndex(region.IDNumber);
        check(entry && entry->classId==popup->id,"popup region resolves owner identity");
        check(unregisterPopupRegion(region.IDNumber),"unregister actual popup region");
        check(gPopupRegionIndex.size()==baseline && !findMouseRegionInIndex(region.IDNumber),
              "popup region index returns to baseline");
        MSYS_RemoveRegion(&region);
    }
}
class DeletePopupCallback final : public popupCallback {
    POPUP*& owner;
    Counts& counts;
public:
    DeletePopupCallback(POPUP*& value,Counts& observed) : owner(value),counts(observed) { ++counts.created; }
    ~DeletePopupCallback() override { ++counts.destroyed; }
    bool call() override {
        ++counts.called;
        POPUP* retiring=owner;
        owner=nullptr;
        delete retiring;
        // The action and this callback have retired. Do not read their members.
        return true;
    }
};
void actionDeletesPopup() {
    const auto baseline=gPopupIndex.size();
    Counts counts;
    POPUP* popup=makePopup().release();
    const UINT32 id=popup->id;
    check(popup->addOption(L"Delete this popup during its action",new DeletePopupCallback(popup,counts))!=nullptr,
          "self-retiring action added to production popup");
    POPUP* invoking=popup;
    // Exercise the public dispatch path without showing a box or loading assets.
    // There is intentionally no assertion about the return value after deletion.
    (void)invoking->callOption(0);
    check(!popup && counts.called==1 && counts.destroyed==1,"action deletes its popup and callback once");
    check(gPopupIndex.size()==baseline && !findPopupInIndex(id),"self-retired popup leaves no registered owner");
}
void leakControl() {
    Counts counts;
    {
        auto popup=makePopup();
        check(popup->addOption(L"Authored heap option for before-repair leak control",
                              new CountingCallback(counts))!=nullptr,"leak-control option insertion");
        check(popup->setCallback(POPUP_CALLBACK_SHOW,new CountingCallback(counts)),"leak-control popup callback");
    }
    // Return normally even with the old explicit-destructor implementation.
    // LSan must diagnose the lost option/callback storage after process exit.
}
}
int main(int argc,char** argv) {
    try {
        MSYS_Init();
        if (argc==2 && std::string(argv[1])=="--leak-control") {
            leakControl();
            MSYS_Shutdown();
            std::cout<<"Completed actual popup leak control; strict LSan must verify storage release\n";
            return 0;
        }
        check(argc==1,"expected no arguments or --leak-control");
        const auto popupBaseline=gPopupIndex.size(),regionBaseline=gPopupRegionIndex.size();
        ownedOptions(); optionReplacement(); popupCallbackReplacement();
        childOwnership(); regionIndexLifetime(); actionDeletesPopup();
        check(gPopupIndex.size()==popupBaseline && gPopupRegionIndex.size()==regionBaseline,
              "all production popup registries return to baseline");
        MSYS_Shutdown();
        std::cout<<"PASS: "<<checks<<" actual popup lifetime assertions; strict LSan must verify storage release\n";
        return 0;
    } catch (const std::exception& error) {
        MSYS_Shutdown();
        std::cerr<<"FAIL after "<<checks<<" assertions: "<<error.what()<<'\n';
        return 1;
    }
}
