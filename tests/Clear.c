#include <assert.h>
#include <stdio.h>
#include "../IDClearLogic.h"
static void eligibility(void) {
    for (unsigned bits = 0; bits < 512; bits++) {
        bool scope=bits&1, identity=bits&2, known=bits&4, critical=bits&8;
        bool options=bits&16, hidden=bits&32, action=bits&64, interaction=bits&128;
        unsigned long long persistence=(bits&256)?1:0;
        bool expected=scope && identity && known && !critical && options && !hidden && !persistence && action && interaction;
        assert(IDClearEligible(scope,identity,known,critical,options,hidden,persistence,action,interaction)==expected);
    }
    assert(!IDClearEligible(true,true,true,false,true,false,~0ULL,true,true));
}
static void gate(void) {
    IDClearGate g={0};
    assert(!IDClearConfirm(&g,0,true)); // no confirmation without a tap
    assert(IDClearBegin(&g)); assert(!IDClearBegin(&g)); // double tap
    assert(!IDClearConfirm(&g,0,false)); // wrong owner/scope/settings/auth/window
    assert(g.pending && !g.dispatched);
    unsigned long old=g.generation; IDClearCancel(&g);
    assert(!IDClearConfirm(&g,old,true));
    assert(IDClearBegin(&g)); assert(!IDClearConfirm(&g,old,true)); // stale alert
    assert(IDClearConfirm(&g,g.generation,true));
    assert(!IDClearConfirm(&g,g.generation,true)); assert(!IDClearBegin(&g));
    IDClearCancel(&g); assert(g.dispatched); assert(!IDClearBegin(&g)); // cannot double dispatch during verification
    g.dispatched=false; assert(IDClearBegin(&g));
    IDClearCancel(&g); assert(!g.pending);
}
static void snapshot(void) {
    for(unsigned flags=0;flags<16;flags++)
        assert(IDClearSnapshotMember(flags&1,flags&2,flags&4,flags&8)==(flags==15));
    // Immutable tap snapshot IDs 1 and 2. Current arrivals include 3. Only iterate snapshot.
    const int tap[]={1,2}, current[]={2,3}; size_t submitted=0; int chosen=0;
    for(size_t i=0;i<2;i++) {
        bool present=false; for(size_t j=0;j<2;j++) if(tap[i]==current[j]) present=true;
        if(IDClearSnapshotMember(true,true,present,true)) {submitted++;chosen=tap[i];}
    }
    assert(submitted==1 && chosen==2); // not 1 (gone), never 3 (new)
    assert(!IDClearSnapshotMember(false,true,true,true)); // same canonical ID, replacement object
    assert(!IDClearSnapshotMember(true,false,true,true)); // same pointer, changed timestamp/UUID/identity
    assert(!IDClearSnapshotMember(true,true,true,false)); // newly critical/protected
    assert(!IDClearSnapshotMember(true,true,false,true)); // duplicate canonical identity
}
int main(void) { eligibility();gate();snapshot();puts("PASS clear production C eligibility (512 cases), one-shot gate, snapshot membership and arrival/replacement scenarios; not an iOS runtime test");return 0;}
