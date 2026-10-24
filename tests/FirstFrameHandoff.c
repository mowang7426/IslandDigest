#include "../IDFirstFrameHandoff.h"
#include <assert.h>
int main(void) {
 IDFirstFrameHandoffInput v={1,1,1,1,1,1,1,1,1,1,3,2};
 assert(IDFirstFrameHandoffSafe(&v));
 v.modelMatch=0; assert(!IDFirstFrameHandoffSafe(&v)); v.modelMatch=1;
 v.protectedData=0; assert(!IDFirstFrameHandoffSafe(&v)); v.protectedData=1;
 v.cachedCards=0; assert(!IDFirstFrameHandoffSafe(&v)); v.cachedCards=3;
 v.ownerMatch=0; assert(!IDFirstFrameHandoffSafe(&v));
 return 0;
}
