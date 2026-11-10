#include "kern_display13_scanout_readout.hpp"
#include <cassert>
#include <map>
#include <vector>
int main() {
    using namespace Display13ScanoutReadout;
    static_assert(Count==41 && Size==45,"published layout");
    assert(regs[2].reg==0x42084 && !regs[2].pipe);
    assert(regs[Count-1].reg==0x70040 && regs[25].reg==0x70040);
    // Every register is dword aligned inside the 16 MiB BAR.
    for(const auto &r:regs)assert(!(r.reg&3) && r.reg<0x1000000);

    // Powered: every register read once, in order; the delay comes right
    // before the final frame-counter read, which then sees the pipe advance.
    std::map<uint32_t,uint32_t> hw {{0x45404,0x3},{0x42084,0x40000000},{0x7019C,0x1000},{0x70040,100}};
    std::vector<uint32_t> order;unsigned delays=0;uint32_t record[Size] {};
    auto read=[&](uint32_t r){order.push_back(r);auto it=hw.find(r);return it==hw.end()?0:it->second;};
    capture(read,[&](uint32_t us){assert(us==SettleUs);++delays;hw[0x70040]+=2;},7,record);
    assert(record[0]==Version && record[1]==7 && record[2]==1 && record[3]==Count);
    assert(delays==1 && order.size()==Count+1);   // +1: the PW1 probe
    for(unsigned i=0;i<Count;++i)assert(order[i+1]==regs[i].reg);
    assert(record[Header+2]==0x40000000 && record[Header+30]==0x1000);
    assert(record[Header+25]==100 && record[Header+Count-1]==102);

    // PW1 off: only global registers are touched; pipe slots marked unread, no delay.
    order.clear();delays=0;hw[0x45404]=0;
    capture(read,[&](uint32_t){++delays;},8,record);
    assert(record[2]==0 && !delays);
    for(unsigned i=0;i<Count;++i) {
        if(regs[i].pipe)assert(record[Header+i]==Unread);
        else assert(record[Header+i]!=Unread);
    }
    for(auto r:order)for(const auto &g:regs)if(g.reg==r && r!=0x45404)assert(!g.pipe);
    return 0;
}
