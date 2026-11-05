"""Compile actual production mailbox and extracted no-op predicate, with mutations."""
from pathlib import Path
import subprocess, tempfile
r=Path(__file__).resolve().parents[1]
t=(r/'Tweak.xm').read_text()
selection=t.split('- (void)selectIdentifier:(NSString *)identifier {',1)[1].split('\n- (void)refresh',1)[0]
guard=selection.split('    if (IDContainedMode &&',1)[1].split(') return;',1)[0]
guard='    if (IDContainedMode &&'+guard+') return YES;\n    return NO;'
source=r'''
#import <Foundation/Foundation.h>
#include <assert.h>
static BOOL IDContainedMode=YES;
static NSUInteger IDModelRevision=4, IDOptionsRevision=7;
static id IDRead(id owner, const char *key) { (void)key; return owner; }
static BOOL IDPanelTransitioning(NSUInteger state) { return state != 0; }
@interface IDFixture : NSObject
@property(nonatomic, strong) id selected, clearMessage, snapshotGroups, snapshotMaster, owner, renderedCards, cards, renderedSelection;
@property NSUInteger cardPage, collapseState, snapshotModelRevision, snapshotOptionsRevision, renderedRevision;
@property BOOL openingPending, inserted, cardsDeferred, enumerationReliable;
@end
@implementation IDFixture
@end
static BOOL IDCanSkip(IDFixture *self, id identifier, BOOL pending) {
 struct { BOOL pending; } _clearGate = {pending};
'''+guard+r'''
}
int main(void) { @autoreleasepool {
 IDFixture *s=[IDFixture new]; s.owner=[NSObject new]; s.snapshotMaster=s.owner;
 s.snapshotGroups=@[]; s.cards=@[]; s.renderedCards=s.cards;
 s.inserted=YES; s.enumerationReliable=YES;
 s.snapshotModelRevision=4; s.snapshotOptionsRevision=7; s.renderedRevision=7;
 assert(IDCanSkip(s,nil,NO)); assert(!IDCanSkip(s,@"QQ",NO)); assert(!IDCanSkip(s,nil,YES));
'''
for prop,value in [('selected','@"QQ"'),('cardPage','1'),('openingPending','YES'),('clearMessage','@"pending"'),('inserted','NO'),('cardsDeferred','YES'),('collapseState','1'),('snapshotGroups','nil'),('enumerationReliable','NO'),('snapshotMaster','nil'),('snapshotModelRevision','5'),('snapshotOptionsRevision','8'),('renderedCards','nil'),('cards','nil'),('renderedSelection','@"QQ"'),('renderedRevision','8')]:
 source+='{ __typeof__(s.'+prop+') old=s.'+prop+'; s.'+prop+'='+value+'; assert(!IDCanSkip(s,nil,NO)); s.'+prop+'=old; }\n'
source+='IDContainedMode=NO; assert(!IDCanSkip(s,nil,NO)); puts("PASS production All-to-All predicate: 19 invalidation cases; app retap/page/privacy/model/options retain work"); } return 0; }\n'
with tempfile.TemporaryDirectory() as d:
 d=Path(d)
 def run(name,code,include,fail=False):
  (d/'IDDeferredSelection.inc').write_text(include)
  p=d/(name+'.m');p.write_text(code)
  exe=d/name
  subprocess.run(['clang','-fobjc-arc','-Wall','-Wextra','-Werror','-framework','Foundation','-I',str(d),str(p),'-o',str(exe)],check=True)
  result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=12)
  assert (result.returncode != 0) == fail, (name,result.returncode,result.stdout,result.stderr)
  print(('KILLED '+name) if fail else result.stdout.strip())
 impl=(r/'IDDeferredSelection.inc').read_text()
 writer=(r/'tests/DeferredSelection.m').read_text()
 run('writer',writer,impl)
 run('predicate',source,impl)
 for name,old,new in [('mutable-input','[value copy]','value'),('lost-latest','if (IDDesiredWriteDirty) IDQueueDesiredAppWrite(IDDesiredWriteValue);','if (NO) IDQueueDesiredAppWrite(IDDesiredWriteValue);'),('unbounded-writes','if (IDDesiredWritePending) return;','if (NO) return;')]:
  assert old in impl
  run(name,writer,impl.replace(old,new),True)
 run('skip-page-reset',source.replace('self.cardPage == 0','YES'),impl,True)
 print('PASS four production mutation kills')
