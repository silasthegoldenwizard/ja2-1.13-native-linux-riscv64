/*
 * P710 native snapshot update: tests/native-core/native_scroll_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Uses the same production scroll/copy code and owned surface/Z storage as the
// platform refresh. Literal expectations do not reimplement the scroll mapping.
#include "NativeScroll.h"
#include "clip_state.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool value,const char* reason) { ++checks;if (!value) throw std::runtime_error(reason); }
UINT16 read(const BYTE* data,std::size_t offset) { UINT16 value;std::memcpy(&value,data+offset,2);return value; }
void write(BYTE* data,std::size_t offset,UINT16 value) { std::memcpy(data+offset,&value,2); }
UINT32 surface() {
    VSURFACE_DESC desc{};desc.fCreateFlags=VSURFACE_CREATE_DEFAULT;desc.usWidth=6;desc.usHeight=6;desc.ubBitDepth=16;
    UINT32 id=0;check(AddVideoSurface(&desc,&id),"real RGB565 surface");return id;
}
struct Case { UINT32 direction;std::array<UINT16,16> back,z; };
const Case cases[]{
 {0x08,{111,11,12,13,121,21,22,23,131,31,32,33,141,41,42,43},
       {0,1012,1013,1014,0,1022,1023,1024,0,1032,1033,1034,0,1042,1043,1044}},
 {0x04,{12,13,14,114,22,23,24,124,32,33,34,134,42,43,44,144},
       {1011,1012,1013,0,1021,1022,1023,0,1031,1032,1033,0,1041,1042,1043,0}},
 {0x01,{111,112,113,114,11,12,13,14,21,22,23,24,31,32,33,34},
       {0,0,0,0,1021,1022,1023,1024,1031,1032,1033,1034,1041,1042,1043,1044}},
 {0x02,{21,22,23,24,31,32,33,34,41,42,43,44,141,142,143,144},
       {1011,1012,1013,1014,1021,1022,1023,1024,1031,1032,1033,1034,0,0,0,0}},
 {0x20,{111,112,113,114,121,11,12,13,131,21,22,23,141,31,32,33},
       {0,0,0,0,0,1022,1023,1024,0,1032,1033,1034,0,1042,1043,1044}},
 {0x40,{111,112,113,114,12,13,14,124,22,23,24,134,32,33,34,144},
       {0,0,0,0,1021,1022,1023,0,1031,1032,1033,0,1041,1042,1043,0}},
 {0x80,{111,21,22,23,121,31,32,33,131,41,42,43,141,142,143,144},
       {0,1012,1013,1014,0,1022,1023,1024,0,1032,1033,1034,0,0,0,0}},
 {0x200,{22,23,24,114,32,33,34,124,42,43,44,134,141,142,143,144},
       {1011,1012,1013,0,1021,1022,1023,0,1031,1032,1033,0,0,0,0,0}}
};
void directions(sgp::BackgroundView back,sgp::BackgroundView frame,sgp::BackgroundView z) {
    for (const auto& fixture:cases) {
        std::memset(back.data,0xa5,back.bytes);std::memset(frame.data,0xb6,frame.bytes);std::memset(z.data,0xc7,z.bytes);
        for (UINT32 y=0;y<6;++y) for (UINT32 x=0;x<6;++x) {
            write(back.data,y*back.pitch+x*2,UINT16(y*10+x));
            write(frame.data,y*frame.pitch+x*2,UINT16(100+y*10+x));
            write(z.data,y*z.pitch+x*2,UINT16(1000+y*10+x));
        }
        const std::vector<BYTE> frameBefore(frame.data,frame.data+frame.bytes);
        sgp::NativeScrollRequest request{{1,1,5,5},fixture.direction,1,1};sgp::NativeScrollPlan plan;
        check(sgp::ApplyNativeScroll(back,z,request,&plan),"production scroll all eight directions");
        for (UINT32 i=0;i<plan.stripCount;++i) {
            const auto& r=plan.strips[i];
            check(sgp::CopyBackgroundRect(back,frame,r.iLeft,r.iTop,r.iLeft,r.iTop,
                UINT32(r.iRight-r.iLeft),UINT32(r.iBottom-r.iTop)),"same production exposed-strip copy used by refresh");
        }
        for (UINT32 y=0;y<6;++y) {
            for (UINT32 x=0;x<6;++x) {
                const bool inside=x>=1&&x<5&&y>=1&&y<5;const auto k=(y-1)*4+x-1;
                check(read(back.data,y*back.pitch+x*2)==(inside?fixture.back[k]:UINT16(y*10+x)),"literal full padded-frame result and untouched viewport exterior");
                check(read(z.data,y*z.pitch+x*2)==(inside?fixture.z[k]:UINT16(1000+y*10+x)),"literal exposed-zero Z and retained unshifted depths");
            }
            for (UINT32 x=12;x<back.pitch;++x) check(back.data[y*back.pitch+x]==0xa5,"back row padding untouched");
            for (UINT32 x=12;x<z.pitch;++x) check(z.data[y*z.pitch+x]==0xc7,"Z row padding untouched");
        }
        check(std::memcmp(frame.data,frameBefore.data(),frame.bytes)==0,"strip source frame including padding unchanged");
    }
}
void invalidAndClipping(sgp::BackgroundView back,sgp::BackgroundView z) {
    const std::vector<BYTE> before(back.data,back.data+back.bytes),zBefore(z.data,z.data+z.bytes);
    sgp::NativeScrollRequest request{{1,1,5,5},0x08,1,1};sgp::NativeScrollPlan plan{};plan.stripCount=99;
    for (const auto bad:std::array<sgp::NativeScrollRequest,7>{{
        {{1,1,5,5},0,1,1},{{1,1,5,5},0x09,1,1},{{1,1,5,5},0x08,-1,1},
        {{1,1,5,5},0x08,0,1},{{1,1,5,5},0x08,4,1},{{1,1,5,5},0x01,1,INT32_MAX},{{5,1,1,5},0x08,1,1}}}) {
        check(!sgp::ApplyNativeScroll(back,z,bad,&plan),"invalid direction/increment/viewport fails atomically");
        check(plan.stripCount==99,"failed request does not publish a partial plan");
    }
    auto shortZ=z;shortZ.bytes=1;
    check(!sgp::ApplyNativeScroll(back,shortZ,request),"invalid real Z extent refuses before writes");
    check(!sgp::ApplyNativeScroll(back,back,request),"aliased back/Z storage refuses before writes");
    check(std::memcmp(back.data,before.data(),back.bytes)==0&&std::memcmp(z.data,zBefore.data(),z.bytes)==0,"all failed operations preserve complete allocations");
    // Clipping uses visible dimensions; a wildly outlying caller viewport cannot
    // write into padding. No engine callbacks or shadow global state are used.
    request.viewport={-100,-100,100,100};
    check(sgp::PlanNativeScroll(back,z,request,plan)&&plan.viewport.iLeft==0&&plan.viewport.iTop==0&&plan.viewport.iRight==6&&plan.viewport.iBottom==6,"viewport intersects actual pixel bounds");
    request={{1,1,5,5},0x20,3,2};
    check(sgp::PlanNativeScroll(back,z,request,plan)&&plan.source.iRight-plan.source.iLeft==1&&plan.source.iBottom-plan.source.iTop==2&&plan.strips[0].iRight==4&&plan.strips[1].iLeft==4&&plan.strips[1].iBottom==3,"nonunit diagonal steps retain one-column interior and disjoint strips");
}
}
int main() {
    try {
        check(InitializeVideoSurfaceManager(),"manager initialization");
        const UINT32 ids[]{surface(),surface(),surface(),surface()};
        check(sgp::BindReservedVideoSurfaces(ids),"real reserved surfaces");
        UINT32 pitch=0;check(LockVideoSurface(FRAME_BUFFER,&pitch)!=nullptr,"actual frame pitch");UnLockVideoSurface(FRAME_BUFFER);
        auto* depth=InitZBuffer(pitch,6);check(depth!=nullptr,"actual owned Z allocation");
        UINT32 bp=0,fp=0;auto* b=sgp::LockBackgroundSurface(BACKBUFFER,&bp);auto* f=sgp::LockBackgroundSurface(FRAME_BUFFER,&fp);
        sgp::BackgroundView back,frame,z;
        check(b&&f&&sgp::BackgroundBufferView(b,&back)&&sgp::BackgroundBufferView(f,&frame)&&sgp::BackgroundBufferView(depth,&z),"truthful manager/owned-Z views");
        SGPRect tiny{2,2,3,3};SetClippingRect(&tiny);
        directions(back,frame,z);invalidAndClipping(back,z);
        SGPRect after{};GetClippingRect(&after);
        check(after.iLeft==2&&after.iTop==2&&after.iRight==3&&after.iBottom==3,"global drawing clip remains untouched");
        sgp::UnlockBackgroundSurface(FRAME_BUFFER);sgp::UnlockBackgroundSurface(BACKBUFFER);
        check(ShutdownZBuffer(depth)&&sgp::ReleaseReservedVideoSurfaces()&&ShutdownVideoSurfaceManager(),"owned storage and locks cleanly released");
        std::cout<<"PASS: "<<checks<<" actual native scroll/strip/padded-Z checks\n";return 0;
    } catch (const std::exception& error) { std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';return 1; }
}
