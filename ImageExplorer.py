import sys, os
import functools

TERMI = 0x01


class cNode:
    def __init__(self, xRaw):
        assert xRaw.Depo(0x10) #header
        self.xName = xRaw.GetString()
        self.xPrior = xRaw.Cons()
        xRaw.Depo(TERMI)

        xType = xRaw.Cons()
        assert xType in (0x11, 0x12)
        self.xIsDir = xType == 0x11
        
        if self.xIsDir:
            self.xSubNodes = []
            while xRaw.Peek() == 0x10:
                self.xSubNodes.append(cNode(xRaw))
                
        else:
            self.xContent = xRaw.GetString()
            
    def list(self, i=0):
        print(f'{"    " * i}{self.xName} {self.xPrior}')
        if self.xIsDir:
            for xSub in self.xSubNodes:
                xSub.list(i = i + 1)
        
            

class cRaw:
    def __init__(self, xContent=""):
        self.xContent = xContent
        self.xIndex = 0

    def Peek(self):
        return self.xContent[self.xIndex]
        
    def Cons(self):
        xData = self.Peek()
        self.xIndex += 1
        return xData
        
    def Depo(self, xChar):
        return xChar == self.Cons()
    
    def GetString(self):
        xBuffer = []
        
        while True:
            if self.Peek() == TERMI:
                break

            xBuffer.append(chr(self.Cons()))

        self.Depo(TERMI)
        return "".join(xBuffer)


class cExp:
    def __init__(self):
        self.xImage = None


    def Run(self, xPath):            
        with open(xPath, "rb") as xFile:
            xRaw = cRaw(xFile.read())
            
        self.xImage = cNode(xRaw)
        self.xImage.list()
        
    

if len(sys.argv) < 2:
    print("Image not specified")
    sys.exit(0)

cExp().Run(sys.argv[1])

