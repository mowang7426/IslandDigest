"""Execute production timing helpers with a deterministic Foundation clock."""
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
t=(r/'Tweak.xm').read_text()
helpers=t.split('static double IDPerformanceStart(void)',1)[1].split('#include "IDDeferredStatus.inc"',1)[0]
source=r'''
#import <Foundation/Foundation.h>
#import "prefs/IDDiagnosticsSnapshot.h"
#include <assert.h>
static BOOL IDPerformanceDiagnostics;
static NSMutableDictionary *IDDiagnostics;
static double now;
static unsigned reads;
static double IDMonotonic(void) { reads++; return now; }
'''+ 'static double IDPerformanceStart(void)'+helpers+r'''
int main(void) { @autoreleasepool {
 IDDiagnostics=[NSMutableDictionary new]; now=1;
 assert(IDPerformanceStart()==0 && reads==0);
 IDRecordPerformance(@"scrubCards",0); assert(!IDDiagnostics.count && reads==0);
 IDPerformanceDiagnostics=YES;
 double start=IDPerformanceStart(); now+=0.010;
 IDRecordPerformance(@"scrubCards",start);
 assert([IDDiagnostics[@"perfCount_scrubCards"] unsignedIntegerValue]==1);
 double maximum=[IDDiagnostics[@"perfMaxMs_scrubCards"] doubleValue];
 assert(maximum>9.9 && maximum<10.1);
 start=IDPerformanceStart(); now+=0.001; IDRecordPerformance(@"scrubCards",start);
 assert([IDDiagnostics[@"perfMaxMs_scrubCards"] doubleValue]==maximum);
 IDDiagnostics[@"perfCount_scrubCards"]=@1000000;
 IDRecordPerformance(@"scrubCards",now); assert([IDDiagnostics[@"perfCount_scrubCards"] unsignedIntegerValue]==1000000);
 for (NSString *stage in @[@"cancelSnapshot",@"restoreFilter",@"scrubCards",@"restoreNative",@"dropContent",@"ownerWillDisappearPre",@"ownerWillDisappearOrig",@"ownerWillDisappearPost",@"ownerWillDisappearTotal"]) {
  IDRecordPerformance(stage,now);
  NSString *text=IDDiagnosticsSummary(IDDiagnosticSnapshot(IDDiagnostics,0));
  assert([text containsString:[@"perfCount_" stringByAppendingString:stage]]);
  assert([text containsString:[@"perfMaxMs_" stringByAppendingString:stage]]);
 }
 assert(IDDiagnostics.count==18);
 puts("PASS production performance helpers: disabled clock/read/write zero, bounded counters/maxima, all nine diagnostic-page stages visible");
} }
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'Perf.m';p.write_text(source)
 subprocess.run(['clang','-fobjc-arc','-Wall','-Wextra','-Werror','-framework','Foundation','-I',str(r),str(p),'-o',d+'/test'],check=True)
 subprocess.run([d+'/test'],check=True)
