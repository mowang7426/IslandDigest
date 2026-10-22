#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../IDFirstFrameLogic.h"
#include "../IDContainedLogic.h"
#include "../IDPaletteLogic.h"
int main(void) {
    /* Exercise the production predicate, not a duplicate lifecycle model. */
    for (int bits=0; bits<256; bits++) {
        int a[8]; for(int i=0;i<8;i++) a[i]=(bits>>i)&1;
        int result=IDFirstFrameShouldPrepare(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]);
        assert(result == (bits == 15));
    }
    int prepared=0;
    assert(!IDFirstFrameShouldPrepare(1,1,0,0,0,0,0,prepared)); /* no force load */
    assert(!IDFirstFrameShouldPrepare(1,1,1,0,0,0,0,prepared)); /* willAppear detached */
    if (IDFirstFrameShouldPrepare(1,1,1,1,0,0,0,prepared)) prepared=1;
    assert(prepared && !IDFirstFrameShouldPrepare(1,1,1,1,0,0,0,prepared)); /* didAppear no second rebuild */
    prepared=0; /* endAppearance scrubs strings; next appearance can revalidate */
    assert(IDFirstFrameShouldPrepare(1,1,1,1,0,0,0,prepared));
    for(int bits=0; bits<16; bits++) {
        int compact=bits&1, capsule=(bits>>1)&1, viewport=(bits>>2)&1, cards=(bits>>3)&1;
        int ready=IDFirstFrameRepresentationReady(compact,capsule,viewport,cards);
        assert(ready == (compact ? capsule : (viewport && cards)));
        /* Every native safety input must still veto a first-frame representation. */
        for(int gate=0;gate<8;gate++) {
            int ok[8]={1,1,1,1,1,0,1,1};
            ok[gate]=gate==5 ? 1 : 0;
            assert(!IDContainedShouldHide(ok[0],ok[1],ok[2],ok[3],ok[4],ok[5],ok[6]&&ready,ok[7],1));
        }
        assert(IDContainedShouldHide(1,1,1,1,1,0,ready,1,1)==ready);
        assert(!IDContainedShouldHide(1,1,1,1,1,0,ready,1,0));
    }
    const char *names[]={"cyanPurple","pinkOrange","whiteCyan","cyanBluePurplePink","yellowOrangePinkPurple","whiteCyanBluePurple"};
    for(size_t p=0;p<6;p++) {
        const IDPalette *palette=IDPaletteForIdentifier(names[p]);
        assert(strcmp(palette->identifier,names[p])==0);
        assert(palette->count==(p<3 ? 2u:4u));
        assert(IDPaletteStop(palette,0)==0 && IDPaletteStop(palette,palette->count-1)==1);
        for(size_t i=0;i<palette->count;i++) {
            assert(fabs(IDPaletteStop(palette,i)-(p<3 ? (double)i : (double)i/3))<1e-12);
            IDPaletteColor c=palette->colors[i];
            assert(c.r>=0 && c.r<=1 && c.g>=0 && c.g<=1 && c.b>=0 && c.b<=1);
            for(size_t j=0;j<i;j++) assert(memcmp(&c,&palette->colors[j],sizeof c)!=0);
        }
    }
    assert(IDPaletteForIdentifier(NULL)==&IDPalettes[0]);
    assert(IDPaletteForIdentifier("unknown")==&IDPalettes[0]);
    assert(IDPalettes[0].colors[0].r==.55 && IDPalettes[0].colors[1].b==1);
    puts("PASS production first-frame 256 states, lifecycle gating, expanded/capsule 16 states and safety vetoes; 6 palettes and exact 0,1/3,2/3,1 stops");
}
