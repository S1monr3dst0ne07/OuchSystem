from dataclasses import dataclass
import typing
import sys, os
import functools

TERMI = 0x01


class cNode:
    "don't even say anything"
    def __init__(self, 
                xName     = "", 
                xPrior    = 0, 
                xIsFile   = False, 
                xContent  = "", 
                xSubNodes = []):
        self.xName      = xName
        self.xPrior     = xPrior
        self.xIsFile    = xIsFile
        self.xContent   = xContent
        self.xSubNodes  = xSubNodes
        
       
    @staticmethod 
    def LoadHost(xPath):
        #check if object to parse is file
        xIsFile = os.path.isfile(xPath)

        xNew = cNode(
            xName = os.path.basename(xPath),
            xPrior = 0xff,
            xIsFile = xIsFile,
        )
        
        if xIsFile:
            try:
                with open(xPath, "r") as xFilePtr:
                    xNew.xContent = xFilePtr.read()    
            except UnicodeDecodeError:
                xNew.xContent = ""
            
            xNameRaw = os.path.basename(xPath)                
            if xNameRaw.count('.') > 1:
                (xBaseName, xPrior, xExt) = xNameRaw.split('.')
                xRes = f'{xBaseName}.{xExt}', int(xPrior)

            else: xRes = (xNameRaw, 0xff)                
            xNew.xName, xNew.xPrior = xRes
                                   
        else:
            (xRoot, xSubdirs, xFiles) = next(os.walk(xPath))
            xSubObjs = xSubdirs + xFiles
            assert xRoot == xPath  

            xNew.xSubNodes = [cNode.LoadHost(os.path.join(xPath, x)) for x in xSubObjs]
        
        return xNew
    
    def List(self, xLvl=0):
        print(f'{"    " * xLvl}{self.xName} {self.xPrior}')
        for xSub in self.xSubNodes:
            xSub.List(xLvl=xLvl+1)
    
    
def repl():
    xBuffer = None
    
    while True:
        try:
            
            xInj = input(f'{xBuffer} ').lower().strip()
            xInjs = xInj.split(" ")
            if len(xInjs) < 1: continue
            
            xOp, *xArgs = xInjs
            if xOp == 'drop':
                xBuffer = None
    
            elif xOp == 'list':
                xBuffer.List()
    
            elif xOp == 'loadhost':
                xBuffer = cNode.LoadHost(xArgs[0])

        except KeyboardInterrupt:
            break        
        except Exception as E:
            print(E)

if __name__ == '__main__':
    repl()