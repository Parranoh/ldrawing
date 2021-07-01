#pragma once

#include <iostream>
#include <sstream>
#include "graph.hpp"

graph_t read_graph(std::istream &);

void write_raw(std::ostream &, const l_drawing_t &);

void write_raw(std::ostream &, const rectangular_dual_t &);

void write_latex_header(std::ostream &);

void write_tikz(std::ostream &, const graph_t &graph, const l_drawing_t &);

void write_tikz(std::ostream &, const graph_t &graph, const rectangular_dual_t &);

void write_latex_footer(std::ostream &);
