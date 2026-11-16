#ifndef KERN_DISPLAY13_SCANOUT_READOUT_HPP
#define KERN_DISPLAY13_SCANOUT_READOUT_HPP
#include <stdint.h>
// Read-only Display 13 pipe A scanout snapshot, published as a registry
// record after primary-plane updates. It answers "running pipe, black panel":
// DPT/GGTT mode (CHICKEN_MISC_2 bit 30), DP transport, PSR, the primary plane,
// scaler, DDI buffer, PLL selection and pipe A interrupt state. Nothing is
// written. Pipe-domain registers are read only while PW1 reports powered.
namespace Display13ScanoutReadout {
struct Reg {uint32_t reg;bool pipe;};
// Order is the published layout: record[Header+i] = value of regs[i].
constexpr Reg regs[] {
    {0x45404,false},  // PWR_WELL_CTL2
    {0x45504,false},  // DC_STATE_EN
    {0x42084,false},  // CHICKEN_MISC_2 (bit 30: GGTT, clear: DPT)
    {0x164280,false}, // DPCLKA_CFGCR0
    {0x46010,false},  // DPLL0_ENABLE
    {0x46014,false},  // DPLL1_ENABLE
    {0x164284,false}, // DPLL0_CFGCR0
    {0x164288,false}, // DPLL0_CFGCR1
    {0x16428C,false}, // DPLL1_CFGCR0
    {0x164290,false}, // DPLL1_CFGCR1
    {0x44400,true},   // DE_PIPE_A_ISR
    {0x44408,true},   // DE_PIPE_A_IIR
    {0x60000,true},   // TRANS_HTOTAL_A
    {0x6000C,true},   // TRANS_VTOTAL_A
    {0x60400,true},   // TRANS_DDI_FUNC_CTL_A
    {0x60404,true},   // TRANS_DDI_FUNC_CTL2_A
    {0x60540,true},   // DP_TP_CTL_A
    {0x60544,true},   // DP_TP_STATUS_A
    {0x60800,true},   // PSR_CTL_A
    {0x60840,true},   // PSR_STATUS_A
    {0x60900,true},   // PSR2_CTL_A
    {0x60940,true},   // PSR2_STATUS_A
    {0x64000,true},   // DDI_BUF_CTL_A
    {0x70008,true},   // TRANSCONF_A
    {0x70030,true},   // PIPEMISC_A
    {0x70040,true},   // PIPE_FRMCOUNT_A (first read)
    {0x70180,true},   // PLANE_CTL_1_A
    {0x70188,true},   // PLANE_STRIDE_1_A
    {0x7018C,true},   // PLANE_POS_1_A
    {0x70190,true},   // PLANE_SIZE_1_A
    {0x7019C,true},   // PLANE_SURF_1_A
    {0x701A4,true},   // PLANE_OFFSET_1_A
    {0x701AC,true},   // PLANE_SURFLIVE_1_A
    {0x701C0,true},   // PLANE_AUX_DIST_1_A
    {0x701CC,true},   // PLANE_COLOR_CTL_1_A
    {0x70240,true},   // PLANE_WM_1_A_0
    {0x7027C,true},   // PLANE_BUF_CFG_1_A
    {0x68170,true},   // PS_WIN_POS_1_A
    {0x68174,true},   // PS_WIN_SZ_1_A
    {0x68180,true},   // PS_CTRL_1_A
    {0x70040,true},   // PIPE_FRMCOUNT_A (second read, after Settle)
};
constexpr unsigned Count=sizeof(regs)/sizeof(regs[0]),Header=4,Size=Header+Count;
constexpr uint32_t Version=1,Unread=0xDEADBEEF,SettleUs=20000;
// record: {version, sequence, flags (bit0 PW1 powered), register count, values...}.
// delay(us) runs once before the final frame-counter read.
template<class Read,class Delay> void capture(Read read,Delay delay,uint32_t sequence,uint32_t (&record)[Size]) {
    record[0]=Version;record[1]=sequence;record[3]=Count;
    const bool pw1=(read(0x45404)&1)!=0;record[2]=pw1 ? 1U : 0U;
    for(unsigned i=0;i<Count;++i) {
        if(regs[i].pipe && !pw1){record[Header+i]=Unread;continue;}
        if(i==Count-1)delay(SettleUs);
        record[Header+i]=read(regs[i].reg);
    }
}
}
#endif
