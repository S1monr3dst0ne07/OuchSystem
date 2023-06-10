import os
import struct
import sys
import argparse
import functools

MAXNAMELEN  = 256
TDIR        = 0x1
TFILE       = 0x2

def GenNode(xPath):
    #print(xPath)
    if not os.path.exists(xPath): return b''
    xIsFile = os.path.isfile(xPath)

    if xIsFile:
        with open(xPath, "rb") as xFile:
            xContent = xFile.read()
            
    #take a walk
    else:
        xSubDirs = [os.path.join(xPath, x) for x in os.listdir(xPath)]
        xSubNodes = [GenNode(x) for x in xSubDirs]
        xContent = functools.reduce(lambda x, y: x+y, xSubNodes)
    
    xName = os.path.basename(xPath)[:(MAXNAMELEN - 1)]
    print(xPath, xName)
    xType = TFILE if xIsFile else TDIR
    
    print(f'ContLen: {len(xContent)}')  

    print(f'Name: {xName.encode("utf-8")}')
    xHeader = struct.pack(f'<B{MAXNAMELEN}sI', xType, xName.encode('utf-8'), len(xContent))
    print(len(xHeader))
    print(f'Header: {xHeader}')
    xOut = xHeader + xContent
    
    return xOut
    
    
if __name__ == '__main__':
    xPar = argparse.ArgumentParser(description='Image generator for S1os')
    xPar.add_argument("src", help='source path')
    xPar.add_argument("-o", dest="dst", help='destination file')
    
    xArgs = xPar.parse_args()
    xOut = GenNode(xArgs.src)
    
    xOutPath = xArgs.dst if xArgs.dst else "image.bin"
    with open(xOutPath, "wb") as xFile:
        xFile.write(xOut)
    
    