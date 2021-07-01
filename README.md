# Planar L-drawings

This program implements the algorithm of [Angelini, Chaplick, Cornelsen and Da
Lozzo](https://arxiv.org/abs/2008.07834) to construct planar L-drawings of plane
bimodal triangulations. To decompose the input graph into its 4-connected
components, it does not use the algorithm cited in their paper, but rather one
inspired by the left-right planarity test.

For sampling triangulations, the algorithm of [Poulalhon and
Schaeffer](https://doi.org/10.1007/s00453-006-0114-8) is used.

## Building

```sh
cd src
make
```

To build without optimizations, with debugging symbols and debugging messages,
use `make optimize=no` instead. It is also possible to build some objects with
debugging messages and some without, for example:

```
make optimize=no port_assignment.o  # with debugging messages
make ldrawing  # all other objects without messages, and link
```

## Running

### Sampling

To sample a plane bimodal triangulation with n+2 vertices, 3n edges and 2n
triangles:

```sh
./sample-triangulation [--2-cycles] n
```

If `--2-cycles` is specified, 2-cycles will be inserted wherever
possible without destroying bimodality. These cannot be handled by the
`ldrawing` program.

The generated bitstring will be written to stderr, the graph to stdout,
according to `graph-format.md`.

### Drawing

```sh
./ldrawing [--rect-dual] [[--print-duals] --tikz]
```

If `--rect-dual` is specified, the input must be an irreducible triangulation
and will be drawn as a rectangular dual. Otherwise the input must be a
triangulation and will be drawn as a planar L-drawing.

If `--tikz` is specified, the output is a LaTeX document containing TikZ code
to be compiled, for example by `pdflatex`. Otherwise the output contains one
line per vertex in the input graph either of the format

```
x y
```

for planar L-drawings or, for rectangular duals:

```
x_min y_min x_max y_max
```

If `--print-duals` is specified in addition to `--tikz`, the rectangular duals
of the 4-connected components of the input graph are also added to the output
document.

The input graph is taken from stdin, the drawing is output to stdout, any errors
encountered are reported to stderr.

## Testing

### Correctness

```sh
../tests/test.zsh
```

This will sample 1000 graphs with 1002 vertices each, draw each of them and
check for intersections in the drawing. If any are found, the bitstring that the
graph was generated from is written to the `tests` directory and the script will
return 1.

### Timing

```sh
../tests/time.zsh
```

This will samples graphs of various sizes, draw each of them and for each graph
report the size of the graph, the time spent doing I/O, decomposing the graph
into it 4-connected components, drawing the rectangular dual for each such
component and doing the port assigment.
