"""Independent Pillow and zlib validation; host test tool, never a DOS dependency."""
import struct,sys,zlib
from pathlib import Path
from PIL import Image
root=Path(sys.argv[1])
def rgb(index):
    dac=[index*4]*3 if index<16 else [((index-16)//d%6)*12 for d in (1,6,36)]
    return tuple(round(c*255/63) for c in dac)
def verify(name,w,h,pixel):
    raw=(root/name).read_bytes(); assert raw[:8]==b'\x89PNG\r\n\x1a\n'
    pos=8; idat=b''
    while pos<len(raw):
        n,=struct.unpack_from('>I',raw,pos); kind=raw[pos+4:pos+8];data=raw[pos+8:pos+8+n]
        assert zlib.crc32(kind+data)==struct.unpack_from('>I',raw,pos+8+n)[0]
        if kind==b'IDAT':idat+=data
        pos+=12+n
    decoded=zlib.decompress(idat); assert len(decoded)==(w*4+1)*h
    expected=bytes(v for y in range(h) for x in range(w) for v in pixel(x,y))
    assert decoded==b''.join(b'\0'+expected[y*w*4:(y+1)*w*4] for y in range(h))
    with Image.open(root/name) as im:
        im.load();assert im.size==(w,h) and im.mode=='RGBA' and im.tobytes()==expected
    print(name,len(raw),'bytes: exact RGBA, zlib, CRC and Pillow pass')
def frame(f,x,y,flatten=False):
    offset=y*16+x; index=(f*384+offset)%256
    alpha=(f==0 and offset==1) or (f==1 and offset==0) or (f==2 and offset<8)
    return rgb(141 if flatten and alpha else index)+(0 if alpha and not flatten else 255,)
verify('ALPHA.PNG',16,24,lambda x,y:frame(0,x,y))
for f in range(3): verify(f'RGBA{f+1:04}.PNG',16,24,lambda x,y,f=f:frame(f,x,y))
verify('HORIZ.PNG',48,24,lambda x,y:frame(x//16,x%16,y))
verify('VERT.PNG',16,72,lambda x,y:frame(y//24,x,y%24))
for name,flatten in [('SHEET.PNG',False),('FLAT.PNG',True)]:
    verify(name,32,48,lambda x,y,flatten=flatten: frame(y//24*2+x//16,x%16,y%24,flatten) if y//24*2+x//16<3 else (rgb(141)+(255,) if flatten else rgb(0)+(0,)))
verify('BIGRGBA.PNG',256,64,lambda x,y:rgb(0)+(0 if x==0 and y==0 else 255,))
