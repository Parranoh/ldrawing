#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "include/port_assignment.hpp"
#include "include/rectangular_dual.hpp"
#include "include/decompose.hpp"

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

l_drawing_t read_drawing(std::istream &is)
{
    l_drawing_t drawing;
    coord_t x, y;
    while (is >> x >> y)
        drawing.push_back({ x, y });
    return drawing;
}

int is_planar(const graph_t graph, const l_drawing_t drawing)
{
    struct horizontal_segment_t { coord_t y, from, to; };
    struct vertical_segment_t   { coord_t x, from, to; };
    std::vector<horizontal_segment_t> horiz;
    std::vector<vertical_segment_t> vert;
    for (const edge_t &e : graph.edges)
    {
        if (drawing[e.tail].x < drawing[e.head].x)
            horiz.push_back({ drawing[e.head].y, drawing[e.tail].x, drawing[e.head].x });
        else
            horiz.push_back({ drawing[e.head].y, drawing[e.head].x, drawing[e.tail].x });
        if (drawing[e.tail].y < drawing[e.head].y)
            vert.push_back({ drawing[e.tail].x, drawing[e.tail].y, drawing[e.head].y });
        else
            vert.push_back({ drawing[e.tail].x, drawing[e.head].y, drawing[e.tail].y });
    }
    for (const auto &h : horiz)
        for (const auto &v : vert)
            if (h.from < v.x && v.x < h.to
                    && v.from < h.y && h.y < v.to)
            {
#ifdef DEBUG_PRINT
                std::cerr << "segments h" << h.y << '[' << h.from << ',' << h.to << ']'
                    << " and v" << v.x << '[' << v.from << ',' << v.to << "] intersect"
                    << std::endl;
#endif // DEBUG_PRINT
                return 1;
            }
    return 0;
}

int main(void)
{
    graph_t graph = read_graph(std::cin);
    l_drawing_t drawing = read_drawing(std::cin);

    return is_planar(graph, drawing);
}
