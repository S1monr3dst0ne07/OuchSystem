from dataclasses import dataclass
import typing
import sys, os
import functools
from functools import reduce
import operator
import time
import copy

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
        
        
    def GetSub(self, xName):
        for xSub in self.xSubNodes:
            if xSub.xName == xName:
                return xSub
        return None
    
    def GetNodeByPath(self, xPath):
        if len(xPath) == 0: return self
        
        xHead, *xTail = xPath
        xNode = self.GetSub(xHead)        
        return xNode.GetNodeByPath(xTail)
        
        
       
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
    
        
    
    def SaveRoot(self, xPath):
        xBin = self.GetBin()
        print(xBin)
        
        with open(xPath, "wb") as xFile:
            xFile.write(bytes(xBin))
    
    def GetBin(self):
        xName    = [ord(x) for x in self.xName]
        xSubCont = [0x12] + [ord(x)     for x in self.xContent ] if self.xIsFile else \
                   [0x11] + reduce(operator.add, [x.GetBin() for x in self.xSubNodes])
        
        return [0x10, *xName, TERMI, self.xPrior, TERMI, *xSubCont, TERMI]
        
    
    def List(self, xLvl=0):
        print(f'{"    " * xLvl}{self.xName} {self.xPrior}')
        for xSub in self.xSubNodes:
            xSub.List(xLvl=xLvl+1)

    @staticmethod
    def LoadRoot(xRaw):
        assert xRaw.pop(0) == 0x10
    
        xName = []
        while ((xTemp := xRaw.pop(0)) != TERMI): xName.append(xTemp)
    
        xPrior = xRaw.pop(0)
        assert xRaw.pop(0) == TERMI
        
        xIsFile = xRaw.pop(0) == 0x12
        xContent = []
        xSubNodes = []
        if xIsFile:
            while xRaw[0] != TERMI:
                xContent.append(xRaw.pop(0))
            
        else:
            while xRaw[0] == 0x10:
                xSubNodes.append(cNode.LoadRoot(xRaw))

        assert xRaw.pop(0) == TERMI
                            
        return cNode(
            xName = "".join(map(chr, xName)),
            xPrior = xPrior,
            xIsFile = xIsFile,
            xContent = "".join(map(chr, xContent)),
            xSubNodes = xSubNodes,    
        )


    
    
def repl():
    xBuffer = None
    xTime = 0.0
    
    while True:
        try:
            
            xInj = input(f'{xTime}s {xBuffer} ').lower().strip()
            xInjs = xInj.split(" ")
            if len(xInjs) < 1: continue
            
            xStart = time.time()
            
            xOp, *xArgs = xInjs
            if xOp == 'drop':
                xBuffer = None
    
            elif xOp == 'list':
                xBuffer.List()
    
            elif xOp == 'loadhost':
                xPath = xArgs[0]
                xBuffer = cNode.LoadHost(xPath)

            elif xOp == 'saveroot':
                xPath = xArgs[0]
                if input(f"You are about to override {xPath}, do you confirm [y/n]? ") != 'y': 
                    continue
                
                xBuffer.SaveRoot(xPath)
                
            elif xOp == 'loadroot':
                with open(xArgs[0], "rb") as xFile:
                    xBin = list(xFile.read())

                xBuffer = cNode.LoadRoot(xBin)

                
            elif xOp == 'cat':
                xPath = xArgs[0].split("/")
                xNode = xBuffer.GetNodeByPath(xPath)
                print(xNode.xContent)
                
            elif xOp == 'len':
                xPath = xArgs[0].split("/")
                xNode = xBuffer.GetNodeByPath(xPath)
                xLen = len(xNode.xContent)
                print(f'{xArgs[0]}: {xLen}')
                
            elif xOp == 'exit':
                break
            elif xOp == 'clear':
                os.system("cls")
                
            xEnd = time.time()
            xTime = xEnd - xStart

        except KeyboardInterrupt:
            break

        except Exception as E:
            print(E)

if __name__ == '__main__':
    repl()