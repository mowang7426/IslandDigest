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
    const IDTextPaletteSpec *palette = IDTextPaletteForIdentifier(IDTextPalette.UTF8String);
    NSUInteger pixelWidth = (NSUInteger)llround(width * scale);
    NSUInteger pixelHeight = (NSUInteger)ceil(scale);
    NSString *key = [NSString stringWithFormat:@"%s/%lu/%lu/%.17g", palette->identifier,
        (unsigned long)pixelWidth, (unsigned long)pixelHeight, (double)scale];
    UIImage *image = [cache objectForKey:key];
    if (image) return image;
    CGFloat components[16] = {0}, locations[4] = {0};
    for (size_t i = 0; i < palette->count * 4; ++i) components[i] = palette->rgba[i];
    for (size_t i = 0; i < palette->count; ++i) locations[i] = IDTextPaletteLocation(palette, i);
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(width, 1), NO, scale);
    CGContextRef context = UIGraphicsGetCurrentContext();
    if (!context) { UIGraphicsEndImageContext(); return nil; }
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGGradientRef gradient = space ? CGGradientCreateWithColorComponents(space, components, locations, palette->count) : NULL;
    if (gradient) CGContextDrawLinearGradient(context, gradient, CGPointZero, CGPointMake(width, 0),
        kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
    image = gradient ? UIGraphicsGetImageFromCurrentImageContext() : nil;
    UIGraphicsEndImageContext();
    if (gradient) CGGradientRelease(gradient);
    if (space) CGColorSpaceRelease(space);
    if (image.CGImage) [cache setObject:image forKey:key cost:
        CGImageGetBytesPerRow(image.CGImage) * CGImageGetHeight(image.CGImage)];
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
