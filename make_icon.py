from PIL import Image, ImageEnhance, ImageOps
from pathlib import Path
# The original portrait is a build-time input only; never copied into a package.
root = Path(__file__).resolve().parent
src = Image.open('/var/minis/attachments/uploads/photo_7764197E.jpeg').convert('RGB')
crop = ImageEnhance.Color(ImageEnhance.Contrast(src.crop((145, 18, 1005, 878))).enhance(1.04)).enhance(1.05)
master = ImageOps.fit(crop, (1024, 1024), method=Image.Resampling.LANCZOS, centering=(0.53, 0.46)).convert('RGBA')
master.save(root / 'IslandDigest-icon.png', 'PNG', optimize=True)
# A 29-point ENTRY image, not a 1024-point image with transparent subject padding.
# Both PreferenceLoader's adjacent path and the preferences bundle have exact scale siblings.
for folder in (root/'layout/Library/PreferenceLoader/Preferences', root/'prefs/Resources'):
    for suffix, size in (('', 29), ('@2x', 58), ('@3x', 87)):
        master.resize((size, size), Image.Resampling.LANCZOS).save(folder/f'IslandDigest{suffix}.png', 'PNG', optimize=True)
# Keep the high-resolution master separate from the registered logical-size resource.
master.save(root/'prefs/Resources/IslandDigest-master.png', 'PNG', optimize=True)
for name, size in (('IslandDigest-icon@2x.png', 120), ('IslandDigest-icon@3x.png', 180)):
    master.resize((size,size), Image.Resampling.LANCZOS).save(root/name, 'PNG', optimize=True)
# Superseded bundle assets are not part of the payload.
for name in ('IslandDigest-icon.png', 'IslandDigest-icon@2x.png', 'IslandDigest-icon@3x.png'):
    (root/'prefs/Resources'/name).unlink(missing_ok=True)
