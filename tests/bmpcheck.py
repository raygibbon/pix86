"""External Pillow verification of original C/DOS export fixtures (test tool only)."""
import sys
from pathlib import Path
from PIL import Image
root=Path(sys.argv[1])

def pattern(frame, offset, size):
    return ((frame*size+offset)*13+frame*71)%256

def verify(name,w,h,pixel):
    with Image.open(root/name) as im:
        im.load()
        assert im.format=='BMP' and im.mode=='P' and im.size==(w,h), name
        assert im.info.get('compression',0)==0 and 'transparency' not in im.info
        actual=list(im.getdata())
        assert actual==[pixel(x,y) for y in range(h) for x in range(w)], name
        pal=im.getpalette()
        expected=[]
        for i in range(256):
            dac=[i*4]*3 if i<16 else [((i-16)//factor%6)*12 for factor in (1,6,36)]
            expected.extend(round(v*255/63) for v in dac)
        assert pal==expected,name
verify('ODD.BMP',3,5,lambda x,y:(x+3*y)%256)
verify('LARGE.BMP',257,257,lambda x,y:(x+3*y)%256)
verify('FRAME.BMP',16,24,lambda x,y:pattern(3,y*16+x,384))
for frame in range(1,5):
    verify(f'PLY{frame+1:04}.BMP',16,24,lambda x,y,f=frame:pattern(f,y*16+x,384))
verify('SHEET.BMP',48,48,lambda x,y:pattern((y//24)*3+x//16,(y%24)*16+x%16,384) if (y//24)*3+x//16<5 else 141)
verify('WIDE.BMP',1024,64,lambda x,y:pattern(x//64,y*64+x%64,4096) if x//64<4 else 141)
print('PASS: Pillow opens BMPs with exact indices, full RGB palette, frame order and dimensions.')
