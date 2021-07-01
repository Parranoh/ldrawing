#pragma once

#include <vector>
#include <string>
#include <functional>

typedef size_t vertex_t;
typedef unsigned int coord_t;
typedef unsigned char port_assignment_t;
/* port_assignment_t pa;
 * if (pa & 0b01) then west else east
 * if (pa & 0b10) then north else south
 * if (pa & 0b100) then do not assign to pa */

struct edge_t {
    vertex_t tail, head;
    size_t index_at_tail = 0, index_at_head = 0;
};

struct graph_t {
    std::vector<edge_t> edges;
    std::vector<std::vector<size_t>> vertices; // indices into edges
    std::vector<vertex_t> outer_face;
    std::vector<std::string> labels;
    size_t num_vertices()     const { return vertices.size(); }
    size_t degree(vertex_t v) const { return vertices[v].size(); }
    size_t num_edges()        const { return edges.size(); }

    vertex_t neighbor(vertex_t v, size_t edge_ix) const
    {
        auto &e = edges[vertices[v][edge_ix]];
        if (v == e.head)
            return e.tail;
        else
            return e.head;
    }

    void for_neighbors(vertex_t v, const std::function<void(vertex_t, size_t)> &callback) const
    {
        for (size_t i = 0; i < degree(v); ++i)
        {
            const vertex_t n = neighbor(v, i);
            callback(n, vertices[v][i]);
        }
    }

    void for_neighbors(vertex_t v, const std::function<void(vertex_t)> &callback) const
    {
        for_neighbors(v, [&](vertex_t n, size_t) { callback(n); });
    }

    size_t neighbor_index(vertex_t v, const edge_t &e) const
    {
        if (v == e.head)
            return e.index_at_head;
        else
            return e.index_at_tail;
    }

    size_t neighbor_index(vertex_t v, size_t e) const
    { return neighbor_index(v, edges[e]); }

    void update_neighbor_index()
    {
        for (vertex_t v = 0; v < num_vertices(); ++v)
            for (size_t i = 0; i < degree(v); ++i)
            {
                edge_t &e = edges[vertices[v][i]];
                if (v == e.head)
                    e.index_at_head = i;
                else
                    e.index_at_tail = i;
            }
    }
};

struct position_t {
    coord_t x, y;
};

struct rectangle_t {
    coord_t x_min, y_min, x_max, y_max;
};

typedef std::vector<rectangle_t> rectangular_dual_t;
typedef std::vector<position_t> l_drawing_t;

struct four_connected_component_t : public graph_t {
    std::vector<size_t> original_edge{};
    std::vector<size_t> designated_face{};
};
typedef std::vector<four_connected_component_t> four_block_tree_t;
