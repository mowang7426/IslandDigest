#include <assert.h>
#include <stdio.h>
#include "../IDScrollLogic.h"
int main(void) {
    assert(IDScrollTapAllowed(1,0,0,0,0,1200,200));
    assert(IDScrollTapAllowed(1,0,0,600,600,1200,200));
    assert(IDScrollTapAllowed(1,0,0,1000,1000,1200,200));
    assert(IDScrollTapAllowed(1,0,0,0,0,80,200));
    assert(!IDScrollTapAllowed(0,0,0,0,0,1200,200));
    assert(!IDScrollTapAllowed(1,1,0,0,0,1200,200));
    assert(!IDScrollTapAllowed(1,0,1,0,0,1200,200));
    assert(!IDScrollTapAllowed(1,0,0,0,-1,1200,200));
    assert(!IDScrollTapAllowed(1,0,0,1001,1001,1200,200));
    assert(!IDScrollTapAllowed(1,0,0,200,300,1200,200));
    assert(!IDScrollTapAllowed(1,0,0,NAN,0,1200,200));
    assert(!IDScrollTapAllowed(1,0,0,0,INFINITY,1200,200));
    assert(!IDScrollTapAllowed(1,0,0,0,0,NAN,200));
    assert(!IDScrollTapAllowed(1,0,0,0,0,1200,0));
    assert(!IDScrollTapAllowed(1,0,0,0,0,-1,200));
    puts("PASS shared production tap gate: stationary/middle/bottom/short, pan/deceleration/overscroll/offset/invalid rejection (not UIKit gesture tests)");
}
