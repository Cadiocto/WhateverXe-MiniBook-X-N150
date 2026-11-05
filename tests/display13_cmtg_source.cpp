#define main existing_dp_test_main
#include "display13_linear_dp.cpp"
#undef main
static IO secondary(){auto io=firmware(0,0,4,true);io.regs[0x60404]=8;io.regs[0x6fa88]=0x80800000;return io;}
int main(){unsigned cases=0;
 {auto io=secondary();auto boot=readDisplay(io,true);assert(dpSource(boot,0,0,nullptr,true));assert(!dpSource(boot,0,0,nullptr,false));
  auto r=reinitializeLinear(io,boot,true,0,GOPProtocol::EDP);assert(r.status==LinearStatus::Success&&r.writes);
  assert(io.regs[0x6fa88]==0x80800000&&io.regs[0x60404]==8);for(auto w:io.writes)assert(w.offset!=0x6fa88&&w.offset!=0x60404&&w.offset!=0x60400&&w.offset!=0x46140);++cases;}
 for(unsigned bit=0;bit<32;++bit){auto io=secondary();io.regs[0x6fa88]^=1U<<bit;auto boot=readDisplay(io,true);assert(!dpSource(boot,0,0,nullptr,true));auto r=reinitializeLinear(io,boot,true,0,GOPProtocol::EDP);assert(r.status==LinearStatus::InvalidDPSource&&io.writes.empty());++cases;}
 for(unsigned bit=0;bit<32;++bit){if(bit==3)continue;auto io=secondary();io.regs[0x60404]^=1U<<bit;auto r=reinitializeLinear(io,readDisplay(io,true),true,0,GOPProtocol::EDP);assert(r.status==LinearStatus::InvalidDPSource&&io.writes.empty());++cases;}
 for(unsigned tick=1;tick<=6;++tick){auto io=secondary();auto boot=readDisplay(io,true);io.changeReg=0x6fa88;io.changeTick=tick;auto r=reinitializeLinear(io,boot,true,0,GOPProtocol::EDP);assert(r.status==LinearStatus::StateChanged&&!r.restored);for(auto w:io.writes)assert(w.offset!=0x6fa88&&w.offset!=0x60404);++cases;}
 for(unsigned kind=0;kind<5;++kind){auto io=secondary();auto boot=readDisplay(io,true);
  if(kind==0)for(unsigned i=0;i<boot.count;++i)if(boot.registers[i].offset==0x6fa88)boot.registers[i].offset=0;
  if(kind==1)boot.activePipes=3;
  if(kind==2)io.regs[0x6fa88]^=1;
  if(kind==3)io.regs[0x46140]^=1;
  if(kind==4)io.regs[0x60404]^=1;
  assert(!dpSourceUnchanged(io,boot,0,0,0,nullptr,true));assert(io.writes.empty());++cases;
 }
 printf("PASS CMTG preservation %u source/control/route/drift cases; no CMTG/transcoder/clock writes\n",cases);
}
