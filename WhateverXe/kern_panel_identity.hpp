#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace XELP { namespace PanelIdentity {
enum class Status { Invalid,NotRequested,UnsupportedPointers,Missing,Ambiguous,Exact,DateFree };
struct Result {Status status{Status::Invalid};uint32_t index{UINT32_MAX};bool ignoredUnrelatedTail{};};
inline uint16_t u16(const uint8_t*p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline uint32_t u32(const uint8_t*p){return uint32_t(u16(p))|(uint32_t(u16(p+2))<<16);}
// Pure, immutable CPU core. No runtime/OS object, pointer publication or hardware access.
// Initial qualification covers modern VBTs with missing block41 (Linux generated
// layout: FP38 + DTD18 + PNP10). Explicit pointer-table support is refused.
inline Result select(const void*vb,size_t vn,const void*ed,size_t en){
 Result r;auto v=static_cast<const uint8_t*>(vb),e=static_cast<const uint8_t*>(ed);
 static const uint8_t header[]{0,255,255,255,255,255,255,0};
 if(!v||!e||vn<48||vn>65536||en!=128||memcmp(e,header,8)||e[18]!=1||e[126])return r;
 unsigned sum=0;for(size_t i=0;i<128;++i)sum+=e[i];if((sum&255)||!(e[20]&128))return r;
 const auto m=(uint16_t(e[8])<<8)|e[9];if((m&0x8000)||!((m>>10)&31)||((m>>10)&31)>26||!((m>>5)&31)||((m>>5)&31)>26||!(m&31)||(m&31)>26)return r;
 if(memcmp(v,"$VBT",4))return r;size_t h=u16(v+22),n=u16(v+24),bo=u32(v+28);
 if(h<48||n<h||n>vn||bo<h||bo>n||n-bo<22)return r;
 auto b=v+bo;size_t bh=u16(b+18),bn=u16(b+20);unsigned version=u16(b+16);
 if(memcmp(b,"BIOS_DATA_BLOCK ",16)||bh<22||bn<bh||bn>n-bo||version<216||version>256)return r;
 const uint8_t*options=nullptr,*data=nullptr;size_t on=0,dn=0;bool pointers=false;
 for(size_t at=bh;at<bn;){
  if(bn-at<3){r.ignoredUnrelatedTail=true;break;}size_t count=u16(b+at+1);
  if(b[at]==53&&bn-at>=4&&b[at+3]>=3){if(bn-at<8){r.ignoredUnrelatedTail=true;break;}count=u32(b+at+4);}
  if(count>bn-at-3){r.ignoredUnrelatedTail=true;break;}
  if(b[at]==40){if(options)return {};options=b+at+3;on=count;}
  if(b[at]==41){if(pointers)return {};pointers=true;}
  if(b[at]==42){if(data)return {};data=b+at+3;dn=count;}
  at+=3+count;
 }
 if(!options||!on||!data||dn<16*66)return r;
 if(options[0]!=255){r.status=Status::NotRequested;return r;}
 if(pointers){r.status=Status::UnsupportedPointers;return r;}
 unsigned full=0,datefree=0,fi=0,di=0;uint8_t key[10];memcpy(key,e+8,10);uint8_t noDate[10];memcpy(noDate,key,10);noDate[8]=noDate[9]=0;
 for(unsigned i=0;i<16;++i){const auto p=data+i*66+56;if(!memcmp(p,key,10)){++full;fi=i;}if(!memcmp(p,noDate,10)){++datefree;di=i;}}
 if(full==1){r.status=Status::Exact;r.index=fi;}else if(full>1){r.status=Status::Ambiguous;}else if(datefree==1){r.status=Status::DateFree;r.index=di;}else r.status=datefree?Status::Ambiguous:Status::Missing;
 return r;
}
}

} // namespace XELP
