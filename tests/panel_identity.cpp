#include "kern_vbt.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace XELP;
static void w16(std::vector<uint8_t>&v,size_t p,unsigned n){v[p]=n;v[p+1]=n>>8;}
static void block(std::vector<uint8_t>&v,unsigned id,const std::vector<uint8_t>&data){v.push_back(id);v.push_back(data.size());v.push_back(data.size()>>8);v.insert(v.end(),data.begin(),data.end());}
static void edidChecksum(std::vector<uint8_t>&v){v[127]=0;unsigned sum=0;for(auto n:v)sum+=n;v[127]=uint8_t(-sum);}
int main(){
 std::vector<uint8_t> e(128);const uint8_t h[]={0,255,255,255,255,255,255,0};memcpy(e.data(),h,8);
 e[8]=0x30;e[9]=0xe4;e[10]=0x23;e[11]=1;e[17]=31;e[18]=1;e[19]=4;e[20]=128;edidChecksum(e);
 unsigned cases=0;
 for(unsigned index=0;index<16;++index){
  std::vector<uint8_t> v(70);memcpy(v.data(),"$VBT",4);w16(v,22,48);v[28]=48;
  memcpy(v.data()+48,"BIOS_DATA_BLOCK ",16);w16(v,64,251);w16(v,66,22);
  std::vector<uint8_t> child(44);child[4]=39;child[7]=6;child[8]=0x18;child[21]=10;block(v,2,child);
  block(v,40,{255});block(v,27,std::vector<uint8_t>(212));block(v,44,std::vector<uint8_t>(56));
  std::vector<uint8_t> rows(16*66);memcpy(rows.data()+index*66+56,e.data()+8,10);block(v,42,rows);
  w16(v,24,v.size());w16(v,68,v.size()-48);const auto saved=v;
  auto p=VBT::parse(v.data(),v.size(),Family::RaptorLakeP,e.data(),e.size());
  assert(p.valid()&&p.panel.valid&&p.panel.type==index&&p.panel.ddi==0&&v==saved);++cases;
  auto noHint=VBT::parse(v.data(),v.size(),Family::RaptorLakeP);assert(noHint.valid()&&!noHint.panel.valid);++cases;
  for(size_t n=0;n<v.size();++n){auto r=VBT::parse(v.data(),n,Family::RaptorLakeP,e.data(),e.size());assert(!r.panel.valid);++cases;}
  for(size_t n=0;n<128;++n){auto r=VBT::parse(v.data(),v.size(),Family::RaptorLakeP,e.data(),n);assert(!r.panel.valid);++cases;}
  for(unsigned n=0;n<128;++n){auto bad=e;bad[n]^=1;auto r=VBT::parse(v.data(),v.size(),Family::RaptorLakeP,bad.data(),bad.size());assert(!r.panel.valid);++cases;}
  auto duplicate=v;const size_t rowStart=v.size()-rows.size();memcpy(duplicate.data()+rowStart+((index+1)%16)*66+56,e.data()+8,10);
  p=VBT::parse(duplicate.data(),duplicate.size(),Family::RaptorLakeP,e.data(),e.size());assert(p.valid()&&!p.panel.valid&&p.panelIdentity.status==PanelIdentity::Status::Ambiguous);++cases;
 }
 // A base block whose extension count is non-zero (checksum still valid) is accepted.
 {auto ext=e;ext[126]=1;edidChecksum(ext);
  std::vector<uint8_t> v(70);memcpy(v.data(),"$VBT",4);w16(v,22,48);v[28]=48;
  memcpy(v.data()+48,"BIOS_DATA_BLOCK ",16);w16(v,64,251);w16(v,66,22);
  std::vector<uint8_t> child(44);child[4]=39;child[7]=6;child[8]=0x18;child[21]=10;block(v,2,child);
  block(v,40,{255});block(v,27,std::vector<uint8_t>(212));block(v,44,std::vector<uint8_t>(56));
  std::vector<uint8_t> rows(16*66);memcpy(rows.data()+3*66+56,ext.data()+8,10);block(v,42,rows);
  w16(v,24,v.size());w16(v,68,v.size()-48);
  auto p=VBT::parse(v.data(),v.size(),Family::RaptorLakeP,ext.data(),ext.size());assert(p.valid()&&p.panel.valid&&p.panel.type==3);++cases;
  ext[126]=2;auto q=VBT::parse(v.data(),v.size(),Family::RaptorLakeP,ext.data(),ext.size());assert(!q.panel.valid);++cases;}   // checksum broken
 printf("PASS integrated panel identity: %u synthetic selection/truncation/mutation controls; VBT immutable\n",cases);
}
