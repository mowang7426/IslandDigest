from PIL import Image, ImageEnhance, ImageOps
from pathlib import Path
src=Image.open('/var/minis/attachments/uploads/photo_7764197E.jpeg').convert('RGB')
# Portrait-focused square crop: keep face, scarf and a little car/sky context.
crop=src.crop((145, 18, 1005, 878))
crop=ImageEnhance.Contrast(crop).enhance(1.04)
crop=ImageEnhance.Color(crop).enhance(1.05)
out=Path('/var/minis/workspace/islanddigest-src/IslandDigest-0.1.4')
for name,size in [('IslandDigest-icon.png',1024),('IslandDigest-icon@3x.png',180),('IslandDigest-icon@2x.png',120)]:
    im=ImageOps.fit(crop,(size,size),method=Image.Resampling.LANCZOS,centering=(0.53,0.46))
    im.save(out/name,'PNG',optimize=True)
# PreferenceLoader resolves a package-level icon; retain identical high-res source-derived PNG.
(out/'layout/Library/PreferenceLoader/Preferences/IslandDigest.png').write_bytes((out/'IslandDigest-icon.png').read_bytes())
(out/'prefs/Resources/IslandDigest.png').write_bytes((out/'IslandDigest-icon.png').read_bytes())
