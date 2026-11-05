#include "kern_alderlake_n.hpp"
#include <map>
#include <vector>
#include <cassert>
#include <cstdio>
struct IO {
 std::map<uint32_t,uint32_t> regs;std::vector<uint32_t>reads;unsigned deny=0,change=0;bool captured=false;
 bool readable(uint32_t r){return r!=deny;}
 uint32_t read(uint32_t r){reads.push_back(r);if(r==0x6fa88)captured=true;if(captured&&r==change)return regs[r]^1;return regs[r];}
};
static IO base(){IO x;x.regs={{0x45404,0x403},{0x60400,0x8a010102},{0x60404,8},{0x46140,0x10000000},{0x64000,0x80000002},{0x6fa88,0x80800000}};return x;}
int main(){unsigned cases=0;
 for(unsigned kind=0;kind<13;++kind){auto io=base();
  if(kind==1)io.regs[0x45404]&=~(1U<<10);
  if(kind==2)io.regs[0x60400]&=~(1U<<31);
  if(kind==3)io.regs[0x60400]=0x92010002;
  if(kind==4)io.regs[0x60404]=0;
  if(kind==5)io.regs[0x60404]=0x18;
  if(kind==6)io.regs[0x46140]=0;
  if(kind==7)io.regs[0x64000]&=~(1U<<31);
  if(kind==8)io.deny=0x6fa88;
  if(kind>=9&&kind<=11)io.change=kind==9?0x60400:kind==10?0x46140:0x64000;
  auto r=AlderLakeN::readDisplay(io,kind!=12);uint32_t v=0;
  bool got=AlderLakeN::value(r,0x6fa88,v);
  const bool expected=kind==0||(kind>=9&&kind<=11);
  assert(got==expected);if(got)assert(v==0x80800000);
  if(kind>=9&&kind<=11)assert(r.status==AlderLakeN::ReadoutStatus::PowerChanged);
  if(kind==0)assert(r.status==AlderLakeN::ReadoutStatus::Complete);
  unsigned cmtgReads=0;for(auto q:io.reads)if(q==0x6fa88)++cmtgReads;assert(cmtgReads==unsigned(expected));
  assert(!AlderLakeN::dpSource(r,0,0,nullptr,true)); // diagnostic never weakens admission
  ++cases;
 }
 printf("PASS CMTG diagnostic readout %u powered-source/admission/refusal/drift controls; no writes\n",cases);
}
