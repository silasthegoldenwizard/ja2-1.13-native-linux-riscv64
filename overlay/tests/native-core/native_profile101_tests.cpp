/*
 * P710 native snapshot update: tests/native-core/native_profile101_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Authored Windows profile wire records through genuine original Load,
// crypto/conversion/competence and production FileMan/bfVFS. No game assets.
#include "soldier profile type.h"
#include "Soldier Profile.h"
#include "SaveLoadGame.h"
#include "Tactical Save.h"
#include "FileMan.h"
#include "BufferMan.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

extern UINT8* GetRotationArray();
extern UINT32 guiJA2EncryptionSet;
extern UINT16 CalcCompetence(MERCPROFILESTRUCT*);
extern INT16 CalcMedicalDeposit(MERCPROFILESTRUCT*);
namespace fs=std::filesystem;
namespace {
unsigned checks=0;
void check(bool b,const char* why) { ++checks; if(!b) throw std::runtime_error(why); }
using Bytes=std::vector<UINT8>;
void put16(Bytes& b,std::size_t at,unsigned v) { b.at(at)=v&255; b.at(at+1)=(v>>8)&255; }
void put32(Bytes& b,std::size_t at,unsigned v) { put16(b,at,v);put16(b,at+2,v>>16); }
Bytes record(unsigned seed) {
    // Literal Windows offsets: deliberately independent of host offsetof/sizeof.
    Bytes b(716,0);
    put16(b,0,'A'+seed);put16(b,2,0x03a9);put16(b,4,0xd83d);put16(b,6,0xde42);
    put16(b,60,'N');put16(b,62,'0'+seed);
    for(unsigned i=0;i<7;++i)put32(b,80+4*i,0x10203040u+seed+i);
    b[108]=41+seed;
    for(unsigned i=0;i<120;++i)b[109+i]='a'+i%26;
    b[229]=1;b[232]=2;b[240]=253;put16(b,242,0xff85); // signed -3/-123
    b[260]=2;b[261]=60;
    for(unsigned i=0;i<4;++i)put16(b,262+2*i,100+i+seed);
    for(unsigned i=0;i<4;++i)put32(b,272+4*i,0x12345000+i+seed);
    put16(b,288,9);put16(b,290,1);put32(b,292,12345+seed);
    b[296]=70;b[297]=80;
    put16(b,310,100);put16(b,312,40);put16(b,314,1234);put16(b,316,456);
    put16(b,318,20);put16(b,320,8);put16(b,322,19);
    put32(b,328,0x12345678);put16(b,332,987);b[334]=75;b[335]=50;
    b[337]=3;b[339]=20;b[340]=7;b[341]=40;
    for(unsigned i=0;i<5;++i){b[342+i]=i+5;b[347+i]=i+10;}
    b[352]=4;b[353]=90;b[355]=30;b[357]=1;
    for(unsigned i=0;i<19;++i){b[358+i]=60+i;b[377+i]=i+1;put16(b,416+i*2,1000+seed*20+i);}
    for(unsigned i=0;i<4;++i)put16(b,396+i*2,200+i);
    b[405]=60;b[406]=1;put16(b,408,0xfedc);b[410]=21;b[411]=40;
    b[413]=12;b[414]=13;
    for(unsigned i=0;i<20;++i)b[454+i]=i-10;
    for(unsigned i=0;i<12;++i){put16(b,474+i*2,300+i);put16(b,498+i*2,400+i);}
    b[522]=5;b[523]=14;b[524]=15;
    put32(b,540,0x11223344+seed);put32(b,544,0x55667788+seed);
    b[548]=1;b[549]=254;b[550]=55;put16(b,552,2500);b[554]=22;
    for(unsigned i=0;i<4;++i)b[555+i]=i+30;
    for(unsigned i=0;i<12;++i)b[559+i]=i+40;
    b[571]=6;b[572]=7;put16(b,574,1234);
    for(unsigned i=0;i<75;++i)b[576+i]=i-35;
    b[651]=1;b[652]=255;b[669]=2;put16(b,670,0xabcd);
    put32(b,676,0xffedcba9);put16(b,680,0xff9c);b[682]=9;b[683]=10;
    put32(b,684,0x87654321);b[688]=17;b[689]=18;b[690]=19;b[691]=20;
    put32(b,692,0x31415926);put32(b,696,0xa1b2c3d4);put16(b,700,0xf123);
    b[702]=33;b[703]=34;put32(b,704,0xfffffffd);put32(b,708,0xf1e2d3c4+seed);
    for(unsigned i=0;i<4;++i)b[712+i]=0xe0+i;
    return b;
}
Bytes encrypt(const Bytes& plain,int mode) {
    if(!mode)return plain;
    // An independent encoder, not the production writer or decoder. The old
    // key is a literal public-source vector; new key comes from public policy.
    static const UINT8 oldKey[46]={132,235,125,99,15,220,140,89,205,132,254,144,217,78,156,58,215,76,163,187,55,49,65,48,156,140,201,68,184,13,45,69,102,185,122,225,23,250,160,220,114,240,64,175,47,233};
    const UINT8* key=mode==1?oldKey:GetRotationArray();
    const unsigned length=mode==1?46:NEW_ROTATION_ARRAY_SIZE;
    Bytes out=plain;unsigned previous=0;
    for(std::size_t i=0;i<out.size();++i){out[i]=(plain[i]+previous+key[i%length])&255;previous=out[i];}
    return out;
}
void verify(MERCPROFILESTRUCT& p,unsigned seed) {
    check(p.zName[0]==static_cast<wchar_t>('A'+seed)&&p.zName[1]==0x03a9&&p.zName[2]==0x1f642&&p.zName[3]==0,"UTF16 ASCII/BMP/surrogate name conversion");
    check(p.zNickname[0]=='N'&&p.zNickname[1]==static_cast<wchar_t>('0'+seed)&&!p.zNickname[2],"nickname independent offset");
    check(p.uiAttnSound==0x10203040+seed&&p.uiOkSound==0x10203046+seed&&p.ubFaceIndex==41+seed,"sounds and face anchor");
    for(unsigned i=0;i<30;++i){check(p.PANTS[i]=='a'+i%26,"palette pants");check(p.HAIR[i]=='a'+(i+90)%26,"palette hair");}
    check(p.fRegresses&&p.bScientific==-3&&p.sExpLevelGain==-123,"signed fields and original evolution conversion");
    check(p.bMedical==60&&p.usEyesX==100+seed&&p.usMouthY==103+seed&&p.uiExpressionFrequency==0x12345003+seed,"eye/medical alignment");
    check(p.sSectorX==9&&p.sSectorY==1&&p.uiDayBecomesAvailable==12345+seed,"sector fields");
    check(p.bStrength==70&&p.bLifeMax==80&&p.bExpLevel==4&&p.bMarksmanship==90&&p.bWisdom==30&&p.bAgility==60&&p.bMechanical==40,"competence stat offsets");
    check(p.bDexterity==50&&p.bExplosive==20&&p.bLeadership==40&&p.sSalary==987&&p.bLife==75,"remaining competence stats");
    check(p.bSkillTraits[0]==3&&p.bSkillTraits[1]==7&&p.bSkillTraits[2]==0,"original skill conversion and zeroed modern tail");
    const unsigned positions[19]={HELMETPOS,VESTPOS,LEGPOS,HEAD1POS,HEAD2POS,HANDPOS,SECONDHANDPOS,BIGPOCK1POS,BIGPOCK2POS,BIGPOCK3POS,BIGPOCK4POS,SMALLPOCK1POS,SMALLPOCK2POS,SMALLPOCK3POS,SMALLPOCK4POS,SMALLPOCK5POS,SMALLPOCK6POS,SMALLPOCK7POS,SMALLPOCK8POS};
    for(unsigned i=0;i<19;++i){check(p.inv[positions[i]]==1000+seed*20+i,"original inventory index mapping");check(p.bInvStatus[positions[i]]==60+i&&p.bInvNumber[positions[i]]==i+1,"original inventory status/count mapping");}
    check(p.records.usKillsElites==25&&p.records.usKillsRegulars==50&&p.records.usKillsAdmins==25&&p.records.usAssistsMercs==30&&p.records.usAssistsMilitia==10,"original record arithmetic");
    check(p.records.usBattlesTactical==15&&p.records.usBattlesAutoresolve==5&&p.records.usShotsFired==1234&&p.records.usShotsHit==456&&p.usTotalDaysServed==19,"original combat record conversion");
    check(p.uiBodyTypeSubFlags==0x12345678&&p.sGridNo==static_cast<INT16>(0xfedc)&&p.ubQuoteActionID==21,"body/grid signed conversion");
    for(unsigned i=0;i<4;++i)check(p.usApproachFactor[i]==200+i&&p.ubApproachVal[i]==30+i,"approach array conversion");
    for(unsigned i=0;i<12;++i)check(p.usStatChangeChances[i]==300+i&&p.usStatChangeSuccesses[i]==400+i&&p.ubApproachMod[i/4][i%4]==40+i,"stats/approach arrays");
    // The retained original converter copies two raw bytes into each modern
    // UINT16[2], rather than widening the entries. Preserve that exact behavior.
    check(p.usRoomRangeStart[0]==0x0d0c&&p.usRoomRangeStart[1]==0&&p.usRoomRangeEnd[0]==0x0f0e&&p.usRoomRangeEnd[1]==0,"original room array raw-copy semantics");
    check(p.uiWeeklySalary==0x11223344+seed&&p.uiBiWeeklySalary==0x55667788+seed&&p.bMedicalDeposit==1&&p.sMedicalDepositAmount==2500&&p.bAttitude==-2,"salary/deposit offsets");
    for(unsigned i=0;i<75;++i)check(p.bMercOpinion[i]==static_cast<INT8>(i-35),"75 original opinions");
    check(p.bMercOpinion[75]==0&&p.bMercStatus==-1&&p.bSectorZ==2&&p.usStrategicInsertionData==0xabcd,"opinion boundary/status offsets");
    check(p.iBalance==static_cast<INT32>(0xffedcba9)&&p.sTrueSalary==-100&&p.uiMoney==0x87654321&&p.uiPrecedentQuoteSaid==0x31415926,"signed/unsigned tail fields");
    check(p.uiProfileChecksum==0xa1b2c3d4&&p.sPreCombatGridNo==static_cast<INT16>(0xf123)&&p.ubTimeTillNextHatedComplaint==33&&p.iMercMercContractLength==-3&&p.uiTotalCostToDate==0xf1e2d3c4+seed,"final wire anchors");
    check(CalcCompetence(&p)==369,"unchanged original competence expected literal369");
    check(CalcMedicalDeposit(&p)==1800,"unchanged original medical deposit expected literal1800");
}
struct Temp { fs::path root;Temp(){char p[]="/tmp/p710-profile101-XXXXXX";auto* s=mkdtemp(p);if(!s)throw std::runtime_error("mkdtemp");root=s;}~Temp(){std::error_code ec;fs::remove_all(root,ec);}};
void write(const fs::path& p,const Bytes& b){std::ofstream s(p,std::ios::binary);s.write(reinterpret_cast<const char*>(b.data()),b.size());s.close();check(bool(s),"fixture write");}
void mount(const fs::path& root){auto p=std::make_unique<vfs::CVirtualProfile>("profile101-fixtures",vfs::Path(root.u8string()),false);auto d=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));check(d->init(),"fixture VFS init");auto* l=d.get();p->addLocation(d.release());auto* raw=p.get();getVFS()->getProfileStack()->pushProfile(p.release());check(getVFS()->addLocation(l,raw),"fixture VFS mount");}
struct BufferBounds { BufferBounds(Bytes& b){gpLoadBufferStart=reinterpret_cast<char*>(b.data());giLoadBufferSize=b.size();gfBufferLoadFailed=FALSE;}~BufferBounds(){gpLoadBufferStart=nullptr;giLoadBufferSize=0;gfBufferLoadFailed=FALSE;}};
void publicLoad(const Temp& t,int mode) {
    guiCurrentSaveGameVersion=mode==2?87:0;guiJA2EncryptionSet=0;
    auto a=encrypt(record(1),mode),b=encrypt(record(2),mode);Bytes both=a;both.insert(both.end(),b.begin(),b.end());both.insert(both.end(),256,0xa5);
    const std::string name="mode"+std::to_string(mode);
    HWFILE h=FileOpen(const_cast<char*>(name.c_str()),FILE_ACCESS_READ,FALSE);check(h!=0,"production FileOpen");
    MERCPROFILESTRUCT p;
    check(p.Load(h,true,mode==1,mode!=0),"file old profile Load success");
    // This first check must fail before the wire fix: old host read consumes796.
    check(FileGetPos(h)==716,"file consumes original716byte record, not native796");verify(p,1);
    check(p.Load(h,true,mode==1,mode!=0)&&FileGetPos(h)==1432,"second encrypted record preserves boundary/reset");verify(p,2);
    UINT8 canary=0;UINT32 n=0;check(FileRead(h,&canary,1,&n)&&canary==0xa5,"post-record canary untouched");FileClose(h);
    BufferBounds bounds(both);char* cursor=reinterpret_cast<char*>(both.data());
    check(p.Load(&cursor,true,mode==1,mode!=0)&&cursor==reinterpret_cast<char*>(both.data()+716),"buffer first716byte boundary");verify(p,1);
    check(p.Load(&cursor,true,mode==1,mode!=0)&&cursor==reinterpret_cast<char*>(both.data()+1432),"buffer second716byte boundary");verify(p,2);
    check(static_cast<UINT8>(*cursor)==0xa5&&!gfBufferLoadFailed,"buffer canary and bounds status");
}
void rejection(const Temp& t) {
    guiCurrentSaveGameVersion=0;
    for(unsigned length:{0u,59u,79u,351u,715u}) {
        auto b=record(1);b.resize(length);std::string name="short"+std::to_string(length);
        auto h=FileOpen(const_cast<char*>(name.c_str()),FILE_ACCESS_READ,FALSE);MERCPROFILESTRUCT p;p.bExpLevel=9;p.zName[0]='Z';p.inv[HANDPOS]=77;
        check(!p.Load(h,true,true,true),"short file rejected");check(p.bExpLevel==9&&p.zName[0]=='Z'&&p.inv[HANDPOS]==77,"short file target preserved");FileClose(h);
        // Keep a live allocation for zero-length source pointer subtraction.
        Bytes backing=b;if(backing.empty())backing.push_back(0);BufferBounds bounds(backing);giLoadBufferSize=length;char* c=reinterpret_cast<char*>(backing.data());auto* start=c;
        check(!p.Load(&c,true,true,true)&&c==start&&gfBufferLoadFailed,"bounded short buffer rejected without cursor advance");check(p.bExpLevel==9&&p.inv[HANDPOS]==77,"short buffer target preserved");
    }
    for(unsigned at:{0u,60u})for(unsigned unit:{0xd800u,0xdc00u}) {
        auto b=record(1);put16(b,at,unit);put16(b,at+2,'X');auto encoded=encrypt(b,1);std::string name="invalid"+std::to_string(at)+"-"+std::to_string(unit);
        auto h=FileOpen(const_cast<char*>(name.c_str()),FILE_ACCESS_READ,FALSE);MERCPROFILESTRUCT p;p.bExpLevel=9;p.inv[HANDPOS]=77;
        check(!p.Load(h,true,true,true)&&FileGetPos(h)==716,"invalid UTF16 file rejected after one full decrypt");check(p.bExpLevel==9&&p.inv[HANDPOS]==77,"invalid UTF16 file target preserved");FileClose(h);
        BufferBounds bounds(encoded);char* c=reinterpret_cast<char*>(encoded.data());auto* start=c;check(!p.Load(&c,true,true,true)&&c==start,"invalid UTF16 buffer rejected transactionally");check(p.bExpLevel==9&&p.inv[HANDPOS]==77,"invalid UTF16 buffer target preserved");
    }
    for(bool terminated:{false,true}) {
        auto b=record(1);for(unsigned i=0;i<30;++i)put16(b,i*2,'a'+i%26);for(unsigned i=0;i<10;++i)put16(b,60+i*2,'0'+i);
        if(terminated){put16(b,58,0);put16(b,78,0);}BufferBounds bounds(b);char* c=reinterpret_cast<char*>(b.data());MERCPROFILESTRUCT p;
        check(p.Load(&c,true,false,false),"full fixed arrays accepted without forced termination");check(p.zName[29]==(terminated?0:'d')&&p.zNickname[9]==(terminated?0:'9'),"fixed-capacity name semantics retained");
    }
}
void prepare(const Temp& t) {
    guiJA2EncryptionSet=0;
    for(int mode=0;mode<3;++mode){auto a=encrypt(record(1),mode),b=encrypt(record(2),mode);a.insert(a.end(),b.begin(),b.end());a.insert(a.end(),256,0xa5);write(t.root/("mode"+std::to_string(mode)),a);}
    for(unsigned length:{0u,59u,79u,351u,715u}){auto b=record(1);b.resize(length);write(t.root/("short"+std::to_string(length)),b);}
    for(unsigned at:{0u,60u})for(unsigned unit:{0xd800u,0xdc00u}){auto b=record(1);put16(b,at,unit);put16(b,at+2,'X');write(t.root/("invalid"+std::to_string(at)+"-"+std::to_string(unit)),encrypt(b,1));}
}
}
int main(){try{Temp temp;prepare(temp);mount(temp.root);check(InitializeFileManager(nullptr),"real FileMan initializes");for(int mode=0;mode<3;++mode)publicLoad(temp,mode);rejection(temp);ShutdownFileManager();std::cout<<"profile101: "<<checks<<" checks passed; original engine Load/crypto/conversion/competence, synthetic wire only\n";return 0;}catch(const std::exception& e){ShutdownFileManager();std::cerr<<"profile101 failed at check "<<checks<<": "<<e.what()<<'\n';return 1;}}
