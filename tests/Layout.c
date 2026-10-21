#include "../IDLayoutLogic.h"
#include <assert.h>
#include <stdio.h>
static unsigned tests;
static void bounded(double width, double height, double safeTop, double safeBottom, double offset, double inset, double preferred, double cap) {
    IDPanelRect r = IDLayoutPanel(0,safeTop,width,height-safeBottom,offset,inset,preferred,cap);
    assert(isfinite(r.x)&&isfinite(r.y)&&isfinite(r.width)&&isfinite(r.height));
    assert(r.width>=0 && r.height>=0 && r.height<=cap);
    assert(r.y>=safeTop);
    if(r.width>0) assert(r.x>=0 && r.x+r.width<=width+0.0001);
    if(r.height>0) assert(r.y+r.height<=fmax(safeTop,height-safeBottom-110)+0.0001);
    ++tests;
}
int main(void) {
    IDPanelRect portrait = IDLayoutPanel(0,59,393,818,220,8,360,360);
    assert(portrait.x==8 && portrait.y==279 && portrait.width==377 && portrait.height==360); ++tests;
    IDPanelRect shortContent = IDLayoutPanel(0,59,393,818,220,8,210,360);
    assert(shortContent.height==210); ++tests;
    IDPanelRect landscape = IDLayoutPanel(59,0,785,372,420,64,600,600);
    assert(landscape.y==82 && landscape.height==180); ++tests;
    IDPanelRect small = IDLayoutPanel(0,0,100,80,220,8,360,360);
    assert(small.height==0); ++tests;
    assert(IDLayoutClamp(NAN,8,64)==8); ++tests;
    for(int w=160;w<=1024;w+=72) for(int h=180;h<=1366;h+=73)
      for(int offset=-50;offset<=600;offset+=50)
       for(int inset=8;inset<=64;inset+=8)
        for(int preferred=44;preferred<=620;preferred+=96)
         bounded(w,h,44,34,offset,inset,preferred,360);
    puts("PASS layout: real production geometry, portrait/landscape/tiny windows, dynamic content and bounds");
    printf("%u portable assertions/cases; no UIKit/runtime/clock-frame claim\n",tests);
    return 0;
}
