# Graph representation

All indices start with 1.  The program understands planar graphs as follows:

* The first line has the form `n m o` where `n` is the number of vertices, `m`
  the number of edges and `o` the number of vertices on the outer face.
* The second line has the form `v1 v2 v3 … vo` where `vi` are the vertices on
  the outer face in counter-clockwise order.
* The next `n` lines are of arbitrary content and will be used verbatim as
  labels for the vertices in the output. No escaping will done in the output.
* The next `m` lines are of the form `t h` where `t` is the tail of an edge and
  `h` the head. The first of these lines describes edge 1, the second edge 2,
  and so on.
* The next `n` lines are of the form `e1 e2 … ed` where `ej` is the `j`-th edge
  incident to `vi` in counter-clockwise order if this is the `i`-th of these
  lines.
