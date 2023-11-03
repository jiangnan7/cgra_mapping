import random
import os
import random
import time
import sys
# from  subgraph_matching.GraphAnalyze import GraphAnalyze
import configparser
import subprocess as sp
import datetime as dt
import networkx as nx

def placeCore(dfg, compat):
    filename = dfg[ : len(dfg) - 3] + "log"
    fo =open(filename,"wb")
    proPlace = sp.Popen(['./build/place','place_matching', dfg, compat], stderr=fo)
    timeplaceStart = dt.datetime.now()
    finished = False
    countQuited = 0
    while not finished and countQuited < 1:
        time.sleep(random.random() * 0.2)
        if not proPlace.poll() is None:
            countQuited += 1
            # timeplace = dt.datetime.now() - timeplaceStart
            if proPlace.poll() == 0:
                finished = True
                break
    fo.flush()
    fo.close()
    if not finished: 
        print("[", dfg, "]"," FAILED ! CHANGE THE GIVEN II OR ARCHITCTURE")
        sys.exit(1)


def defineDFG(dfg):
    G = nx.Graph()            
    dot = set()
    dfg = dfg.replace(".txt", ".dot")
    dot.add(dfg)
    result = GraphAnalyze(dot)
    return result.nx_graph()

def main(dfg, compat, pred = False):

    config = configparser.ConfigParser()
    config.read("./arch/arch.ini")
    if not pred:
        placeCore(dfg, compat)
    else:
        os.system('python3 -m subgraph_matching.analyze.alignment --dfg %s'%(dfg))
        placeCore(dfg, compat)
        
if __name__ == '__main__':

    dfg = sys.argv[1]
    compat = sys.argv[2]

    import matplotlib.pyplot as plt
    main(dfg, compat, pred = False)

