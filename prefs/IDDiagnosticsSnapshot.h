#import <Foundation/Foundation.h>

// A bounded, type-checked display snapshot. No preference/app identifiers or notification content.
static NSString *IDDiagnosticScalar(id value) {
    if ([value isKindOfClass:NSNumber.class]) return [value stringValue];
    if (![value isKindOfClass:NSString.class] || [value length] > 256) return @"未知";
    if ([value rangeOfString:@"0x" options:NSCaseInsensitiveSearch].location != NSNotFound) return @"已隐藏地址";
    return value;
}
static NSString *IDAuditFlag(id value) {
    return [value isKindOfClass:NSNumber.class] ? ([value boolValue] ? @"是" : @"否") : @"未知";
}
static NSString *IDAuditText(id value) { return IDDiagnosticScalar(value); }
static NSDictionary *IDDiagnosticSnapshot(id input, NSUInteger depth) {
    if (![input isKindOfClass:NSDictionary.class] || depth > 3) return @{};
    NSMutableDictionary *result = [NSMutableDictionary new];
    NSArray *scalarKeys = @[@"alwaysHideActive", @"acceptEvent", @"accepted", @"active", @"activeOwnerCount", @"alpha", @"apps", @"beforeDefinition", @"beginAppearance", @"candidateClass", @"capsuleExists", @"capsuleHideReady", @"capsuleReachable", @"cardPage", @"cardPages", @"cardsContentHeight", @"cardsLayoutReady", @"cardsReadinessHeight", @"cardsReadinessIndex", @"cardsReadinessReason", @"cardsReadinessType", @"cardsReadinessWidth", @"cardsRedacted", @"cardsRendered", @"cardsScrollEnabled", @"cardsStackHeight", @"cardsViewportHeight", @"class", @"clearAvailable", @"clearCompact", @"clearCycle", @"clearEvent", @"clearLive", @"clearReason", @"clearResult", @"clearSource", @"collapseAppliedOwners", @"collapseCycle", @"collapseCycleCompact", @"collapseCycleLive", @"collapseEndHook", @"collapseExplicitManagerHook", @"collapseExplicitSliderHook", @"collapseGestureCount", @"collapseHelperHook", @"collapseIndirectHook", @"collapseInputHook", @"collapseLastEnd", @"collapseLastReason", @"collapseScreenHook", @"collapseSignal", @"collectionClass", @"collectionCount", @"compact", @"constraintHeight", @"consumed", @"containedEnabled", @"contentHeight", @"currentOwner", @"currentPrefCollapse", @"currentPrefEnabled", @"currentPrefGrouping", @"cycle", @"directApplied", @"directApply", @"directOwners", @"disabled", @"effectiveCollapse", @"effectiveContained", @"effectiveEnabled", @"effectiveGrouping", @"entry", @"enumerationCaptured", @"enumerationComplete", @"enumerationReadPath", @"enumerationSlices", @"enumerationState", @"enumerationSupported", @"enumerationVisits", @"event", @"filterAvailable", @"fromAppeared", @"height", @"hidden", @"hideNativeRequested", @"hooksInstalled", @"initAttempts", @"inserted", @"installedExpected", @"lastAccept", @"lastActiveReadCancelReason", @"lastActiveReadCompletion", @"lastReject", @"live", @"loaded", @"lockKnown", @"main", @"masterEventHooks", @"masterSeen", @"nativeCandidateClass", @"nativeCandidateSameWindow", @"nativeCountKnown", @"nativeCurrentOwner", @"nativeDecisionStage", @"nativeHidden", @"nativeHideReason", @"nativeHideSupported", @"nativeModelCount", @"nativeVisibilityHooks", @"occupied", @"offMain", @"overlay", @"owner", @"ownerCount", @"ownerSeen", @"panelAlpha", @"panelCollapseState", @"panelHeight", @"panelHidden", @"panelWindow", @"perfCount_disappear", @"perfCount_refresh", @"perfCount_render", @"perfCount_snapshot", @"perfMaxMs_disappear", @"perfMaxMs_refresh", @"perfMaxMs_render", @"perfMaxMs_snapshot", @"pid", @"post", @"postEvent", @"pre", @"presentation", @"presentationGestureSupported", @"presenting", @"previewPolicyReason", @"primary", @"primaryEndAppeared", @"primaryLifecycleHook", @"reason", @"recognizer", @"rejectEvent", @"rejected", @"render", @"requests", @"retainedExpandedLayoutReady", @"runtimeClass", @"runtimeDefinition", @"runtimeExpectedNow", @"runtimeManager", @"runtimeSlider", @"sameWindow", @"sections", @"selectedOwnerState", @"selectedRequests", @"source", @"stackHeight", @"stage", @"state", @"system", @"targetExpectedNow", @"total", @"uiLocked", @"updated", @"userGesture", @"version", @"viewportHeight", @"viewportWidth", @"visibleOwnerSeen", @"window"];
    for (NSString *key in scalarKeys) {
        id value = input[key];
        result[key] = [value isKindOfClass:NSNumber.class] ? value : IDDiagnosticScalar(value);
    }
    if ([input[@"updated"] isKindOfClass:NSDate.class]) result[@"updated"] = input[@"updated"];
    for (NSString *key in @[@"beforeExitSample", @"beginAppearanceSample", @"collapseDiagnostic", @"directApplySample", @"helper", @"indirect", @"lastConsumed", @"nativeLastActiveDecision", @"primaryBoundary", @"raw", @"renderSample", @"transitionSample"]) result[key] = IDDiagnosticSnapshot(input[key], depth + 1);
    for (NSString *key in @[@"states", @"postStates", @"ownerAncestors", @"hookAudit"]) {
        id source = input[key]; NSMutableArray *values = [NSMutableArray new];
        if ([source isKindOfClass:NSArray.class]) for (id value in source) {
            if (values.count >= 16) break;
            if ([key isEqualToString:@"hookAudit"]) [values addObject:IDDiagnosticSnapshot(value, depth + 1)];
            else if ([key isEqualToString:@"ownerAncestors"]) [values addObject:IDDiagnosticScalar(value)];
            else [values addObject:[value isKindOfClass:NSNumber.class] ? value : @"未知"];
        }
        result[key] = values;
    }
    return [input count] ? result : @{};
}
static NSString *IDDiagnosticFreshness(NSDictionary *s) {
    id updated = s[@"updated"];
    if (![updated isKindOfClass:NSDate.class]) return @"更新时间缺失或无效，时效未知";
    NSTimeInterval age = -[updated timeIntervalSinceNow];
    if (age < 0) return @"时间异常（未来记录），时效未知";
    return age <= 300 ? @"5 分钟内的记录（不代表当前仍有效）" : @"已过期：超过 5 分钟，可能属于上次进程";
}
static NSString *IDDiagnosticAvailability(NSDictionary *s, NSString *version) {
    if (!s.count) return @"未收到 SpringBoard 诊断：文件缺失、不可读或格式错误；不能确认失败原因。";
    NSString *match = [s[@"version"] isEqual:version] ? @"版本匹配" : @"旧版本或版本不匹配：以下仅为历史记录，不代表当前运行状态";
    return [NSString stringWithFormat:@"%@\n记录版本：%@ / 设置版本：%@\n%@\n缺失/无效字段显示未知；未知不等于否。", match, IDDiagnosticScalar(s[@"version"]), version, IDDiagnosticFreshness(s)];
}

static NSString *IDCollapseSummary(NSDictionary *s) {
    NSDictionary *d = s[@"collapseDiagnostic"];
    if (!d.count) return @"暂无折叠诊断：缺失或旧版本状态，不能据此判断成功或失败。";
    NSDictionary *r = d[@"raw"], *h = d[@"helper"], *i = d[@"indirect"];
    NSArray *states = r[@"states"];
    NSDictionary *v = @{};
    for (NSString *key in @[@"lastConsumed", @"renderSample", @"beginAppearanceSample", @"directApplySample"]) {
        if ([d[key] count]) { v = d[key]; break; }
    }
    NSString *fresh = IDDiagnosticFreshness(s);
    return [NSString stringWithFormat:
        @"版本 %@ / iOS %@ / PID %@（%@）\n钩子 主%@ 辅%@ 间%@\n开关 有效%@ 保存%@ 内置%@\n主入口 %@ 次，前%@→后%@，主线程%@\n状态 P/B/C/E/X/F：%@\nhelper %@ / indirect %@ / 事件#%@\n接受%@ 拒绝%@ / 周期%@ / 消费%@\n直用%@ 出现%@ 渲染%@ / 应用拥有者%@\n最后接受：%@ (#%@)\n最后拒绝：%@ (#%@)\n消费/检查：C%@ %@ / %@ / owner%@\n胶囊 hidden%@ α%@ window%@ 高%@\n面板高%@ / 约束高%@\n清理：%@（此前 live%@ compact%@）",
        IDDiagnosticScalar(s[@"version"]), IDDiagnosticScalar(s[@"system"]), IDDiagnosticScalar(s[@"pid"]), fresh,
        [IDDiagnosticScalar(s[@"collapseInputHook"]) isEqual:@"supported"] ? @"✓" : IDDiagnosticScalar(s[@"collapseInputHook"]) ?: @"?",
        [IDDiagnosticScalar(s[@"collapseHelperHook"]) isEqual:@"supported"] ? @"✓" : IDDiagnosticScalar(s[@"collapseHelperHook"]) ?: @"?",
        [IDDiagnosticScalar(s[@"collapseIndirectHook"]) isEqual:@"supported"] ? @"✓" : IDDiagnosticScalar(s[@"collapseIndirectHook"]) ?: @"?",
        IDDiagnosticScalar(s[@"effectiveCollapse"]), IDDiagnosticScalar(s[@"currentPrefCollapse"]), IDDiagnosticScalar(s[@"effectiveContained"]),
        IDDiagnosticScalar(r[@"total"]), IDDiagnosticScalar(r[@"pre"]), IDDiagnosticScalar(r[@"post"]), IDDiagnosticScalar(r[@"main"]),
        states.count == 7 ? [[states subarrayWithRange:NSMakeRange(0, 6)] componentsJoinedByString:@"/"] : @"—",
        IDDiagnosticScalar(h[@"total"]), IDDiagnosticScalar(i[@"total"]), IDDiagnosticScalar(d[@"event"]), IDDiagnosticScalar(d[@"accepted"]), IDDiagnosticScalar(d[@"rejected"]), IDDiagnosticScalar(d[@"cycle"]), IDDiagnosticScalar(d[@"consumed"]),
        IDDiagnosticScalar(d[@"directApply"]), IDDiagnosticScalar(d[@"beginAppearance"]), IDDiagnosticScalar(d[@"render"]), IDDiagnosticScalar(d[@"directApplied"]) ?: @"未知",
        IDDiagnosticScalar(d[@"lastAccept"]), IDDiagnosticScalar(d[@"acceptEvent"]) ?: @"未知", IDDiagnosticScalar(d[@"lastReject"]), IDDiagnosticScalar(d[@"rejectEvent"]) ?: @"未知",
        IDDiagnosticScalar(v[@"cycle"]) ?: @"未知", IDDiagnosticScalar(v[@"stage"]) ?: @"未消费", IDDiagnosticScalar(v[@"reason"]) ?: IDDiagnosticScalar(d[@"directApplySample"][@"reason"]) ?: @"—", IDDiagnosticScalar(v[@"owner"]) ?: @"未知",
        IDDiagnosticScalar(v[@"hidden"]) ?: @"—", IDDiagnosticScalar(v[@"alpha"]) ?: @"—", IDDiagnosticScalar(v[@"window"]) ?: @"—", IDDiagnosticScalar(v[@"height"]) ?: @"—",
        IDDiagnosticScalar(v[@"panelHeight"]) ?: @"—", IDDiagnosticScalar(v[@"constraintHeight"]) ?: @"—",
        IDDiagnosticScalar(d[@"clearReason"]), IDDiagnosticScalar(d[@"clearLive"]) ?: @"未知", IDDiagnosticScalar(d[@"clearCompact"]) ?: @"未知"];
}
static NSString *IDRuntimeSummary(NSDictionary *s) {
    NSDictionary *d = s[@"collapseDiagnostic"];
    if (!d.count) return @"运行时未知：尚无诊断。";
    NSMutableArray *lines = [NSMutableArray arrayWithObjects:
        [NSString stringWithFormat:@"runtimeManager：%@", IDAuditText(d[@"runtimeManager"])],
        [NSString stringWithFormat:@"runtimeSlider：%@", IDAuditText(d[@"runtimeSlider"])], nil];
    NSArray *ancestors = d[@"ownerAncestors"];
    if ([ancestors isKindOfClass:NSArray.class] && ancestors.count) {
        for (id name in ancestors) if ([name isKindOfClass:NSString.class])
            [lines addObject:[@"ownerAncestors：" stringByAppendingString:name]];
    } else [lines addObject:@"ownerAncestors：未知"];
    [lines addObject:@"hookAudit：支持仅指ABI；装=安装时IMP，现=目标当前IMP，实=实例当前IMP。未知不等于否。"];
    NSArray *names = @[@"主手势", @"辅手势", @"间接手势", @"结束", @"熄屏", @"manager展示", @"slider展示"];
    NSArray *keys = @[@"collapseInputHook", @"collapseHelperHook", @"collapseIndirectHook", @"collapseEndHook", @"collapseScreenHook", @"collapseExplicitManagerHook", @"collapseExplicitSliderHook"];
    NSArray *rows = d[@"hookAudit"];
    for (NSUInteger n = 0; n < 7; n++) {
        NSDictionary *row = [rows isKindOfClass:NSArray.class] && n < rows.count ? rows[n] : @{};
        [lines addObject:[NSString stringWithFormat:@"%@ 支持:%@ 装%@ 现%@ 实%@", names[n], IDAuditText(s[keys[n]]), IDAuditFlag(row[@"installedExpected"]), IDAuditFlag(row[@"targetExpectedNow"]), IDAuditFlag(row[@"runtimeExpectedNow"])]];
        [lines addObject:[NSString stringWithFormat:@"  定义 %@ → %@ / 类 %@", IDAuditText(row[@"beforeDefinition"]), IDAuditText(row[@"runtimeDefinition"]), IDAuditText(row[@"runtimeClass"])]];
    }
    NSDictionary *boundary = d[@"primaryBoundary"];
    [lines addObject:[NSString stringWithFormat:@"主周期 ABI:%@ 从已显示%@ 主实例%@ 锁已知%@ 锁定%@ 占用%@\n%@ / 结束已显示%@", IDAuditText(s[@"primaryLifecycleHook"]), IDAuditFlag(boundary[@"fromAppeared"]), IDAuditFlag(boundary[@"primary"]), IDAuditFlag(boundary[@"lockKnown"]), IDAuditFlag(boundary[@"uiLocked"]), IDAuditFlag(boundary[@"occupied"]), IDAuditText(boundary[@"reason"]), IDAuditFlag(d[@"primaryEndAppeared"])]];
    NSDictionary *t = d[@"transitionSample"];
    [lines addObject:[NSString stringWithFormat:@"transitionSample：%@\n展示%@ 用户%@ 主slider%@ 锁已知%@ 锁定%@\n结果：%@", IDAuditText(t[@"entry"]), IDAuditFlag(t[@"presenting"]), IDAuditFlag(t[@"userGesture"]), IDAuditFlag(t[@"primary"]), IDAuditFlag(t[@"lockKnown"]), IDAuditFlag(t[@"uiLocked"]), IDAuditText(t[@"reason"])]];
    return [lines componentsJoinedByString:@"\n"];
}
static NSString *IDDiagnosticsSummary(NSDictionary *status) {
    if (!status.count) return @"未收到 SpringBoard 诊断。可能尚未加载、未启用插件注入、处于安全模式，或诊断文件不可读；这不是已确认的失败原因。请确认插件注入开启，并核对所装包适合当前越狱方案。";
    NSMutableArray *lines = [NSMutableArray new];
    NSDate *updated = status[@"updated"];
    if ([updated isKindOfClass:NSDate.class] && -updated.timeIntervalSinceNow > 300) [lines addObject:@"注意：诊断已超过 5 分钟，可能是上次进程的记录；请打开通知中心后刷新。"];
    if ([status[ @"alwaysHideActive"] boolValue]) [lines addObject:@"高风险始终隐藏已激活：若面板空白，关闭始终隐藏与隐藏原生列表后重新打开通知中心恢复。"];
    // Whitelist only: local pin/selection preferences never enter diagnostics.
    for (NSString *key in @[@"version", @"system", @"pid", @"updated", @"stage", @"reason", @"effectiveEnabled", @"effectiveGrouping", @"currentPrefEnabled", @"currentPrefGrouping", @"lastActiveReadCompletion", @"lastActiveReadCancelReason", @"initAttempts", @"hooksInstalled", @"ownerSeen", @"visibleOwnerSeen", @"masterSeen", @"enumerationSupported", @"enumerationComplete", @"enumerationState", @"enumerationReadPath", @"collectionClass", @"collectionCount", @"nativeCountKnown", @"nativeModelCount", @"presentationGestureSupported", @"collapseInputHook", @"collapseHelperHook", @"collapseIndirectHook", @"collapseEndHook", @"collapseScreenHook", @"collapseSignal", @"collapseGestureCount", @"effectiveCollapse", @"currentPrefCollapse", @"effectiveContained", @"collapseCycle", @"collapseCycleLive", @"collapseCycleCompact", @"collapseLastReason", @"collapseAppliedOwners", @"collapseLastEnd", @"masterEventHooks", @"enumerationVisits", @"enumerationSlices", @"enumerationCaptured", @"selectedRequests", @"cardPage", @"cardPages", @"ownerCount", @"activeOwnerCount", @"selectedOwnerState", @"sections", @"apps", @"requests", @"filterAvailable", @"presentation", @"clearAvailable", @"clearReason", @"clearResult", @"containedEnabled", @"hideNativeRequested", @"nativeHideSupported", @"nativeHidden", @"nativeHideReason", @"nativeVisibilityHooks", @"nativeCandidateClass", @"nativeCandidateSameWindow", @"nativeCurrentOwner", @"nativeDecisionStage", @"previewPolicyReason", @"cardsRendered", @"cardsRedacted", @"cardsLayoutReady", @"cardsReadinessReason", @"cardsReadinessIndex", @"cardsReadinessType", @"cardsReadinessWidth", @"cardsReadinessHeight", @"cardsContentHeight", @"cardsViewportHeight", @"cardsStackHeight", @"cardsScrollEnabled", @"panelCollapseState", @"retainedExpandedLayoutReady", @"capsuleReachable", @"capsuleHideReady", @"perfCount_snapshot", @"perfMaxMs_snapshot", @"perfCount_render", @"perfMaxMs_render", @"perfCount_refresh", @"perfMaxMs_refresh", @"perfCount_disappear", @"perfMaxMs_disappear"])
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
    NSDictionary *collapse = status[@"collapseDiagnostic"];
    if ([collapse isKindOfClass:NSDictionary.class]) {
        for (NSString *key in @[@"event", @"cycle", @"source", @"accepted", @"rejected", @"acceptEvent", @"rejectEvent", @"lastAccept", @"lastReject", @"clearReason", @"clearEvent", @"clearCycle", @"clearLive", @"clearCompact", @"clearSource", @"directOwners", @"directApplied", @"directApply", @"beginAppearance", @"render", @"consumed"])
            if (collapse[key]) [lines addObject:[NSString stringWithFormat:@"collapse.%@: %@", key, collapse[key]]];
        for (NSString *section in @[@"raw", @"helper", @"indirect", @"directApplySample", @"beginAppearanceSample", @"renderSample", @"lastConsumed", @"beforeExitSample"]) {
            NSDictionary *sample = collapse[section];
            if (![sample isKindOfClass:NSDictionary.class]) continue;
            for (NSString *key in @[@"total", @"pre", @"post", @"event", @"postEvent", @"main", @"offMain", @"recognizer", @"class", @"stage", @"reason", @"consumed", @"cycle", @"source", @"owner", @"live", @"compact", @"state", @"active", @"disabled", @"overlay", @"inserted", @"loaded", @"capsuleExists", @"hidden", @"alpha", @"window", @"height", @"panelHidden", @"panelAlpha", @"panelWindow", @"panelHeight", @"constraintHeight"]) {
                id value = sample[key];
                if ([value isKindOfClass:NSString.class] || [value isKindOfClass:NSNumber.class])
                    [lines addObject:[NSString stringWithFormat:@"collapse.%@.%@: %@", section, key, value]];
            }
            for (NSString *key in @[@"states", @"postStates"]) {
                NSArray *values = sample[key];
                if (![values isKindOfClass:NSArray.class] || values.count != 7) continue;
                BOOL safe = YES; for (id value in values) if (![value isKindOfClass:NSNumber.class]) safe = NO;
                if (safe) [lines addObject:[NSString stringWithFormat:@"collapse.%@.%@ (P/B/C/E/X/F/unknown): %@", section, key, [values componentsJoinedByString:@"/"]]];
            }
        }
    }
    return [lines componentsJoinedByString:@"\n"];
}
