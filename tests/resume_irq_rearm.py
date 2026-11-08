#!/usr/bin/env python3
"""Pin the hibernation-park display IRQ re-arm: armed only by a completed park,
retried only at a running pipe's VBL enable, cleared on success."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'WhateverXe/kern_xe.cpp').read_text()
park=s[s.index('Display13ResumePark::park(parkIO'):s.index('if(!Display13FBIRQ::consumersStopped(io,7,c.transcoders))result.status')]
assert 'if(park.status==Display13ResumePark::Status::Complete)' in park and 'irq.parkRearm=true' in park
vbl=s[s.index('void XE::wrapDisplay13FBConfigureVBL'):s.index('bool XE::wrapDisplay13FBProcessIRQ')]
g0=vbl.index('if(c.parkRearm');gate=vbl[g0:vbl.index('Display13FBIRQ::restoreAfterPowerLoss(',g0)]
for need in ['c.parkRearm','c.powerLost','enabled','!c.quiescing','!c.state.faulted','Display13FBIRQ::powered(','Display13Transcoder::running(']:
    assert need in gate,need
assert 'if(rearm==Display13FBIRQ::Status::Complete) {c.powerLost=false;c.parkRearm=false;}' in vbl
# The retry precedes the existing multi-output extension in the same callback.
assert vbl.index('Display13FBIRQ::restoreAfterPowerLoss(') < vbl.index('extendActivePipes')
assert s.count('parkRearm=true')==1
print('PASS: resume park IRQ re-arm is park-gated and retried only at a running pipe VBL enable')
