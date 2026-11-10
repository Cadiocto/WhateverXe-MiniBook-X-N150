#!/usr/bin/env python3
"""Pin the deferred display IRQ (re)arm: native enableIRQ with no running pipe
(display wake, hibernation resume park) defers instead of faulting; the pipe's
native VBL enable completes it (power-loss restore, else a normal enable)."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'WhateverXe/kern_xe.cpp').read_text()
en=s[s.index('IOReturn XE::wrapDisplay13FBEnableIRQ'):s.index('IOReturn XE::wrapDisplay13FBDisableIRQ')]
# Power-loss restore refused with no running pipe -> deferred, not dropped.
assert 'else if(Display13FBIRQ::deferRearm(io,c.state))c.rearmPending=true;' in en
# Display wake without power loss: defer before enable() can fault.
d=en.index('} else if(!c.quiescing && !c.state.enabled && Display13FBIRQ::deferRearm(io,c.state)) {')
assert d < en.index('} else if(Display13FBTopology::multiOutputWiring') and 'c.rearmPending=true;' in en[d:en.index('} else if(Display13FBTopology::multiOutputWiring')]
vbl=s[s.index('void XE::wrapDisplay13FBConfigureVBL'):s.index('bool XE::wrapDisplay13FBProcessIRQ')]
g0=vbl.index('if(c.rearmPending');gate=vbl[g0:vbl.index('{',vbl.index('Display13Transcoder::running(',g0))]
for need in ['c.rearmPending','enabled','!c.quiescing','!c.state.faulted','(c.powerLost || !c.state.enabled)',
             'Display13FBIRQ::powered(','Display13Transcoder::running(']:
    assert need in gate,need
body=vbl[g0:vbl.index('if(Display13FBTopology::multiOutputWiring(callbackXE->adlnOutputWiring) && enabled && !c.quiescing)',g0)]
assert 'if(c.powerLost)rearm=Display13FBIRQ::restoreAfterPowerLoss(' in body and 'Display13FBIRQ::enable(io,c.state' in body
assert 'if(rearm==Display13FBIRQ::Status::Complete) {c.powerLost=false;c.rearmPending=false;}' in body
# The retry precedes the existing multi-output extension in the same callback.
assert vbl.index('if(c.rearmPending') < vbl.index('extendActivePipes')
# The hibernation park arms the same deferral; nothing else does outside enableIRQ.
park=s[s.index('Display13ResumePark::park(parkIO'):s.index('if(!Display13FBIRQ::consumersStopped(io,7,c.transcoders))result.status')]
assert 'if(park.status==Display13ResumePark::Status::Complete)' in park and 'irq.rearmPending=true' in park
assert s.count('rearmPending=true')==3 and 'parkRearm' not in s
print('PASS: display IRQ (re)arm is deferred while no pipe runs and completed at a running pipe VBL enable')
