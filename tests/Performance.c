#include <assert.h>
#include <stdio.h>
#include "../IDPerformanceLogic.h"
int main(void) {
    for (int a=0;a<2;a++) for(int d=0;d<2;d++) for(int l=0;l<2;l++)
    for(int w=0;w<2;w++) for(int h=0;h<2;h++) {
        int allowed=IDPerformanceWorkAllowed(a,d,l,w,h);
        assert(allowed == (a && !d && l && w && !h));
        assert(!IDPerformanceEnqueue(allowed,1));
        assert(IDPerformanceEnqueue(allowed,0)==allowed);
    }
    int pending=0, callbacks=0;
    for (int i=0;i<10000;i++) if(IDPerformanceEnqueue(1,pending)) {pending=1;callbacks++;}
    assert(callbacks==1);
    pending=0;
    assert(!IDPerformanceEnqueue(IDPerformanceWorkAllowed(0,0,1,1,0),pending));
    assert(IDPerformanceEnqueue(IDPerformanceWorkAllowed(1,0,1,1,0),pending));
    puts("PASS shared production work/coalescing predicates: 32 states, 10000-event burst, teardown/reopen");
    return 0;
}
