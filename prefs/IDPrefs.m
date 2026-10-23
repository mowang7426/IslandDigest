#import <Preferences/PSListController.h>
#import <Preferences/PSSpecifier.h>
#import <UIKit/UIKit.h>
#import "../IDShared.h"
#import "../IDTextPalette.h"

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
    [item setProperty:@(!([key isEqualToString:@"RememberFilter"] || [key isEqualToString:@"HideNativeList"] || [key isEqualToString:@"AllowPreviewWhenUnlocked"] || [key isEqualToString:@"PanelBlur"] || [key isEqualToString:@"GradientText"] || [key isEqualToString:@"TextShadow"] || [key isEqualToString:@"PerformanceDiagnostics"])) forKey:@"default"];
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
    return @(IDPref(key, !([key isEqualToString:@"RememberFilter"] || [key isEqualToString:@"HideNativeList"] || [key isEqualToString:@"AllowPreviewWhenUnlocked"] || [key isEqualToString:@"PanelBlur"] || [key isEqualToString:@"GradientText"] || [key isEqualToString:@"TextShadow"] || [key isEqualToString:@"PerformanceDiagnostics"])));
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
    IDWritePreference(key, value);
    IDNotifyPreferencesChanged();
    _specifiers = nil; [self reloadSpecifiers];
}
- (NSString *)diagnostics {
    NSDictionary *status = [NSDictionary dictionaryWithContentsOfFile:IDStatusPath];
    if (!status) return @"未收到 SpringBoard 诊断。可能尚未加载、未启用插件注入、处于安全模式，或诊断文件不可读；这不是已确认的失败原因。请确认当前越狱的插件注入开启，并确认安装的是原生 RootHide 包。";
    NSMutableArray *lines = [NSMutableArray new];
    NSDate *updated = status[@"updated"];
    if ([updated isKindOfClass:NSDate.class] && -updated.timeIntervalSinceNow > 300) [lines addObject:@"注意：诊断已超过 5 分钟，可能是上次进程的记录；请打开通知中心后刷新。"];
    // Whitelist only: local pin/selection preferences never enter diagnostics.
    for (NSString *key in @[@"version", @"system", @"pid", @"updated", @"stage", @"reason", @"effectiveEnabled", @"effectiveGrouping", @"currentPrefEnabled", @"currentPrefGrouping", @"lastActiveReadCompletion", @"lastActiveReadCancelReason", @"initAttempts", @"hooksInstalled", @"ownerSeen", @"visibleOwnerSeen", @"masterSeen", @"enumerationSupported", @"enumerationComplete", @"enumerationState", @"enumerationReadPath", @"collectionClass", @"collectionCount", @"nativeCountKnown", @"nativeModelCount", @"masterEventHooks", @"enumerationVisits", @"enumerationSlices", @"enumerationCaptured", @"selectedRequests", @"cardPage", @"cardPages", @"ownerCount", @"activeOwnerCount", @"selectedOwnerState", @"sections", @"apps", @"requests", @"filterAvailable", @"presentation", @"clearAvailable", @"clearReason", @"clearResult", @"containedEnabled", @"hideNativeRequested", @"nativeHideSupported", @"nativeHidden", @"nativeHideReason", @"nativeVisibilityHooks", @"nativeCandidateClass", @"nativeCandidateSameWindow", @"nativeCurrentOwner", @"nativeDecisionStage", @"previewPolicyReason", @"cardsRendered", @"cardsRedacted", @"cardsLayoutReady", @"cardsReadinessReason", @"cardsReadinessIndex", @"cardsReadinessType", @"cardsReadinessWidth", @"cardsReadinessHeight", @"cardsContentHeight", @"cardsViewportHeight", @"cardsStackHeight", @"cardsScrollEnabled", @"panelCollapseState", @"retainedExpandedLayoutReady", @"capsuleReachable", @"capsuleHideReady", @"perfCount_snapshot", @"perfMaxMs_snapshot", @"perfCount_render", @"perfMaxMs_render", @"perfCount_refresh", @"perfMaxMs_refresh", @"perfCount_disappear", @"perfMaxMs_disappear"])
        if (status[key]) [lines addObject:[NSString stringWithFormat:@"%@: %@", key, status[key]]];
    // The saved active verdict survives an inactive teardown sample. Whitelist
    // nested scalars too: never display arbitrary diagnostic dictionary content.
    id lastActive = status[@"nativeLastActiveDecision"];
    if ([lastActive isKindOfClass:NSDictionary.class]) {
        [lines addObject:@"nativeLastActiveDecision:"];
        for (NSString *key in @[@"stage", @"candidateClass", @"sameWindow", @"currentOwner", @"nativeHidden", @"cardsRendered", @"cardsLayoutReady", @"viewportWidth", @"viewportHeight", @"contentHeight", @"stackHeight", @"reason"]) {
            id value = lastActive[key];
            if ([value isKindOfClass:NSString.class] || [value isKindOfClass:NSNumber.class])
                [lines addObject:[NSString stringWithFormat:@"  %@: %@", key, value]];
        }
    }
    return [lines componentsJoinedByString:@"\n"];
}
- (NSArray *)specifiers {
    if (_specifiers) return _specifiers;
    IDReloadPreferences();
    NSMutableArray *items = [NSMutableArray new];
    [items addObject:[self group:@"灵屿通知 0.3.10 · MoWang" footer:@"作者：MoWang。插件内通知卡片在满足严格解锁门控时显示原生标题、副标题和正文；不联网、不生成摘要、不写入日志或偏好。锁定或受保护数据不可用时立即清空卡片内容。"]];
    [items addObject:[self toggle:@"启用灵屿通知" key:@"Enabled"]];
    [items addObject:[self toggle:@"按应用分组 / 筛选" key:@"Grouping"]];
    [items addObject:[self toggle:@"显示通知数量" key:@"ShowCounts"]];
    [items addObject:[self toggle:@"解锁时显示通知预览（默认关闭）" key:@"AllowPreviewWhenUnlocked"]];
    [items addObject:[self group:@"插件内通知卡片（实验性）" footer:@"仅当设备已解锁、受保护数据可用、拥有者界面可见且本开关开启时读取原生 title/subtitle/message。iOS 17 私有 contentPreviewSetting 的整型值缺少可核验枚举映射，因此不猜值；默认关闭，只有你明确开启后才允许解锁预览。该开关可能无法保持每个 App 的“永不显示预览”语义，请仅在理解风险时开启；锁屏仍始终隐藏。锁定时不读取正文，锁定通知到达即同步清空已显示卡片。最多 160 条。原生列表默认保留。隐藏开关仅在插件内模式开启时可用，接口不匹配或布局不可用时保留原生。"]];
    [items addObject:[self toggle:@"插件内显示通知卡片" key:@"ContainedMode"]];
    PSSpecifier *hide = [self toggle:@"隐藏原生通知列表（默认关闭）" key:@"HideNativeList"];
    [hide setProperty:@(IDPref(@"ContainedMode", YES)) forKey:@"enabled"];
    [items addObject:hide];
    [items addObject:[self group:@"通透背景" footer:@"默认通透：外层背景 0 且磨砂关闭，不修改壁纸或系统视图。0 = 完全透明，100 = 完全不透明；磨砂可独立开启。卡片保留轻量语义底色以改善文字对比，文字和分类仍会覆盖壁纸。此设置不会开启通知预览或更改隐私门控。"]];
    [items addObject:[self slider:@"面板背景不透明度（0 透明 / 100 不透明）" key:@"PanelBackgroundOpacity" low:0 high:100 fallback:0 step:1]];
    [items addObject:[self toggle:@"磨砂背景（默认关闭）" key:@"PanelBlur"]];
    [items addObject:[self group:@"通知文字颜色" footer:@"仅影响插件卡片的标题、副标题和正文；不修改系统文字、分类胶囊或隐私策略。渐变默认关闭，静态无动画。可选择纯白/纯黑搭配轻阴影；渐变不保证所有壁纸的对比度，复杂壁纸建议同时提高背景不透明度。保留 UIKit 多行、大字和 emoji 绘制；没有缓存通知文字图片。"]];
    [items addObject:[self toggle:@"静态渐变文字（默认关闭）" key:@"GradientText"]];
    [items addObject:[self choice:@"渐变配色（双色 / 四色）" key:@"TextPalette" values:IDTextPaletteValues() titles:IDTextPaletteTitles()]];
    [items addObject:[self choice:@"纯色文字（关闭渐变时）" key:@"SolidText" values:@[@"label", @"white", @"black"] titles:@[@"跟随系统（默认）", @"纯白", @"纯黑"]]];
    [items addObject:[self toggle:@"轻阴影增强对比（默认关闭）" key:@"TextShadow"]];
    [items addObject:[self group:@"面板位置与大小（单位：点）" footer:@"锁屏与通知中心共用。顶部偏移从窗口安全区与通知控制器的交集顶部向下计算；默认 220 点是保守预留，不是已验证的时钟避让，UIKit 安全区本身不含锁屏时钟。不会移动或读取时钟视图。如仍挡住时间，请增加顶部偏移。左右边距控制宽度；高度是上限，卡片少时自动缩小。旋转后按实际窗口重新限界，底部预留 110 点给快捷按钮；极小空间保留原生列表。拖动约每 120ms 合并更新，无需注销。"]];
    [items addObject:[self slider:@"顶部向下偏移（默认 220）" key:@"PanelTopOffset" low:0 high:420 fallback:220 step:10]];
    [items addObject:[self slider:@"左右边距（默认 8）" key:@"PanelSideInset" low:8 high:64 fallback:8 step:2]];
    [items addObject:[self slider:@"面板最大高度（默认 360）" key:@"PanelMaxHeight" low:240 high:1000 fallback:360 step:10]];
    PSSpecifier *resetLayout = [PSSpecifier preferenceSpecifierNamed:@"恢复默认位置与大小" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    resetLayout.buttonAction = @selector(resetPanelLayout); [items addObject:resetLayout];
    [items addObject:[self group:@"手动清理通知" footer:@"点击面板垃圾桶：选中应用只清理该应用；全部则处理全部应用。两种范围始终先确认，系统真实清除不可撤销。仅处理点击时的快照，新到通知不纳入。不自动/定时清理。关键、受保护、不可清理或保护能力未知的通知跳过；接口或解锁状态无法验证时按钮停用，原因显示在面板与诊断中。应用筛选不会扩大清理范围。"]];
    [items addObject:[self toggle:@"启用手动通知清理（始终确认）" key:@"EnableClearing"]];
    [items addObject:[self group:@"面板布局与排序" footer:@"默认保留横向紧凑胶囊；网格默认每行三个应用（不是强制三行）；小屏或较大辅助字体自动降为两列或一列，长名称截断，图标、计数及置顶操作保留。网格最多显示三行，超出后在面板内部滚动。置顶应用总是优先，其余按所选方式排序；相同项使用固定应用标识打破平局。"]];
    [items addObject:[self choice:@"分类视觉密度" key:@"CategoryStyle" values:@[@"mini", @"standard"] titles:@[@"迷你（默认，保留 44 点可访问点击区）", @"标准"]]];
    [items addObject:[self choice:@"显示布局" key:@"LayoutMode" values:@[@"compact", @"grid"] titles:@[@"紧凑横向胶囊（默认）", @"多行网格"]]];
    [items addObject:[self choice:@"网格每行应用数" key:@"GridColumns" values:@[@"three", @"two", @"auto"] titles:@[@"三列（默认，小屏或大字自动降列）", @"两列（必要时降为一列）", @"自动（按宽度与字体选择）"]]];
    [items addObject:[self choice:@"应用排序" key:@"SortMode" values:@[@"recent", @"count", @"name"] titles:@[@"最近通知优先（默认）", @"通知数量从多到少", @"应用名称（本地语言）"]]];
    [items addObject:[self group:@"应用置顶" footer:@"长按应用胶囊可置顶 / 取消，带图钉标记；长按不会同时切换筛选。也可在下面用应用名称管理，无需输入应用标识。"]];
    [items addObject:[PSSpecifier preferenceSpecifierNamed:@"管理置顶应用" target:self set:NULL get:NULL detail:IDPinnedAppsController.class cell:PSLinkCell edit:Nil]];
    [items addObject:[self group:@"每次打开通知中心" footer:@"默认每次打开显示全部。开启后仅在下次真正打开时恢复上次应用；关闭界面时仍先恢复全部原生通知。若该应用已无通知，自动回到全部。关键提醒不会因筛选而隐藏。"]];
    [items addObject:[self toggle:@"记住上次筛选的应用" key:@"RememberFilter"]];
    [items addObject:[self group:@"性能诊断" footer:@"默认关闭。开启后记录单调时钟测量的 snapshot/render/refresh/disappear 次数及最大耗时（毫秒）；只记录阶段和数字，不含通知正文。慢任务超过 32ms 时最多每 5 秒一条日志。打开通知中心后刷新下方状态，可用于排查后台切换卡顿；这些计时不能替代真机帧率跟踪。"]];
    [items addObject:[self toggle:@"性能计时（默认关闭）" key:@"PerformanceDiagnostics"]];
    [items addObject:[self group:@"运行状态 / 诊断" footer:[self diagnostics]]];
    PSSpecifier *refresh = [PSSpecifier preferenceSpecifierNamed:@"刷新运行状态" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    refresh.buttonAction = @selector(refreshStatus); [items addObject:refresh];
    PSSpecifier *retry = [PSSpecifier preferenceSpecifierNamed:@"重试初始化 / 恢复分组" target:self set:NULL get:NULL detail:Nil cell:PSButtonCell edit:Nil];
    retry.buttonAction = @selector(retry); [items addObject:retry];
    [items addObject:[self group:@"兼容性与体验版边界" footer:@"0.1.5 已获用户确认可用；用户已确认 0.1.8 在 iOS17 + Relaxin 可隐藏原生列表，但卡片区域空白；0.1.9 修正布局，尚未完成真实消息预览；新交互仍需你的 iOS 17 + Relaxin 真机验收。私有通知接口可能因系统小版本而异。Relaxin 使用原生 RootHide 包；Rootless 仅供其他 Rootless 环境。设置入口依赖 PreferenceLoader。hooks-installed 不等同于运行验证通过。"]];
    _specifiers = items; return _specifiers;
}
- (void)refreshStatus { _specifiers = nil; [self reloadSpecifiers]; }
- (void)retry {
    CFNotificationCenterPostNotification(CFNotificationCenterGetDarwinNotifyCenter(), CFSTR("com.local.islanddigest/retry"), NULL, NULL, YES);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{ [self refreshStatus]; });
}
- (void)viewWillAppear:(BOOL)animated { [super viewWillAppear:animated]; [self refreshStatus]; }
@end
