#import <Preferences/PSListController.h>
#import <Preferences/PSSpecifier.h>
#import <UIKit/UIKit.h>
#import "../IDShared.h"

@interface IDPrefsController : PSListController
@end
@implementation IDPrefsController
- (PSSpecifier *)group:(NSString *)title footer:(NSString *)footer {
    PSSpecifier *group = [PSSpecifier preferenceSpecifierNamed:title target:self set:NULL get:NULL detail:Nil cell:PSGroupCell edit:Nil];
    [group setProperty:footer forKey:@"footerText"];
    return group;
}
- (PSSpecifier *)toggle:(NSString *)title key:(NSString *)key {
    PSSpecifier *item = [PSSpecifier preferenceSpecifierNamed:title target:self
        set:@selector(setValue:specifier:) get:@selector(valueForSpecifier:)
        detail:Nil cell:PSSwitchCell edit:Nil];
    [item setProperty:key forKey:@"key"];
    return item;
}
- (id)valueForSpecifier:(PSSpecifier *)specifier {
    return @(IDPref([specifier propertyForKey:@"key"], YES));
}
- (void)setValue:(id)value specifier:(PSSpecifier *)specifier {
    NSString *key = [specifier propertyForKey:@"key"];
    CFPreferencesSetValue((__bridge CFStringRef)key, (__bridge CFPropertyListRef)value,
                         IDPrefsDomain, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    CFPreferencesSynchronize(IDPrefsDomain, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    CFNotificationCenterPostNotification(CFNotificationCenterGetDarwinNotifyCenter(),
        CFSTR("com.local.islanddigest/changed"), NULL, NULL, YES);
}
- (NSString *)diagnostics {
    NSDictionary *status = [NSDictionary dictionaryWithContentsOfFile:IDStatusPath];
    if (!status) return @"未收到 SpringBoard 诊断。可能尚未加载、未启用插件注入、处于安全模式，或诊断文件不可读；这不是已确认的失败原因。请确认 Relaxin 的插件注入开启，并确认安装的是原生 RootHide 包。";
    NSMutableArray *lines = [NSMutableArray new];
    NSDate *updated = status[@"updated"];
    if ([updated isKindOfClass:NSDate.class] && -updated.timeIntervalSinceNow > 300)
        [lines addObject:@"注意：诊断已超过 5 分钟，可能是上次进程的记录；请打开通知中心后刷新。"];
    // Whitelist diagnostic keys: never include notification text, titles or bundle IDs.
    for (NSString *key in @[@"version", @"system", @"pid", @"updated", @"stage", @"reason", @"initAttempts", @"hooksInstalled", @"ownerSeen", @"visibleOwnerSeen", @"masterSeen", @"enumerationSupported", @"sections", @"apps", @"requests", @"filterAvailable", @"presentation"])
        if (status[key]) [lines addObject:[NSString stringWithFormat:@"%@: %@", key, status[key]]];
    return [lines componentsJoinedByString:@"\n"];
}
- (NSArray *)specifiers {
    if (_specifiers) return _specifiers;
    IDReloadPreferences();
    NSMutableArray *items = [NSMutableArray new];
    [items addObject:[self group:@"灵屿通知 0.1.5" footer:@"按应用列出通知与数量；点选应用筛选原生通知。保留系统回复、展开和清除，不生成 AI 摘要。开关实时生效，无需自动注销。"]];
    [items addObject:[self toggle:@"启用灵屿通知" key:@"Enabled"]];
    [items addObject:[self toggle:@"按应用分组 / 筛选" key:@"Grouping"]];
    [items addObject:[self toggle:@"显示通知数量" key:@"ShowCounts"]];
    [items addObject:[self group:@"运行状态 / 诊断" footer:[self diagnostics]]];
    PSSpecifier *refresh = [PSSpecifier preferenceSpecifierNamed:@"刷新运行状态" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    refresh.buttonAction = @selector(refreshStatus);
    [items addObject:refresh];
    PSSpecifier *retry = [PSSpecifier preferenceSpecifierNamed:@"重试初始化 / 恢复分组" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    retry.buttonAction = @selector(retry);
    [items addObject:retry];
    [items addObject:[self group:@"兼容性" footer:@"已编译 iOS 15–17，尚未在你的 iOS 17 真机确认私有通知接口。Relaxin 官方说明采用 RootHide 架构；Rootless 包仅供其他 Rootless 环境。设置入口依赖 PreferenceLoader。状态 hooks-installed 仅代表已注册钩子，不代表面板已显示。"]];
    _specifiers = items;
    return _specifiers;
}
- (void)refreshStatus { _specifiers = nil; [self reloadSpecifiers]; }
- (void)retry {
    CFNotificationCenterPostNotification(CFNotificationCenterGetDarwinNotifyCenter(),
        CFSTR("com.local.islanddigest/retry"), NULL, NULL, YES);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{ [self refreshStatus]; });
}
- (void)viewWillAppear:(BOOL)animated { [super viewWillAppear:animated]; [self refreshStatus]; }
@end
