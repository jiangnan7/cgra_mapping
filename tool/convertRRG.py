def readArchFile(filename): 
    with open(filename, "r") as fin: 
        lines = fin.readlines()
    lines = list(map(lambda x: x.strip(';\r\n'), lines))
    while lines[0] != "[ARCHGRAPH]": 
        lines = lines[1:]
    lines = lines[2:]
    while lines[0] != "CGRA": 
        lines = lines[1:]
    #list(map(lambda x: print(x), lines))
    return lines

def parseModule(lines): 

    def convertPortName(name): 
        if name == "in_a": 
            return "in0"
        elif name == "in_b": 
            return "in1"
        elif name == "in": 
            return "in0"
        elif name == "out": 
            return "out0"
        elif name == "data_in": 
            return "in0"
        elif name == "data_out": 
            return "out0"
        elif name == "addr": 
            return "in1"
        return name
    
    def convertVertexName(vertex): 
        splited = vertex.split(".")
        prefix = ".".join(splited[:-1])
        postfix = convertPortName(splited[-1])
        return prefix + "." + postfix
    
    def getSubName(root, name): 
        if name[:len(root)] == root: 
            return name.split(".")[1]
        return name

    def getPrefix(name, c):
        tmp = name.split(c)
        if len(tmp) == 1:
            return tmp
        return name[0:-len(tmp[-1])-1]
    
    def getPostfix(name, c):
        tmp = name.split(c)
        if len(tmp) == 1:
            return tmp
        return tmp[-1]
    
    vertices = set()
    edges = []
    usedEdges = set()
    
    index = 0
    prefix = lines[index]
    stackPrefix = [prefix]
    while len(stackPrefix) > 0 and index < len(lines)-1: 
        index += 1
        splited = lines[index].strip().split("->")
        if len(splited) == 2: 
            vertexFrom = prefix + "." + getSubName(prefix.split(".")[-1], splited[0])
            vertexTo   = prefix + "." + getSubName(prefix.split(".")[-1], splited[1])
            vertexFrom = convertVertexName(vertexFrom)
            vertexTo   = convertVertexName(vertexTo)
            if not vertexFrom in vertices: 
                vertices.add(vertexFrom)
            if not vertexTo in vertices: 
                vertices.add(vertexTo)
            edgeName = vertexFrom + "->" + vertexTo
            if 'block_' in vertexFrom.split(".")[-2] and 'block_' in vertexTo.split(".")[-2]\
            and (('out' in vertexFrom.split(".")[-1] and 'out' in vertexTo.split(".")[-1])\
            or ('in' in vertexFrom.split(".")[-1] and 'in' in vertexTo.split(".")[-1])):
                print("WARN: Ignore edge,", edgeName)
            elif not edgeName in usedEdges: 
                edges.append((vertexFrom, vertexTo))
                usedEdges.add(edgeName)
            else: 
                print("Warning: Duplicated edge,", edgeName)
        if len(splited) == 1:
            if 'block' in splited[0]:
                if 'block' in stackPrefix[-1]:
                    stackPrefix.pop()
                stackPrefix.append(splited[0])
            prefix = ""
            for item in stackPrefix:
                if prefix:
                    prefix += "." + item
                else:
                    prefix += item

    edgesMap = {}
    for edge in edges:
        if edge[1] not in edgesMap:
            edgesMap[edge[1]] = []
        edgesMap[edge[1]].append(edge[0])
    for edgeTo in edgesMap:
        if len(edgesMap[edgeTo]) > 1:
            print("WARN: multiple edgeFrom to an edgeTo, may cause error:")
            for edgeFrom in edgesMap[edgeTo]:
                print(" " + edgeFrom + "->" + edgeTo)   
    
    addedVertices = set()
    addedEdges = []
    for vertex in vertices:
        name = getPrefix(vertex, '.')
        final = getPostfix(name, '.')
        if 'block_' in final:
            continue
        if name in vertices:
            print("WARN: Ignore adding vertex " + name + " to vertices" )
            continue
        port = getPostfix(vertex, '.')
        if 'in' in port or 'bidir' in port:
            if (vertex + "->" + name) not in usedEdges:
                addedEdges.append([vertex, name])
            else:
                print("WARN: Ignore adding edge [" + vertex + " " + name + "] to edges" )
        elif 'out' in port:
            if (name + "->" + vertex) not in usedEdges:
                addedEdges.append([name, vertex])
            else:
                print("WARN: Ignore adding edge [" + name + " " + vertex + "] to edges" )
        else:
            print("WARN: Ignore adding edge [" + vertex + " " + name + "] to edges" )
        addedVertices.add(name)
    for vertex in addedVertices:
        vertices.add(vertex)
    for edge in addedEdges:
        edges.append(edge)
    
    MUXs = {}
    FUs = {}
    special = {}
    inportsUnits = {}
    outportsUnits = {}
    for vertex in vertices:
        postfix = getPostfix(vertex, '.')
        if ('in' in postfix or 'out' in postfix or 'bidir' in postfix) and ('reg_in' not in postfix and 'reg_out' not in postfix):
            continue
        if 'mux' in postfix or 'mem_port' in postfix or 'reg_in' in postfix or 'reg_out' in postfix or 'reg' in vertex or 'rf' in postfix:
            # print(vertex , postfix) 
            MUXs[vertex] = postfix
        # elif postfix in set(["func", "OE", "const", "rf", "mem_unit", "register0", "register1", "register2", "register3", "io"]):
        elif postfix in set(["func",  "const", "mem_unit" , "OE" , "io"]):#
            # if postfix in set(["register0", "register1", "register2", "register3"]):
            #     FUs[vertex] = "register"
            # else:
            #     FUs[vertex] = postfix
            FUs[vertex] = postfix
        else:
            print("WARN: Ignore vertex " + vertex)
    for vertex in vertices:
        postfix = getPostfix(vertex, '.')
        if vertex in FUs or vertex in MUXs or vertex in special:
            continue
        if 'block_' in getPostfix(getPrefix(vertex, '.'), '.'):
            if 'in' in postfix:
                special[vertex] = "block_port_in"
            elif 'out' in postfix or 'bidir' in postfix:
                special[vertex] = "block_port_out"

    for vertex in vertices:
        if vertex in FUs or vertex in MUXs or vertex in special:
            continue
        prefix = getPrefix(vertex, '.')
        if prefix not in FUs and prefix not in MUXs and prefix not in special:
            print("WARN: Ignore vertex " + vertex + " cause of failed to classify")
            continue
        postfix = getPostfix(vertex, '.')
        if prefix not in inportsUnits:
            inportsUnits[prefix] = set()
            outportsUnits[prefix] = set()
        if 'in' in postfix:
            inportsUnits[prefix].add(postfix)
        elif 'out' in postfix or 'bidir' in postfix:
            outportsUnits[prefix].add(postfix)
        else:
            print("WARN: Ignore port " + postfix + " for " + vertex)

    fus = ""
    for fu in FUs.keys(): 
        fus += fu + " " + FUs[fu] + " " + FUs[fu] + " inputs "
        for inport in inportsUnits[fu]: 
            fus += inport + " "
        fus += "outputs "
        for outport in outportsUnits[fu]: 
            fus += outport + " "
        fus += "\n"

    content = ""
    for vertex in vertices: 
        content += "vertex " + vertex + "\n"
        prefix = getPrefix(vertex, '.')
        postfix = getPostfix(vertex, '.')
        if vertex in FUs: 
            content += "attr type str " + FUs[vertex] + "\n"
            content += "attr device str " + FUs[vertex] + "\n"
        elif prefix in FUs:
            if postfix in inportsUnits[prefix]:
                content += "attr type str __ELEMENT_INPUT_PORT__\n"
                content += "attr device str " + FUs[prefix] + "." + postfix + "\n"
            elif postfix in outportsUnits[prefix]:
                content += "attr type str __ELEMENT_OUTPUT_PORT__\n"
                content += "attr device str " + FUs[prefix] + "." + postfix + "\n"
            else:
                print("WARNING: unknown vertex:", vertex + ';', "skip [code 0]")
        elif vertex in MUXs:
            content += "attr type str MUX\n"
            content += "attr device str " + "MUX_" + MUXs[vertex] + "\n"
        elif prefix in MUXs:
            if postfix in inportsUnits[prefix]:
                content += "attr type str __ELEMENT_INPUT_PORT__\n"
                content += "attr device str " + MUXs[prefix] + "." + postfix + "\n"
            elif postfix in outportsUnits[prefix]:
                content += "attr type str __ELEMENT_OUTPUT_PORT__\n"
                content += "attr device str " + MUXs[prefix] + "." + postfix + "\n"
            else:
                print("WARNING: unknown vertex:", vertex + ';', "skip [code 1]")
        elif vertex in special:
            if special[vertex] == "block_port_in":
                content += "attr type str __MODULE_INPUT_PORT__\n"
                content += "attr device str " + vertex + "\n"
            elif special[vertex] == "block_port_out":
                content += "attr type str __MODULE_OUTPUT_PORT__\n"
                content += "attr device str " + vertex + "\n"
            else:
                print("WARNING: unknown vertex:", vertex + ';', "skip [code 2]")
        else:
            print("WARNING: unknown vertex:", vertex + ';', "skip [code 3]")
    for edge in edges: 
        content += "edge " + edge[0] + " " + edge[1] + "\n"


    return content, fus, vertices, edges

if __name__ == "__main__": 
    import sys
    filename = sys.argv[1]
    path = sys.argv[2]
    lines = readArchFile(filename)
    # for line in lines:
    #     print(line)
    # print("---------------------------------------------------")
    content, fus, vertices, edges = parseModule(lines)
    # print(content)
    with open(path + "/" + filename.split("/")[-1].split(".")[0]+"_RRG.txt", "w") as fout: 
        fout.write(content)
    # print(fus)
    with open(path + "/" +  filename.split("/")[-1].split(".")[0]+"_FUs.txt", "w") as fout: 
        fout.write(fus)
