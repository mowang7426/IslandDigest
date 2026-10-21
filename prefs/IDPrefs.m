#import <Preferences/PSListController.h>
#import <Preferences/PSSpecifier.h>
#import <UIKit/UIKit.h>
#import "../IDShared.h"

// Present in Preferences runtime (iOS 15–17), omitted by Theos' minimal header.
@interface PSSpecifier (IDListValues)
- (void)setValues:(NSArray *)values titles:(NSArray *)titles;
@end

@interface IDPinnedAppsController : PSListController
@end
@interface IDPrefsController : PSListController
@end

static PSSpecifier *IDGroupSpecifier(NSString *title, NSString *footer) {
    PSSpecifier *item = [PSSpecifier preferenceSpecifierNamed:title target:nil set:NULL get:NULL detail:Nil cell:PSGroupCell edit:Nil];
    [item setProperty:footer forKey:@"footerText"];
    return item;
}
@implementation IDPinnedAppsController
- (NSArray *)specifiers {
    if (_specifiers) return _specifiers;
    IDReloadPreferences();
    self.title = @"管理置顶应用";
    NSMutableArray *items = [NSMutableArray arrayWithObject:IDGroupSpecifier(@"本机通知中出现的应用", @"长按通知分组中的应用胶囊也可置顶或取消。仅保存已置顶和当前有通知的应用名称，不读取通知内容；列表不枚举已安装应用。无通知的置顶应用保留在这里，有新通知时自动靠前。最多置顶 128 个应用。")];
    NSArray *pins = IDPinnedIdentifiers();
    id savedNames = IDPreferenceValue(@"AppNames");
    NSDictionary *names = [savedNames isKindOfClass:NSDictionary.class] ? savedNames : @{};
    NSMutableSet *identifiers = [NSMutableSet setWithArray:pins];
    id observed = IDPreferenceValue(@"ObservedApps");
    if ([observed isKindOfClass:NSArray.class]) for (id identifier in observed) {
        if ([identifier isKindOfClass:NSString.class] && [identifier length]) [identifiers addObject:identifier];
        if (identifiers.count >= 256) break;
    }
    NSArray *sorted = [identifiers.allObjects sortedArrayUsingComparator:^NSComparisonResult(NSString *a, NSString *b) {
        BOOL pa = [pins containsObject:a], pb = [pins containsObject:b];
        if (pa != pb) return pa ? NSOrderedAscending : NSOrderedDescending;
        NSString *an = [names[a] isKindOfClass:NSString.class] ? names[a] : @"未命名应用";
        NSString *bn = [names[b] isKindOfClass:NSString.class] ? names[b] : @"未命名应用";
        NSComparisonResult order = [an localizedStandardCompare:bn];
        return order == NSOrderedSame ? [a compare:b] : order;
    }];
    for (NSString *identifier in sorted) {
        NSString *name = [names[identifier] isKindOfClass:NSString.class] ? names[identifier] : @"未命名应用（可取消置顶）";
        PSSpecifier *item = [PSSpecifier preferenceSpecifierNamed:name target:self set:@selector(setPinned:specifier:) get:@selector(isPinned:) detail:Nil cell:PSSwitchCell edit:Nil];
        [item setProperty:identifier forKey:@"appIdentifier"];
        [items addObject:item];
    }
    if (!sorted.count) [items addObject:IDGroupSpecifier(@"暂无应用", @"先打开通知中心，收到通知后即可在这里管理，也可直接长按应用胶囊。")];
    PSSpecifier *reset = [PSSpecifier preferenceSpecifierNamed:@"取消全部置顶" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    reset.buttonAction = @selector(resetPins);
    [items addObject:reset];
    _specifiers = items;
    return _specifiers;
}
- (id)isPinned:(PSSpecifier *)specifier { return @([IDPinnedIdentifiers() containsObject:[specifier propertyForKey:@"appIdentifier"]]); }
- (void)setPinned:(id)value specifier:(PSSpecifier *)specifier {
    IDReloadPreferences();
    NSMutableArray *pins = IDPinnedIdentifiers().mutableCopy;
    NSString *identifier = [specifier propertyForKey:@"appIdentifier"];
    if ([value boolValue] && pins.count < 128 && ![pins containsObject:identifier]) [pins addObject:identifier];
    else if (![value boolValue]) [pins removeObject:identifier];
    IDWritePreference(@"PinnedApps", pins);
    IDNotifyPreferencesChanged();
    _specifiers = nil; [self reloadSpecifiers];
}
- (void)resetPins {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"取消全部置顶？" message:@"不会删除或清除任何通知。" preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"保留" style:UIAlertActionStyleCancel handler:nil]];
    [alert addAction:[UIAlertAction actionWithTitle:@"取消全部置顶" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action) {
        IDWritePreference(@"PinnedApps", @[]); IDNotifyPreferencesChanged();
        self->_specifiers = nil; [self reloadSpecifiers];
    }]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)viewWillAppear:(BOOL)animated { [super viewWillAppear:animated]; _specifiers = nil; [self reloadSpecifiers]; }
@end

@implementation IDPrefsController
- (PSSpecifier *)group:(NSString *)title footer:(NSString *)footer { return IDGroupSpecifier(title, footer); }
- (PSSpecifier *)toggle:(NSString *)title key:(NSString *)key {
    PSSpecifier *item = [PSSpecifier preferenceSpecifierNamed:title target:self set:@selector(setValue:specifier:) get:@selector(valueForSpecifier:) detail:Nil cell:PSSwitchCell edit:Nil];
    [item setProperty:key forKey:@"key"];
    [item setProperty:@(![key isEqualToString:@"RememberFilter"]) forKey:@"default"];
    return item;
}
- (PSSpecifier *)choice:(NSString *)title key:(NSString *)key values:(NSArray *)values titles:(NSArray *)titles {
    PSSpecifier *item = [PSSpecifier preferenceSpecifierNamed:title target:self set:@selector(setValue:specifier:) get:@selector(valueForSpecifier:) detail:NSClassFromString(@"PSListItemsController") cell:PSLinkListCell edit:Nil];
    [item setProperty:key forKey:@"key"];
    [item setProperty:values.firstObject forKey:@"default"];
    [item setValues:values titles:titles];
    return item;
}
- (id)valueForSpecifier:(PSSpecifier *)specifier {
    NSString *key = [specifier propertyForKey:@"key"];
    if ([key isEqualToString:@"SortMode"]) return IDStringOption(key, @[@"recent", @"count", @"name"], @"recent");
    if ([key isEqualToString:@"LayoutMode"]) return IDStringOption(key, @[@"compact", @"grid"], @"compact");
    return @(IDPref(key, ![key isEqualToString:@"RememberFilter"]));
}
- (void)setValue:(id)value specifier:(PSSpecifier *)specifier {
    IDWritePreference([specifier propertyForKey:@"key"], value);
    IDNotifyPreferencesChanged();
}
- (NSString *)diagnostics {
    NSDictionary *status = [NSDictionary dictionaryWithContentsOfFile:IDStatusPath];
    if (!status) return @"未收到 SpringBoard 诊断。可能尚未加载、未启用插件注入、处于安全模式，或诊断文件不可读；这不是已确认的失败原因。请确认 Relaxin 的插件注入开启，并确认安装的是原生 RootHide 包。";
    NSMutableArray *lines = [NSMutableArray new];
    NSDate *updated = status[@"updated"];
    if ([updated isKindOfClass:NSDate.class] && -updated.timeIntervalSinceNow > 300) [lines addObject:@"注意：诊断已超过 5 分钟，可能是上次进程的记录；请打开通知中心后刷新。"];
    // Whitelist only: local pin/selection preferences never enter diagnostics.
    for (NSString *key in @[@"version", @"system", @"pid", @"updated", @"stage", @"reason", @"initAttempts", @"hooksInstalled", @"ownerSeen", @"visibleOwnerSeen", @"masterSeen", @"enumerationSupported", @"sections", @"apps", @"requests", @"filterAvailable", @"presentation"])
        if (status[key]) [lines addObject:[NSString stringWithFormat:@"%@: %@", key, status[key]]];
    return [lines componentsJoinedByString:@"\n"];
}
- (NSArray *)specifiers {
    if (_specifiers) return _specifiers;
    IDReloadPreferences();
    NSMutableArray *items = [NSMutableArray new];
    [items addObject:[self group:@"灵屿通知 0.1.6 体验版" footer:@"只改变应用筛选面板，不重排系统通知卡片。保留回复、展开和清除；不生成 AI 摘要、不联网、不自动清除。设置实时生效，无需自动注销。"]];
    [items addObject:[self toggle:@"启用灵屿通知" key:@"Enabled"]];
    [items addObject:[self toggle:@"按应用分组 / 筛选" key:@"Grouping"]];
    [items addObject:[self toggle:@"显示通知数量" key:@"ShowCounts"]];
    [items addObject:[self group:@"面板布局与排序" footer:@"默认保留横向紧凑胶囊；网格最多显示三行，超出后在面板内部滚动。置顶应用总是优先，其余按所选方式排序；相同项使用固定应用标识打破平局。"]];
    [items addObject:[self choice:@"显示布局" key:@"LayoutMode" values:@[@"compact", @"grid"] titles:@[@"紧凑横向胶囊（默认）", @"多行网格"]]];
    [items addObject:[self choice:@"应用排序" key:@"SortMode" values:@[@"recent", @"count", @"name"] titles:@[@"最近通知优先（默认）", @"通知数量从多到少", @"应用名称（本地语言）"]]];
    [items addObject:[self group:@"应用置顶" footer:@"长按应用胶囊可置顶 / 取消，带图钉标记；长按不会同时切换筛选。也可在下面用应用名称管理，无需输入应用标识。"]];
    [items addObject:[PSSpecifier preferenceSpecifierNamed:@"管理置顶应用" target:self set:NULL get:NULL detail:IDPinnedAppsController.class cell:PSLinkCell edit:Nil]];
    [items addObject:[self group:@"每次打开通知中心" footer:@"默认每次打开显示全部。开启后仅在下次真正打开时恢复上次应用；关闭界面时仍先恢复全部原生通知。若该应用已无通知，自动回到全部。关键提醒不会因筛选而隐藏。"]];
    [items addObject:[self toggle:@"记住上次筛选的应用" key:@"RememberFilter"]];
    [items addObject:[self group:@"运行状态 / 诊断" footer:[self diagnostics]]];
    PSSpecifier *refresh = [PSSpecifier preferenceSpecifierNamed:@"刷新运行状态" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    refresh.buttonAction = @selector(refreshStatus); [items addObject:refresh];
    PSSpecifier *retry = [PSSpecifier preferenceSpecifierNamed:@"重试初始化 / 恢复分组" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    retry.buttonAction = @selector(retry); [items addObject:retry];
    [items addObject:[self group:@"兼容性与体验版边界" footer:@"0.1.5 已获用户确认可用；0.1.6 新交互仍需你的 iOS 17 + Relaxin 真机验收。私有通知接口可能因系统小版本而异。Relaxin 使用原生 RootHide 包；Rootless 仅供其他 Rootless 环境。设置入口依赖 PreferenceLoader。hooks-installed 不等同于运行验证通过。"]];
    _specifiers = items; return _specifiers;
}
- (void)refreshStatus { _specifiers = nil; [self reloadSpecifiers]; }
- (void)retry {
    CFNotificationCenterPostNotification(CFNotificationCenterGetDarwinNotifyCenter(), CFSTR("com.local.islanddigest/retry"), NULL, NULL, YES);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{ [self refreshStatus]; });
}
- (void)viewWillAppear:(BOOL)animated { [super viewWillAppear:animated]; [self refreshStatus]; }
@end
