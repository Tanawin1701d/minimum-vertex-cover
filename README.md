![](https://img.shields.io/badge/Status-prepare_for_stage_2-orange.svg) ![](https://img.shields.io/badge/Author-Tanawin_devaveja-green.svg)
# Minimun Vertex Cover (simple optimization)

- This project is part of 2110452-HIGH PERFORMANCE ARCHITECTURE final project(university of chulalongkorn)

## What does the project obtain ?

- To provide simple solution, that may optimize minimum number of selected vertex in non-weight undirected graph.

## What had this project done ?

- use 128 bit integer(virtual) to represent state in searching space. 
- use highest covered vertex hueristic priority queue to aggresive search for solutions.
- use graph renaming to let the machine cut useless state space more quickly due to better locality.
- able to distribute workload to multi-core.
- use runAhead execution to mitigate load imbalancing.

## What is the input file look like ?
```
4 // number of nodes (node id start at 0 to n-1) 
3 // number of edge that you need to specify below.
0 1   // node number id=0 connect to id=1 
1 2
2 3

```
## What is the output file look like ?
```
2:0101
#  ^ ^--- node id=3 is selected.
#  ^--- node id=1 is selected.

```
## How to build the program ?
- I recommend you to use docker to start the program
- if you intend to run on local system fopenmp, cmake and gcc will be required.
```
 docker build -t <your name tag> <path to current directory>

```
## How I can run the program.
```
    sudo time docker run \
    -v <path to project dir>/input/:/input  \
    -v <path to project dir>/input/output/:/output \
    <your name tag> \
    prog /input/<name of input file> /output/<name of output file>

```