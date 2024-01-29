#!/usr/bin/env python3

'''
Parse the file
'''
import os
fname = "output_log.csv"

def CheckLogFile():
    LogFileValues = []
    with open(fname) as f:
        data = f.readlines()
    prevVal = None 
    linect = 1
    for line in data:
        if line in ['\n', '\r\n']:
            continue
        temp_data = line.split(",") # Seperate the timestamp and nodeId, Start/End notification data
        temp_data[2] = temp_data[2].strip() # removes \n from Start/End
        LogFileValues.append(temp_data) #Store timestamp as keys and id as corresponding value
        val = str(temp_data[1]+"-"+temp_data[2]) # Node_ID - Start/End
        if(prevVal != None and linect%2 == 0):
            currLine = val.split("-")
            prevLine = prevVal.split("-")
            if(prevLine[0] == currLine[0]):
                if(prevLine[1].lower() == "start" and currLine[1].lower() == "end"):
                    prevVal = None
                    linect += 1
                    continue
                elif((prevLine[1].lower() == "start" and currLine[1].lower() == "start") or (prevLine[1].lower() == "end" and currLine[1].lower() == "end")):
                    print("System is violating the protocol")
                    print(f"{fname}: error at line {linect}: {temp_data}")
                    return
            print("System is violating the protocol")
            print(f"{fname}: error at line {linect}: {temp_data}")
            return
        prevVal = val
        linect += 1
    print("Protocol is working fine")

CheckLogFile()
            
