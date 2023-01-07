import sys, os
import functools

TERMI = 0x01

def flat(l):
    if len(l) == 0: return []
    return functools.reduce(lambda x, y: x+y, l)

class cFile:
    def __init__(self, xHostPath):
        try:
            with open(xHostPath, "r") as xFilePtr:
                self.xContent = xFilePtr.read()

        except UnicodeDecodeError:
            self.xContent = ""
        
        xNameRaw = os.path.basename(xHostPath)

        if xNameRaw.count('.') > 1:
            (xBaseName, xPrior, xExt) = xNameRaw.split('.')
            self.xName  = f'{xBaseName}.{xExt}'
            self.xPrior = int(xPrior)
                        
        else:
            self.xName = xNameRaw
            self.xPrior = 0xff        

    def GetBin(self):        
        xName = [ord(x) for x in self.xName]
        return [0x10, *xName, TERMI, self.xPrior, TERMI, 0x12, *[ord(x) for x in self.xContent], TERMI]

    def Print(self, i):
        print(f'{"    "*i}{self.xName} {self.xPrior}')
        

class cNode:
    def __init__(self, xHostPath):
        (xRoot, xSubdirs, xFiles) = next(os.walk(xHostPath))
        assert(xRoot == xHostPath)
        
        self.xFiles   = [cFile(os.path.join(xHostPath, x)) for x in xFiles]
        self.xSubDirs = [cNode(os.path.join(xHostPath, x)) for x in xSubdirs]

        self.xName    = os.path.basename(xHostPath)
        self.xPrior = 0xff
        
    def GetBin(self):       
        
        xName = [ord(x) for x in self.xName]
        xSubFiles = flat([x.GetBin() for x in self.xFiles])
        xSubDirs  = flat([x.GetBin() for x in self.xSubDirs])
        
        xSubNodes = xSubFiles + xSubDirs
        return [0x10, *xName, TERMI, self.xPrior, TERMI, 0x11, *xSubNodes, TERMI]

    def Print(self, i=0):
        print(f'{"    "*i}{self.xName} {self.xPrior}')

        for xIter in self.xFiles+self.xSubDirs:
            xIter.Print(i+1)
        


xRoot = cNode(sys.argv[1])
xRoot.Print()
xBin = [x for x in xRoot.GetBin() if x < 0x100]

with open("image.bin", "wb") as xFilePtr:
    xFilePtr.write(bytearray(xBin))

