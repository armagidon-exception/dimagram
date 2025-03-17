# Dimagram

Generate diagrams from text input.

## Description

Dimagram (the name will change at some point) is a tool to create diagrams from text input.
All you have to do is write a text file with markup in a super simple markup language and compile this file to an image.

## Features

- Supports for ER diagrams in Chen notation
- Generates diagrams with evenly spaces nodes
- Renders diagrams from markup language

## Limitations

- Graphs induced by diagrams by conform to planarity, i.e. there's gotta be a way to draw it without edges crossing. If you try to compile a graph that does not conform to planarity compiler will give you an error.
- Diagrams must not be disconnected, i.e. there must be a path from one node to the other by links.
- Only ER diagrams are supported so far.
- No labels on the edges or near terminals of nodes.
- No key attributes out of the box
- No associative entities out of the box
- No weak entities out of the box
- Nodes are rendered in raster mode

## Notes

Some node types are not supported out of the box. But you can use your own image of the node.

## Building from sources

### Prerequisites

- `igraph`
- `cairo`
- `criterion`
- `make`
- `gcc`
- Unix like OS

### Building

To build in release mode

```
make build-release
```

To build in test mode (the compiled program might not output file correctly, idk why)

```
make build-test
```

## How to use

Dimagram supports ED diagrams in Chen's notation.
Simple markup language which we shall call elementary graphics language has the following notation
- To define an entity you use this notation
```
entity <identifier> "<label>"
```
- To define a relationship you use this notatation
```
relationship <identifier> "<label>"
```
- To define an attribute you you this notation
```
attribute <identifier> "<label>"
```
- To define any other node with you own image
```
node <identifier> "<filename>"
```

!! Quotation marks are **mandatory**
!! Identifier is a string of alphanumeric characters with underscores and no spaces or any other blank characters.
!! Images in node must be of raster type

- To compile the file you use
```sh
<path to dimagram> -i <input> -o <output>
```
- Or you could run it like this
```sh
<path to dimagram> < <input> > <output>
```

## Inspirations

- [Graphviz](https://www.graphviz.org/)
- [PlantUML](https://plantuml.com/)

## Used libraries

- [Log.c](https://github.com/rxi/log.c/) - logging library for C
- [igraph](https://igraph.org/c/) - Graph library for C
- [Cairo](https://www.cairographics.org/) - Graphics library for C
- [zhash-c](https://github.com/zfletch/zhash-c) - Hashmap implementation for C
- [Boyer-Myrvold Planarity Tester](https://jgaa.info/index.php/jgaa/article/view/paper91) - Original Boyer-Myrvold implementation by the authors of the original paper. (I tried to write my own one, but it was too hard ;( )

## References

- Harel, D., Sardas, M. An Algorithm for Straight-Line Drawing of Planar Graphs . Algorithmica 20, 119–135 (1998). https://doi.org/10.1007/PL00009189
- Boyer, J., & Myrvold, W. (2004). On the Cutting Edge: Simplified O(n) Planarity by Edge Addition. Journal of Graph Algorithms and Applications, 8(3), 241–273. https://doi.org/10.7155/jgaa.00091
- Chen's notation Wikipedia. https://en.wikipedia.org/wiki/Entity%E2%80%93relationship_model
