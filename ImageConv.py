import sys, os
import functools

TERMI = 0x01

def flat(l):
    if len(l) == 0: return []
    return functools.reduce(lambda x, y: x+y, l)

class cFile:   
    def LoadHost(self, xHostPath):
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

        return self

    def SaveImg(self):        
        xName = [ord(x) for x in self.xName]
        return [0x10, *xName, TERMI, self.xPrior, TERMI, 0x12, *[ord(x) for x in self.xContent], TERMI]

    def Print(self, i):
        print(f'{"    "*i}{self.xName} {self.xPrior}')
        

class cNode:
    def LoadHost(self, xHostPath):
        (xRoot, xSubdirs, xFiles) = next(os.walk(xHostPath))
        assert(xRoot == xHostPath)
        
        self.xFiles   = [cFile().LoadHost(os.path.join(xHostPath, x)) for x in xFiles]
        self.xSubDirs = [cNode().LoadHost(os.path.join(xHostPath, x)) for x in xSubdirs]

        self.xName    = os.path.basename(xHostPath)
        self.xPrior = 0xff
        
    def SaveImg(self):       
        
        xName = [ord(x) for x in self.xName]
        xSubFiles = flat([x.SaveImg() for x in self.xFiles])
        xSubDirs  = flat([x.SaveImg() for x in self.xSubDirs])
        
        xSubNodes = xSubFiles + xSubDirs
        return [0x10, *xName, TERMI, self.xPrior, TERMI, 0x11, *xSubNodes, TERMI]

    def Print(self, i=0):
        print(f'{"    "*i}{self.xName} {self.xPrior}')

        for xIter in self.xFiles+self.xSubDirs:
            xIter.Print(i+1)
        


xRoot = cNode()

while True:
    try:
        xInj = input(f"'{xRoot}' >>> ").strip().lower()
        (xCom, xArgs*) = xInj.split(" ")
        
        if xCom == "clear":
            xRoot = cNode()
        
        elif xCom == "print":
            xRoot.Print()
            
        elif xCom == "loadhost":
            xRoot.LoadHost(xArgs[0])
            
        elif xCom == "help":
            print("""
clear    - reset buffer
print    - print buffer
loadhost - load image from host filesystem

            """)
        
        else:
            print("Unknown injection")
        

    except Exception as E:
        print(E)

"""
xBin = [x for x in xRoot.GetBin() if x < 0x100]

with open("image.bin", "wb") as xFilePtr:
    xFilePtr.write(bytearray(xBin))
"""
