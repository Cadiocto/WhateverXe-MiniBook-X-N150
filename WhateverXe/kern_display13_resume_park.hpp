// Hibernation (S4) resume: boot firmware and boot.efi relight the internal eDP
// panel on pipe A to show the resume preview. After a real power loss (S3) every
// display consumer is stopped, and shared resume (clock, MBUS, DBUF) requires
// that. Park exactly that one firmware scanout, in the Linux disable order used
// by the boot GOP retirement (intel_disable_transcoder / ddi post-disable):
// planes, pipe, scalers, transcoder function, DDI buffer, transcoder clock, DDB.
// Panel power, backlight, PLL and power wells stay with the native enable path.
// Anything other than one enabled eDP SST scanout on pipe A is refused without
// writes.
#ifndef KERN_DISPLAY13_RESUME_PARK_HPP
#define KERN_DISPLAY13_RESUME_PARK_HPP
#include <stdint.h>
#include "kern_xelp_gop_stop.hpp"
#include "kern_display13_transcoder.hpp"
namespace Display13ResumePark {
enum class Status : uint32_t {Complete,Topology,Readback,Timeout,PeerChanged};
struct Result {
    Status status {Status::Topology};
    uint32_t stage {},writes {},port {UINT32_MAX},lastReg {},expected {},observed {};
    uint32_t conf {},trans {},buffer {},clock {},transport {},planes {};
    uint32_t ctl2 {},cmtg {};bool cmtgSecondary {};
};
// Firmware (normal boot and the hibernation cold boot alike) runs eDP A as a
// CMTG secondary: TRANS_DDI_FUNC_CTL2 bit3 with CMTG_A_CTL ENABLE|STATE. The
// boot handoff preserves it and the native enable path never rewrites CTL2,
// so clearing CTL2 here leaves CMTG running against a non-secondary
// transcoder. Keep CTL2 for exactly that configuration.
constexpr uint32_t CMTGSecondaryCTL2=8,CMTGRunning=(1U<<31)|(1U<<23);
constexpr uint32_t Enable=0x80000000U;
inline uint32_t planeCtl(unsigned n){return 0x70180+0x100*n;}
inline uint32_t planeSurf(unsigned n){return 0x7019C+0x100*n;}
// Read-only qualification. panelPorts is the board's eDP DDI mask.
template<class IO> bool qualify(IO &io,const Display13Transcoder::Bindings &t,uint32_t panelPorts,Result &r) {
    r.status=Status::Topology;
    if(!t.valid() || t.dsi(0) || t.dsi0Pipe!=UINT32_MAX)return false;
    for(unsigned p=1;p<3;++p) {
        const auto conf=io.read(0x70008+0x1000*p),trans=io.read(0x60400+0x1000*p);
        if(conf==UINT32_MAX || trans==UINT32_MAX || (conf&0xC0000000U) || (trans&Enable))return false;
    }
    r.conf=io.read(0x70008);r.trans=io.read(0x60400);
    if(r.conf==UINT32_MAX || r.trans==UINT32_MAX || (r.conf&0xC0000000U)!=0xC0000000U || !(r.trans&Enable))return false;
    const auto selector=(r.trans>>27)&15,mode=(r.trans>>24)&7;
    if(selector<1 || selector>3 || mode!=2 || ((r.trans>>20)&7))return false;
    r.port=selector-1;
    if(!(panelPorts&(1U<<r.port)))return false;
    r.buffer=io.read(0x64000+0x100*r.port);r.clock=io.read(0x46140);r.transport=io.read(0x60540);
    r.ctl2=io.read(0x60404);r.cmtg=io.read(0x6FA88);
    if(r.ctl2==UINT32_MAX || r.cmtg==UINT32_MAX)return false;
    r.cmtgSecondary=r.port==0 && r.ctl2==CMTGSecondaryCTL2 && r.cmtg==CMTGRunning;
    if(r.ctl2 && !r.cmtgSecondary)return false; // unknown CTL2 use: refuse
    if(r.buffer==UINT32_MAX || !(r.buffer&Enable) || r.clock==UINT32_MAX || (r.clock>>28)!=r.port+1 ||
       r.transport==UINT32_MAX)return false;
    const auto cursor=io.read(0x70080);
    if(cursor==UINT32_MAX || (cursor&63))return false;
    for(unsigned n=0;n<5;++n) {
        const auto ctl=io.read(planeCtl(n));
        if(ctl==UINT32_MAX)return false;
        if(ctl&Enable)r.planes|=1U<<n;
    }
    if(!(r.planes&1) || (r.planes&~1U))return false; // Firmware presents one primary plane.
    for(unsigned d=0;d<3;++d) {
        // Display13FBIRQ::DSBControl(0,d) = DSBInterrupt(0,d)-0x20.
        const auto ctl=io.read(0x70B08+0x100*d);
        if(ctl==UINT32_MAX || (ctl&(Enable|1)))return false;
    }
    return true;
}
template<class IO> Result park(IO &io,const Display13Transcoder::Bindings &t,uint32_t panelPorts) {
    Result r;
    if(!qualify(io,t,panelPorts,r))return r;
    const auto power=io.read(0x45404);
    // Nothing else may change shared power while this transaction runs.
    auto stable=[&]{return io.read(0x45404)==power;};
    auto write=[&](uint32_t reg,uint32_t value){io.write(reg,value);++r.writes;io.read(reg);return true;};
    auto wait=[&](uint32_t reg,uint32_t mask,uint32_t expected) {
        r.lastReg=reg;r.expected=expected;
        return XELPGOP::poll(io,stable,[&](uint32_t v){return (v&mask)==expected;},reg,r.observed,r.status,Status::PeerChanged);
    };
    if(power==UINT32_MAX){r.status=Status::Readback;return r;}
    // 1. Primary plane off, latched by a surface write; drain two real frames.
    r.stage=1;
    const auto frames=io.read(0x70040),ctl=io.read(planeCtl(0)),surface=io.read(planeSurf(0));
    write(planeCtl(0),ctl&~Enable);write(planeSurf(0),surface);
    uint32_t after=0;
    if(!XELPGOP::poll(io,stable,[&](uint32_t v){return uint32_t(v-frames)>=2 && uint32_t(v-frames)<0x10000000U;},
        0x70040,after,r.status,Status::PeerChanged,false))return r;
    if(!wait(planeCtl(0),Enable,0))return r;
    // 2. Pipe/transcoder A off; wait for the state bit.
    r.stage=2;write(0x70008,r.conf&~Enable);
    if(!wait(0x70008,0xC0000000U,0))return r;
    // 3. Detach enabled pipe A scalers (firmware may scale the GOP mode).
    r.stage=3;
    for(unsigned scaler=0;scaler<2;++scaler) {
        const auto base=0x68170+0x100*scaler;const auto sctl=io.read(base+0x10);
        if(sctl==UINT32_MAX){r.status=Status::Readback;return r;}
        if(sctl&Enable) {
            write(base+0x10,0);write(base,0);write(base+4,0);
            if(!wait(base+0x10,UINT32_MAX,0) || !wait(base,UINT32_MAX,0) || !wait(base+4,UINT32_MAX,0))return r;
        }
    }
    // 4. Transcoder function off (DDI select, mode cleared).
    r.stage=4;if(!r.cmtgSecondary)write(0x60404,0);write(0x60400,r.trans&~(Enable|(15U<<27)|(7U<<24)));
    if(!wait(0x60400,Enable,0))return r;
    // 5. DP transport and DDI buffer off, buffer idle.
    r.stage=5;
    if(!XELPGOP::stopBuffer(write,wait,0,0x64000+0x100*r.port,r.buffer,r.transport,true))return r;
    // 6. Transcoder clock select off.
    r.stage=6;write(0x46140,r.clock&0x0FFFFFFFU);
    if(!wait(0x46140,UINT32_MAX,r.clock&0x0FFFFFFFU))return r;
    // 7. Release pipe A DDB allocations (shared DBUF resume rebuilds them).
    r.stage=7;write(0x7027C,0);write(0x7037C,0);write(0x7017C,0);
    if(!wait(0x7027C,UINT32_MAX,0) || !wait(0x7037C,UINT32_MAX,0) || !wait(0x7017C,UINT32_MAX,0))return r;
    r.stage=8;r.status=Status::Complete;return r;
}
}
#endif
