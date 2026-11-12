#!/usr/bin/env python3
"""Replay production DSI fixed-mode ownership/dispatch with IOKit leaves mocked."""
from pathlib import Path
from production_source import read_source
import hashlib, re, subprocess, sys, tempfile

root = Path(__file__).resolve().parents[1]
source = read_source(root/'WhateverXe/kern_xe.cpp')
owner = source[source.index('struct DSIFixedModeOwner {'):source.index('// AlderLakeN start bypasses')]
names = ['recordDSIClient', 'releaseDSIFixedMode', 'bindDSIFixedMode', 'wrapDSISetDetailedTimings', 'wrapDSIValidateMode',
         'wrapDSIGetModes', 'wrapDSIGetModeCount', 'wrapDSIGetModeInformation', 'wrapDSIGetCurrentMode', 'wrapDSIGetOnlineInfo', 'wrapDSIDDC',
         'wrapDSIScalerSupport']
diagnostics = re.search(r'#ifndef WEX_DSI_ROTATION_DIAGNOSTICS.*?#endif\s+#if WEX_DSI_ROTATION_DIAGNOSTICS.*?#endif', source, re.S)[0]
functions = diagnostics+'\n'+'\n'.join(re.search(r'(?:void|bool|uint32_t) XE::'+name+r'\(.*?\n\}', source, re.S)[0] for name in names)
if len(sys.argv) == 2:
    binary = Path(sys.argv[1]).read_bytes()
    assert hashlib.sha256(binary).hexdigest() == 'c004fc1ac9ee4157e2e12736f40fa6639bde025a279326beeb9dd48c9b445731'
    installer = re.search(r'bool XE::processDisplay13DSIModes\(.*?\n\}', source, re.S)[0]
    offsets = [int(x, 0) for x in re.search(r'offsets\[\]=\{([^}]+)', installer)[1].split(',')]
    entries = re.search(r'entries\[\]\[16\]=\{(.*?)\};', installer, re.S)[1]
    entries = [bytes(int(x, 0) for x in row.split(',')) for row in re.findall(r'\{([^}]+)\}', entries)]
    assert len(entries) == len(offsets) == 9
    for offset, entry in zip(offsets, entries):
        assert binary[offset:offset+16] == entry, hex(offset)

code = r'''
#include "kern_dsi_apple_timing.hpp"
#include "kern_dsi_readout.hpp"
#include "kern_display13_transcoder.hpp"
#include "kern_display13_output_wiring.hpp"
#include <cassert>
#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include <atomic>
#include <mutex>
#include <thread>
constexpr uint32_t kIOReturnSuccess=0,kIOReturnNotReady=0xe00002d8U,
 kIOReturnUnsupported=0xe00002c7U,kIOReturnBadArgument=0xe00002c2U,kIOReturnNoResources=0xe00002bdU;
void *current_thread(){return reinterpret_cast<void *>(1);}
#define arrsize(x) (sizeof(x)/sizeof(*(x)))
std::atomic<unsigned> alive{0},appendCalls{0};unsigned calls[5]{},scalerDelegateCalls=0,failAppendAt=0;
bool dataOK=true,arrayOK=true,publishOK=true,geometryOK=true,clearOK=true,capacityOK=true,appendOK=true;
 bool failNextCapacity=false,failTimingSnapshotPublication=false,failTimingErrorPublication=false;
struct Meta {unsigned getClassSize(){return 0x4b90;}} meta;
struct OSObject {
 std::atomic<unsigned> refs{1};OSObject(){++alive;}virtual ~OSObject(){--alive;}
 void retain(){auto old=refs.fetch_add(1,std::memory_order_relaxed);assert(old);}
 void release(){auto old=refs.fetch_sub(1,std::memory_order_acq_rel);assert(old);if(old==1)delete this;}
 Meta *getMetaClass(){return &meta;}
};
struct OSNumber:OSObject {uint64_t value=0;uint64_t unsigned64BitValue(){return value;}};
struct OSData:OSObject {
 std::vector<uint8_t> bytes;
 bool nullBytes=false;
 static OSData *withCapacity(unsigned n){if(failNextCapacity){failNextCapacity=false;return nullptr;}if(!capacityOK)return nullptr;auto d=new OSData;d->bytes.reserve(n);return d;}
 bool appendBytes(const void *p,unsigned n){const auto call=++appendCalls;if(!appendOK || call==failAppendAt)return false;auto b=static_cast<const uint8_t *>(p);bytes.insert(bytes.end(),b,b+n);return true;}
 static OSData *withBytes(const void *p,unsigned n){if(!dataOK)return nullptr;auto d=new OSData;
  d->bytes.assign(static_cast<const uint8_t*>(p),static_cast<const uint8_t*>(p)+n);return d;}
 unsigned getLength(){return bytes.size();}const void *getBytesNoCopy(){return nullBytes?nullptr:bytes.data();}
};
struct OSArray:OSObject {
 std::vector<OSObject*> objects;
 static OSArray *withCapacity(unsigned n){assert(n>=1 && n<=2);return arrayOK?new OSArray:nullptr;}
 bool setObject(OSObject *p){p->retain();objects.push_back(p);return true;}
 bool replaceObject(unsigned i,OSObject *p){if(i>=objects.size() || !p)return false;p->retain();auto old=objects[i];objects[i]=p;old->release();return true;}
 void removeObject(unsigned i){if(i>=objects.size())return;auto old=objects[i];objects.erase(objects.begin()+i);old->release();}
 unsigned getCount(){return objects.size();}OSObject *getObject(unsigned i){return i<objects.size()?objects[i]:nullptr;}
 ~OSArray(){for(auto p:objects)p->release();}
};
struct IOService:OSObject {
 alignas(8) uint8_t storage[0x4c00]{};std::map<std::string,OSObject*> properties;std::mutex propertiesMutex;
 bool setProperty(const char *k,OSObject *p){if(!publishOK || !p)return false;p->retain();std::lock_guard<std::mutex> guard(propertiesMutex);
  auto i=properties.find(k);if(i!=properties.end()){i->second->release();i->second=p;}else properties.emplace(k,p);return true;}
 void setProperty(const char *,bool){}
 bool setProperty(const char *k,const void *p,unsigned n){auto d=OSData::withBytes(p,n);if(!d)return false;bool ok=setProperty(k,d);d->release();return ok;}
 OSObject *getProperty(const char *k){std::lock_guard<std::mutex> guard(propertiesMutex);auto i=properties.find(k);return i==properties.end()?nullptr:i->second;}
 OSObject *copyProperty(const char *k){std::lock_guard<std::mutex> guard(propertiesMutex);auto i=properties.find(k);auto p=i==properties.end()?nullptr:i->second;if(p)p->retain();return p;}
 void removeProperty(const char *k){std::lock_guard<std::mutex> guard(propertiesMutex);auto i=properties.find(k);if(i!=properties.end()){i->second->release();properties.erase(i);}}
 ~IOService(){for(auto i:properties)i.second->release();}
};
#define OSDynamicCast(T,p) dynamic_cast<T*>(p)
struct IORegistryEntry {
 std::map<std::string,OSObject*> properties;std::mutex propertiesMutex;
 bool setProperty(const char *key,OSObject *object){if(!object)return false;
  if(failTimingSnapshotPublication && !strncmp(key,"WEXDSITimingInput",17) && strncmp(key,"WEXDSITimingInputError",22))return false;
  if(failTimingErrorPublication && !strncmp(key,"WEXDSITimingInputError",22))return false;
  auto d=OSDynamicCast(OSData,object);assert(d);
  if(!strcmp(key,"WEXDSITimingRejected"))assert(d->getLength()==160);
  else if(!strcmp(key,"WEXDSITimingList"))assert(!(d->getLength()%160));
  else if(!strncmp(key,"WEXDSITimingInputError",22))assert(d->getLength()==48);
  else if(!strncmp(key,"WEXDSITimingInput",17))assert(d->getLength()>=sizeof(uint32_t)*6);
  else if(!strncmp(key,"WEXDSIScalerCall",16))assert(d->getLength()==29*sizeof(uint64_t));
  else assert(false);
  object->retain();std::lock_guard<std::mutex> guard(propertiesMutex);auto i=properties.find(key);
  if(i!=properties.end()){i->second->release();i->second=object;}else properties.emplace(key,object);return true;}
 bool setProperty(const char *key,const void *p,unsigned n){
  if(failTimingErrorPublication && !strncmp(key,"WEXDSITimingInputError",22))return false;
  if(!strcmp(key,"WEXDSITimingInputError") || !strncmp(key,"WEXDSITimingInputError",22))assert(n==48);
  else if(!strncmp(key,"WEXDSIModeInfo",14))assert(n==48);
  else if(!strcmp(key,"WEXDSITimingList"))assert(!(n%160));
  else if(!strcmp(key,"WEXDSITimingRejected"))assert(n==160);
  else if(!strncmp(key,"WEXDSIScalerCall",16))assert(n==29*sizeof(uint64_t));
  else if(!strcmp(key,"WEXDSITimingOffer"))assert(n==128);
  else {assert(n==128);const auto w=static_cast<const uint64_t *>(p);assert(w[0]==1 && w[1]<13 && w[2]);}
  auto d=OSData::withBytes(p,n);if(!d)return false;const auto ok=setProperty(key,d);d->release();return ok;}
 OSObject *getProperty(const char *key){std::lock_guard<std::mutex> guard(propertiesMutex);auto i=properties.find(key);return i==properties.end()?nullptr:i->second;}
 unsigned countPrefix(const char *prefix){std::lock_guard<std::mutex> guard(propertiesMutex);unsigned count=0;for(const auto &p:properties)
  if(!strncmp(p.first.c_str(),prefix,strlen(prefix)) && p.first.find("Error")==std::string::npos)++count;return count;}
 unsigned countPrefixAll(const char *prefix){std::lock_guard<std::mutex> guard(propertiesMutex);unsigned count=0;for(const auto &p:properties)
  if(!strncmp(p.first.c_str(),prefix,strlen(prefix)))++count;return count;}
 bool latestTimingError(uint64_t out[6]){std::lock_guard<std::mutex> guard(propertiesMutex);bool found=false;uint64_t newest=0;
  for(const auto &p:properties)if(!strncmp(p.first.c_str(),"WEXDSITimingInputError",22)){
   auto d=OSDynamicCast(OSData,p.second);if(!d || d->getLength()!=6*sizeof(uint64_t))continue;
   const auto record=static_cast<const uint64_t*>(d->getBytesNoCopy());if(record[1]>newest){memcpy(out,record,6*sizeof(uint64_t));newest=record[1];found=true;}}
  return found;}
 void removeProperty(const char *key){std::lock_guard<std::mutex> guard(propertiesMutex);auto i=properties.find(key);if(i!=properties.end()){i->second->release();properties.erase(i);}}
 void clear(){std::lock_guard<std::mutex> guard(propertiesMutex);while(!properties.empty()){auto i=properties.begin();i->second->release();properties.erase(i);}}
 ~IORegistryEntry(){clear();}
 static IORegistryEntry *getRegistryRoot(){static IORegistryEntry r;return &r;}
};
struct IORecursiveLock {std::recursive_mutex mutex;unsigned depth=0;};struct IOLock {unsigned depth=0;};
void IORecursiveLockLock(IORecursiveLock *l){l->mutex.lock();++l->depth;}void IORecursiveLockUnlock(IORecursiveLock *l){assert(l->depth);--l->depth;l->mutex.unlock();}
void IOLockLock(IOLock *l){++l->depth;}void IOLockUnlock(IOLock *l){assert(l->depth);--l->depth;}
#define OSSafeReleaseNULL(p) do{if(p){p->release();p=nullptr;}}while(0)
#define FunctionCast(f,p) reinterpret_cast<decltype(&f)>(p)
OSObject *testPath=nullptr;
namespace Display13FBMemory {
 template<class T>T &field(void *p,unsigned o){return *reinterpret_cast<T*>(static_cast<uint8_t*>(p)+o);}
 template<class T>T singlePath(void *){return static_cast<T>(testPath);}
}
namespace TGLFBBinding {enum class Status {Bad,Complete};}
struct Binding {TGLFBBinding::Status status=TGLFBBinding::Status::Complete;uint32_t ddi=0,pipe=0;} binding;
Binding adlnInspectPathBinding(IOService *,OSObject *path,void *,const Display13OutputWiring &){assert(path);return binding;}
struct {IOLock *lock;void *controller;Display13Transcoder::Bindings transcoders;XELP::DSI::Readout bootDSI;
 bool memoryFaulted=false;struct {struct {bool faulted=false;} state;} bridge;} display13FBPower;
struct XE {
 bool dsiPanelOnly=false;bool display13DSIModeHooksReady=true;Display13OutputWiring adlnOutputWiring;void *adlnTopologyController=nullptr;
 uint8_t adlnPlatform[0x1c0]{};uintptr_t orgDSIMode[9]{};
 uintptr_t orgDSIScalerSupport{};
 static bool updateReady;
 static bool dsiNativeUpdateAllowed(IOService *){return updateReady;}
 static uint32_t wrapDSIDDC(IOService *,uint32_t,uint8_t *,uint64_t *,uint32_t,OSObject *);
 static uint32_t wrapDSIGetCurrentMode(IOService *,int32_t *,int32_t *);
 static uint32_t wrapDSIGetOnlineInfo(IOService *,OSObject *,uint8_t *,uint8_t *);
 static void recordDSIClient(IOService *,unsigned,uint64_t,uint32_t);
 bool releaseDSIFixedMode(IOService *);
 bool bindDSIFixedMode(IOService *,const XELP::DSI::Profile &);
 static uint32_t wrapDSISetDetailedTimings(IOService *,OSArray *);
 static uint32_t wrapDSIValidateMode(IOService *,int32_t,const void **,const void **);
 static uint32_t wrapDSIGetModes(IOService *,int32_t *);
 static uint32_t wrapDSIGetModeCount(IOService *);
 static uint32_t wrapDSIGetModeInformation(IOService *,int32_t,void *);
 static bool wrapDSIScalerSupport(void *,uint32_t,uint32_t,uint32_t,uint32_t,uint64_t,uint32_t,bool);
} xe,*callbackXE=&xe;
bool XE::updateReady=true;
OWNER
FUNCTIONS
uint32_t publish(IOService *fb,OSArray *a){++calls[0];if(a)fb->setProperty("IOFBDetailedTimings",a);else {if(!clearOK)return 0;fb->removeProperty("IOFBDetailedTimings");}
 Display13FBMemory::field<OSArray*>(fb,0x4260)=a;return 0;}
uint32_t validate(IOService *fb,int32_t mode,const void **desc,const void **timing){++calls[1];
 auto a=Display13FBMemory::field<OSArray*>(fb,0x4260);assert(a && (mode&1023)==0);
 if(desc)*desc=reinterpret_cast<void*>(1);if(timing)*timing=static_cast<OSData*>(a->getObject(0))->getBytesNoCopy();return 0;}
uint32_t modes(IOService *,int32_t *p){++calls[2];if(p)*p=42;return 0;}
uint32_t count(IOService *){++calls[3];return 3;}
uint32_t information(IOService *fb,int32_t mode,void *p){++calls[4];const void *t=nullptr;
 auto r=XE::wrapDSIValidateMode(fb,mode,nullptr,&t);if(r)return r;
 auto timing=static_cast<const XELP::DSI::AppleAdapter::Timing*>(t);
 auto info=static_cast<XELP::DSI::AppleAdapter::ModeInformation*>(p);*info={};
 info->width=geometryOK?(timing->horizontalScaled?timing->horizontalScaled:timing->horizontalActive):1;info->height=timing->verticalScaled?timing->verticalScaled:timing->verticalActive;info->depth=2;return 0;}
unsigned nativeCurrentCalls=0,nativeOnlineCalls=0,nativeDDCCalls=0;
uint32_t nativeDDC(IOService *,uint32_t block,uint8_t *data,uint64_t *length,uint32_t flags,OSObject *path){
 assert(block==2 && flags==1 && path==testPath);++nativeDDCCalls;*length=128;data[0]=42;return 9;}

uint32_t nativeCurrent(IOService *,int32_t *m,int32_t *d){++nativeCurrentCalls;if(m)*m=77;if(d)*d=3;return 7;}
uint32_t nativeOnline(IOService *,OSObject *,uint8_t *on,uint8_t *edid){++nativeOnlineCalls;if(on)*on=9;if(edid)edid[0]=42;return 8;}
bool nativeScaler(void *,uint32_t outputW,uint32_t outputH,uint32_t sourceW,uint32_t sourceH,uint64_t clock,uint32_t flags,bool format){
 ++scalerDelegateCalls;assert(outputW==1920 && outputH==1080 && sourceW==1080 && sourceH==1920);
 assert(clock==135260000 && flags==0x1234 && format);return true;}
#if WEX_DSI_ROTATION_DIAGNOSTICS
uint64_t timingSequenceForSlot(IORegistryEntry *root,unsigned slot){char key[40];snprintf(key,sizeof(key),"WEXDSITimingInput%u",slot);
 auto d=OSDynamicCast(OSData,root->getProperty(key));if(!d || d->getLength()!=sizeof(DSITimingTraceHeader)+2*(sizeof(DSITimingTraceEntry)+sizeof(XELP::DSI::AppleAdapter::Timing)))return 0;
 return static_cast<const DSITimingTraceHeader*>(d->getBytesNoCopy())->sequence;}
uint64_t scalerSequenceForSlot(IORegistryEntry *root,unsigned slot){char key[40];snprintf(key,sizeof(key),"WEXDSIScalerCall%u",slot);
 auto d=OSDynamicCast(OSData,root->getProperty(key));if(!d || d->getLength()!=29*sizeof(uint64_t))return 0;
 return static_cast<const uint64_t*>(d->getBytesNoCopy())[1];}
uint64_t timingErrorSequenceForSlot(IORegistryEntry *root,unsigned slot){char key[48];snprintf(key,sizeof(key),"WEXDSITimingInputError%u",slot);
 auto d=OSDynamicCast(OSData,root->getProperty(key));if(!d || d->getLength()!=6*sizeof(uint64_t))return 0;
 return static_cast<const uint64_t*>(d->getBytesNoCopy())[1];}
#endif
int main(){
 using namespace XELP::DSI;using namespace AppleAdapter;IORecursiveLock fbLock;IOLock ownerLock;
 {const auto before=alive.load();auto object=OSData::withBytes("a",1);auto array=OSArray::withCapacity(1);
  assert(object && array && object->refs==1 && array->setObject(object) && object->refs==2);
  object->release();assert(array->getObject(0)->refs==1);
  auto replacement=OSData::withBytes("b",1);assert(replacement && array->replaceObject(0,replacement));
  assert(replacement->refs==2);replacement->release();array->removeObject(0);array->release();assert(alive.load()==before);}
 auto fb=new IOService;auto peer=new IOService;testPath=peer;
 Display13FBMemory::field<IORecursiveLock*>(fb,0x4220)=&fbLock;
 display13FBPower.lock=&ownerLock;display13FBPower.controller=xe.adlnTopologyController=peer;
 display13FBPower.transcoders.dsi0Pipe=0;xe.adlnOutputWiring.valid=true;xe.adlnOutputWiring.dsi=1;
 xe.orgDSIMode[0]=reinterpret_cast<uintptr_t>(publish);xe.orgDSIMode[1]=reinterpret_cast<uintptr_t>(validate);
 xe.orgDSIMode[2]=reinterpret_cast<uintptr_t>(modes);xe.orgDSIMode[3]=reinterpret_cast<uintptr_t>(count);
 xe.orgDSIMode[4]=reinterpret_cast<uintptr_t>(information);xe.orgDSIMode[8]=reinterpret_cast<uintptr_t>(nativeDDC);
 xe.orgDSIMode[6]=reinterpret_cast<uintptr_t>(nativeCurrent);xe.orgDSIMode[7]=reinterpret_cast<uintptr_t>(nativeOnline);
 xe.orgDSIScalerSupport=reinterpret_cast<uintptr_t>(nativeScaler);
 Display13FBMemory::field<uint32_t>(peer,0x1DC)=1;
 Profile p;p.status=ProfileStatus::Valid;p.lanes=4;p.mode={135255,1080,46,22,2,1920,82,40,2,86,154};
 auto plan=linkPlan(p);auto &r=display13FBPower.bootDSI;r.status=ReadoutStatus::Complete;r.identity={0,0,0,1};
 r.afeKHz=811500;r.htotal=plan.htotal;r.hsync=plan.hsync;r.vtotal=plan.vtotal;r.vsync=plan.vsync;
 for(unsigned failure=0;failure<4;++failure){const auto before=alive.load();dataOK=failure!=0;arrayOK=failure!=1;
  binding.status=failure==2?TGLFBBinding::Status::Bad:TGLFBBinding::Status::Complete;
  display13FBPower.memoryFaulted=failure==3;
  assert(!xe.bindDSIFixedMode(fb,p) && !dsiModeTarget(fb) && alive==before && !calls[0]);}
 dataOK=arrayOK=true;display13FBPower.memoryFaulted=false;
 assert(xe.bindDSIFixedMode(fb,p) && fb->refs==2 && dsiFixedMode.array->refs==2 && dsiFixedMode.data->refs==2);
 uint32_t expectedScalerInfo[12]={0x101,0x202,0x303,0x404,0x505,0x606,0x707,0x808,0x909,0xA0A,0xB0B,0xC0C};
 auto scalerInfoData=OSData::withBytes(expectedScalerInfo,sizeof(expectedScalerInfo));
 assert(scalerInfoData && fb->setProperty("IOFBScalerInfo",scalerInfoData));scalerInfoData->release();
 alignas(8) uint8_t scalerObject[0x20]{};
 Display13FBMemory::field<IOService*>(scalerObject,0x10)=fb;
 auto registryRoot=IORegistryEntry::getRegistryRoot();
 for(unsigned i=0;i<70;++i)assert(!XE::wrapDSIScalerSupport(scalerObject,1920,1080,1080,1920,135260000,0x1234,true));
#if WEX_DSI_ROTATION_DIAGNOSTICS
 assert(registryRoot->countPrefix("WEXDSIScalerCall")==64);
 auto scalerRecordData=OSDynamicCast(OSData,registryRoot->getProperty("WEXDSIScalerCall5"));
 assert(scalerRecordData && scalerRecordData->getLength()==29*sizeof(uint64_t));
 const auto scalerRecord=static_cast<const uint64_t *>(scalerRecordData->getBytesNoCopy());
 assert(scalerRecord[0]==1 && scalerRecord[1]==70 && scalerRecord[2]==reinterpret_cast<uintptr_t>(scalerObject));
 assert(scalerRecord[3]==reinterpret_cast<uintptr_t>(fb) && scalerRecord[4]==1920 && scalerRecord[5]==1080);
 assert(scalerRecord[6]==1080 && scalerRecord[7]==1920 && scalerRecord[8]==135260000 && scalerRecord[9]==0x1234);
 assert(scalerRecord[10]==1 && scalerRecord[11]==1 && scalerRecord[12]==0 && scalerRecord[13]);
 assert(scalerRecord[14]==48 && scalerRecord[15]==1 && scalerRecord[28]==64 && !scalerDelegateCalls);
 for(unsigned i=0;i<12;++i)assert(scalerRecord[16+i]==expectedScalerInfo[i]);
 for(unsigned i=0;i<64;++i){const auto expected=i<6?i+65:i+1;assert(scalerSequenceForSlot(registryRoot,i)==expected);}
 // Concurrent callers may finish ring publication out of sequence, but each
 // stored record must still identify its originating slot and unique sequence.
 std::vector<std::thread> scalerThreads;
 for(unsigned t=0;t<8;++t)scalerThreads.emplace_back([&]{for(unsigned i=0;i<25;++i)
  assert(!XE::wrapDSIScalerSupport(scalerObject,1920,1080,1080,1920,135260000,0x1234,true));});
 for(auto &thread:scalerThreads)thread.join();
 bool scalerSeen[512]{};unsigned scalerRecords=0;
 for(unsigned i=0;i<64;++i){const auto serial=scalerSequenceForSlot(registryRoot,i);assert(serial>0 && serial<512 && (serial-1)%64==i && !scalerSeen[serial]);scalerSeen[serial]=true;++scalerRecords;}
 assert(scalerRecords==64 && !scalerDelegateCalls);
#else
 assert(registryRoot->countPrefix("WEXDSIScalerCall")==0);
#endif
 auto nullScalerInfo=OSData::withBytes(expectedScalerInfo,sizeof(expectedScalerInfo));assert(nullScalerInfo);nullScalerInfo->nullBytes=true;
 assert(fb->setProperty("IOFBScalerInfo",nullScalerInfo));nullScalerInfo->release();
 assert(!XE::wrapDSIScalerSupport(scalerObject,1920,1080,1080,1920,135260000,0x1234,true));
#if WEX_DSI_ROTATION_DIAGNOSTICS
 auto nullScalerRecord=OSDynamicCast(OSData,registryRoot->getProperty("WEXDSIScalerCall14"));
 assert(nullScalerRecord && nullScalerRecord->getLength()==29*sizeof(uint64_t));
 const auto nullScalerWords=static_cast<const uint64_t*>(nullScalerRecord->getBytesNoCopy());
 assert(nullScalerWords[1]==271 && nullScalerWords[14]==UINT32_MAX && !nullScalerWords[15]);
 for(unsigned i=16;i<28;++i)assert(!nullScalerWords[i]);
#endif
 alignas(8) uint8_t peerScaler[0x20]{};Display13FBMemory::field<IOService*>(peerScaler,0x10)=peer;
 const auto scalerRecordsBefore=registryRoot->countPrefix("WEXDSIScalerCall");
 assert(XE::wrapDSIScalerSupport(peerScaler,1920,1080,1080,1920,135260000,0x1234,true));
 assert(scalerDelegateCalls==1 && registryRoot->countPrefix("WEXDSIScalerCall")==scalerRecordsBefore);
 // copyProperty must keep a replaced IOFBScalerInfo alive until the copy is released.
 auto heldScalerInfo=OSDynamicCast(OSData,fb->copyProperty("IOFBScalerInfo"));assert(heldScalerInfo);
 auto replacementScalerInfo=OSData::withBytes(expectedScalerInfo,sizeof(expectedScalerInfo));assert(replacementScalerInfo);
 assert(fb->setProperty("IOFBScalerInfo",replacementScalerInfo));replacementScalerInfo->release();
 assert(heldScalerInfo->getLength()==sizeof(expectedScalerInfo));heldScalerInfo->release();
#if WEX_DSI_ROTATION_DIAGNOSTICS
 // Replacing the property while calls copy it must keep each old object alive
 // until its trace has copied the words and released its reference.
 std::vector<std::thread> propertyThreads;
 for(unsigned t=0;t<4;++t)propertyThreads.emplace_back([&]{for(unsigned i=0;i<40;++i){
  auto next=OSData::withBytes(expectedScalerInfo,sizeof(expectedScalerInfo));assert(next);
  assert(fb->setProperty("IOFBScalerInfo",next));next->release();
  assert(!XE::wrapDSIScalerSupport(scalerObject,1920,1080,1080,1920,135260000,0x1234,true));}});
 for(auto &thread:propertyThreads)thread.join();
 for(unsigned slot=0;slot<64;++slot){char key[40];snprintf(key,sizeof(key),"WEXDSIScalerCall%u",slot);
  auto d=OSDynamicCast(OSData,registryRoot->getProperty(key));assert(d && d->getLength()==29*sizeof(uint64_t));
  const auto record=static_cast<const uint64_t*>(d->getBytesNoCopy());
  assert(record[1] && (record[1]-1)%64==slot && record[14]==48 && record[15]==1);
  for(unsigned i=0;i<12;++i)assert(record[16+i]==expectedScalerInfo[i]);}
#endif
 // Profile-derived enumeration never calls native EDID/AUX.
 uint8_t online=0,edid[128];memset(edid,0xCC,sizeof(edid));uint64_t length=128;
 assert(!XE::wrapDSIDDC(fb,1,edid,&length,0,nullptr) && length==128 && edid[0]==0 && edid[1]==255 && !nativeDDCCalls);
 uint8_t expected[128];assert(makePanelEDID(p.mode,expected) && !memcmp(edid,expected,128));
 length=127;memset(edid,0xCC,128);assert(XE::wrapDSIDDC(fb,1,edid,&length,0,nullptr)==kIOReturnBadArgument && !length && edid[0]==0xCC);
 length=128;assert(XE::wrapDSIDDC(fb,2,edid,&length,0,nullptr)==kIOReturnUnsupported && !length);
 {const auto saved=dsiFixedMode.mode;dsiFixedMode.mode={136887,1200,68,40,12,1920,239,205,2,141,226};
  length=128;assert(!XE::wrapDSIDDC(fb,1,edid,&length,0,nullptr) && edid[126]==1);
  length=128;assert(!XE::wrapDSIDDC(fb,2,edid,&length,0,nullptr) && length==128 && edid[0]==0x70);
  uint8_t block[128];assert(makePanelEDID(dsiFixedMode.mode,block,2) && !memcmp(block,edid,128));
  dsiFixedMode.mode=saved;length=128;assert(!XE::wrapDSIDDC(fb,1,edid,&length,0,nullptr));}
 length=128;assert(XE::wrapDSIDDC(fb,1,edid,&length,1,nullptr)==kIOReturnUnsupported && !length);
 assert(XE::wrapDSIDDC(peer,2,edid,&length,1,testPath)==9 && length==128 && nativeDDCCalls==1);
 xe.adlnOutputWiring.dsi=0;assert(XE::wrapDSIDDC(fb,2,edid,&length,1,testPath)==9 && nativeDDCCalls==2);xe.adlnOutputWiring.dsi=1;
 assert(!XE::wrapDSIGetOnlineInfo(fb,peer,&online,edid) && online==1 && !nativeOnlineCalls);
 assert(!memcmp(edid,expected,128));
 assert(XE::wrapDSIGetOnlineInfo(fb,fb,&online,edid)==kIOReturnNotReady && !online);
 assert(XE::wrapDSIGetOnlineInfo(fb,peer,nullptr,edid)==kIOReturnBadArgument);
 int32_t current=-9,depth=-9;
 Display13FBMemory::field<uint8_t>(fb,0x1E0)=1;
 Display13FBMemory::field<uint32_t>(fb,0x4250)=uint32_t(-5);
 Display13FBMemory::field<uint32_t>(fb,0x4254)=0;
 assert(!XE::wrapDSIGetCurrentMode(fb,&current,&depth) && current==FixedModeID && !depth && !nativeCurrentCalls);
 assert(Display13FBMemory::field<uint32_t>(fb,0x4250)==FixedModeID);
 XE::updateReady=false;current=-9;
 assert(XE::wrapDSIGetCurrentMode(fb,&current,&depth)==kIOReturnNotReady && current==-9);
 assert(XE::wrapDSIGetOnlineInfo(fb,peer,&online,edid)==kIOReturnNotReady && !online);XE::updateReady=true;
 Display13FBMemory::field<uint8_t>(fb,0x1E0)=0;
 assert(XE::wrapDSIGetCurrentMode(fb,&current,&depth)==kIOReturnNotReady);
 Display13FBMemory::field<uint8_t>(fb,0x1E0)=1;
 assert(XE::wrapDSIGetCurrentMode(peer,&current,&depth)==7 && current==77 && depth==3);
 assert(XE::wrapDSIGetOnlineInfo(peer,peer,&online,edid)==8 && online==9 && edid[0]==42);
 xe.dsiPanelOnly=true;auto beforeOnline=nativeOnlineCalls;
 assert(!XE::wrapDSIGetOnlineInfo(peer,peer,&online,edid) && !online && !edid[0] && nativeOnlineCalls==beforeOnline);
 assert(XE::wrapDSIGetOnlineInfo(peer,peer,nullptr,edid)==kIOReturnBadArgument);xe.dsiPanelOnly=false;
 xe.adlnOutputWiring.dsi=0;assert(XE::wrapDSIGetCurrentMode(fb,&current,&depth)==7);
 assert(XE::wrapDSIGetOnlineInfo(fb,peer,&online,edid)==8);xe.adlnOutputWiring.dsi=1;
 assert(nativeCurrentCalls==2 && nativeOnlineCalls==2);
 auto callsBeforeRepeat=calls[0];assert(xe.bindDSIFixedMode(fb,p) && calls[0]==callsBeforeRepeat && fb->refs==2 && dsiFixedMode.array->refs==2);
 auto other=p;++other.mode.clockKHz;assert(!xe.bindDSIFixedMode(fb,other));
 assert(XE::wrapDSIGetModeCount(fb)==1 && XE::wrapDSIGetModeCount(peer)==3);
 int32_t peerMode=0;assert(!XE::wrapDSIGetModes(peer,&peerMode) && peerMode==42);
 assert(XE::wrapDSIGetModes(fb,nullptr)==kIOReturnBadArgument);
 int32_t mode=0;assert(!XE::wrapDSIGetModes(fb,&mode) && mode==FixedModeID);
 const void *timing=nullptr,*description=nullptr;
 assert(!XE::wrapDSIValidateMode(fb,mode,&description,&timing) && timing==dsiFixedMode.data->getBytesNoCopy());
 auto validations=calls[1];assert(XE::wrapDSIValidateMode(fb,mode+0x400,&description,&timing)==kIOReturnBadArgument);
 assert(!description && !timing && calls[1]==validations);
 ModeInformation info;assert(!XE::wrapDSIGetModeInformation(fb,mode,&info));
 assert(info.width==1080 && info.height==1920 && info.widthMM==86 && info.heightMM==154 && info.depth==0);
 assert(!(info.flags&0x00400000));
 auto invalid=p.mode;invalid.width=0;assert(!annotateMode(invalid,info));
 assert(XE::wrapDSIGetModeInformation(fb,mode,nullptr)==kIOReturnBadArgument);
 assert(info.refresh>59*65536 && info.refresh<61*65536 && (info.flags&0x02000407)==0x02000407);
 geometryOK=false;assert(XE::wrapDSIGetModeInformation(fb,mode,&info)==kIOReturnNotReady);geometryOK=true;
 auto replacement=OSArray::withCapacity(1);auto data=OSData::withBytes(&dsiFixedMode.timing,sizeof(Timing));
 replacement->setObject(data);assert(!XE::wrapDSISetDetailedTimings(fb,replacement));
 assert(Display13FBMemory::field<OSArray*>(fb,0x4260)==dsiFixedMode.array && replacement->refs==1);
 data->bytes.resize(1);assert(XE::wrapDSISetDetailedTimings(fb,replacement)==kIOReturnUnsupported);
 assert(XE::wrapDSISetDetailedTimings(fb,nullptr)==kIOReturnUnsupported);
 data->release();replacement->release();
 auto offered=dsiFixedMode.timing;offered.reservedA[0]=0x80001001;offered.signalLevels=1;
 offered.pixelClock=offered.minPixelClock=offered.maxPixelClock=135260000;
 offered.numLinks=2; // CoreDisplay hint on a verified single-link panel
 auto list=OSArray::withCapacity(1);auto wrong=offered;wrong.horizontalActive=640;
 auto wrongData=OSData::withBytes(&wrong,sizeof(wrong));auto offeredData=OSData::withBytes(&offered,sizeof(offered));
 list->setObject(wrongData);list->setObject(offeredData);
 assert(!XE::wrapDSISetDetailedTimings(fb,list));assert(dsiFixedMode.clientMode==0x80001001 && list->refs==1);
 assert(dsiModeCoherent(fb) && Display13FBMemory::field<OSArray*>(fb,0x4260)==dsiFixedMode.array);
 assert(!XE::wrapDSIValidateMode(fb,static_cast<int32_t>(0x80001001),nullptr,&timing));
 assert(timing==dsiFixedMode.data->getBytesNoCopy());
 assert(XE::wrapDSIValidateMode(fb,static_cast<int32_t>(0x80001000),nullptr,&timing)==kIOReturnBadArgument);
 assert(!XE::wrapDSIGetModes(fb,&mode) && static_cast<uint32_t>(mode)==0x80001001);
 assert(!XE::wrapDSIGetCurrentMode(fb,&current,&depth) && static_cast<uint32_t>(current)==0x80001001);
 assert(!XE::wrapDSIGetModeInformation(fb,mode,&info) && info.width==1080 && info.height==1920 && !info.depth);
 offeredData->bytes[64]^=1;
 assert(XE::wrapDSISetDetailedTimings(fb,list)==kIOReturnUnsupported && dsiFixedMode.clientMode==0x80001001);
 offered.reservedA[0]=0x80003000;
 auto renewed=OSArray::withCapacity(1);auto renewedData=OSData::withBytes(&offered,sizeof(offered));renewed->setObject(renewedData);
 assert(!XE::wrapDSISetDetailedTimings(fb,renewed) && dsiFixedMode.clientMode==0x80003000);
 assert(dsiModeCoherent(fb) && renewed->refs==1);
 assert(XE::wrapDSIValidateMode(fb,static_cast<int32_t>(0x80001001),nullptr,&timing)==kIOReturnBadArgument);
 assert(!XE::wrapDSIValidateMode(fb,static_cast<int32_t>(0x80003000),nullptr,&timing) && timing==dsiFixedMode.data->getBytesNoCopy());
 assert(!XE::wrapDSIGetModes(fb,&mode) && static_cast<uint32_t>(mode)==0x80003000);
 assert(!XE::wrapDSIGetCurrentMode(fb,&current,&depth) && static_cast<uint32_t>(current)==0x80003000);
 // Rotation offers must survive validation without changing the retained wire mode.
 auto rotated=offered;rotated.reservedA[0]=0x80005000;rotated.scalerFlags=0x30;rotated.horizontalScaled=1920;rotated.verticalScaled=1080;
 auto rotationList=OSArray::withCapacity(2);auto rotationData=OSData::withBytes(&rotated,sizeof(rotated));
 rotationList->setObject(renewedData);rotationList->setObject(rotationData);
 assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));
#if WEX_DSI_ROTATION_DIAGNOSTICS
 // The diagnostic ring stores the raw Timing bytes consumed by the wrapper.
 OSData *timingSnapshot=nullptr;uint64_t timingSequence=0;
 for(const auto &property:registryRoot->properties)if(!strncmp(property.first.c_str(),"WEXDSITimingInput",17) &&
    property.first.find("Error")==std::string::npos) {
  auto snapshot=OSDynamicCast(OSData,property.second);if(!snapshot || snapshot->getLength()<sizeof(DSITimingTraceHeader))continue;
  auto h=static_cast<const DSITimingTraceHeader *>(snapshot->getBytesNoCopy());
  if(h->sequence>timingSequence){timingSequence=h->sequence;timingSnapshot=snapshot;}}
 assert(timingSnapshot && timingSequence && timingSnapshot->getLength()==sizeof(DSITimingTraceHeader)+2*(sizeof(DSITimingTraceEntry)+sizeof(Timing)));
 auto traceHeader=static_cast<const DSITimingTraceHeader *>(timingSnapshot->getBytesNoCopy());
 assert(traceHeader->version==2 && traceHeader->arrayCount==2 && traceHeader->capturedEntries==2);
 assert(traceHeader->timingSize==160 && traceHeader->ringSlots==16 && traceHeader->scalerInfoLength==48 && !traceHeader->flags);
 for(unsigned i=0;i<12;++i)assert(traceHeader->scalerInfo[i]==expectedScalerInfo[i]);
 auto traceEntry0=reinterpret_cast<const DSITimingTraceEntry *>(static_cast<const uint8_t *>(timingSnapshot->getBytesNoCopy())+sizeof(*traceHeader));
 const auto raw0=reinterpret_cast<const uint8_t *>(traceEntry0+1);
 auto traceEntry1=reinterpret_cast<const DSITimingTraceEntry *>(raw0+traceEntry0->storedBytes);
 const auto raw1=reinterpret_cast<const uint8_t *>(traceEntry1+1);
 assert(traceEntry0->index==0 && traceEntry0->kind==1 && traceEntry0->length==160 && traceEntry0->storedBytes==160);
 assert(traceEntry1->index==1 && traceEntry1->kind==1 && traceEntry1->length==160 && traceEntry1->storedBytes==160);
 assert(!memcmp(raw0,renewedData->getBytesNoCopy(),160) && !memcmp(raw1,rotationData->getBytesNoCopy(),160));
 const auto tracedRotation=reinterpret_cast<const Timing *>(raw1);
 assert(tracedRotation->scalerFlags==0x30 && tracedRotation->horizontalScaled==1920 && tracedRotation->verticalScaled==1080);
#endif
 // A size-valid OSData with a null bytes pointer must be skipped safely in
 // both the first policy pass and the later accepted-timing copy pass.
 auto nullTiming=OSData::withBytes(&rotated,sizeof(rotated));assert(nullTiming);nullTiming->nullBytes=true;
 auto mixedNullList=OSArray::withCapacity(2);mixedNullList->setObject(renewedData);mixedNullList->setObject(nullTiming);
 assert(!XE::wrapDSISetDetailedTimings(fb,mixedNullList));
#if WEX_DSI_ROTATION_DIAGNOSTICS
 OSData *nullSnapshot=nullptr;uint64_t nullSequence=0;
 for(const auto &property:registryRoot->properties)if(!strncmp(property.first.c_str(),"WEXDSITimingInput",17) &&
    property.first.find("Error")==std::string::npos){auto snapshot=OSDynamicCast(OSData,property.second);
  if(snapshot && snapshot->getLength()>=sizeof(DSITimingTraceHeader)){
   auto h=static_cast<const DSITimingTraceHeader*>(snapshot->getBytesNoCopy());
   if(h->sequence>nullSequence){nullSequence=h->sequence;nullSnapshot=snapshot;}}}
 assert(nullSnapshot && nullSnapshot->getLength()==sizeof(DSITimingTraceHeader)+sizeof(DSITimingTraceEntry)*2+sizeof(Timing));
 auto nullHeader=static_cast<const DSITimingTraceHeader*>(nullSnapshot->getBytesNoCopy());
 auto nullEntry0=reinterpret_cast<const DSITimingTraceEntry*>(static_cast<const uint8_t*>(nullSnapshot->getBytesNoCopy())+sizeof(*nullHeader));
 auto nullRaw0=reinterpret_cast<const uint8_t*>(nullEntry0+1);
 auto nullEntry1=reinterpret_cast<const DSITimingTraceEntry*>(nullRaw0+nullEntry0->storedBytes);
 assert(nullHeader->arrayCount==2 && nullEntry0->kind==1 && nullEntry0->storedBytes==160);
 assert(nullEntry1->index==1 && nullEntry1->kind==2 && nullEntry1->length==160 && !nullEntry1->storedBytes);
#else
 assert(registryRoot->countPrefix("WEXDSITimingInput")==0);
#endif
 auto nullOnlyList=OSArray::withCapacity(1);nullOnlyList->setObject(nullTiming);
 assert(XE::wrapDSISetDetailedTimings(fb,nullOnlyList)==kIOReturnUnsupported);
 nullOnlyList->release();mixedNullList->release();nullTiming->release();
 assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));
#if WEX_DSI_ROTATION_DIAGNOSTICS
 // Exercise each trace error path while confirming valid driver behavior is
 // unchanged for allocation, append and registry-publication failures.
 uint64_t traceError[6]{};
 auto checkTraceError=[&](uint64_t reason,uint64_t count){assert(registryRoot->latestTimingError(traceError));
  assert(traceError[0]==2 && traceError[1] && traceError[2]==count && traceError[3]<=64 && traceError[4] && traceError[5]==reason);};
 failNextCapacity=true;assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));checkTraceError(1,2);
 failAppendAt=appendCalls+2;assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));failAppendAt=0;checkTraceError(2,2);
 failTimingSnapshotPublication=true;assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));failTimingSnapshotPublication=false;checkTraceError(3,2);
 const auto errorsBefore=registryRoot->countPrefixAll("WEXDSITimingInputError");
 failTimingErrorPublication=true;failNextCapacity=true;assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));failNextCapacity=false;failTimingErrorPublication=false;
 assert(registryRoot->countPrefixAll("WEXDSITimingInputError")==errorsBefore);
 auto savedFBLock=Display13FBMemory::field<IORecursiveLock*>(fb,0x4220);
 Display13FBMemory::field<IORecursiveLock*>(fb,0x4220)=nullptr;
 assert(XE::wrapDSISetDetailedTimings(fb,rotationList)==kIOReturnNotReady);
 Display13FBMemory::field<IORecursiveLock*>(fb,0x4220)=savedFBLock;checkTraceError(4,UINT32_MAX);
 for(unsigned i=0;i<20;++i)assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));
 assert(registryRoot->countPrefix("WEXDSITimingInput")==DSITimingTraceSlots);
 uint64_t minTimingSequence=UINT64_MAX,maxTimingSequence=0;bool timingSequenceSeen[4096]{};
 for(unsigned i=0;i<DSITimingTraceSlots;++i){const auto serial=timingSequenceForSlot(registryRoot,i);
  assert(serial && serial<4096 && (serial-1)%DSITimingTraceSlots==i && !timingSequenceSeen[serial]);
  timingSequenceSeen[serial]=true;if(serial<minTimingSequence)minTimingSequence=serial;if(serial>maxTimingSequence)maxTimingSequence=serial;}
 assert(maxTimingSequence-minTimingSequence==DSITimingTraceSlots-1);
 // Concurrent callback completions may publish out of sequence after releasing
 // the FB lock; sequence/slot metadata still lets readers reject stale entries.
 std::vector<std::thread> timingThreads;
 for(unsigned t=0;t<4;++t)timingThreads.emplace_back([&]{for(unsigned i=0;i<12;++i)
  assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));});
 for(auto &thread:timingThreads)thread.join();
 memset(timingSequenceSeen,0,sizeof(timingSequenceSeen));unsigned timingRecords=0;
 for(unsigned i=0;i<DSITimingTraceSlots;++i){const auto serial=timingSequenceForSlot(registryRoot,i);
  assert(serial && serial<4096 && (serial-1)%DSITimingTraceSlots==i && !timingSequenceSeen[serial]);
  char key[40];snprintf(key,sizeof(key),"WEXDSITimingInput%u",i);
  auto snapshot=OSDynamicCast(OSData,registryRoot->getProperty(key));auto h=static_cast<const DSITimingTraceHeader*>(snapshot->getBytesNoCopy());
  assert(h->version==2 && h->arrayCount==2 && h->capturedEntries==2 && h->timingSize==160 && !h->flags);
  timingSequenceSeen[serial]=true;++timingRecords;}
 assert(timingRecords==DSITimingTraceSlots);
 auto errorLock=Display13FBMemory::field<IORecursiveLock*>(fb,0x4220);
 Display13FBMemory::field<IORecursiveLock*>(fb,0x4220)=nullptr;
 for(unsigned i=0;i<20;++i)assert(XE::wrapDSISetDetailedTimings(fb,rotationList)==kIOReturnNotReady);
 Display13FBMemory::field<IORecursiveLock*>(fb,0x4220)=errorLock;
 assert(registryRoot->countPrefixAll("WEXDSITimingInputError")==DSITimingTraceSlots);
 minTimingSequence=UINT64_MAX;maxTimingSequence=0;memset(timingSequenceSeen,0,sizeof(timingSequenceSeen));
 for(unsigned i=0;i<DSITimingTraceSlots;++i){const auto serial=timingErrorSequenceForSlot(registryRoot,i);
  assert(serial && serial<4096 && (serial-1)%DSITimingTraceSlots==i && !timingSequenceSeen[serial]);
  timingSequenceSeen[serial]=true;if(serial<minTimingSequence)minTimingSequence=serial;if(serial>maxTimingSequence)maxTimingSequence=serial;}
 assert(maxTimingSequence-minTimingSequence==DSITimingTraceSlots-1);
#else
 assert(registryRoot->countPrefix("WEXDSITimingInput")==0);
 for(unsigned i=0;i<16;++i)assert(!registryRoot->getProperty((std::string("WEXDSITimingInputError")+std::to_string(i)).c_str()));
#endif
 assert(XE::wrapDSIGetModeCount(fb)==2);int32_t pair[2];assert(!XE::wrapDSIGetModes(fb,pair));
 assert(static_cast<uint32_t>(pair[1])==rotated.reservedA[0]);
 assert(!XE::wrapDSIValidateMode(fb,pair[1],nullptr,&timing));
 assert(static_cast<const Timing *>(timing)->numLinks==1 && rotated.numLinks==2);
 assert(static_cast<const Timing *>(timing)->scalerFlags==0x30 && !dsiFixedMode.timing.scalerFlags);
 assert(!XE::wrapDSIGetModeInformation(fb,pair[1],&info) && info.width==1920 && info.height==1080);
 // A large OS list with many distinct scaled dimensions must not be
 // rejected by the former 32-mode capacity or 1024-index gates.
 auto many=OSArray::withCapacity(1);
 for(unsigned i=0;i<1100;++i){auto t=rotated;t.reservedA[0]=0x80006000+i;
  t.horizontalScaled=1280+2*i;t.verticalScaled=720+i;
  t.pixelClock=t.minPixelClock=t.maxPixelClock=(i&1)?135255000:135260000;
  auto data=OSData::withBytes(&t,sizeof(t));many->setObject(data);data->release();}
 auto previous=dsiFixedMode.offeredData;const auto beforePublish=calls[0];
 capacityOK=false;
 assert(XE::wrapDSISetDetailedTimings(fb,many)==kIOReturnNoResources);
 capacityOK=true;appendOK=false;
 assert(XE::wrapDSISetDetailedTimings(fb,many)==kIOReturnNoResources);
 appendOK=true;
 assert(dsiFixedMode.offeredData==previous && XE::wrapDSIGetModeCount(fb)==2 && calls[0]==beforePublish);
 assert(!XE::wrapDSISetDetailedTimings(fb,many) && XE::wrapDSIGetModeCount(fb)==1100);
#if WEX_DSI_ROTATION_DIAGNOSTICS
 OSData *largeSnapshot=nullptr;uint64_t largeSequence=0;
 for(const auto &property:registryRoot->properties)if(!strncmp(property.first.c_str(),"WEXDSITimingInput",17) &&
    property.first.find("Error")==std::string::npos){auto snapshot=OSDynamicCast(OSData,property.second);
  if(snapshot && snapshot->getLength()>=sizeof(DSITimingTraceHeader)){
   auto h=static_cast<const DSITimingTraceHeader*>(snapshot->getBytesNoCopy());
   if(h->sequence>largeSequence){largeSequence=h->sequence;largeSnapshot=snapshot;}}}
 assert(largeSnapshot && largeSnapshot->getLength()==sizeof(DSITimingTraceHeader)+64*(sizeof(DSITimingTraceEntry)+sizeof(Timing)));
 auto largeHeader=static_cast<const DSITimingTraceHeader*>(largeSnapshot->getBytesNoCopy());
 assert(largeHeader->arrayCount==1100 && largeHeader->capturedEntries==64 && largeHeader->flags==2);
 auto largeEntry=reinterpret_cast<const DSITimingTraceEntry*>(static_cast<const uint8_t*>(largeSnapshot->getBytesNoCopy())+sizeof(*largeHeader));
 const auto largeRaw=reinterpret_cast<const uint8_t*>(largeEntry+1);
 assert(largeEntry->index==0 && largeEntry->kind==1 && largeEntry->storedBytes==160);
 assert(!memcmp(largeRaw,static_cast<OSData*>(many->getObject(0))->getBytesNoCopy(),160));
#endif
 int32_t modes[1100];assert(!XE::wrapDSIGetModes(fb,modes));
 for(unsigned i=0;i<1100;++i){assert(static_cast<uint32_t>(modes[i])==0x80006000+i);
  assert(!XE::wrapDSIValidateMode(fb,modes[i],nullptr,&timing));
  assert(static_cast<const Timing *>(timing)->horizontalScaled==1280+2*i);
  assert(!XE::wrapDSIGetModeInformation(fb,modes[i],&info) && info.width==1280+2*i && info.height==720+i);}
 many->release();assert(!XE::wrapDSISetDetailedTimings(fb,rotationList));
 // A new quarter-turn generation can contain no unrotated wire alias.
 auto landscape=OSArray::withCapacity(2);auto scaled=rotated;
 scaled.reservedA[0]=0x80005003;scaled.horizontalScaled=1638;scaled.verticalScaled=1024;
 auto scaledData=OSData::withBytes(&scaled,sizeof(scaled));landscape->setObject(scaledData);landscape->setObject(rotationData);
 assert(!XE::wrapDSISetDetailedTimings(fb,landscape) && dsiFixedMode.clientMode==rotated.reservedA[0]);
 assert(!XE::wrapDSIGetCurrentMode(fb,&current,&depth) && static_cast<uint32_t>(current)==rotated.reservedA[0]);
 assert(XE::wrapDSIValidateMode(fb,static_cast<int32_t>(offered.reservedA[0]),nullptr,&timing)==kIOReturnBadArgument);
 assert(!XE::wrapDSISetDetailedTimings(fb,rotationList) && dsiFixedMode.clientMode==offered.reservedA[0]);
 scaledData->release();landscape->release();rotationData->release();rotationList->release();
 // Legacy UI attributes alone must never fabricate a presentation timing.
 auto orientation=new OSNumber;fb->setProperty("IOFBTransform",orientation);
 for(auto transform:{3ULL,5ULL,0ULL}) {
  orientation->value=transform;
  assert(!XE::wrapDSIValidateMode(fb,static_cast<int32_t>(offered.reservedA[0]),nullptr,&timing));
  assert(static_cast<const Timing *>(timing)->scalerFlags==0);
  assert(!static_cast<const Timing *>(timing)->horizontalScaled);
  assert(!static_cast<const Timing *>(timing)->verticalScaled);
  assert(!dsiFixedMode.timing.scalerFlags && static_cast<const Timing *>(timing)->horizontalActive==1080);
 }
 orientation->value=7;
 assert(!XE::wrapDSIValidateMode(fb,static_cast<int32_t>(offered.reservedA[0]),nullptr,&timing));
 assert(!static_cast<const Timing *>(timing)->scalerFlags);
 fb->removeProperty("IOFBTransform");orientation->release();
 renewedData->bytes[64]^=1;
 assert(XE::wrapDSISetDetailedTimings(fb,renewed)==kIOReturnUnsupported && dsiFixedMode.clientMode==0x80003000);
 renewedData->release();renewed->release();
 wrongData->release();offeredData->release();list->release();
 fb->removeProperty("IOFBDetailedTimings");assert(!XE::wrapDSIGetModeCount(fb) && dsiFixedMode.array->refs==1);
 assert(XE::wrapDSIValidateMode(fb,mode,nullptr,nullptr)==kIOReturnNotReady);
 const auto cleanup=[&]{
  assert(!xe.releaseDSIFixedMode(peer));dsiFixedMode.ready=true;
  assert(!xe.releaseDSIFixedMode(fb));dsiFixedMode.ready=false;
  const auto refs=dsiFixedMode.array->refs.load();clearOK=false;
  assert(!xe.releaseDSIFixedMode(fb) && dsiModeTarget(fb) && dsiFixedMode.array->refs==refs && fb->refs==2);
  clearOK=true;
  assert(xe.releaseDSIFixedMode(fb) && !dsiModeTarget(fb) && fb->refs==1);
  assert(!Display13FBMemory::field<OSArray*>(fb,0x4260) && !fb->getProperty("IOFBDetailedTimings"));
  assert(xe.releaseDSIFixedMode(fb));};
 cleanup();publishOK=false;assert(!xe.bindDSIFixedMode(fb,p));
 assert(dsiModeTarget(fb) && dsiFixedMode.array->refs==1 && Display13FBMemory::field<OSArray*>(fb,0x4260)==dsiFixedMode.array);
 assert(!XE::wrapDSIGetModeCount(fb));cleanup();
 assert(!fbLock.depth && !ownerLock.depth);fb->release();peer->release();registryRoot->clear();assert(!alive);
 puts("PASS production DSI fixed mode: retained native array, publication failure, dynamic enumeration, metadata and foreign FB dispatch");
}
'''.replace('OWNER', owner).replace('FUNCTIONS', functions)
with tempfile.TemporaryDirectory(prefix='dsi-apple-adapter-') as directory:
    path = Path(directory)
    (path/'test.cpp').write_text(code)
    for name,diagnostics_enabled in [('diagnostics-default',None),('diagnostics-on','1'),('diagnostics-off','0')]:
        executable=path/name
        command=['clang++', '-std=c++17', '-O1', '-pthread', '-fsanitize=address,undefined']
        if diagnostics_enabled is not None:command+=['-DWEX_DSI_ROTATION_DIAGNOSTICS='+diagnostics_enabled]
        command+=['-I'+str(root/'WhateverXe'), str(path/'test.cpp'), '-o', str(executable)]
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True)
