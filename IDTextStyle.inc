/* Only notification title/subtitle/body use this label. UIKit still draws text,
 * wrapping, truncation, Dynamic Type and color emoji; no glyph masks/text images.
 * Cache contains COLOR ONLY and cannot retain protected notification strings. */
@interface IDNotificationLabel : UILabel
@property(nonatomic) CGFloat gradientWidth;
@property(nonatomic) CGFloat gradientScale;
@property(nonatomic) NSUInteger styleRevision;
@property(nonatomic, strong) UIColor *originalColor;
@end
static UIImage *IDGradientStrip(CGFloat width, CGFloat scale) {
    static NSCache<NSString *, UIImage *> *cache;
    if (!cache) { cache = [NSCache new]; cache.countLimit = 48; cache.totalCostLimit = 1024 * 1024; }
    width = MIN(2048, MAX(1, ceil(width * scale) / scale));
    NSString *key = [NSString stringWithFormat:@"%@/%.2f/%.1f", IDTextPalette, width, scale];
    UIImage *image = [cache objectForKey:key];
    if (image) return image;
    UIColor *first, *last;
    if ([IDTextPalette isEqualToString:@"pinkOrange"]) {
        first = [UIColor colorWithRed:1 green:0.68 blue:0.85 alpha:1];
        last = [UIColor colorWithRed:1 green:0.85 blue:0.58 alpha:1];
    } else if ([IDTextPalette isEqualToString:@"whiteCyan"]) {
        first = UIColor.whiteColor; last = [UIColor colorWithRed:0.60 green:1 blue:1 alpha:1];
    } else {
        first = [UIColor colorWithRed:0.55 green:1 blue:1 alpha:1];
        last = [UIColor colorWithRed:0.87 green:0.72 blue:1 alpha:1];
    }
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(width, 1), NO, scale);
    CGContextRef context = UIGraphicsGetCurrentContext();
    if (!context) { UIGraphicsEndImageContext(); return nil; }
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGGradientRef gradient = CGGradientCreateWithColors(space, (__bridge CFArrayRef)@[(__bridge id)first.CGColor, (__bridge id)last.CGColor], NULL);
    CGContextDrawLinearGradient(context, gradient, CGPointZero, CGPointMake(width, 0),
        kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
    image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext(); CGGradientRelease(gradient); CGColorSpaceRelease(space);
    if (image) [cache setObject:image forKey:key cost:(NSUInteger)(width * scale * ceil(scale) * 4)];
    return image;
}
@implementation IDNotificationLabel
- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat scale = self.window.screen.scale ?: UIScreen.mainScreen.scale;
    CGFloat width = ceil(self.bounds.size.width * scale) / scale;
    if (width <= 0 || (self.styleRevision == IDOptionsRevision + 1
        && self.gradientWidth == width && self.gradientScale == scale)) return;
    self.styleRevision = IDOptionsRevision + 1;
    self.gradientWidth = width; self.gradientScale = scale;
    UIColor *solid = [IDSolidText isEqualToString:@"white"] ? UIColor.whiteColor
        : [IDSolidText isEqualToString:@"black"] ? UIColor.blackColor : self.originalColor;
    UIImage *strip = IDGradientText ? IDGradientStrip(width, scale) : nil;
    self.textColor = strip ? [UIColor colorWithPatternImage:strip] : solid;
    self.shadowColor = IDTextShadow ? ([IDSolidText isEqualToString:@"black"] && !IDGradientText
        ? [UIColor.whiteColor colorWithAlphaComponent:0.85] : [UIColor.blackColor colorWithAlphaComponent:0.85]) : nil;
    self.shadowOffset = IDTextShadow ? CGSizeMake(0, 1) : CGSizeZero;
}
@end
