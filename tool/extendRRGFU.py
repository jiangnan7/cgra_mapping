def extendFUsWithII(filename, II = 2): 
    assert II > 1
    with open(filename, "r") as fin: 
        lines = fin.readlines()
    lines = list(map(lambda x: x.strip(), lines))
    extendedLines = [[] for _ in range(II)]
    for iteration in range(II): 
        for line in lines: 
            if len(line) > 0: 
                splited = line.split()
                elements = splited[0].split(".")
                elements[0] = elements[0] + str(iteration)
                splited[0] = ".".join(elements)
                extendedLines[iteration].append(" ".join(splited))
    content = ""
    for lines in extendedLines: 
        for line in lines: 
            content += line + "\n"

    return content, extendedLines

def extendRRGWithII(filename, II = 2): 
    assert II > 1
    with open(filename, "r") as fin: 
        lines = fin.readlines()
    lines = list(map(lambda x: x.strip(), lines))
    content = ""

    verticesLines = []
    verticesLineIdx2attrs = {}
    edgeLines = []
    idx = 0
    fu2Iports = {}
    fu2Oports = {}
    for line in lines:
        if 'vertex ' == line[0:7]:
            verticesLines.append(line)
            verticesLineIdx2attrs[idx] = []
            idx+=1
        elif 'attr ' == line[0:5]:
            verticesLineIdx2attrs[idx-1].append(line)
        elif 'edge ' == line[0:5] or 'net ' == line[0:4]:
            edgeLines.append(line)

    for idx in range(len(verticesLines)):
        if('_PORT__' not in verticesLineIdx2attrs[idx][0]):
            vertexName = verticesLines[idx][7:].replace('\n','')
            fu2Iports[vertexName[5:]] = []
            fu2Oports[vertexName[5:]] = []
    for idx in range(len(verticesLines)):
        vertexName = verticesLines[idx][12:].replace('\n','')
        if('.' not in vertexName):
            continue
        fu = vertexName[:len(vertexName)-len(vertexName.split('.')[-1])-1]
        if fu in fu2Iports and '__ELEMENT_INPUT_PORT__' in verticesLineIdx2attrs[idx][0]:
            fu2Iports[fu].append(vertexName)
        elif '__ELEMENT_OUTPUT_PORT__' in verticesLineIdx2attrs[idx][0]:
            fu2Oports[fu].append(vertexName)

    for idx in range(0, len(verticesLines)):
        element = verticesLines[idx][7:].split('.')[0]
        for iteration in range(II):
            content += verticesLines[idx].replace(element, element+str(iteration), 1) + "\n"
            for attr in verticesLineIdx2attrs[idx]:
                content += attr + "\n"
    for edge in edgeLines:
        if 'edge ' == line[0:5]:
            element = edge[5:].split('.')[0]
        else:
            element = edge[4:].split('.')[0]
        for iteration in range(II):
            content += edge.replace(element, element+str(iteration), 2) + "\n"


    for idx in range(0, len(verticesLines)):
        deviceType = ""
        for attr in verticesLineIdx2attrs[idx]:
            if attr[0:14] == "attr type str ":
                deviceType = attr[14:]
        if deviceType == "mem_unit" or deviceType == "const" or deviceType == "register"  or deviceType == "io"   or deviceType == "func":
            element = verticesLines[idx][7:].split('.')[0]
            vertex = verticesLines[idx][7:]
            vertexName = vertex[5:]
            # for iteration in range(II):
            #     itFrom = iteration
            #     itTo = (iteration + 1) % II
            #     content += "edge " + vertex.replace(element, element+str(itFrom), 1) + " " + vertex.replace(element, element+str(itTo), 1) + "\n"
            # print(vertexName)
            for iteration in range(II):
                for oport in fu2Oports[vertexName]:
                    for iport in fu2Iports[vertexName]:
                        itFrom = iteration
                        itTo = (iteration + 1) % II   
                        content += ("edge " + element + str(itFrom) + '.' + oport + ' ' + element + str(itTo) + '.' + iport + "\n")           

    return content


if __name__ == "__main__": 
    import sys
    rrgname = sys.argv[1]
    funame = sys.argv[2]
    II = int(sys.argv[3])

    outrrgname = rrgname[:-4] + "_II_" + str(II) + ".txt"
    outfuname = funame[:-4] + "_II_" + str(II) + ".txt"

    content, extendedLines = extendFUsWithII(funame, II)
    with open(outfuname, "w") as fout: 
        fout.write(content)

    content = extendRRGWithII(rrgname, II)
    with open(outrrgname, "w") as fout: 
        fout.write(content)



