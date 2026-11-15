/*
 * P710 native snapshot update: tests/native-core/native_popup_callback_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native port callback verification, 2026-11-12. No game data or UI runtime.
#include "popup_callback.h"
#include "FileMan.h"
#include "mousesystem.h"
#include "Item Types.h"
#include <limits>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace
{
unsigned checks=0;
int observed=0;
void check(bool value, const char* label)
{ ++checks; if (!value) throw std::runtime_error(label); }
void action0() { observed=17; }
void replacement0() { observed=23; }
int condition0() { return 42; }
int unavailable0() { return 0; }
void action1(UINT32 value) { observed=static_cast<int>(value); }
void replacement1(UINT32 value) { observed=-static_cast<int>(value); }
bool condition1(void* value) { return value!=nullptr; }
BOOLEAN booleanCondition(void* value) { return value && *static_cast<int*>(value)>0 ? TRUE : FALSE; }
INT32 integerCondition(void* value) { return value ? *static_cast<int*>(value) : 0; }
INT32 throwingAdaptedCondition(void*) { throw std::runtime_error("adapted predicate exception"); }
double invalidFloatingCondition(void*) { return 1.0; }
INT32 invalidArgument(int*) { return 1; }
// These are authored predicates with the real source signatures, not engine
// replacements. The borrowed TransformInfoStruct is an actual engine type.
BOOLEAN unavailableBoolean0() { return FALSE; }
BOOLEAN availableBoolean0() { return 7; }
BOOLEAN throwingBoolean0() { throw std::runtime_error("zero BOOLEAN exception"); }
BOOLEAN objectBooleanCondition(OBJECTTYPE* object) { return object ? TRUE : FALSE; }
BOOLEAN transformBooleanCondition(TransformInfoStruct* transform)
{ return transform && transform->usAPCost ? 9 : FALSE; }
BOOLEAN throwingTransformCondition(TransformInfoStruct*) { throw std::runtime_error("typed pointer exception"); }
template<typename Callback, typename Function, typename = void>
struct CanBind : std::false_type {};
template<typename Callback, typename Function>
struct CanBind<Callback, Function, std::void_t<decltype(std::declval<Callback&>().bind(std::declval<Function>()))>> : std::true_type {};
template<typename Value, typename = void>
struct CanMakeNumericID : std::false_type {};
template<typename Value>
struct CanMakeNumericID<Value, std::void_t<decltype(SoldierID::FromLegacyNumericID(std::declval<Value>()))>> : std::true_type {};
static_assert(sizeof(SoldierID) == sizeof(UINT16), "SoldierID storage/resource width must remain UINT16");
static_assert(alignof(SoldierID) == alignof(UINT16));
static_assert(std::is_standard_layout<SoldierID>::value && std::is_trivially_copyable<SoldierID>::value);
static_assert(CanMakeNumericID<std::int64_t>::value && CanMakeNumericID<std::uint64_t>::value);
static_assert(!CanMakeNumericID<void*>::value && !CanMakeNumericID<OBJECTTYPE*>::value);
static_assert(!CanMakeNumericID<UINT8>::value && !CanMakeNumericID<INT8>::value && !CanMakeNumericID<bool>::value);
static_assert(!std::is_constructible<SoldierID,UINT8>::value && !std::is_constructible<SoldierID,INT8>::value);
static_assert(std::is_constructible<popupCallbackFunction<BOOLEAN,void>,decltype(&unavailableBoolean0)>::value);
static_assert(!std::is_constructible<popupCallbackFunction<BOOLEAN,void*>,decltype(&unavailableBoolean0),void*>::value);
static_assert(!std::is_constructible<popupCallbackFunction<BOOLEAN,TransformInfoStruct*>,decltype(&objectBooleanCondition),TransformInfoStruct*>::value);
static_assert(!CanBind<popupCallbackFunction<BOOLEAN,TransformInfoStruct*>,decltype(&objectBooleanCondition)>::value);
static_assert(!CanBind<popupCallbackFunction<BOOLEAN,void>,decltype(&objectBooleanCondition)>::value);
void action2(INT16 first, UINT32 second) { observed=first+static_cast<int>(second); }
int condition2(INT16 first, UINT32 second) { return first-static_cast<int>(second); }
void action3(int* target, UINT16 value, int* other) { *target=value+*other; }
int condition3(int* target, UINT16 value, int* other) { return *target-value-*other; }
void throwing0() { throw std::runtime_error("callback exception"); }
void throwing1(UINT32) { throw std::runtime_error("callback exception"); }
int throwingCondition() { throw std::runtime_error("predicate exception"); }
struct Owned : popupCallbackFunction<void,void>
{
    bool& destroyed;
    explicit Owned(bool& value) : popupCallbackFunction(&action0),destroyed(value) {}
    ~Owned() override { destroyed=true; }
};
// A pointer to an unrelated signature and an erased object pointer must never
// cross a callback constructor. Actual game callers pass free-function pointers.
static_assert(std::is_constructible<popupCallbackFunction<void,UINT32>,decltype(&action1),UINT32>::value);
static_assert(!std::is_constructible<popupCallbackFunction<void,UINT32>,decltype(&action0),UINT32>::value);
static_assert(!std::is_constructible<popupCallbackFunction<void,UINT32>,void*,UINT32>::value);
static_assert(!std::is_constructible<popupCallbackFunction<void,void>,void*>::value);
static_assert(!std::is_constructible<popupCallbackFunction<int,void>,void*>::value);
static_assert(!std::is_constructible<popupCallbackFunction2<int,INT16,UINT32>,void*,INT16,UINT32>::value);
static_assert(!std::is_constructible<popupCallbackFunction2<void,INT16,UINT32>,void*,INT16,UINT32>::value);
static_assert(!std::is_constructible<popupCallbackFunction3<int,int*,UINT16,int*>,void*,int*,UINT16,int*>::value);
static_assert(!std::is_constructible<popupCallbackFunction3<void,int*,UINT16,int*>,void*,int*,UINT16,int*>::value);
static_assert(std::has_virtual_destructor<popupCallback>::value);
static_assert(std::is_constructible<popupCallbackFunction<bool,void*>,decltype(&booleanCondition),void*>::value);
static_assert(std::is_constructible<popupCallbackFunction<bool,void*>,decltype(&integerCondition),void*>::value);
static_assert(!std::is_constructible<popupCallbackFunction<bool,void*>,decltype(&invalidFloatingCondition),void*>::value);
static_assert(!std::is_constructible<popupCallbackFunction<bool,void*>,decltype(&invalidArgument),void*>::value);
static_assert(!std::is_constructible<popupCallbackFunction<bool,void*>,void*,void*>::value);
}
int main()
{
    try
    {
        popupCallbackFunction<void,void> zero(&action0);
        popupCallback* base=&zero;
        check(base->call() && observed==17,"zero-argument void dispatch through base");
        zero.bind(&replacement0); check(zero.call() && observed==23,"typed zero-argument rebind");
        popupCallbackFunction<int,void> condition(&condition0);
        check(condition.call(),"nonzero return coerces to true");
        condition.bind(&unavailable0); check(!condition.call(),"zero return coerces to false");
        popupCallbackFunction<void,UINT32> one(&action1,200);
        check(one.call() && observed==200,"one-argument void callback");
        one.bind(&replacement1); check(one.call() && observed==-200,"one-argument typed rebind retains captured parameter");
        popupCallbackFunction<void,UINT32> defaultOne;
        defaultOne.bind(&action1); check(defaultOne.call() && observed==0,"default callback captures value-initialized parameter");
        int object=7;
        popupCallbackFunction<bool,void*> available(&condition1,&object), unavailable(&condition1,nullptr);
        check(available.call() && !unavailable.call(),"actual availability pointer signature");
        popupCallbackFunction<bool,void*> adaptedBoolean(&booleanCondition,&object);
        check(adaptedBoolean.call(),"BOOLEAN predicate converts true without ABI cast");
        object=0; check(!adaptedBoolean.call(),"BOOLEAN predicate observes changing borrowed state");
        adaptedBoolean.bind(&integerCondition); object=-27;
        check(adaptedBoolean.call(),"negative INT32 predicate converts to true");
        object=0; check(!adaptedBoolean.call(),"INT32 zero predicate converts to false");
        adaptedBoolean.bind(static_cast<INT32 (*)(void*)>(nullptr));
        check(!adaptedBoolean.call(),"null integral predicate is unavailable");
        adaptedBoolean.bind(&condition1); check(adaptedBoolean.call(),"rebind from integral to bool retains captured pointer");
        adaptedBoolean.bind(nullptr); check(!adaptedBoolean.call(),"null bool predicate is unavailable");
        popupCallbackFunction<bool,void*> defaultPredicate;
        check(!defaultPredicate.call(),"default predicate is unavailable");
        defaultPredicate.bind(&integerCondition); check(!defaultPredicate.call(),"default predicate has null captured state");
        adaptedBoolean.bind(&throwingAdaptedCondition);
        bool adaptedPropagated=false;
        try { adaptedBoolean.call(); } catch (const std::runtime_error&) { adaptedPropagated=true; }
        check(adaptedPropagated,"adapted predicate exceptions retain propagation");
        popupCallbackFunction<BOOLEAN,void> typedOff(&unavailableBoolean0);
        check(!typedOff.call(),"real zero-argument BOOLEAN availability arity");
        typedOff.bind(&availableBoolean0); check(typedOff.call(),"nonzero BOOLEAN zero-argument predicate");
        typedOff.bind(nullptr); check(!typedOff.call(),"null exact zero-argument predicate is unavailable");
        popupCallbackFunction<BOOLEAN,void> defaultTypedOff;
        check(!defaultTypedOff.call(),"default exact zero-argument predicate is unavailable");
        typedOff.bind(&throwingBoolean0);
        bool typedZeroPropagated=false;
        try { typedOff.call(); } catch (const std::runtime_error&) { typedZeroPropagated=true; }
        check(typedZeroPropagated,"exact BOOLEAN zero-argument predicate exceptions propagate");
        popupCallbackFunction<BOOLEAN,OBJECTTYPE*> objectPredicate(&objectBooleanCondition,nullptr);
        check(!objectPredicate.call(),"exact OBJECTTYPE pointer null payload");
        TransformInfoStruct transform{};
        popupCallbackFunction<BOOLEAN,TransformInfoStruct*> transformPredicate(&transformBooleanCondition,&transform);
        check(!transformPredicate.call(),"actual TransformInfoStruct borrowed zero state");
        transform.usAPCost=4;
        check(transformPredicate.call(),"actual TransformInfoStruct borrowed state mutation");
        transformPredicate.bind(nullptr); check(!transformPredicate.call(),"null typed pointer predicate is unavailable");
        transformPredicate.bind(&throwingTransformCondition);
        bool typedPointerPropagated=false;
        try { transformPredicate.call(); } catch (const std::runtime_error&) { typedPointerPropagated=true; }
        check(typedPointerPropagated,"exact typed pointer predicate exceptions propagate");
        popupCallbackFunction<BOOLEAN,TransformInfoStruct*> defaultTransform;
        check(!defaultTransform.call(),"default exact typed pointer predicate is unavailable");
        defaultTransform.bind(&transformBooleanCondition);
        check(!defaultTransform.call(),"default typed pointer payload remains null");
        check(SoldierID::FromLegacyNumericID(std::int64_t{0}).i==0,"wide numeric zero ID");
        check(SoldierID::FromLegacyNumericID(std::int64_t{TOTAL_SOLDIERS-1}).i==TOTAL_SOLDIERS-1,"last valid numeric ID");
        check(SoldierID::FromLegacyNumericID(std::int64_t{TOTAL_SOLDIERS}).i==NOBODY.i,"numeric sentinel ID");
        check(SoldierID::FromLegacyNumericID(std::int64_t{-1}).i==NOBODY.i,"legacy negative-one sentinel");
        check(SoldierID::FromLegacyNumericID(std::int64_t{65536}).i==SoldierID(INT32{65536}).i,"preserve signed32 wrap before clamp");
        check(SoldierID::FromLegacyNumericID(std::uint64_t{65536}).i==SoldierID(UINT32{65536}).i,"preserve unsigned32 wrap before clamp");
        check(SoldierID::FromLegacyNumericID(std::int64_t{-65536}).i==SoldierID(INT32{-65536}).i,"preserve legacy negative32 wrap");
        check(SoldierID::FromLegacyNumericID(std::int64_t{std::numeric_limits<INT32>::min()}).i==SoldierID(std::numeric_limits<INT32>::min()).i,"signed32 minimum retains constructor behavior");
        check(SoldierID::FromLegacyNumericID(std::int64_t{std::numeric_limits<INT32>::max()}).i==SoldierID(std::numeric_limits<INT32>::max()).i,"signed32 maximum retains constructor behavior");
        check(SoldierID::FromLegacyNumericID(std::uint64_t{std::numeric_limits<UINT32>::max()}).i==SoldierID(std::numeric_limits<UINT32>::max()).i,"unsigned32 maximum retains constructor behavior");
        check(SoldierID::FromLegacyNumericID(std::int64_t{std::numeric_limits<INT32>::max()}+1).i==NOBODY.i,"refuse positive values outside signed32 domain");
        check(SoldierID::FromLegacyNumericID(std::int64_t{std::numeric_limits<INT32>::min()}-1).i==NOBODY.i,"refuse values below signed32 domain");
        check(SoldierID::FromLegacyNumericID(std::uint64_t{std::numeric_limits<UINT32>::max()}+1).i==NOBODY.i,"refuse values outside unsigned32 domain");
        check(SoldierID::FromLegacyNumericID(std::numeric_limits<std::int64_t>::min()).i==NOBODY.i,"wide signed minimum cannot become an ID");
        check(SoldierID::FromLegacyNumericID(std::numeric_limits<std::uint64_t>::max()).i==NOBODY.i,"wide unsigned maximum cannot become an ID");
        object=7;
        popupCallbackFunction2<void,INT16,UINT32> two(&action2,-5,12);
        check(two.call() && observed==7,"two captured arguments retain signed/unsigned types");
        popupCallbackFunction2<int,INT16,UINT32> twoResult(&condition2,12,12);
        check(!twoResult.call(),"two-argument false result");
        int output=0, other=9;
        popupCallbackFunction3<void,int*,UINT16,int*> three(&action3,&output,123,&other);
        check(three.call() && output==132,"three captured pointer/scalar arguments");
        popupCallbackFunction3<int,int*,UINT16,int*> threeResult(&condition3,&output,123,&other);
        check(!threeResult.call(),"three-argument return value");
        other=8; check(threeResult.call(),"callback keeps borrowed pointer semantics");
        zero.bind(&throwing0); check(!zero.call(),"existing void-zero exception becomes false");
        one.bind(&throwing1); check(!one.call(),"existing void-one exception becomes false");
        condition.bind(&throwingCondition);
        bool propagated=false; try { condition.call(); } catch (const std::runtime_error&) { propagated=true; }
        check(propagated,"existing predicate exception propagation retained");
        bool destroyed=false;
        { std::unique_ptr<popupCallback> owned(new Owned(destroyed)); check(owned->call(),"owned callback invocation"); }
        check(destroyed,"destruction through actual polymorphic ownership");
        std::cout << "PASS: " << checks << " typed popup callback checks\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n'; return 1;
    }
}
