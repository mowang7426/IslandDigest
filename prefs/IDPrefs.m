#import <Preferences/PSListController.h>
#import <Preferences/PSSpecifier.h>
#import <UIKit/UIKit.h>
#import "../IDShared.h"
#import "../IDTextPalette.h"
#import "IDDiagnosticsController.h"

// Present in Preferences runtime (iOS 15–17), omitted by Theos' minimal header.
@interface PSSpecifier (IDListValues)
- (void)setValues:(NSArray *)values titles:(NSArray *)titles;
@end

@interface IDPinnedAppsController : PSListController
@end
@interface IDPrefsController : PSListController
@property(nonatomic) NSUInteger layoutChangeGeneration;
@property(nonatomic) BOOL layoutChangePending;
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
    NSMutableArray *items = [NSMutableArray arrayWithObject:IDGroupSpecifier(@"应用置顶", @"长按应用胶囊也可置顶或取消，最多 128 个。仅保存应用名称与标识，不读取通知内容、不枚举已安装应用。")];
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
    [item setProperty:@(!([key isEqualToString:@"CollapseOnPresentation"] || [key isEqualToString:@"RememberFilter"] || [key isEqualToString:@"HideNativeList"] || [key isEqualToString:@"AllowPreviewWhenUnlocked"] || [key isEqualToString:@"PanelBlur"] || [key isEqualToString:@"GradientText"] || [key isEqualToString:@"TextShadow"] || [key isEqualToString:@"PerformanceDiagnostics"])) forKey:@"default"];
    return item;
}
- (PSSpecifier *)choice:(NSString *)title key:(NSString *)key values:(NSArray *)values titles:(NSArray *)titles {
    PSSpecifier *item = [PSSpecifier preferenceSpecifierNamed:title target:self set:@selector(setValue:specifier:) get:@selector(valueForSpecifier:) detail:NSClassFromString(@"PSListItemsController") cell:PSLinkListCell edit:Nil];
    [item setProperty:key forKey:@"key"];
    [item setProperty:values.firstObject forKey:@"default"];
    [item setValues:values titles:titles];
    return item;
}
- (PSSpecifier *)slider:(NSString *)title key:(NSString *)key low:(double)low high:(double)high fallback:(double)fallback step:(double)step {
    PSSpecifier *item = [PSSpecifier preferenceSpecifierNamed:title target:self set:@selector(setValue:specifier:) get:@selector(valueForSpecifier:) detail:Nil cell:PSSliderCell edit:Nil];
    [item setProperty:key forKey:@"key"]; [item setProperty:@(low) forKey:@"min"];
    [item setProperty:@(high) forKey:@"max"]; [item setProperty:@(fallback) forKey:@"default"];
    [item setProperty:@(step) forKey:@"step"]; [item setProperty:@YES forKey:@"showValue"];
    [item setProperty:@YES forKey:@"isContinuous"];
    return item;
}
- (void)resetPanelLayout {
    self.layoutChangeGeneration++;
    self.layoutChangePending = NO;
    for (NSString *key in @[@"PanelTopOffset", @"PanelSideInset", @"PanelMaxHeight"]) IDWritePreference(key, nil);
    IDNotifyPreferencesChanged(); _specifiers = nil; [self reloadSpecifiers];
}
- (id)valueForSpecifier:(PSSpecifier *)specifier {
    NSString *key = [specifier propertyForKey:@"key"];
    if ([key hasPrefix:@"Panel"] && ![key isEqualToString:@"PanelBlur"]) return @(IDNumberOption(key, [[specifier propertyForKey:@"default"] doubleValue],
        [[specifier propertyForKey:@"min"] doubleValue], [[specifier propertyForKey:@"max"] doubleValue]));
    if ([key isEqualToString:@"TextPalette"]) return IDStringOption(key, IDTextPaletteValues(), @"cyanPurple");
    if ([key isEqualToString:@"SolidText"]) return IDStringOption(key, @[@"label", @"white", @"black"], @"label");
    if ([key isEqualToString:@"SortMode"]) return IDStringOption(key, @[@"recent", @"count", @"name"], @"recent");
    if ([key isEqualToString:@"GridColumns"]) return IDStringOption(key, @[@"three", @"two", @"auto"], @"three");
    if ([key isEqualToString:@"CategoryStyle"]) return IDStringOption(key, @[@"mini", @"standard"], @"mini");
    if ([key isEqualToString:@"LayoutMode"]) return IDStringOption(key, @[@"compact", @"grid"], @"compact");
    return @(IDPref(key, !([key isEqualToString:@"CollapseOnPresentation"] || [key isEqualToString:@"RememberFilter"] || [key isEqualToString:@"HideNativeList"] || [key isEqualToString:@"AllowPreviewWhenUnlocked"] || [key isEqualToString:@"PanelBlur"] || [key isEqualToString:@"GradientText"] || [key isEqualToString:@"TextShadow"] || [key isEqualToString:@"PerformanceDiagnostics"])));
}
- (void)setValue:(id)value specifier:(PSSpecifier *)specifier {
    NSString *key = [specifier propertyForKey:@"key"];
    if ([key hasPrefix:@"Panel"] && ![key isEqualToString:@"PanelBlur"]) {
        double low = [[specifier propertyForKey:@"min"] doubleValue], high = [[specifier propertyForKey:@"max"] doubleValue];
        double number = [value doubleValue], step = [[specifier propertyForKey:@"step"] doubleValue];
        if (!isfinite(number) || step <= 0) return;
        number = fmin(high, fmax(low, round(number/step)*step));
        if (fabs([[self valueForSpecifier:specifier] doubleValue]-number)<0.5) return;
        IDWritePreference(key, @(number));
        if (!self.layoutChangePending) {
            self.layoutChangePending = YES;
            NSUInteger generation = self.layoutChangeGeneration;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.12*NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                if (generation != self.layoutChangeGeneration) return;
                self.layoutChangePending = NO;
                IDNotifyPreferencesChanged();
            });
        }
        return; // Do not rebuild Settings cells while the slider is being dragged.
    }
    if (!IDWritePreference(key, value)) {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"设置保存失败" message:@"设置未能保存，请重新打开本页核对后重试。" preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
        [self presentViewController:alert animated:YES completion:nil];
        _specifiers = nil; [self reloadSpecifiers];
        return;
    }
    IDNotifyPreferencesChanged();
    _specifiers = nil; [self reloadSpecifiers];
}
- (NSArray *)specifiers {
    if (_specifiers) return _specifiers;
    IDReloadPreferences();
    NSMutableArray *items = [NSMutableArray new];
    [items addObject:[self group:@"灵屿通知 0.3.24 · MoWang" footer:@"通知整理与显示。设置即时保存，无需注销。"]];
    [items addObject:[self toggle:@"启用灵屿通知" key:@"Enabled"]];
    [items addObject:[self toggle:@"插件通知卡片" key:@"ContainedMode"]];
    [items addObject:[self group:@"预览与隐私" footer:@"预览默认关闭。仅在解锁、受保护数据可用且界面可见时读取内容；锁定即清空。可能无法遵循每个 App 的“永不显示预览”，请谨慎开启。内容不联网、不写入日志或偏好。"]];
    [items addObject:[self toggle:@"解锁时显示预览" key:@"AllowPreviewWhenUnlocked"]];
    [items addObject:[self group:@"展开与原生列表" footer:@"先折叠默认关闭，仅用于插件卡片；点击胶囊展开。同次展示的新通知不改变展开状态；不支持的 supplementary 面板保持展开。"]];
    PSSpecifier *collapse = [self toggle:@"每次下拉先折叠" key:@"CollapseOnPresentation"];
    [collapse setProperty:@(IDPref(@"ContainedMode", YES)) forKey:@"enabled"];
    [items addObject:collapse];
    [items addObject:[self group:@"原生列表可见性" footer:@"隐藏默认关闭。始终隐藏为高风险选项：不等待卡片就隐藏，插件故障可能导致空白。恢复时请关闭“隐藏原生列表”，或关闭插件。"]];
    PSSpecifier *hide = [self toggle:@"隐藏原生列表" key:@"HideNativeList"];
    [hide setProperty:@(IDPref(@"ContainedMode", YES)) forKey:@"enabled"];
    [items addObject:hide];
    PSSpecifier *alwaysHide = [self toggle:@"始终隐藏（高风险）" key:@"AlwaysHideNativeList"];
    [alwaysHide setProperty:@(IDPref(@"ContainedMode", YES) && IDPref(@"HideNativeList", NO)) forKey:@"enabled"];
    [items addObject:alwaysHide];
    [items addObject:[self group:@"应用分组与排序" footer:@"置顶应用优先，其余按所选方式排序。记住筛选默认关闭；上次应用无通知时恢复全部。"]];
    [items addObject:[self toggle:@"按应用分组 / 筛选" key:@"Grouping"]];
    [items addObject:[self toggle:@"显示通知数量" key:@"ShowCounts"]];
    [items addObject:[self choice:@"应用排序" key:@"SortMode" values:@[@"recent", @"count", @"name"] titles:@[@"最近通知优先（默认）", @"通知数量从多到少", @"应用名称（本地语言）"]]];
    [items addObject:[self toggle:@"记住上次筛选的应用" key:@"RememberFilter"]];
    [items addObject:[PSSpecifier preferenceSpecifierNamed:@"管理置顶应用" target:self set:NULL get:NULL detail:IDPinnedAppsController.class cell:PSLinkCell edit:Nil]];
    [items addObject:[self group:@"分类布局" footer:@"网格最多显示三行，超出后内部滚动；小屏或较大字体自动减少列数。长按应用胶囊可管理置顶。"]];
    [items addObject:[self choice:@"分类视觉密度" key:@"CategoryStyle" values:@[@"mini", @"standard"] titles:@[@"迷你（默认）", @"标准"]]];
    [items addObject:[self choice:@"显示布局" key:@"LayoutMode" values:@[@"compact", @"grid"] titles:@[@"紧凑横向胶囊（默认）", @"多行网格"]]];
    [items addObject:[self choice:@"网格每行应用数" key:@"GridColumns" values:@[@"three", @"two", @"auto"] titles:@[@"三列（默认）", @"两列", @"自动"]]];
    [items addObject:[self group:@"背景与文字" footer:@"背景 0 为透明、100 为不透明，磨砂独立控制。文字默认跟随系统；复杂壁纸建议增加背景不透明度以保持对比。仅影响插件，不改变隐私规则。"]];
    [items addObject:[self slider:@"背景不透明度" key:@"PanelBackgroundOpacity" low:0 high:100 fallback:0 step:1]];
    [items addObject:[self toggle:@"磨砂背景" key:@"PanelBlur"]];
    [items addObject:[self toggle:@"静态渐变文字" key:@"GradientText"]];
    [items addObject:[self choice:@"渐变配色（双色 / 四色）" key:@"TextPalette" values:IDTextPaletteValues() titles:IDTextPaletteTitles()]];
    [items addObject:[self choice:@"纯色文字" key:@"SolidText" values:@[@"label", @"white", @"black"] titles:@[@"跟随系统（默认）", @"纯白", @"纯黑"]]];
    [items addObject:[self toggle:@"文字轻阴影" key:@"TextShadow"]];
    [items addObject:[self group:@"位置与大小" footer:@"单位：点。顶部偏移默认 220，不保证避开锁屏时钟；遮挡时请增大偏移。高度为上限，内容少时自动缩小。"]];
    [items addObject:[self slider:@"顶部偏移" key:@"PanelTopOffset" low:0 high:420 fallback:220 step:10]];
    [items addObject:[self slider:@"左右边距" key:@"PanelSideInset" low:8 high:64 fallback:8 step:2]];
    [items addObject:[self slider:@"最大高度" key:@"PanelMaxHeight" low:240 high:1000 fallback:360 step:10]];
    PSSpecifier *resetLayout = [PSSpecifier preferenceSpecifierNamed:@"恢复默认位置与大小" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    resetLayout.buttonAction = @selector(resetPanelLayout); [items addObject:resetLayout];
    [items addObject:[self group:@"手动清理" footer:@"垃圾桶按当前应用或全部范围清理，始终确认且不可撤销。仅处理点击时快照；受保护、不可清理或能力未知的通知跳过。不自动清理。"]];
    [items addObject:[self toggle:@"启用手动清理" key:@"EnableClearing"]];
    [items addObject:[self group:@"高级与恢复" footer:@"性能计时默认关闭，仅记录阶段与耗时，不含通知内容。分组异常可重试初始化；不会清除通知。"]];
    [items addObject:[self toggle:@"性能计时" key:@"PerformanceDiagnostics"]];
    PSSpecifier *retry = [PSSpecifier preferenceSpecifierNamed:@"重试初始化 / 恢复分组" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    retry.buttonAction = @selector(retry); [items addObject:retry];
    [items addObject:[self group:@"关于与兼容性" footer:@"MoWang · iOS 15–17。Relaxin 使用 RootHide 包；其他 Rootless 环境使用 Rootless 包。依赖 PreferenceLoader；私有系统接口兼容性需真机验证。"]];
    [items addObject:[PSSpecifier preferenceSpecifierNamed:@"诊断信息" target:self set:NULL get:NULL detail:IDDiagnosticsController.class cell:PSLinkCell edit:Nil]];
    _specifiers = items; return _specifiers;
}
- (void)refreshStatus { _specifiers = nil; [self reloadSpecifiers]; }
- (void)retry {
    CFNotificationCenterPostNotification(CFNotificationCenterGetDarwinNotifyCenter(), CFSTR("com.local.islanddigest/retry"), NULL, NULL, YES);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{ [self refreshStatus]; });
}
- (void)viewWillAppear:(BOOL)animated { [super viewWillAppear:animated]; [self refreshStatus]; }
@end
