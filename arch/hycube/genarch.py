import sys
from unittest import result 
sys.path.append(".")

import xmltodict
import json

from arch import *
from adres import *
def _basicFunctions(): 
    return {
        "__INPUT__":  {"input": ["in0", ], "output": ["out0", ], "width": "32"}, 
        "__OUTPUT__": {"input": ["in0", ], "output": ["out0", ], "width": "32"}, 
        "__IO__":  {"input": ["in0", ], "output": ["out0", ], "width": "32"}, 
        "__ADD__": {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__SUB__": {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__MUL__": {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__ACC__": {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__AND__": {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__OR__":  {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__XOR__": {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__NOT__": {"input": ["in0", ],      "output": ["out0", ], "width": "32"}, 
        "__LSHIFT__":  {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__RSHIFT__":  {"input": ["in0", "in1"], "output": ["out0", ], "width": "32"}, 
        "__MAD__":     {"input": ["in0", "in1", "in2"], "output": ["out0", ], "width": "32"}, 
        "__CONST__":   {"input": ["in0", ], "output": ["out0", ], "width": "32"}, 
        # "__REG__":     {"input": ["din", "ctr", ], "output": ["dout", ], "width": "32"}, 
        "__REG__":     {"input": ["in0", "in1", ], "output": ["out0", ], "width": "32"}, 
        # "__MEM__":     {"input": ["addr", "din", "ctr", ], "output": ["dout", ], "width": "32"}, 
        "__MEM__":     {"input": ["in0", "in1", "in2", ], "output": ["out0", ], "width": "32"}, 
        "__EQUAL__":   {"input": ["in0", "in1", ], "output": ["out0", ], "width": "32"}, 
        "__GREATER__": {"input": ["in0", "in1", ], "output": ["out0", ], "width": "32"}, 
        "__LESS__":    {"input": ["in0", "in1", ], "output": ["out0", ], "width": "32"}, 
        "__BRANCH2__": {"input": ["in0", ], "output": ["out0", "out1", ], "width": "32"}, 
        "__BRANCH3__": {"input": ["in0", ], "output": ["out0", "out1", "out2", ], "width": "32"}, 
        "__BRANCH4__": {"input": ["in0", ], "output": ["out0", "out1", "out2", "out3", ], "width": "32"}, 
        "__JOIN2__":   {"input": ["in0", "in1", "select"], "output": ["out0", ], "width": "32"}, 
        "__JOIN3__":   {"input": ["in0", "in1", "in2", "select"], "output": ["out0", ], "width": "32"}, 
        "__JOIN4__":   {"input": ["in0", "in1", "in2", "in3", "select"], "output": ["out0", ], "width": "32"}, 
    } 

def _templateSimple(): 
    arch = {}

    arch["Function"] = _basicFunctions()

    arch["Unit"] = {
        "IO": {"input": ["in0", ], "output": ["out0", ], 
               "function": {"FUNC_IO0": "__IO__", }, 
               "pattern": {"PATN_IO0": {"unit": ["FUNC_IO0", ], 
                                        "port": {"in0": "FUNC_IO0.in0", 
                                                 "out0": "FUNC_IO0.out0"}, 
                                        "connection": [], }, }, 
               "compat": {}, }, 
        "CONST": {"input": ["in0", ], "output": ["out0", ], 
                  "function": {"FUNC_CONST0": "__CONST__", }, 
                  "pattern": {"PATN_CONST0": {"unit": ["FUNC_CONST0", ], 
                                              "port": {"in0": "FUNC_CONST0.in0", 
                                                       "out0": "FUNC_CONST0.out0"}, 
                                              "connection": [], }, }, 
                  "compat": {}, }, 
        "REG": {"input": ["in0", "in1", ], "output": ["out0", ], 
                "function": {"FUNC_REG0": "__REG__", }, 
                "pattern": {"PATN_REG0": {"unit": ["FUNC_REG0", ], 
                                          "port": {"in0": "FUNC_REG0.in0", 
                                                   "in1": "FUNC_REG0.in1", 
                                                   "out0": "FUNC_REG0.out0"}, 
                                          "connection": [], }, }, 
                "compat": {}, }, 
        "MEM": {"input": ["in0", "in1", "in2", ], "output": ["out0", ], 
                "function": {"FUNC_MEM0": "__MEM__", }, 
                "pattern": {"PATN_MEM0": {"unit": ["FUNC_MEM0", ], 
                                          "port": {"in0": "FUNC_MEM0.in0", 
                                                   "in1": "FUNC_MEM0.in1", 
                                                   "in2": "FUNC_MEM0.in2", 
                                                   "out0": "FUNC_MEM0.out0"}, 
                                          "connection": [], }, }, 
                "compat": {}, }, 
        "ALU": {"input": ["in0", "in1", "in2", ], "output": ["out0", ], 
                "function": {"FUNC_ADD0": "__ADD__", 
                             "FUNC_SUB0": "__SUB__", 
                             "FUNC_MUL0": "__MUL__", 
                             "FUNC_AND0": "__AND__", 
                             "FUNC_OR0":  "__OR__", 
                             "FUNC_XOR0":  "__XOR__", 
                             "FUNC_LSHIFT0":  "__LSHIFT__", 
                             "FUNC_RSHIFT0":  "__RSHIFT__", 
                             "FUNC_SELECT0":  "__JOIN2__", }, 
                "pattern": {"PATN_ADD0": {"unit": ["FUNC_ADD0", ], 
                                        "port": {"in0": "FUNC_ADD0.in0", 
                                                    "in1": "FUNC_ADD0.in1", 
                                                    "out0": "FUNC_ADD0.out0"}, 
                                        "connection": [], }, 
                            "PATN_SUB0": {"unit": ["FUNC_SUB0", ], 
                                        "port": {"in0": "FUNC_SUB0.in0", 
                                                    "in1": "FUNC_SUB0.in1", 
                                                    "out0": "FUNC_SUB0.out0"}, 
                                        "connection": [], }, 
                            "PATN_MUL0": {"unit": ["FUNC_MUL0", ], 
                                        "port": {"in0": "FUNC_MUL0.in0", 
                                                    "in1": "FUNC_MUL0.in1", 
                                                    "out0": "FUNC_MUL0.out0"}, 
                                        "connection": [], }, 
                            "PATN_AND0": {"unit": ["FUNC_AND0", ], 
                                        "port": {"in0": "FUNC_AND0.in0", 
                                                    "in1": "FUNC_AND0.in1", 
                                                    "out0": "FUNC_AND0.out0"}, 
                                        "connection": [], }, 
                            "PATN_OR0": {"unit": ["FUNC_OR0", ], 
                                        "port": {"in0": "FUNC_OR0.in0", 
                                                "in1": "FUNC_OR0.in1", 
                                                "out0": "FUNC_OR0.out0"}, 
                                        "connection": [], }, 
                            "PATN_XOR0": {"unit": ["FUNC_XOR0", ], 
                                        "port": {"in0": "FUNC_XOR0.in0", 
                                                    "in1": "FUNC_XOR0.in1", 
                                                    "out0": "FUNC_XOR0.out0"}, 
                                        "connection": [], }, 
                            "PATN_LSHIFT0": {"unit": ["FUNC_LSHIFT0", ], 
                                            "port": {"in0": "FUNC_LSHIFT0.in0", 
                                                    "in1": "FUNC_LSHIFT0.in1", 
                                                    "out0": "FUNC_LSHIFT0.out0"}, 
                                            "connection": [], }, 
                            "PATN_RSHIFT0": {"unit": ["FUNC_RSHIFT0", ], 
                                            "port": {"in0": "FUNC_RSHIFT0.in0", 
                                                    "in1": "FUNC_RSHIFT0.in1", 
                                                    "out0": "FUNC_RSHIFT0.out0"}, 
                                            "connection": [], }, 
                            "PATN_SELECT": {"unit": ["FUNC_SELECT0", ], 
                                            "port": {"in0": "FUNC_SELECT0.in0", 
                                                    "in1": "FUNC_SELECT0.in1", 
                                                    "in2": "FUNC_SELECT0.select", 
                                                    "out0": "FUNC_SELECT0.out0"}, 
                                            "connection": [], }, },
                "compat": {}, },
    }

    arch["Switch"] = {
        "FULLYCONN_2X1": {
            "input":  ["in0", "in1", ], 
            "output": ["out0", ], 
            "required": ["in0->out0", "in1->out0", ], 
            "graph": "", 
        }, 
        "FULLYCONN_4X1": {
            "input":  ["in0", "in1", "in2", "in3", ], 
            "output": ["out0", ], 
            "required": ["in0->out0", "in1->out0", "in2->out0", "in3->out0", ], 
            "graph": "", 
        }, 
        "FULLYCONN_5X1": {
            "input":  ["in0", "in1", "in2", "in3", "in4"], 
            "output": ["out0", ], 
            "required": ["in0->out0", "in1->out0", "in2->out0", "in3->out0", "in4->out0"], 
            "graph": "", 
        }, 
        "FULLYCONN_6X1": {
            "input":  ["in0", "in1", "in2", "in3", "in4", "in5", ], 
            "output": ["out0", ], 
            "required": ["in0->out0", "in1->out0", "in2->out0", "in3->out0", "in4->out0", "in5->out0",], 
            "graph": "", 
        }, 
        "FULLYCONN_10X1": {
            "input":  ["in0", "in1", "in2", "in3", "in4", "in5", "in6", "in7", "in8", "in9"], 
            "output": ["out0", ], 
            "required": ["in0->out0", "in1->out0", "in2->out0", "in3->out0", "in4->out0", 
                         "in5->out0", "in6->out0", "in7->out0", "in8->out0", "in9->out0", ], 
            "graph": "", 
        }, 
        "FULLYCONN_PE": {
            "input":  [], 
            "output": ["out0", ], 
            "required": [], 
            "graph": "", 
        }, 
        "FULLYCONN_MEM": {
            "input":  [], 
            "output": [], 
            "required": [], 
            "graph": "", 
        }, 
        "FULLYCONN_IO": {
            "input":  [], 
            "output": ["out0", ], 
            "required": [], 
            "graph": "", 
        }, 
        "CROSSBAR_PE": {
            "input": [
            ],
            "output": [
            ],
            "required": [
            ],
            "graph": ""
        },
        "FULLCONN_ALU": {
            "input": [
            ],
            "output": [
            ],
            "required": [
            ],
            "graph": ""
        }
    }

    arch["Module"] = {
        "PE0": {
            "input": [], 
            "output": [], 
            "module": {}, 
            "element": {
                "ALU0": "ALU", 
                "CONST0": "CONST", 
                "REG0": "REG", 
            }, 
            "switch": {}, 
            "connection": []
        }, 
        "PE1": {
            "input": [], 
            "output": [], 
            "module": {}, 
            "element": {
                "ALU0": "ALU", 
                "CONST0": "CONST", 
                # "CONST1": "CONST", 
            }, 
            "switch": {}, 
            "connection": []
        }, 
        "MEMORY": {
            "input": [], 
            "output": [
            ], 
            "module": {}, 
            "element": {
            }, 
            "switch": {
            }, 
            "connection": []
        }, 
        "IOPAD": {
            "input": [], 
            "output": [
                "out0", 
            ], 
            "module": {}, 
            "element": {
                "IO0": "IO", 
            }, 
            "switch": {
                "SW_in0": "FULLYCONN_IO", 
            }, 
            "connection": []
        }, 
    }

    return arch

def _basearch(): 
    arch = _templateSimple()
    
    arch["Function"]["__DUMMY__"] = {"input": ["in0", ], "output": ["out0", ], "width": "32"}
    
    arch["Unit"]["DUMMY"] = {
        "input": ["in0", ], "output": ["out0", ], 
        "function": {"FUNC_DUMMY0": "__DUMMY__", }, 
        "pattern": {"PATN_DUMMY0": {"unit": ["FUNC_DUMMY0", ], 
                                    "port": {"in0": "FUNC_DUMMY0.in0", 
                                             "out0": "FUNC_DUMMY0.out0"}, 
                                    "connection": [], }, }, 
        "compat": {}, 
    }

    arch["Module"]["CGRA"] = {
        "input": [], 
        "output": [], 
        "module": {}, 
        "element": {}, 
        "switch": {}, 
        "connection": []
    }
    return arch

def genarch(sizeArray, typePE, typeConn, interMem, interIO): 

    arch = _basearch()
    
    rows = sizeArray[0]
    cols = sizeArray[1]
    
    inputsPE = ["in0", "in1", "in2", "in3", ]
    outputsPE = ["out0", "out1", "out2", "out3",]
    arch["Module"]["PE0"]["input"].extend(inputsPE)
    arch["Module"]["PE0"]["output"].extend(outputsPE)
    #crossbar
    sizeIn = len(inputsPE)+3
    sizeOut = len(outputsPE)+5
    for idx in range(sizeIn): 
        arch["Switch"]["CROSSBAR_PE"]["input"].append("in" + str(idx))
    for idx in range(sizeOut):
        arch["Switch"]["CROSSBAR_PE"]["output"].append("out" + str(idx))
    for idx in range(sizeIn): 
        for idy in range(sizeOut):
            arch["Switch"]["CROSSBAR_PE"]["required"].append("in" + str(idx) + "->out" + str(idy))
    # Connections in PEs
    arch["Module"]["PE0"]["switch"]["SW0"] = "CROSSBAR_PE"
    for idx in range(len(inputsPE)): 
        arch["Module"]["PE0"]["connection"].append(inputsPE[idx] + "->SW0.in" + str(idx))
    for idx in range(len(outputsPE)):
        arch["Module"]["PE0"]["connection"].append("SW0.out" + str(idx) + "->" + outputsPE[idx])
    for idx in range(len(inputsPE)): 
        for jdx in range(len(outputsPE)):
            arch["Module"]["PE0"]["connection"].append("SW0.in" + str(idx) + "->SW0.out" + str(jdx))
    arch["Module"]["PE0"]["connection"].append("ALU0.out0->SW0.in"+str(len(inputsPE)))
    arch["Module"]["PE0"]["connection"].append("CONST0.out0->SW0.in"+str(len(inputsPE)+1))
    arch["Module"]["PE0"]["connection"].append("REG0.out0->SW0.in"+str(len(inputsPE)+1))
    arch["Module"]["PE0"]["connection"].append("REG0.out0->SW0.in"+str(len(inputsPE)+2))

    for idx in range(4): 
        arch["Switch"]["FULLCONN_ALU"]["input"].append("in" + str(idx))
    for idx in range(3): 
        arch["Switch"]["FULLCONN_ALU"]["output"].append("out" + str(idx))
    for idx in range(4): 
        for idy in range(3):
            arch["Switch"]["FULLCONN_ALU"]["required"].append("in" + str(idx) + "->out" + str(idy))
    arch["Module"]["PE0"]["switch"]["FULLCONN_ALU0"] = "FULLCONN_ALU"
    arch["Module"]["PE0"]["connection"].append("SW0.out"+str(len(outputsPE))+"->FULLCONN_ALU0.in0")
    arch["Module"]["PE0"]["connection"].append("SW0.out"+str(len(outputsPE)+1)+"->FULLCONN_ALU0.in1")
    arch["Module"]["PE0"]["connection"].append("SW0.out"+str(len(outputsPE)+2)+"->FULLCONN_ALU0.in2")
    arch["Module"]["PE0"]["connection"].append("SW0.out"+str(len(outputsPE)+1)+"->REG0.in0")
    arch["Module"]["PE0"]["connection"].append("SW0.out"+str(len(outputsPE)+2)+"->REG0.in1")
    arch["Module"]["PE0"]["connection"].append("SW0.out"+str(len(outputsPE)+3)+"->REG0.in0")
    arch["Module"]["PE0"]["connection"].append("SW0.out"+str(len(outputsPE)+4)+"->REG0.in1")
    arch["Module"]["PE0"]["connection"].append("CONST0.out0->FULLCONN_ALU0.in3")
    arch["Module"]["PE0"]["connection"].append("FULLCONN_ALU0.out0->ALU0.in0")
    arch["Module"]["PE0"]["connection"].append("FULLCONN_ALU0.out1->ALU0.in1")
    arch["Module"]["PE0"]["connection"].append("FULLCONN_ALU0.out2->ALU0.in2")

    # MemoryBANK
    for idx in range(4*rows):
        arch["Module"]["MEMORY"]["input"].extend(["in"+str(idx)])
    for idx in range(rows):
        arch["Module"]["MEMORY"]["output"].extend(["out"+str(idx)])
    for idx in range(rows):
        arch["Switch"]["FULLYCONN_MEM"]["input"].append("in" + str(idx))
    arch["Switch"]["FULLYCONN_MEM"]["output"].append("out0")
    for idx in range(rows):
        arch["Module"]["MEMORY"]["element"]["MEM"+str(idx)]="MEM"
    for idx in range(rows):
        arch["Module"]["MEMORY"]["switch"]["SW_"+str(idx)] = "FULLYCONN_5X1"
        arch["Module"]["MEMORY"]["connection"].append("MEM"+str(idx)+".out0->SW_"+str(idx)+".in4")
        for jdx in range(4):
            arch["Module"]["MEMORY"]["connection"].append("in"+str(4*idx+jdx)+"->SW_"+str(idx)+".in"+str(jdx))
    for idx in range(rows):
        for jdx in range(3):
            arch["Module"]["MEMORY"]["switch"]["SW_"+str(idx)+"_to_in"+str(jdx)] = "FULLYCONN_MEM"
            for kdx in range(rows):
                arch["Module"]["MEMORY"]["connection"].append("SW_"+str(kdx)+".out0->SW_"+str(idx)+"_to_in"+str(jdx)+".in"+str(kdx))
                arch["Module"]["MEMORY"]["connection"].append("SW_"+str(idx)+"_to_in"+str(jdx)+".in"+str(kdx)+"->SW_"+str(idx)+"_to_in"+str(jdx)+".out0")
            arch["Module"]["MEMORY"]["connection"].append("SW_"+str(idx)+"_to_in"+str(jdx)+".out0->"+"MEM"+str(idx)+".in"+str(jdx))

        arch["Module"]["MEMORY"]["connection"].append("MEM"+str(idx)+".out0->out"+str(idx))


    # IO
    inputsIO = ["in0", "in1", "in2", "in3", "in4"]
    arch["Module"]["IOPAD"]["input"].extend(inputsIO)
    
    sizeSwitchIO = len(inputsIO)
    for idx in range(sizeSwitchIO): 
        arch["Switch"]["FULLYCONN_IO"]["input"].append("in" + str(idx))
        arch["Switch"]["FULLYCONN_IO"]["required"].append("in" + str(idx) + "->out0")

    for idx in range(len(inputsIO)): 
        arch["Module"]["IOPAD"]["connection"].append(inputsIO[idx] + "->SW_in0.in" + str(idx))
    arch["Module"]["IOPAD"]["connection"].append("SW_in0.out0->IO0.in0")
    arch["Module"]["IOPAD"]["connection"].append("IO0.out0->out0")
    arch["Module"]["IOPAD"]["connection"].append("IO0.out0->SW_in0.in4")

    # Construct the array
    listPEs = []
    for idx in range(rows): 
        for jdx in range(cols): 
            index = idx * cols + jdx
            name = "PE_" + str(idx) + "_" + str(jdx)
            listPEs.append(name)
            arch["Module"]["CGRA"]["module"][name] = typePE[index]
    arch["Module"]["CGRA"]["module"]["mem_0"] = "MEMORY"
    arch["Module"]["CGRA"]["module"]["mem_" + str(cols)] = "MEMORY"
    for idx in range(cols): 
        arch["Module"]["CGRA"]["module"]["io" + str(idx)] = "IOPAD"

    # PE <-> PE
    for idx in range(rows): 
        for jdx in range(cols): 
            fromPE = "PE_" + str(idx) + "_" + str(jdx)
            toPEs = []
            toPEs.append("PE_" + str(idx-1) + "_" + str(jdx))
            toPEs.append("PE_" + str(idx) + "_" + str(jdx+1))
            toPEs.append("PE_" + str(idx+1) + "_" + str(jdx))
            toPEs.append("PE_" + str(idx) + "_" + str(jdx-1))
            for kdx in range(4):
                toPE = toPEs[kdx]
                if toPE in listPEs:
                    arch["Module"]["CGRA"]["connection"].append(fromPE+".out"+str(kdx)+"->"+toPE+".in"+str(kdx))
    # IO <-> PE
    for idx in range(cols):
        for jdx in range(4):
            arch["Module"]["CGRA"]["connection"].append("PE_0_" + str(idx)+".out"+str(jdx)+"->"+"io"+str(idx)+".in"+str(jdx))
        arch["Module"]["CGRA"]["connection"].append("io"+str(idx)+".out0"+"->PE_0_" + str(idx)+".in2")
    # mem <-> PE
    for idx in range(rows):
        for col in range(cols):
            for jdx in range(4):
                arch["Module"]["CGRA"]["connection"].append("PE_"+str(idx)+"_" + str(col) +".out"+str(jdx)+"->"+"mem_0.in"+str(4*idx+jdx))
            arch["Module"]["CGRA"]["connection"].append("mem_0.out" + str(idx) + "->PE_"+str(idx)+"_" + str(col) +".in2")
    for idx in range(rows):
        for col in range(cols):
            for jdx in range(4):
                arch["Module"]["CGRA"]["connection"].append("PE_"+str(idx)+"_"+str(col)+".out"+str(jdx)+"->"+"mem_" + str(cols)+".in"+str(4*idx+jdx))
            arch["Module"]["CGRA"]["connection"].append("mem_" + str(cols) +".out" + str(idx) + "->PE_"+str(idx)+"_"+str(col)+".in2")
                    

    arch["Module"]["TOP"] = {
        "input": [

        ], 
        "output": [
            
        ], 
        "module": {
            "CGRA0": "CGRA", 
        }, 
        "element": {
            
        }, 
        "switch": {
            
        }, 
        "connection": [

        ]
    }

    return arch

    
if __name__ == "__main__": 
    coord = [10,10]
    str1 = ",".join(repr(e) for e in coord)
    sizeArray = coord
    typePE = ["PE0"] * (sizeArray[0] * sizeArray[1])
    typeConn = "HyCUBE"# 
    interMem = 1
    interIO = 1
    arch = genarch(sizeArray, typePE, typeConn, interMem, interIO)
    with open("./arch/hycube/arch.json", "w") as fout: 
        fout.write(json.dumps(arch, indent=4))
    arch = Arch("./arch/hycube/arch.json")
    with open("./arch/hycube/exampleRRG.txt", "w") as fout: 
       fout.write(arch.top().dumpGraph())
    with open("./arch/hycube/exampleFUs.txt", "w") as fout: 
        fout.write(arch.fus())
    with open("./arch/hycube/info.txt", "w") as fout: 
        fout.write(str1)
       