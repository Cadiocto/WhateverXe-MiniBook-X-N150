#!/usr/bin/env python3
"""Pin the Display 13 GGTT scanout-mode restore: it runs in shared resume after
every consumer is stopped (with or without the hibernation park), the park IO
cannot write CHICKEN_MISC_2, and the park reports denied writes."""
from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'WhateverXe/kern_xe.cpp').read_text()
h=(R/'WhateverXe/kern_display13_resume_park.hpp').read_text()
fn=s[s.index('bool XE::restoreDisplay13ResourcesLocked'):]
fn=fn[:fn.index('\nbool XE::',1) if '\nbool XE::' in fn[1:] else len(fn)]
park=fn.index('Display13ResumePark::park(parkIO')
stopped=fn.index('if(!Display13FBIRQ::consumersStopped(io,7,c.transcoders))result.status')
mbus=fn.index('if(cold[5]!=c.dbuf.mbus)result.status=Display13Power::Status::NotReady')
ggtt=fn.index('Display13ResumePark::restoreGGTTMode(')
assert park < stopped < mbus < ggtt, 'GGTT restore must follow the park and the consumers-stopped check'
assert fn.count('restoreGGTTMode(')==1 and 'cold[12]=before;cold[13]=after;' in fn and 'cold[14]' in fn
io=s[s.index('struct Display13FBResumeParkIO'):s.index('struct Display13FBTCIO')]
assert 'bool write(uint32_t r,uint32_t v)' in io and 'return false;' in io and '0x42084' not in io
assert 'Denied' in h and 'if(!io.write(reg,value)) {r.status=Status::Denied' in h
assert 'parkRearmRecord' not in s
print('PASS: GGTT mode restored in shared resume after consumers stop; park IO denies unlisted writes')
