#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "include/port_assignment.hpp"
#include "include/rectangular_dual.hpp"
#include "include/decompose.hpp"
#include "include/io.hpp"
#include "include/timer.hpp"

graph_t read_graph(std::istream &is)
{
    graph_t out_graph;
    size_t line_num = 0;
    auto error = [&]()
    {
        std::cerr << "Error reading input on line " << line_num << std::endl;
        exit(1);
    };
    std::string line;

    /* header */
    ++line_num;
    std::getline(is, line);
    size_t num_vertices, num_edges, outer_face_degree;
    if (!(std::istringstream(line) >> num_vertices >> num_edges >> outer_face_degree))
        error();
    out_graph.vertices.clear();
    out_graph.edges.clear();
    out_graph.labels.clear();
    out_graph.outer_face.clear();
    out_graph.vertices.resize(num_vertices);
    out_graph.edges.reserve(num_edges);
    out_graph.labels.reserve(num_vertices);
    out_graph.outer_face.reserve(outer_face_degree);

    /* outer face */
    ++line_num;
    std::getline(is, line);
    std::istringstream iss(line);
    vertex_t v1;
    while (iss >> v1)
        out_graph.outer_face.push_back(v1 - 1);

    /* labels */
    for (vertex_t v = 0; v < num_vertices; ++v)
    {
        ++line_num;
        std::getline(is, line);
        out_graph.labels.push_back(line);
    }

    /* edges */
    for (size_t e = 0; e < num_edges; ++e)
    {
        ++line_num;
        std::getline(is, line);
        vertex_t tail1, head1;
        if (!(std::istringstream(line) >> tail1 >> head1))
            error();
        out_graph.edges.push_back({ tail1 - 1, head1 - 1 });
    }

    /* embedding */
    for (vertex_t v = 0; v < num_vertices; ++v)
    {
        ++line_num;
        std::getline(is, line);
        iss = std::istringstream(line);
        size_t e1;
        while (iss >> e1)
            out_graph.vertices[v].push_back(e1 - 1);
    }

    out_graph.update_neighbor_index();

    return out_graph;
}

void write_raw(std::ostream &os, const l_drawing_t &drawing)
{
    for (const auto &v : drawing)
        os << v.x << ' ' << v.y << std::endl;
}

void write_raw(std::ostream &os, const rectangular_dual_t &drawing)
{
    for (const auto &v : drawing)
        os << v.x_min << ' ' << v.y_min << ' ' << v.x_max << ' ' << v.y_max << std::endl;
}

void write_latex_header(std::ostream &os)
{
    os  << "\\documentclass{article}\n"
        << "\\usepackage{tikz}\n"
        << "\\begin{document}" << std::endl;
}

void write_tikz(std::ostream &os, const graph_t &graph, const l_drawing_t &drawing)
{
    os  << "\\resizebox{\\textwidth}{!}{\\begin{tikzpicture}\n";
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
        os  << "\\node (" << v + 1 << ") at ("
            << drawing[v].x << ',' << drawing[v].y << ") {" << graph.labels[v] << "};\n";
    for (const edge_t &e : graph.edges)
        os  << "\\draw[rounded corners] (" << e.tail + 1 << ") |- (" << e.head + 1 << ");\n";
    os << "\\end{tikzpicture}}" << std::endl;
}

void write_tikz(std::ostream &os, const graph_t &graph, const rectangular_dual_t &drawing)
{
    os  << "\\resizebox{\\textwidth}{!}{\\begin{tikzpicture}\n";
    for (vertex_t v = 0; v < drawing.size(); ++v)
        os  << "\\draw[rounded corners] (" << drawing[v].x_min << ',' << drawing[v].y_min
            << ") rectangle node {" << graph.labels[v] << "} (" << drawing[v].x_max
            << ',' << drawing[v].y_max << ");\n";
    os << "\\end{tikzpicture}}" << std::endl;
}

void write_latex_footer(std::ostream &os)
{
    os << "\\end{document}" << std::endl;
}

int main(int argc, char **argv)
{
    timer::start(timer::IO);
    graph_t graph = read_graph(std::cin);
    timer::stop(timer::IO);

    const bool tikz = (std::string(argv[argc - 1]) == std::string("--tikz"));
    if (argc >= 2 && std::string(argv[1]) == std::string("--rect-dual"))
    {
        timer::start(timer::RECT_DUAL);
        rectangular_dual_t drawing = compute_rect_dual(graph);
        timer::stop(timer::RECT_DUAL);

        timer::start(timer::IO);
        if (tikz)
        {
            write_latex_header(std::cout);
            write_tikz(std::cout, graph, drawing);
            write_latex_footer(std::cout);
        }
        else
            write_raw(std::cout, drawing);
        timer::stop(timer::IO);
    }
    else
    {
        const bool print_duals = tikz && std::string(argv[1]) == std::string("--print-duals");
        timer::start(timer::DECOMPOSE);
        four_block_tree_t four_block_tree = build_four_block_tree(graph);
        timer::stop(timer::DECOMPOSE);

        if (tikz)
            write_latex_header(std::cout);

        l_drawing_t drawing = construct_drawing(graph, four_block_tree, print_duals);

        timer::start(timer::IO);
        if (tikz)
        {
            write_tikz(std::cout, graph, drawing);
            write_latex_footer(std::cout);
        }
        else
            write_raw(std::cout, drawing);
        timer::stop(timer::IO);
    }

    if (argc >= 2 && std::string(argv[1]) == std::string("--time"))
        timer::print_times(std::cerr);

    return 0;
}
