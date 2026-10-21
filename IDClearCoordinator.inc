// All methods run on the main queue. Never add a broad-section or clear-all fallback.
- (UIViewController *)clearPresenter {
    UIViewController *presenter = self.owner;
    for (NSUInteger i = 0; i < 16 && presenter.parentViewController; i++) presenter = presenter.parentViewController;
    if (!presenter.viewIfLoaded.window || presenter.presentedViewController || presenter.isBeingDismissed) return nil;
    return presenter;
}
- (BOOL)clearAuthenticated {
    // Fail closed if CoverSheet authentication cannot be established; no lockscreen bypass.
    UIViewController *node = self.owner;
    Class cover = objc_getClass("CSCoverSheetViewController");
    for (NSUInteger i = 0; node && i < 16; i++, node = node.parentViewController) {
        BOOL authenticated = NO;
        if (cover && [node isKindOfClass:cover]) return IDClearBool(node, "isAuthenticated", &authenticated) && authenticated;
    }
    return NO;
}
- (NSArray *)clearRequests {
    id master = IDRead(self.owner, "masterList");
    id all = IDRead(master, "allNotificationRequests");
    if (IDClearCollection(all)) return [IDItems(all) copy];
    id sections = IDRead(master, "_notificationSectionListsForEnumeration");
    if (!sections) sections = IDRead(master, "sectionLists");
    if (!IDClearCollection(sections) && ![sections isKindOfClass:NSDictionary.class]) return nil;
    NSMutableArray *result = [NSMutableArray new];
    NSHashTable *seen = [NSHashTable hashTableWithOptions:NSPointerFunctionsObjectPointerPersonality];
    for (id section in IDItems(sections)) {
        id values = IDRead(section, "allNotificationRequests");
        if (!IDClearCollection(values)) return nil;
        for (id request in IDItems(values)) if (![seen containsObject:request]) { [seen addObject:request]; [result addObject:request]; }
    }
    return result.copy;
}
- (BOOL)clearEligible:(id)request scope:(NSString *)scope delegate:(id)delegate {
    NSArray *key = IDClearKey(request);
    BOOL critical = YES, hidden = YES;
    BOOL criticalKnown = IDClearBool(request, "isCriticalAlert", &critical);
    id options = IDRead(request, "options");
    BOOL optionsKnown = IDClearBool(options, "hideClearActionInList", &hidden)
        && IDMethod(object_getClass(options), "lockScreenPersistence", "Q", 2);
    unsigned long long persistence = optionsKnown ? ((unsigned long long (*)(id, SEL))objc_msgSend)(options, sel_registerName("lockScreenPersistence")) : 1;
    BOOL interaction = IDClearPairMethod(delegate, IDInteractionSelector, "B")
        && ((BOOL (*)(id, SEL, id, id))objc_msgSend)(delegate, sel_registerName(IDInteractionSelector), self.owner, request);
    return IDClearEligible(!scope || [key.firstObject isEqual:scope], key != nil, criticalKnown, critical,
        optionsKnown, hidden, persistence, IDRead(request, "clearAction") != nil, interaction);
}
- (NSString *)clearUnavailableReason {
    if (!IDEnabled || !IDGrouping || !IDPref(@"EnableClearing", YES)) return @"清理功能已在设置关闭";
    if (!self.active || self.disabled || !self.owner.viewIfLoaded.window) return @"通知界面不可用";
    if (![self clearAuthenticated]) return @"请先解锁；无法验证解锁状态时不允许清理";
    id delegate = IDRead(self.owner, "delegate");
    if (!IDClearPairMethod(delegate, IDClearSelector, "v") || !IDClearPairMethod(delegate, IDInteractionSelector, "B"))
        return @"当前系统缺少已验证的精确请求清理接口";
    NSArray *requests = [self clearRequests];
    if (!requests) return @"无法可靠枚举当前通知，已停用清理";
    BOOL eligible = NO;
    for (id request in requests) if ([self clearEligible:request scope:self.selected delegate:delegate]) { eligible = YES; break; }
    if (!eligible) return @"当前范围无可安全清理项；关键、受保护或保护能力未知的通知保留";
    return nil;
}
- (void)updateClearButton {
    if (!self.panel.isViewLoaded) return;
    IDDigestView *view = (IDDigestView *)self.panel.view;
    NSString *reason = nil;
    @try { reason = [self clearUnavailableReason]; }
    @catch (NSException *exception) { reason = @"通知接口检查失败；已停用清理"; }
    BOOL busy = _clearGate.pending || _clearGate.dispatched;
    view.clearButton.enabled = !reason && !busy;
    view.clearButton.accessibilityHint = reason ?: @"按当前应用或全部建立快照；始终先确认，再清除系统通知";
    IDDiagnostics[@"clearAvailable"] = @(!reason);
    IDDiagnostics[@"clearReason"] = reason ?: @"精确清理接口可用；尚未代表真机清理已验证";
    if (reason && !self.clearMessage) view.hint.text = reason;
    if (self.clearMessage) view.hint.text = self.clearMessage;
}
- (void)cancelClear {
    IDClearCancel(&_clearGate);
    self.clearSnapshot = nil; self.clearScope = nil; self.clearDelegate = nil;
    self.clearPresenterOwner = nil; self.clearWindow = nil; self.clearMaster = nil;
    UIAlertController *alert = self.clearAlert; self.clearAlert = nil;
    if (alert.presentingViewController) [alert dismissViewControllerAnimated:NO completion:nil];
    [self updateClearButton];
}
- (void)clearFeedback:(NSString *)message {
    self.clearMessage = message;
    if (self.panel.isViewLoaded) ((IDDigestView *)self.panel.view).hint.text = message;
    UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, message);
}
- (void)beginClear {
    if (![NSThread isMainThread] || _clearGate.pending || _clearGate.dispatched) return;
    @try {
        NSString *reason = [self clearUnavailableReason];
        if (reason) { [self clearFeedback:reason]; [self updateClearButton]; return; }
        UIViewController *presenter = [self clearPresenter];
        if (!presenter) { [self clearFeedback:@"请关闭其他弹窗后重试"]; return; }
        id delegate = IDRead(self.owner, "delegate");
        NSArray *requests = [self clearRequests];
        NSMutableArray *tickets = [NSMutableArray new]; NSMutableSet *keys = [NSMutableSet new];
        for (id request in requests) if ([self clearEligible:request scope:self.selected delegate:delegate]) {
            NSArray *key = IDClearKey(request);
            // Ambiguous canonical identities are not safe: abort, rather than delete a replacement.
            if ([keys containsObject:key]) { [self clearFeedback:@"通知身份重复，未执行清理"]; return; }
            [keys addObject:key]; [tickets addObject:@{@"request": request, @"key": key}];
        }
        if (!tickets.count) { [self clearFeedback:@"没有可安全清理的通知（关键、受保护或未知状态已保留）"]; return; }
        if (!IDClearBegin(&_clearGate)) return;
        self.clearSnapshot = tickets.copy; self.clearScope = self.selected; self.clearDelegate = delegate;
        self.clearPresenterOwner = presenter; self.clearWindow = self.owner.view.window;
        self.clearMaster = IDRead(self.owner, "masterList");
        self.clearRevision = IDOptionsRevision;
        unsigned long generation = _clearGate.generation;
        NSString *name = @"全部应用";
        for (IDGroup *group in self.groups) if ([group.identifier isEqual:self.selected]) name = group.name;
        NSString *message = [NSString stringWithFormat:@"清除%@的 %lu 条可清理通知？\n这是系统真实清除，无法撤销。保留可检测的关键及受保护通知；之后新到通知不在本次范围内。", name, (unsigned long)tickets.count];
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"确认清理通知" message:message preferredStyle:UIAlertControllerStyleAlert];
        self.clearAlert = alert;
        __weak IDCoordinator *weakSelf = self;
        [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) { [weakSelf cancelClear]; }]];
        [alert addAction:[UIAlertAction actionWithTitle:@"清除通知" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action) { [weakSelf confirmClear:generation]; }]];
        [self updateClearButton];
        [presenter presentViewController:alert animated:YES completion:nil];
        if (!alert.presentingViewController) [self cancelClear];
    } @catch (NSException *exception) { [self cancelClear]; [self clearFeedback:@"清理检查失败，未执行清理"]; }
}
- (void)observeClear:(NSArray *)sent generation:(unsigned long)generation attempt:(NSUInteger)attempt {
    NSArray *current = nil;
    NSUInteger remaining = 0;
    @try {
        current = [self clearRequests];
        if (current) for (NSDictionary *ticket in sent) {
            for (id request in current) if ([IDClearKey(request) isEqual:ticket[@"key"]]) { remaining++; break; }
        }
    } @catch (NSException *exception) { current = nil; remaining = 0; }
    // At most three read-only observations; never repeat the destructive call.
    if (current && remaining && attempt < 2 && generation == _clearGate.generation && self.active) {
        __weak IDCoordinator *weakSelf = self;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC * (attempt ? 1.5 : 0.6))), dispatch_get_main_queue(), ^{
            [weakSelf observeClear:sent generation:generation attempt:attempt + 1];
        });
        return;
    }
    _clearGate.dispatched = false;
    if (generation == _clearGate.generation && self.active) {
        if (!current) [self clearFeedback:@"清理请求已提交；无法核实结果，请查看系统通知列表"];
        else [self clearFeedback:[NSString stringWithFormat:@"本次 %lu 条已不在列表，%lu 条仍在；新通知未纳入", (unsigned long)(sent.count - remaining), (unsigned long)remaining]];
        IDDiagnostics[@"clearResult"] = current ? @"snapshot-observed (not causal proof)" : @"verification-unavailable";
        [self refresh];
    }
    [self updateClearButton];
}
- (void)confirmClear:(unsigned long)generation {
    BOOL submitted = NO;
    @try {
        id delegate = IDRead(self.owner, "delegate");
        BOOL scopeMatches = (!self.clearScope && !self.selected) || [self.clearScope isEqual:self.selected];
        UIViewController *presenter = self.clearPresenterOwner;
        BOOL presentation = presenter && self.clearAlert && self.clearWindow
            && presenter.viewIfLoaded.window == self.clearWindow && self.owner.viewIfLoaded.window == self.clearWindow
            && (!presenter.presentedViewController || presenter.presentedViewController == self.clearAlert);
        BOOL context = ![self clearUnavailableReason] && self.clearRevision == IDOptionsRevision
            && scopeMatches && self.clearDelegate == delegate && presentation
            && self.clearMaster == IDRead(self.owner, "masterList");
        if (!context || generation != _clearGate.generation || !_clearGate.pending) { [self cancelClear]; return; }
        NSArray *current = [self clearRequests];
        if (!current) { [self cancelClear]; [self clearFeedback:@"通知列表已变化，未执行清理"]; return; }
        NSMutableArray *send = [NSMutableArray new], *sent = [NSMutableArray new];
        for (NSDictionary *ticket in self.clearSnapshot) {
            id original = ticket[@"request"]; NSUInteger matchingKeys = 0; BOOL present = NO;
            for (id request in current) {
                if ([IDClearKey(request) isEqual:ticket[@"key"]]) matchingKeys++;
                if (request == original) present = YES;
            }
            if (IDClearSnapshotMember(present, [IDClearKey(original) isEqual:ticket[@"key"]], matchingKeys == 1,
                [self clearEligible:original scope:self.clearScope delegate:delegate])) {
                [send addObject:original]; [sent addObject:ticket];
            }
        }
        if (!send.count) { [self cancelClear]; [self clearFeedback:@"快照通知已消失或受保护，未提交清理"]; return; }
        context = self.active && !self.disabled && self.clearRevision == IDOptionsRevision
            && self.clearDelegate == IDRead(self.owner, "delegate") && self.clearMaster == IDRead(self.owner, "masterList")
            && self.owner.viewIfLoaded.window == self.clearWindow && [self clearAuthenticated]
            && ((!self.clearScope && !self.selected) || [self.clearScope isEqual:self.selected]);
        if (!IDClearConfirm(&_clearGate, generation, context)) { [self cancelClear]; return; }
        // The ONLY destructive dispatch. No UI/capability callbacks between gate and dispatch.
        submitted = YES;
        ((void (*)(id, SEL, id, id))objc_msgSend)(delegate, sel_registerName(IDClearSelector), self.owner, send.copy);
        self.clearAlert = nil; self.clearSnapshot = nil; self.clearScope = nil; self.clearDelegate = nil;
        self.clearPresenterOwner = nil; self.clearWindow = nil; self.clearMaster = nil;
        [self clearFeedback:@"清理请求已提交；正在核实原快照…"];
        [self updateClearButton];
        NSArray *sentSnapshot = sent.copy;
        __weak IDCoordinator *weakSelf = self;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            [weakSelf observeClear:sentSnapshot generation:generation attempt:0];
        });
    } @catch (NSException *exception) {
        _clearGate.dispatched = false; [self cancelClear];
        [self clearFeedback:submitted ? @"清理调用异常，结果不确定；请检查系统列表（不会自动重试）" : @"确认状态检查失败，未执行清理"];
    }
}
