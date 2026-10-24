#import <Foundation/Foundation.h>
#import "../prefs/IDDiagnosticsSnapshot.h"
#include <assert.h>

static NSString *IDTestRender(id input) {
    NSDictionary *snapshot = IDDiagnosticSnapshot(input, 0);
    return [@[IDDiagnosticAvailability(snapshot, @"0.3.20"), IDCollapseSummary(snapshot), IDRuntimeSummary(snapshot), IDDiagnosticsSummary(snapshot)] componentsJoinedByString:@"\n"];
}
int main(void) { @autoreleasepool {
    assert([IDTestRender(nil) containsString:@"未收到"]);
    assert([IDTestRender(@[]) containsString:@"未收到"]);
    assert([IDTestRender(@{@"version": @"0.3.18"}) containsString:@"旧版本或版本不匹配"]);
    assert([IDTestRender(@{@"version": @"0.3.20"}) containsString:@"更新时间缺失或无效"]);
    assert([IDTestRender(@{@"version": @"0.3.20", @"updated": [NSDate dateWithTimeIntervalSinceNow:-301]}) containsString:@"已过期"]);
    assert([IDTestRender(@{@"version": @"0.3.20", @"updated": [NSDate dateWithTimeIntervalSinceNow:600]}) containsString:@"时间异常"]);
    assert([IDTestRender(@{@"version": @"0.3.20", @"updated": [NSDate date]}) containsString:@"版本匹配"]);
    NSDictionary *sample = @{@"version": @"0.3.20", @"alwaysHideActive": @YES,
        @"collapseDiagnostic": @{@"cycle": @9, @"renderSample": @{@"cycle": @9, @"stage": @"render-fixture"},
            @"primaryBoundary": @{@"primary": @YES, @"fromAppeared": @NO},
            @"primaryEndAppeared": @NO, @"runtimeManager": @"FixtureManager",
            @"hookAudit": @[@{@"installedExpected": @YES, @"targetExpectedNow": @NO}],
            @"raw": @{@"states": @[@0, @1, @2, @3, @4, @5, @6]}}};
    NSString *render = IDTestRender(sample);
    assert([render containsString:@"render-fixture"] && [render containsString:@"主周期 ABI:"]);
    assert([render containsString:@"装是 现否 实未知"]);
    assert([render containsString:@"高风险始终隐藏已激活"]);
    assert(![render containsString:@"(null)"]);
    // Type-corrupted old/new files must not invoke dictionary/array selectors on scalars.
    NSArray *bad = @[@"bad", @42, @[], @{}, [NSNull null], [NSDate date], @[@"bad", @42]];
    NSDictionary *shape = IDDiagnosticSnapshot(sample, 0);
    for (id value in bad) {
        NSMutableDictionary *corrupt = [NSMutableDictionary new];
        for (NSString *key in shape) corrupt[key] = value;
        assert(IDTestRender(corrupt).length > 0);
        corrupt[@"collapseDiagnostic"] = corrupt.copy;
        assert(IDTestRender(corrupt).length > 0);
    }
    NSDictionary *private = @{@"body": @"PRIVATE-CONTENT", @"notificationIdentifier": @"PRIVATE-ID", @"PinnedApps": @[@"PRIVATE-APP"],
        @"reason": @{@"body": @"PRIVATE-CONTENT"}, @"collapseDiagnostic": @{@"runtimeManager": @"0x123456789abc", @"hookAudit": @[@{@"runtimeClass": @{@"body": @"PRIVATE-CONTENT"}}]}};
    NSString *safe = IDTestRender(private);
    assert(![safe containsString:@"PRIVATE-"] && ![safe containsString:@"0x123456789abc"]);
    assert([safe containsString:@"未知"] && ![safe containsString:@"(null)"]);
    assert([IDAuditFlag(@"1") isEqualToString:@"未知"]);
    puts("PASS diagnostic snapshot: missing/old/stale/future, malformed nested types, seven-slot audit, primary cycle, privacy whitelist; not UIKit/device validation");
} return 0; }
