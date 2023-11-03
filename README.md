# cgra_mapping


## Getting started
### Requirement:
* Ubuntu (we have tested Ubuntu 18.04, Ubuntu 20.04)
* g++

### How to install


1. CGRA Mapping
You can compile directly. 
    ```sh
    source ./install.sh
    ```

## Running Example:
We provide an example to map DFG using GRAFT. 

### Map DFG using GRAFT
We use 4x4 CGRA for the follwing examples. Here, we show how to map one DFG with GRAFT. 

Run the fast mapper:
```./build/place place_matching ./benchmarks/polybench/cholesky/my_graph_loop_DFG.txt ./benchmarks/polybench/cholesky/my_graph_loop_compat.txt ```

We provide three mapping orders for graph traversal (TVS), forward topology (TOPO_Forward), and reverse topology (TOPO), for example: 
```./build/place place_matching ./benchmarks/cgrame/conv2/conv2_DFG.txt ./benchmarks/cgrame/conv2/conv2_compat.txt TOPO```

Or run the complete process:
```
python3 ./run.py ./benchmarks/polybench/cholesky/my_graph_loop_DFG.txt   ./benchmarks/polybench/cholesky/my_graph_loop_compat.txt
```
The experimental results can be viewed in the *polybench*.

## Tools

### 1. ConvertDFG
We can convert the DFG to a unified graph-based IR. 

    python3 ./tool/convertDFG.py ./benchmarks/express/horner_bezier/horner_bezier.dot
    python ./benchmarks/convertDFG.py ./benchmarks/benchmark_morpher/jacobi-2d/kernel_PartPredDFG_simple.dot morpher
    
Our default attribute is *optype* , for example ```attr optype str add.in0```. 
Users can add custom DFG attributes that affect P&R. 

### 2. GenerateRRG/ConvertRRG
- In *arch/hycube*, RRG can be generated directly using *genarch*.

- We convert the CGRA-ME architecture to RRG and extend it in time. 

    ```
    python3 ./tool/convertRRG.py ./arch/cgrame/4_4/cgrame44.log
    python3 ./tool/extendRRGFU.py ./arch/cgrame/4_4/cgrame44_RRG.txt ./arch/cgrame/4_4/cgrame44_FUs.txt 2
    ```
    Our architecture is graph-based IR, and the vertex  of RRG can be represented as 
    ```
    vertex CGRA.block_2_2.in2
    attr type str __MODULE_INPUT_PORT__
    attr device str CGRA.block_2_2.in2
    ```
    You can add custom attributes, more architectural information or constraints.

### 3. Visualization
```
python3 ./tool/drawGraph.py ./arch/hycube/exampleRRG.txt
```
