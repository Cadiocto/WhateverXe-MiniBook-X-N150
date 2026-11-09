#include "kern_display13_resume_park.hpp"
#include "kern_display13_fb_irq.hpp"
#include <cassert>
#include <map>
#include <algorithm>
#include <vector>
// Register model: status bits follow their enables after a write; the frame
// counter advances on every read.
struct IO {
    std::map<uint32_t,uint32_t> r;std::vector<uint32_t> order;uint32_t frames=100;bool stuckPipe=false;
    uint32_t read(uint32_t reg) {
        if(reg==0x70040)return frames++;
        auto it=r.find(reg);return it==r.end()?0:it->second;
    }
    void write(uint32_t reg,uint32_t v) {
        order.push_back(reg);
        if(reg==0x70008 && !stuckPipe)v=(v&0x80000000U)?(v|0x40000000U):(v&~0x40000000U);
        if(reg>=0x64000 && reg<=0x64200)v=(v&0x80000000U)?(v&~0x80U):(v|0x80U);
        r[reg]=v;
    }
    void delay(){}
};
static IO live(uint32_t port=0) {
    IO io;auto &r=io.r;
    r[0x45404]=0xFFF;r[0x70008]=0xC0000000U;r[0x60400]=0x80000000U|((port+1)<<27)|(2U<<24)|(3U<<1);
    r[0x64000+0x100*port]=0x80000008U;r[0x46140]=(port+1)<<28;r[0x60540]=0x80000000U;
    r[0x70180]=0x84000000U;r[0x7019C]=0x00100000;r[0x68180]=0x80000000U;r[0x7027C]=0x01FF0000;r[0x7037C]=5;r[0x7017C]=7;
    return io;
}
int main() {
    using namespace Display13ResumePark;
    // The DSB qualification uses the driver's own register definition.
    static_assert(Display13FBIRQ::DSBControl(0,0)==0x70B08 && Display13FBIRQ::DSBControl(0,2)==0x70D08,"DSB");
    const Display13Transcoder::Bindings edp {};
    auto io=live();
    auto res=park(io,edp,1);
    assert(res.status==Status::Complete && res.stage==9 && res.port==0 && (io.r[0x42084]&GGTTMode));
    // Linux order: plane, pipe, scaler, transcoder function, DP transport/buffer, clock, DDB.
    const std::vector<uint32_t> want {0x70180,0x7019C,0x70008,0x68180,0x68170,0x68174,0x60404,0x60400,0x64000,0x60540,0x46140,0x7027C,0x7037C,0x7017C,0x42084};
    assert(io.order==want && res.writes==want.size());
    assert(!(io.r[0x70180]&Enable) && !(io.r[0x70008]&0xC0000000U) && !(io.r[0x60400]&Enable) &&
           ((io.r[0x60400]>>27)&15)==0 && !(io.r[0x64000]&Enable) && (io.r[0x64000]&0x80) && !(io.r[0x46140]>>28) &&
           !io.r[0x7027C] && !io.r[0x68180] && io.r[0x7019C]==0x00100000);
    // Non-panel ports, DSI bindings and other topologies refuse without writes.
    auto refuse=[&](IO x,uint32_t panel,const Display13Transcoder::Bindings &b=Display13Transcoder::Bindings{}) {
        const auto copy=x.r;auto r=park(x,b,panel);
        assert(r.status==Status::Topology && !r.writes && x.order.empty() && x.r==copy);
    };
    refuse(live(),0);                 // DDI A is not an eDP port on this board
    refuse(live(1),1);                // active DDI B, only A is eDP
    Display13Transcoder::Bindings dsi;dsi.dsi0Pipe=0;refuse(live(),1,dsi);
    {auto x=live();x.r[0x71008]=0xC0000000U;refuse(x,1);}          // second pipe running
    {auto x=live();x.r[0x61400]=0x80000000U;refuse(x,1);}          // second transcoder enabled
    {auto x=live();x.r[0x60400]&=~(7U<<24);refuse(x,1);}          // HDMI, not DP SST
    {auto x=live();x.r[0x60400]=(x.r[0x60400]&~(7U<<24))|(3U<<24);refuse(x,1);} // MST
    {auto x=live();x.r[0x70280]=0x80000000U;refuse(x,1);}          // second plane
    {auto x=live();x.r[0x70080]=0x27;refuse(x,1);}                 // cursor
    {auto x=live();x.r[0x70B08]=1;refuse(x,1);}                    // DSB
    {auto x=live();x.r[0x46140]=3U<<28;refuse(x,1);}               // clock from another DDI
    {auto x=live();x.r[0x70008]=0;refuse(x,1);}                    // nothing scanning out
    {auto x=live();x.r[0x64000]=0x80U;refuse(x,1);}                // buffer already off
    {auto x=live();x.r[0x70008]=UINT32_MAX;refuse(x,1);}           // unreadable
    // An already set GGTT mode bit is not rewritten; unrelated chicken bits survive.
    {auto x=live();x.r[0x42084]=GGTTMode|0x5;auto r=park(x,edp,1);assert(r.status==Status::Complete && x.r[0x42084]==(GGTTMode|0x5) && x.order.back()==0x7017C);}
    {auto x=live();x.r[0x42084]=0x5;park(x,edp,1);assert(x.r[0x42084]==(GGTTMode|0x5));}
    // CMTG secondary (firmware eDP A): CTL2 is preserved, nothing else changes.
    {auto x=live();x.r[0x60404]=8;x.r[0x6FA88]=0x80800000U;auto r=park(x,edp,1);
     assert(r.status==Status::Complete && r.cmtgSecondary && x.r[0x60404]==8);
     std::vector<uint32_t> w2=want;w2.erase(std::find(w2.begin(),w2.end(),0x60404));assert(x.order==w2);}
    // CTL2 set without a running CMTG, or any other CTL2 use: refuse, no writes.
    {auto x=live();x.r[0x60404]=8;refuse(x,1);}
    {auto x=live();x.r[0x60404]=0x10;x.r[0x6FA88]=0x80800000U;refuse(x,1);}
    // A pipe that never reports off stops at stage 2 with the timeout witness.
    {auto x=live();x.stuckPipe=true;auto r=park(x,edp,1);
     assert(r.status==Status::Timeout && r.stage==2 && r.lastReg==0x70008);}
    // A shared power change mid-transaction is reported as peer drift.
    {struct Drift:IO{unsigned n=0;uint32_t read(uint32_t reg){if(reg==0x45404 && ++n>2)return 0;return IO::read(reg);}};
     Drift x;static_cast<IO &>(x)=live();auto r=park(x,edp,1);assert(r.status==Status::PeerChanged && r.stage==1);}
}
