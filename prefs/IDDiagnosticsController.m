#import "IDDiagnosticsController.h"
#import <Preferences/PSSpecifier.h>
#import "../IDShared.h"
#import "IDDiagnosticsSnapshot.h"

@interface IDDiagnosticsController ()
@property(nonatomic, copy) NSDictionary *idSnapshot;
@end

@implementation IDDiagnosticsController
- (void)idAppendSection:(NSString *)title text:(NSString *)text items:(NSMutableArray *)items {
    // Short native footer groups wrap and grow with system text sizing; no fixed frames/colors.
    NSArray *lines = [text componentsSeparatedByString:@"\n"];
    for (NSUInteger start = 0; start < lines.count; start += 8) {
        NSArray *part = [lines subarrayWithRange:NSMakeRange(start, MIN((NSUInteger)8, lines.count - start))];
        PSSpecifier *group = [PSSpecifier preferenceSpecifierNamed:start ? @"" : title target:nil set:NULL get:NULL detail:Nil cell:PSGroupCell edit:Nil];
        [group setProperty:[part componentsJoinedByString:@"\n"] forKey:@"footerText"];
        [items addObject:group];
    }
}
- (NSArray *)specifiers {
    if (_specifiers) return _specifiers;
    self.title = @"诊断信息";
    NSDictionary *snapshot = self.idSnapshot ?: @{};
    NSMutableArray *items = [NSMutableArray new];
    [self idAppendSection:@"记录状态" text:IDDiagnosticAvailability(snapshot, ID_VERSION) items:items];
    PSSpecifier *refresh = [PSSpecifier preferenceSpecifierNamed:@"刷新诊断信息" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    refresh.buttonAction = @selector(idRefreshDiagnostics);
    [items addObject:refresh];
    [self idAppendSection:@"折叠诊断摘要" text:IDCollapseSummary(snapshot) items:items];
    [self idAppendSection:@"运行时钩子审计（非真机修复证明）" text:IDRuntimeSummary(snapshot) items:items];
    [self idAppendSection:@"运行状态 / 诊断" text:IDDiagnosticsSummary(snapshot) items:items];
    [self idAppendSection:@"诊断与隐私" text:@"仅在进入本页或点击刷新时读取本地状态快照。刷新不触发通知操作、不重试初始化。仅显示白名单运行指标，不显示通知正文、通知标识或内存地址；不联网、不导出。ABI 支持或审计结果不是实际功能修复证明。" items:items];
    _specifiers = items;
    return _specifiers;
}
- (void)idRefreshDiagnostics {
    // One read per explicit refresh; all sections render the same immutable snapshot.
    self.idSnapshot = IDDiagnosticSnapshot([NSDictionary dictionaryWithContentsOfFile:IDStatusPath], 0);
    _specifiers = nil;
    [self reloadSpecifiers];
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self idRefreshDiagnostics];
}
@end
