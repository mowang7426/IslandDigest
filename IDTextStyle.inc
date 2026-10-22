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
    const IDPalette *palette = IDPaletteForIdentifier(IDTextPalette.UTF8String);
    NSString *key = [NSString stringWithFormat:@"%s/%.9g/%.9g", palette->identifier, (double)width, (double)scale];
    UIImage *image = [cache objectForKey:key];
    if (image) return image;
    NSMutableArray *colors = [NSMutableArray arrayWithCapacity:palette->count];
    CGFloat locations[4];
    for (size_t index = 0; index < palette->count; index++) {
        IDPaletteColor rgb = palette->colors[index];
        UIColor *color = [UIColor colorWithRed:rgb.r green:rgb.g blue:rgb.b alpha:1];
        [colors addObject:(__bridge id)color.CGColor];
        locations[index] = IDPaletteStop(palette, index);
    }
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(width, 1), NO, scale);
    CGContextRef context = UIGraphicsGetCurrentContext();
    if (!context) { UIGraphicsEndImageContext(); return nil; }
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGGradientRef gradient = CGGradientCreateWithColors(space, (__bridge CFArrayRef)colors, locations);
    if (!gradient) { UIGraphicsEndImageContext(); CGColorSpaceRelease(space); return nil; }
    CGContextDrawLinearGradient(context, gradient, CGPointZero, CGPointMake(width, 0),
        kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
    image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext(); CGGradientRelease(gradient); CGColorSpaceRelease(space);
    if (image.CGImage) [cache setObject:image forKey:key
        cost:CGImageGetBytesPerRow(image.CGImage) * CGImageGetHeight(image.CGImage)];
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
