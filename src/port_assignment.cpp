#include <iostream>
#include "include/graph.hpp"
#include "include/rectangular_dual.hpp"
#include "include/port_assignment.hpp"
#include "include/debug_print.hpp"
#include "include/io.hpp"
#include "include/timer.hpp"

enum direction_t : size_t { RIGHT = 0, TOP = 1, LEFT = 2, BOTTOM = 3 };
std::ostream &operator<<(std::ostream &os, direction_t dir)
{
    switch (dir)
    {
        case direction_t::RIGHT:
            os << "right";
            break;
        case direction_t::TOP:
            os << "top";
            break;
        case direction_t::LEFT:
            os << "left";
            break;
        default:
            os << "bottom";
            break;
    }
    return os;
}

/* H_SHAPE: TTT
 *          X W
 *          SSS
 * cases a, d, h, y use this
 *
 * LONG_SINK: TXW
 *            T W
 *            TSS
 * case b uses this
 *
 * T_SHAPE: TXW
 *          T W
 *          SSS
 * cases e, g, x use this
 *
 * LONG_SOURCE: WXS
 *              W S
 *              TTS
 * case f uses this
 */
enum class outer_face_t : char { H_SHAPE, LONG_SINK, T_SHAPE, LONG_SOURCE };

void get_edge_info(const rectangular_dual_t &dual, const edge_t &edge, vertex_t vertex,
        vertex_t &out_other, bool &out_outgoing, direction_t &out_direction)
{
    out_outgoing = edge.tail == vertex;
    out_other = out_outgoing ? edge.head : edge.tail;
    if (dual[vertex].x_max == dual[out_other].x_min && dual[vertex].x_max < dual[out_other].x_max)
        out_direction = direction_t::RIGHT;
    else if (dual[vertex].y_max == dual[out_other].y_min && dual[vertex].y_max < dual[out_other].y_max)
        out_direction = direction_t::TOP;
    else if (dual[vertex].x_min == dual[out_other].x_max && dual[vertex].x_min > dual[out_other].x_min)
        out_direction = direction_t::LEFT;
    else
        out_direction = direction_t::BOTTOM;
}

void port_assignment(const four_connected_component_t &graph, const rectangular_dual_t &dual, port_assignment_t *out_pa)
{
    enum class switch_t { ANY, CLOCKWISE, COUNTER_CLOCKWISE };
    std::vector<std::pair<vertex_t, switch_t>> postponed_vertices;
    for (vertex_t tentative_v = 0; tentative_v < graph.num_vertices(); tentative_v += postponed_vertices.empty())
    {
        vertex_t v = tentative_v;
        switch_t switch_direction = switch_t::ANY;
        if (!postponed_vertices.empty())
        {
            v = postponed_vertices.back().first;
            switch_direction = postponed_vertices.back().second;
            postponed_vertices.pop_back();
        }

        DEBUG_PRINT("considering vertex " << graph.labels[v]);
        size_t num_neighbors = graph.degree(v);
        bool *orientations = new bool[num_neighbors];
        /* find sections of edges that go through each side of the rectangle */
        size_t first_edge[] = { num_neighbors, num_neighbors, num_neighbors, num_neighbors };
        size_t num_edges_in_direction[] = { 0, 0, 0, 0 };
        coord_t first_right_y_min = 0, first_top_x_max = 0, first_left_y_max = 0, first_bottom_x_min = 0;
        for (size_t edge_ix = 0; edge_ix < num_neighbors; ++edge_ix)
        {
            DEBUG_PRINT("considering edge " << graph.labels[graph.edges[graph.vertices[v][edge_ix]].tail]
                << "→" << graph.labels[graph.edges[graph.vertices[v][edge_ix]].head]);
            vertex_t other;
            direction_t direction;
            get_edge_info(dual, graph.edges[graph.vertices[v][edge_ix]], v, other, orientations[edge_ix], direction);
            switch (direction)
            {
            case direction_t::RIGHT:
                DEBUG_PRINT((orientations[edge_ix] ? "outgoing" : "incoming") << ", right");
                ++num_edges_in_direction[direction_t::RIGHT];
                if (first_edge[direction_t::RIGHT] != num_neighbors && first_right_y_min < dual[other].y_max)
                    break;
                DEBUG_PRINT("new optimum");
                first_edge[direction_t::RIGHT] = edge_ix;
                first_right_y_min = dual[other].y_min;
                break;
            case direction_t::TOP:
                DEBUG_PRINT((orientations[edge_ix] ? "outgoing" : "incoming") << ", top");
                ++num_edges_in_direction[direction_t::TOP];
                if (first_edge[direction_t::TOP] != num_neighbors && first_top_x_max > dual[other].x_min)
                    break;
                DEBUG_PRINT("new optimum");
                first_edge[direction_t::TOP] = edge_ix;
                first_top_x_max = dual[other].x_max;
                break;
            case direction_t::LEFT:
                DEBUG_PRINT((orientations[edge_ix] ? "outgoing" : "incoming") << ", left");
                ++num_edges_in_direction[direction_t::LEFT];
                if (first_edge[direction_t::LEFT] != num_neighbors && first_left_y_max > dual[other].y_min)
                    break;
                DEBUG_PRINT("new optimum");
                first_edge[direction_t::LEFT] = edge_ix;
                first_left_y_max = dual[other].y_max;
                break;
            default:
                DEBUG_PRINT((orientations[edge_ix] ? "outgoing" : "incoming") << ", bottom");
                ++num_edges_in_direction[direction_t::BOTTOM];
                if (first_edge[direction_t::BOTTOM] != num_neighbors && first_bottom_x_min < dual[other].x_max)
                    break;
                DEBUG_PRINT("new optimum");
                first_edge[direction_t::BOTTOM] = edge_ix;
                first_bottom_x_min = dual[other].x_min;
                break;
            }
        }

        /* every bit except for the msb represents one section of edges;
         * 0 for incoming, 1 for outgoing; counter-clockwise order corresponds
         * to more to less significant */
        unsigned char directions[] = { 0b1, 0b1, 0b1, 0b1 };
        auto get_directions = [&](direction_t current) -> void
        {
            for (size_t i = 0; i < num_edges_in_direction[current]; ++i)
            {
                const size_t edge_ix = (first_edge[current] + i) % num_neighbors;
                if (directions[current] == 0b1 || bool(directions[current] & 0b1) != orientations[edge_ix])
                {
                    directions[current] <<= 1;
                    if (orientations[edge_ix])
                        directions[current] |= 0b1;
                }
            }
            DEBUG_PRINT(graph.labels[v] << "'s " << current << " side has directions " << int(directions[current]));
        };
        get_directions(direction_t::RIGHT);
        get_directions(direction_t::TOP);
        get_directions(direction_t::LEFT);
        get_directions(direction_t::BOTTOM);

        const port_assignment_t canonical_assignment[] = { 0b00, 0b10, 0b11, 0b01 };

        /* canonical ports for mono-directed sides */
        auto assign_mono_directed_sides = [&](direction_t current) -> bool // returns false if assignment has to be postponed
        {
            if (!(directions[current] & 0b1100))
            {
                /* do not assign if v participates in extra rule, unless master
                 * has already been assigned */
                auto check_for_extra_rule = [&](size_t index_of_master, int dir /* ±1 */) -> bool
                {
                    if ((current % 2 == 0) ^ (dir > 0) ^ (orientations[index_of_master])) // switch at master is canonical
                        return false;
                    const vertex_t virtual_vertex = graph.neighbor(v, (num_neighbors + index_of_master + dir) % num_neighbors);
                    if (graph.degree(virtual_vertex) == 1)
                    {
                        const edge_t &edge_to_neighbor = graph.edges[graph.vertices[v][(num_neighbors + index_of_master + 2 * dir) % num_neighbors]];
                        vertex_t neighbor;
                        size_t index_at_neighbor;
                        if (v == edge_to_neighbor.tail)
                        {
                            neighbor = edge_to_neighbor.head;
                            index_at_neighbor = edge_to_neighbor.index_at_head;
                        }
                        else
                        {
                            neighbor = edge_to_neighbor.tail;
                            index_at_neighbor = edge_to_neighbor.index_at_tail;
                        }
                        const vertex_t virtual_vertex_of_neighbor = graph.neighbor(neighbor, (graph.degree(neighbor) + index_at_neighbor - dir) % graph.degree(neighbor));
                        if (graph.degree(virtual_vertex_of_neighbor) == 1)
                        {
                            return (dual[virtual_vertex].x_min == dual[virtual_vertex].x_max
                                        && dual[virtual_vertex].x_max == dual[virtual_vertex_of_neighbor].x_min
                                        && dual[virtual_vertex_of_neighbor].x_min == dual[virtual_vertex_of_neighbor].x_max)
                                    || (dual[virtual_vertex].y_min == dual[virtual_vertex].y_max
                                        && dual[virtual_vertex].y_max == dual[virtual_vertex_of_neighbor].y_min
                                        && dual[virtual_vertex_of_neighbor].y_min == dual[virtual_vertex_of_neighbor].y_max);
                        }
                    }
                    return false;
                };
                unsigned char mono_directed_switch = 0b00;
                if (check_for_extra_rule((num_neighbors + first_edge[current] + num_edges_in_direction[current] - 1) % num_neighbors, 1)
                        || check_for_extra_rule(first_edge[current] % num_neighbors, -1))
                {
                    if (switch_direction == switch_t::ANY)
                        return false;
                    else if ((current % 2 == 0)
                            ^ bool(directions[current] & 0b1)
                            ^ (switch_direction == switch_t::COUNTER_CLOCKWISE))
                    {
                        DEBUG_PRINT("switch due to extra rule for:");
                        mono_directed_switch = 0b11;
                    }
                }

                DEBUG_PRINT(current << " side of vertex " << graph.labels[v] << " is mono-directed");
                for (size_t i = 0; i < num_edges_in_direction[current]; ++i)
                {
                    const size_t edge_ix = (first_edge[current] + i) % num_neighbors;
                    if (!(out_pa[graph.original_edge[graph.vertices[v][edge_ix]]] & 0b100))
                        out_pa[graph.original_edge[graph.vertices[v][edge_ix]]]
                            |= (canonical_assignment[current] ^ mono_directed_switch) & (orientations[edge_ix] ? 0b10 : 0b01);
                }
            }
            return true;
        };
        if (!assign_mono_directed_sides(direction_t::RIGHT)
                || !assign_mono_directed_sides(direction_t::TOP)
                || !assign_mono_directed_sides(direction_t::LEFT)
                || !assign_mono_directed_sides(direction_t::BOTTOM))
            continue; // skip v to come back later

        /* port assingment for 3-directed sides */
        auto assign_3_directed_sides = [&](direction_t current, unsigned char pattern) -> void
        {
            if (directions[current] == pattern)
            {
                DEBUG_PRINT(current << " side of vertex " << graph.labels[v] << " is 3-directed. clockwise");
                /* clockwise switch */
                port_assignment_t first_third = 0b11;
                for (size_t i = 0; i < num_edges_in_direction[current]; ++i)
                {
                    const size_t edge_ix = (first_edge[current] + i) % num_neighbors;
                    if (orientations[edge_ix] == bool(pattern & 0b010))
                        first_third = 0b00;
                    if (!(out_pa[graph.original_edge[graph.vertices[v][edge_ix]]] & 0b100))
                        out_pa[graph.original_edge[graph.vertices[v][edge_ix]]]
                            |= (canonical_assignment[current] ^ first_third) & (orientations[edge_ix] ? 0b10 : 0b01);
                }
            }
            else if (directions[current] == (pattern ^ 0b0111))
            {
                DEBUG_PRINT(current << " side of vertex " << graph.labels[v] << " is 3-directed. counter-clockwise");
                /* counter-clockwise switch */
                /* will be 000 at first, then 100, then 011 */
                port_assignment_t last_third = 0b000;
                for (size_t i = 0; i < num_edges_in_direction[current]; ++i)
                {
                    const size_t edge_ix = (first_edge[current] + i) % num_neighbors;
                    if (orientations[edge_ix] == bool((pattern ^ 0b0111) & 0b010))
                        last_third = 0b100;
                    else if (last_third)
                        last_third = 0b011;
                    if (!(out_pa[graph.original_edge[graph.vertices[v][edge_ix]]] & 0b100))
                        out_pa[graph.original_edge[graph.vertices[v][edge_ix]]]
                            |= (canonical_assignment[current] ^ last_third) & (orientations[edge_ix] ? 0b10 : 0b01);
                }
            }
        };
        assign_3_directed_sides(direction_t::RIGHT, 0b1010);
        assign_3_directed_sides(direction_t::TOP, 0b1101);
        assign_3_directed_sides(direction_t::LEFT, 0b1010);
        assign_3_directed_sides(direction_t::BOTTOM, 0b1101);

        /* bi-directed sides, canonical switches */
        auto assign_canonical_switches = [&](direction_t current, unsigned char pattern) -> void
        {
            if (directions[current] == pattern)
            {\
                DEBUG_PRINT(current << " side of vertex " << graph.labels[v] << " is bi-directed. canonical");
                for (size_t i = 0; i < num_edges_in_direction[current]; ++i)
                {
                    const size_t edge_ix = (first_edge[current] + i) % num_neighbors;
                    if (!(out_pa[graph.original_edge[graph.vertices[v][edge_ix]]] & 0b100))
                        out_pa[graph.original_edge[graph.vertices[v][edge_ix]]]
                            |= canonical_assignment[current] & (orientations[edge_ix] ? 0b10 : 0b01);
                }
            }
        };
        assign_canonical_switches(direction_t::RIGHT, 0b110);
        assign_canonical_switches(direction_t::TOP, 0b101);
        assign_canonical_switches(direction_t::LEFT, 0b110);
        assign_canonical_switches(direction_t::BOTTOM, 0b101);

        /* bi-directed sides, unpleasant switches */
        auto assign_unpleasant_switches = [&](direction_t current, unsigned char pattern) -> void
        {
            if (directions[current] == pattern)
            {
                const size_t next = (current + 1) % 4, prev = (current + 3) % 4;
                DEBUG_PRINT(current << " side of vertex " << graph.labels[v] << " is bi-directed");
                bool v_is_master_in_extra_rule = false;
                switch (switch_direction)
                {
                    case switch_t::CLOCKWISE:
                        goto lbl_clockwise;
                    case switch_t::COUNTER_CLOCKWISE:
                        goto lbl_counter_clockwise;
                    default: // switch_t::ANY
                        size_t ix_edge_to_left_neighbor = first_edge[current];
                        size_t ix_edge_to_right_neighbor;
                        for (size_t i = 1; i < num_edges_in_direction[current]; ++i)
                        {
                            ix_edge_to_right_neighbor = (first_edge[current] + i) % num_neighbors;
                            if (orientations[ix_edge_to_right_neighbor] == bool(directions[current] & 0b001))
                                break; // the switch is from left to right neighbor
                            ix_edge_to_left_neighbor = ix_edge_to_right_neighbor;
                        }
                        const edge_t &edge_to_left_neighbor = graph.edges[graph.vertices[v][ix_edge_to_left_neighbor]];
                        const edge_t &edge_to_right_neighbor = graph.edges[graph.vertices[v][ix_edge_to_right_neighbor]];
                        vertex_t left_neighbor, right_neighbor;
                        size_t index_at_left, index_at_right;
                        if (directions[current] & 0b001) // edge to left is incoming, to right outgoing
                        {
                            left_neighbor = edge_to_left_neighbor.tail;
                            index_at_left = edge_to_left_neighbor.index_at_tail;
                            right_neighbor = edge_to_right_neighbor.head;
                            index_at_right = edge_to_right_neighbor.index_at_head;
                        }
                        else
                        {
                            left_neighbor = edge_to_left_neighbor.head;
                            index_at_left = edge_to_left_neighbor.index_at_head;
                            right_neighbor = edge_to_right_neighbor.tail;
                            index_at_right = edge_to_right_neighbor.index_at_tail;
                        }
                        vertex_t virtual_vertex_of_left = graph.neighbor(left_neighbor, (index_at_left + 1) % graph.degree(left_neighbor));
                        vertex_t virtual_vertex_of_right = graph.neighbor(right_neighbor, (graph.degree(right_neighbor) + index_at_right - 1) % graph.degree(right_neighbor));
                        if (graph.degree(virtual_vertex_of_left) == 1 && graph.degree(virtual_vertex_of_right) == 1)
                        {
                            const bool colinear = (dual[virtual_vertex_of_left].x_min == dual[virtual_vertex_of_left].x_max
                                        && dual[virtual_vertex_of_left].x_max == dual[virtual_vertex_of_right].x_min
                                        && dual[virtual_vertex_of_right].x_min == dual[virtual_vertex_of_right].x_max)
                                    || (dual[virtual_vertex_of_left].y_min == dual[virtual_vertex_of_left].y_max
                                        && dual[virtual_vertex_of_left].y_max == dual[virtual_vertex_of_right].y_min
                                        && dual[virtual_vertex_of_right].y_min == dual[virtual_vertex_of_right].y_max);
                            if (colinear)
                            {
                                DEBUG_PRINT(graph.labels[v] << " is master of " << graph.labels[left_neighbor] << " and " << graph.labels[right_neighbor] << " in extra rule");
                                v_is_master_in_extra_rule = true;
                                postponed_vertices.emplace_back(left_neighbor, switch_t::ANY);
                                postponed_vertices.emplace_back(right_neighbor, switch_t::ANY);
                            }
                        }
                }
                if (directions[next] == (pattern ^ 0b011) || directions[prev] == (pattern ^ 0b011)
                        || directions[next] == (0b010 | (pattern & 0b001)))
                {
                    if (v_is_master_in_extra_rule)
                    {
                        postponed_vertices[0].second = switch_t::COUNTER_CLOCKWISE;
                        postponed_vertices[1].second = switch_t::COUNTER_CLOCKWISE;
                    }
lbl_counter_clockwise:
                    /* there is an adjacent unpleasant switch or the adjacent
                     * mono-directed side does not need a switch */
                    /* counter-clockwise switch */
                    DEBUG_PRINT("counter-clockwise");
                    port_assignment_t last_half = 0b00;
                    for (size_t i = 0; i < num_edges_in_direction[current]; ++i)
                    {
                        const size_t edge_ix = (first_edge[current] + i) % num_neighbors;
                        if (orientations[edge_ix] == bool(directions[current] & 0b001))
                            last_half = 0b11;
                        if (!(out_pa[graph.original_edge[graph.vertices[v][edge_ix]]] & 0b100))
                            out_pa[graph.original_edge[graph.vertices[v][edge_ix]]]
                                |= (canonical_assignment[current] ^ last_half) & (orientations[edge_ix] ? 0b10 : 0b01);
                    }
                }
                else
                {
                    if (v_is_master_in_extra_rule)
                    {
                        postponed_vertices[0].second = switch_t::CLOCKWISE;
                        postponed_vertices[1].second = switch_t::CLOCKWISE;
                    }
lbl_clockwise:
                    /* clockwise switch */
                    DEBUG_PRINT("clockwise");
                    port_assignment_t first_half = 0b11;
                    for (size_t i = 0; i < num_edges_in_direction[current]; ++i)
                    {
                        const size_t edge_ix = (first_edge[current] + i) % num_neighbors;
                        if (orientations[edge_ix] == bool(directions[current] & 0b001))
                            first_half = 0b00;
                        if (!(out_pa[graph.original_edge[graph.vertices[v][edge_ix]]] & 0b100))
                            out_pa[graph.original_edge[graph.vertices[v][edge_ix]]]
                                |= (canonical_assignment[current] ^ first_half) & (orientations[edge_ix] ? 0b10 : 0b01);
                    }
                }
            }
        };
        assign_unpleasant_switches(direction_t::RIGHT, 0b101);
        assign_unpleasant_switches(direction_t::TOP, 0b110);
        assign_unpleasant_switches(direction_t::LEFT, 0b101);
        assign_unpleasant_switches(direction_t::BOTTOM, 0b110);

        delete[] orientations;
    }

    for (size_t edge_ix = 0; edge_ix < graph.num_edges(); ++edge_ix)
    {
        out_pa[graph.original_edge[edge_ix]] |= 0b100;
        DEBUG_PRINT("pa[" << graph.labels[graph.edges[edge_ix].tail]
            << "→" << graph.labels[graph.edges[edge_ix].head] << "] = "
            << int(out_pa[graph.original_edge[edge_ix]] & 0b11));
    }
}

void toposort(const graph_t &graph, coord_t *out_order)
{
    std::vector<vertex_t> sources;
    size_t *indeg = new size_t[graph.num_vertices()];
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
    {
        indeg[v] = 0;
        for (size_t edge_ix : graph.vertices[v])
            if (v == graph.edges[edge_ix].head)
                ++indeg[v];
        if (indeg[v] == 0)
            sources.push_back(v);
    }
    coord_t next_order = 0;
    while (!sources.empty())
    {
        vertex_t v = sources.back();
        sources.pop_back();
        out_order[v] = next_order++;
        for (size_t edge_ix : graph.vertices[v])
            if (v == graph.edges[edge_ix].tail && indeg[graph.edges[edge_ix].head] != 0) // not already visited
                if (--indeg[graph.edges[edge_ix].head] == 0)
                    sources.push_back(graph.edges[edge_ix].head);
    }
    if (next_order != graph.num_vertices())
    {
        std::cerr << "Cycle detected during topological sorting. Exiting." << std::endl;
        exit(1);
    }
    delete[] indeg;
}

void construct_dag(const graph_t &graph, const port_assignment_t *pa, bool y_coords, graph_t &out_dag)
{
    const char mask = y_coords ? 0b10 : 0b01;
    out_dag.vertices.resize(graph.num_vertices());
    out_dag.edges.reserve(graph.num_edges());
    for (size_t i = 0; i < graph.num_edges(); ++i)
    {
        const edge_t &e = graph.edges[i];
        if (pa[i] & mask)
            out_dag.edges.push_back(e);
        else
            out_dag.edges.push_back({ e.head, e.tail });
        out_dag.vertices[e.tail].push_back(i);
        out_dag.vertices[e.head].push_back(i);
    }
}

outer_face_t add_x(four_connected_component_t &graph, const port_assignment_t *pa, size_t dummy_edge)
{
    const vertex_t a = graph.outer_face[0];
    const vertex_t b = graph.outer_face[1];
    const vertex_t c = graph.outer_face[2];
    const size_t e_ab = graph.vertices[a].front();
    const size_t e_bc = graph.vertices[b].front();
    const size_t e_ca = graph.vertices[c].front();
    const bool e_ab_reversed = a == graph.edges[e_ab].head;
    const bool e_bc_reversed = b == graph.edges[e_bc].head;
    const bool e_ca_reversed = c == graph.edges[e_ca].head;

    /* find edge to subdivide */
    size_t target_edge;
    outer_face_t ret;

    if (e_ab_reversed == e_bc_reversed && e_bc_reversed == e_ca_reversed)
    {
        /* cycle */
        const port_assignment_t pa_ab = pa[graph.original_edge[e_ab]];
        const port_assignment_t pa_bc = pa[graph.original_edge[e_bc]];
        const port_assignment_t pa_ca = pa[graph.original_edge[e_ca]];

        if (pa_ab == pa_bc)
        {
            /* y)
             * c─┐
             * │ b─┐
             * └───a
             */
            target_edge = e_ca;
            ret = outer_face_t::H_SHAPE;
        }
        else if (pa_ab == pa_ca)
        {
            /* y) */
            target_edge = e_bc;
            ret = outer_face_t::H_SHAPE;
        }
        else if (pa_bc == pa_ca)
        {
            /* y) */
            target_edge = e_ab;
            ret = outer_face_t::H_SHAPE;
        }
        else if ((pa_ab ^ pa_bc) == 0b11)
        {
            /* x)
             * b───┐
             * │ ┌─a
             * └─c
             */
            if (pa_ab == 0b100 || pa_ab == 0b111)
                target_edge = e_ab;
            else
                target_edge = e_bc;
            ret = outer_face_t::LONG_SINK;
        }
        else if ((pa_ca ^ pa_ab) == 0b11)
        {
            /* x) */
            if (pa_ca == 0b100 || pa_ca == 0b111)
                target_edge = e_ca;
            else
                target_edge = e_ab;
            ret = outer_face_t::LONG_SINK;
        }
        else // if ((pa_bc ^ pa_ca) == 0b11)
        {
            /* x) */
            if (pa_bc == 0b100 || pa_bc == 0b111)
                target_edge = e_bc;
            else
                target_edge = e_ca;
            ret = outer_face_t::LONG_SINK;
        }
    }
    else
    {
        size_t e_st, e_sw, e_wt;
        if (e_ab_reversed == e_ca_reversed)
        {
            e_st = e_bc;
            if (e_ab_reversed)
            {
                e_wt = e_ca;
                e_sw = e_ab;
            }
            else
            {
                e_wt = e_ab;
                e_sw = e_ca;
            }
        }
        else if (e_bc_reversed == e_ab_reversed)
        {
            e_st = e_ca;
            if (e_bc_reversed)
            {
                e_wt = e_ab;
                e_sw = e_bc;
            }
            else
            {
                e_wt = e_bc;
                e_sw = e_ab;
            }
        }
        else // if (e_ca_reversed == e_bc_reversed)
        {
            e_st = e_ab;
            if (e_ca_reversed)
            {
                e_wt = e_bc;
                e_sw = e_ca;
            }
            else
            {
                e_wt = e_ca;
                e_sw = e_bc;
            }
        }

        const port_assignment_t pa_st = pa[graph.original_edge[e_st]];
        const port_assignment_t pa_wt = pa[graph.original_edge[e_wt]];
        const port_assignment_t pa_sw = pa[graph.original_edge[e_sw]];

        if (pa_st == pa_wt && pa_wt == pa_sw)
        {
            /* a)
             * ┌─┬─t
             * ├─w
             * s
             */
            DEBUG_PRINT("case a)");
            ret = outer_face_t::H_SHAPE;
            target_edge = e_st;
        }
        else if (((pa_st ^ pa_sw) & 0b10) && ((pa_st ^ pa_wt) & 0b01))
        {
            /* e)
             * w───┐
             * │   s
             * └─t─┘
             */
            DEBUG_PRINT("case e)");
            ret = outer_face_t::T_SHAPE;
            target_edge = e_st;
        }
        else if (pa_st == pa_sw)
        {
            if ((pa_st & 0b01) == (pa_wt & 0b01))
            {
                /* f)
                 *   w─┐
                 * t─┴─┤
                 *     s
                 */
                DEBUG_PRINT("case f)");
                ret = outer_face_t::LONG_SOURCE;
                target_edge = e_sw;
            }
            else if ((pa_st & 0b10) == (pa_wt & 0b10))
            {
                /* c)
                 * ┌─t─┐
                 * ├───w
                 * s
                 */
                DEBUG_PRINT("case c)");
                ret = outer_face_t::T_SHAPE;
                target_edge = e_wt;
            }
            else
            {
                /* h)
                 * w───┐
                 * └─t─┤
                 *     s
                 */
                DEBUG_PRINT("case h)");
                ret = outer_face_t::H_SHAPE;
                target_edge = e_sw;
            }
        }
        else if (pa_st == pa_wt)
        {
            if ((pa_st & 0b10) == (pa_sw & 0b10))
            {
                /* b)
                 * t─┬─┐
                 *   ├─w
                 *   s
                 */
                DEBUG_PRINT("case b)");
                ret = outer_face_t::LONG_SINK;
                target_edge = e_wt;
            }
            else if ((pa_st & 0b01) == (pa_sw & 0b01))
            {
                /* g)
                 *   w─┐
                 *   │ s
                 * t─┴─┘
                 */
                DEBUG_PRINT("case g)");
                ret = outer_face_t::T_SHAPE;
                target_edge = e_sw;
            }
            else
            {
                /* d)
                 * w─┐
                 * │ s
                 * └─┴─t
                 */
                DEBUG_PRINT("case d)");
                ret = outer_face_t::H_SHAPE;
                target_edge = e_wt;
            }
        }
        else
        {
            std::cerr << "Unrecognized drawing of outer face. Exiting." << std::endl;
            exit(1);
        }
    }

    DEBUG_PRINT("inserting dummy into edge " << graph.labels[graph.edges[target_edge].tail]
            << '-' << graph.labels[graph.edges[target_edge].head]);

    /* insert x */
    const bool right_is_outside = graph.edges[target_edge].index_at_tail == 0;
    const vertex_t x = graph.num_vertices();
    graph.labels.push_back("dummy");
    graph.designated_face.push_back(0);
    const size_t e_xt = graph.num_edges();
    graph.edges.push_back({ x, graph.edges[target_edge].head, 2 * size_t(!right_is_outside), graph.edges[target_edge].index_at_head });
    graph.vertices[graph.edges[target_edge].head][graph.edges[target_edge].index_at_head] = e_xt;
    graph.edges[target_edge].head = x;
    const size_t e_xy = graph.num_edges();

    vertex_t s; // vertex one after x on the outer face in clockwise order
    if (right_is_outside)
    {
        graph.vertices.push_back({ e_xt, e_xy, target_edge });
        graph.edges[target_edge].index_at_head = 2;
        s = graph.edges[target_edge].tail;
    }
    else
    {
        graph.vertices.push_back({ target_edge, e_xy, e_xt });
        graph.edges[target_edge].index_at_head = 0;
        s = graph.edges[e_xt].head;
    }

    const size_t e_sy = graph.vertices[s][1];
    const vertex_t y = graph.edges[e_sy].tail + graph.edges[e_sy].head - s;
    DEBUG_PRINT("adding dummy edge to vertex " << graph.labels[y]);
    const size_t index_of_xy_at_y = graph.edges[e_sy].index_at_tail + graph.edges[e_sy].index_at_head; // - 1 (index at s) + 1
    const bool e_xy_is_in_designated_face_of_y = graph.designated_face[y] && (graph.designated_face[y] - 1) % graph.degree(y) == index_of_xy_at_y % graph.degree(y);
    const bool e_xy_reversed = (y == graph.edges[graph.vertices[y][index_of_xy_at_y % graph.degree(y)]].tail) ^ e_xy_is_in_designated_face_of_y;
    graph.vertices[y].insert(graph.vertices[y].begin() + index_of_xy_at_y, e_xy);
    if (e_xy_reversed)
        graph.edges.push_back({ y, x, index_of_xy_at_y, 1 });
    else
        graph.edges.push_back({ x, y, 1, index_of_xy_at_y });
    if (e_xy_is_in_designated_face_of_y)
        graph.designated_face[y] = 0;

    /* fix edges around y */
    for (size_t i = index_of_xy_at_y + 1; i < graph.degree(y); ++i)
    {
        auto &e = graph.edges[graph.vertices[y][i]];
        if (y == e.tail)
            ++e.index_at_tail;
        else
            ++e.index_at_head;
    }

    /* fix graph.designated_face[y] if necessary */
    if (graph.designated_face[y] > index_of_xy_at_y)
        ++graph.designated_face[y];

    graph.original_edge.push_back(graph.original_edge[target_edge]); // e_xt
    graph.original_edge.push_back(dummy_edge); // e_xy

    /* rotate outer face */
    const vertex_t u = graph.neighbor(x, 0);
    const vertex_t v = graph.neighbor(u, 0);
    const vertex_t w = graph.neighbor(v, 0);
    switch (pa[graph.original_edge[target_edge]])
    {
        case 0b100:
            graph.outer_face = { v, w, x, u };
            break;
        case 0b101:
            graph.outer_face = { w, x, u, v };
            break;
        case 0b110:
            graph.outer_face = { u, v, w, x };
            break;
        default: // 0b111
            graph.outer_face = { x, u, v, w };
            break;
    }

    return ret;
}

/* change the corners of the dual */
void fix_rectangular_dual(const graph_t &graph, rectangular_dual_t &dual, outer_face_t drawing_of_outer_face)
{
    const vertex_t dummy_vertex = graph.num_vertices() - 1;
    switch (drawing_of_outer_face)
    {
        case outer_face_t::T_SHAPE:
            if (dummy_vertex == graph.outer_face[0])
            {
                /* make left side shorter and top and bottom wider */
                ++dual[graph.outer_face[0]].y_min;
                --dual[graph.outer_face[0]].y_max;
                --dual[graph.outer_face[1]].x_min;
                --dual[graph.outer_face[3]].x_min;
            }
            else if (dummy_vertex == graph.outer_face[1])
            {
                /* make top wider and sides shorter */
                --dual[graph.outer_face[0]].y_max;
                --dual[graph.outer_face[2]].y_max;
                --dual[graph.outer_face[3]].x_min;
                ++dual[graph.outer_face[3]].x_max;
            }
            else if (dummy_vertex == graph.outer_face[2])
            {
                /* make right side shorter and top and bottom wider */
                ++dual[graph.outer_face[1]].x_max;
                ++dual[graph.outer_face[2]].y_min;
                --dual[graph.outer_face[2]].y_max;
                ++dual[graph.outer_face[3]].x_max;
            }
            else // if (dummy_vertex == graph.outer_face[3])
            {
                /* make bottom wider and sides shorter */
                ++dual[graph.outer_face[0]].y_min;
                --dual[graph.outer_face[1]].x_min;
                ++dual[graph.outer_face[1]].x_max;
                ++dual[graph.outer_face[2]].y_min;
            }
            break;

        case outer_face_t::H_SHAPE:
            if (dummy_vertex == graph.outer_face[0] || dummy_vertex == graph.outer_face[2])
            {
                /* make top and bottom wider and sides shorter */
                ++dual[graph.outer_face[0]].y_min;
                --dual[graph.outer_face[0]].y_max;
                --dual[graph.outer_face[1]].x_min;
                ++dual[graph.outer_face[1]].x_max;
                ++dual[graph.outer_face[2]].y_min;
                --dual[graph.outer_face[2]].y_max;
                --dual[graph.outer_face[3]].x_min;
                ++dual[graph.outer_face[3]].x_max;
            }
            break;

        case outer_face_t::LONG_SINK:
            if (dummy_vertex == graph.outer_face[0])
            {
                /* dummy_vertex is left, sink is top */
                ++dual[graph.outer_face[0]].y_min;
                --dual[graph.outer_face[0]].y_max;
                --dual[graph.outer_face[1]].x_min;
                --dual[graph.outer_face[2]].y_max;
                --dual[graph.outer_face[3]].x_min;
                ++dual[graph.outer_face[3]].x_max;
            }
            else if (dummy_vertex == graph.outer_face[1])
            {
                /* dummy_vertex is bottom, sink is right */
                --dual[graph.outer_face[0]].y_max;
                --dual[graph.outer_face[3]].x_min;
            }
            else if (dummy_vertex == graph.outer_face[2])
            {
                /* dummy_vertex is right, sink is bottom */
                ++dual[graph.outer_face[0]].y_min;
                --dual[graph.outer_face[1]].x_min;
                ++dual[graph.outer_face[1]].x_max;
                ++dual[graph.outer_face[2]].y_min;
                --dual[graph.outer_face[2]].y_max;
                ++dual[graph.outer_face[3]].x_max;
            }
            else // if (dummy_vertex == graph.outer_face[3])
            {
                /* dummy_vertex is top, sink is left */
                ++dual[graph.outer_face[1]].x_max;
                ++dual[graph.outer_face[2]].y_min;
            }
            break;

        case outer_face_t::LONG_SOURCE:
            if (dummy_vertex == graph.outer_face[0])
            {
                /* dummy_vertex is left, source is bottom */
                ++dual[graph.outer_face[0]].y_min;
                --dual[graph.outer_face[0]].y_max;
                --dual[graph.outer_face[1]].x_min;
                ++dual[graph.outer_face[1]].x_max;
                ++dual[graph.outer_face[2]].y_min;
                --dual[graph.outer_face[3]].x_min;
            }
            else if (dummy_vertex == graph.outer_face[1])
            {
                /* dummy_vertex is bottom, source is left */
                --dual[graph.outer_face[2]].y_max;
                ++dual[graph.outer_face[3]].x_max;
            }
            else if (dummy_vertex == graph.outer_face[2])
            {
                /* dummy_vertex is right, source is top */
                --dual[graph.outer_face[0]].y_max;
                ++dual[graph.outer_face[1]].x_max;
                ++dual[graph.outer_face[2]].y_min;
                --dual[graph.outer_face[2]].y_max;
                --dual[graph.outer_face[3]].x_min;
                ++dual[graph.outer_face[3]].x_max;
            }
            else // if (dummy_vertex == graph.outer_face[3])
            {
                /* dummy_vertex is top, source is right */
                ++dual[graph.outer_face[0]].y_min;
                --dual[graph.outer_face[1]].x_min;
            }
            break;
    }
}

void port_assignment_of_outer_face(const four_connected_component_t &graph, port_assignment_t *pa)
{
    vertex_t a = graph.outer_face[0];
    vertex_t b = graph.outer_face[1];
    vertex_t c = graph.outer_face[2];
    size_t e_ab = graph.vertices[a].front();
    size_t e_bc = graph.vertices[b].front();
    size_t e_ca = graph.vertices[c].front();
    bool e_ab_reversed = a == graph.edges[e_ab].head;
    bool e_bc_reversed = b == graph.edges[e_bc].head;
    bool e_ca_reversed = c == graph.edges[e_ca].head;
    if (e_ab_reversed && e_bc_reversed && e_ca_reversed)
    {
        /* y) */
        pa[graph.original_edge[e_ab]] = 0b100;
        pa[graph.original_edge[e_bc]] = 0b111;
        pa[graph.original_edge[e_ca]] = 0b111;
    }
    else if (!e_ab_reversed && !e_bc_reversed && !e_ca_reversed)
    {
        /* y) */
        pa[graph.original_edge[e_ab]] = 0b101;
        pa[graph.original_edge[e_bc]] = 0b110;
        pa[graph.original_edge[e_ca]] = 0b110;
    }
    else
    {
        /* e) */
        if (e_ab_reversed == e_ca_reversed)
        {
            /* a is w */
            if (e_ab_reversed)
            {
                /* b is s, c is t */
                pa[graph.original_edge[e_ab]] = 0b111;
                pa[graph.original_edge[e_bc]] = 0b101;
                pa[graph.original_edge[e_ca]] = 0b100;
            }
            else
            {
                /* b is t, c is s */
                pa[graph.original_edge[e_ab]] = 0b101;
                pa[graph.original_edge[e_bc]] = 0b100;
                pa[graph.original_edge[e_ca]] = 0b110;
            }
        }
        else if (e_bc_reversed == e_ab_reversed)
        {
            /* b is w */
            if (e_bc_reversed)
            {
                /* a is t, c is s */
                pa[graph.original_edge[e_ab]] = 0b100;
                pa[graph.original_edge[e_bc]] = 0b111;
                pa[graph.original_edge[e_ca]] = 0b101;
            }
            else
            {
                /* a is s, c is t */
                pa[graph.original_edge[e_ab]] = 0b110;
                pa[graph.original_edge[e_bc]] = 0b101;
                pa[graph.original_edge[e_ca]] = 0b100;
            }
        }
        else // if (e_ca_reversed == e_bc_reversed)
        {
            /* c is w */
            if (e_ca_reversed)
            {
                /* a is s, b is t */
                pa[graph.original_edge[e_ab]] = 0b101;
                pa[graph.original_edge[e_bc]] = 0b100;
                pa[graph.original_edge[e_ca]] = 0b111;
            }
            else
            {
                /* a is t, b is s */
                pa[graph.original_edge[e_ab]] = 0b100;
                pa[graph.original_edge[e_bc]] = 0b110;
                pa[graph.original_edge[e_ca]] = 0b101;
            }
        }
    }

    DEBUG_PRINT("pa[" << graph.labels[graph.edges[e_ab].tail] << '-' << graph.labels[graph.edges[e_ab].head] << "] = " << int(pa[graph.original_edge[e_ab]] & 0b11));
    DEBUG_PRINT("pa[" << graph.labels[graph.edges[e_bc].tail] << '-' << graph.labels[graph.edges[e_bc].head] << "] = " << int(pa[graph.original_edge[e_bc]] & 0b11));
    DEBUG_PRINT("pa[" << graph.labels[graph.edges[e_ca].tail] << '-' << graph.labels[graph.edges[e_ca].head] << "] = " << int(pa[graph.original_edge[e_ca]] & 0b11));
}

void add_virtual_edges(four_connected_component_t &graph, rectangular_dual_t &dual, size_t dummy_edge)
{
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
        if (graph.designated_face[v])
        {
            const size_t virtual_vertex = graph.num_vertices();
            const size_t virtual_edge = graph.num_edges();
            size_t face = (graph.designated_face[v] - 1) % graph.degree(v);
            const vertex_t neighbor = graph.neighbor(v, face);

            DEBUG_PRINT("adding virtual edge to vertex " << graph.labels[v] << " before the edge to " << graph.labels[neighbor]);

            graph.vertices.push_back({ virtual_edge });
            graph.labels.push_back("virtual_vertex");
            graph.designated_face.push_back(0);
            graph.original_edge.push_back(dummy_edge);

            if (dual[v].x_max == dual[neighbor].x_min) // neighbor is right of v
            {
                if (dual[v].y_min <= dual[neighbor].y_min)
                    dual.push_back({
                            /* .x_min = */ dual[neighbor].x_min,
                            /* .y_min = */ dual[neighbor].y_min,
                            /* .x_max = */ dual[neighbor].x_max,
                            /* .y_max = */ dual[neighbor].y_min
                        });
                else
                    dual.push_back({
                            /* .x_min = */ dual[neighbor].x_min,
                            /* .y_min = */ dual[neighbor].y_min,
                            /* .x_max = */ dual[neighbor].x_min,
                            /* .y_max = */ dual[v].y_min
                        });
            }
            else if (dual[v].y_max == dual[neighbor].y_min) // neighbor is above v
            {
                if (dual[v].x_max >= dual[neighbor].x_max)
                    dual.push_back({
                            /* .x_min = */ dual[neighbor].x_max,
                            /* .y_min = */ dual[neighbor].y_min,
                            /* .x_max = */ dual[neighbor].x_max,
                            /* .y_max = */ dual[neighbor].y_max
                        });
                else
                    dual.push_back({
                            /* .x_min = */ dual[v].x_max,
                            /* .y_min = */ dual[neighbor].y_min,
                            /* .x_max = */ dual[neighbor].x_max,
                            /* .y_max = */ dual[neighbor].y_min
                        });
            }
            else if (dual[v].x_min == dual[neighbor].x_max) // neighbor is left of v
            {
                if (dual[v].y_max >= dual[neighbor].y_max)
                    dual.push_back({
                            /* .x_min = */ dual[neighbor].x_min,
                            /* .y_min = */ dual[neighbor].y_max,
                            /* .x_max = */ dual[neighbor].x_max,
                            /* .y_max = */ dual[neighbor].y_max
                        });
                else
                    dual.push_back({
                            /* .x_min = */ dual[neighbor].x_max,
                            /* .y_min = */ dual[v].y_max,
                            /* .x_max = */ dual[neighbor].x_max,
                            /* .y_max = */ dual[neighbor].y_max
                        });
            }
            else // neighbor is below v
            {
                if (dual[v].x_min <= dual[neighbor].x_min)
                    dual.push_back({
                            /* .x_min = */ dual[neighbor].x_min,
                            /* .y_min = */ dual[neighbor].y_min,
                            /* .x_max = */ dual[neighbor].x_min,
                            /* .y_max = */ dual[neighbor].y_max
                        });
                else
                    dual.push_back({
                            /* .x_min = */ dual[neighbor].x_min,
                            /* .y_min = */ dual[neighbor].y_max,
                            /* .x_max = */ dual[v].x_min,
                            /* .y_max = */ dual[neighbor].y_max
                        });
            }

            const bool outgoing = v == graph.edges[graph.vertices[v][face]].head;
            if (face == 0)
                face = graph.degree(v); // equivalent, but easier to insert
            graph.vertices[v].insert(graph.vertices[v].begin() + face, virtual_edge);
            if (outgoing)
            {
                graph.edges.push_back({ v, virtual_vertex, face, 0 });
                for (size_t i = face; i < graph.degree(v); ++i)
                    ++graph.edges[graph.vertices[v][i]].index_at_head;
            }
            else
            {
                graph.edges.push_back({ virtual_vertex, v, 0, face });
                for (size_t i = face; i < graph.degree(v); ++i)
                    ++graph.edges[graph.vertices[v][i]].index_at_tail;
            }
        }
}

l_drawing_t construct_drawing(const graph_t &graph, four_block_tree_t &four_block_tree, bool print_duals)
{
    timer::start(timer::PORT_ASSIGNMENT);

    port_assignment_t *pa = new port_assignment_t[graph.num_edges() + 1];
    for (size_t i = 0; i < graph.num_edges() + 1; ++i)
        pa[i] = 0b00;

    port_assignment_of_outer_face(four_block_tree.front(), pa);
    for (auto &component : four_block_tree)
    {
        outer_face_t of = add_x(component, pa, graph.num_edges());

#ifdef DEBUG
        DEBUG_PRINT("after adding x:");
        for (vertex_t v : component.outer_face)
            DEBUG_PRINT('\t' << component.labels[v]);
        for (size_t e_ = 0; e_ < component.num_edges(); ++e_)
        {
            const auto &e = component.edges[e_];
            DEBUG_PRINT(e_ << ": " << component.labels[e.tail] << '-' << component.labels[e.head] << " (" << e.index_at_tail << ',' << e.index_at_head << ") was " << component.original_edge[e_]);
        }
        for (vertex_t v = 0; v < component.num_vertices(); ++v)
        {
            DEBUG_PRINT(component.labels[v]);
            for (size_t e : component.vertices[v])
                DEBUG_PRINT('\t' << e);
        }
#endif // DEBUG

        timer::stop(timer::PORT_ASSIGNMENT);

        timer::start(timer::RECT_DUAL);
        rectangular_dual_t rect_dual = compute_rect_dual(component);
        timer::stop(timer::RECT_DUAL);

        timer::start(timer::PORT_ASSIGNMENT);

        fix_rectangular_dual(component, rect_dual, of);

        if (print_duals)
        {
            timer::stop(timer::PORT_ASSIGNMENT);
            timer::start(timer::IO);
            write_tikz(std::cout, component, rect_dual);
            timer::stop(timer::IO);
            timer::start(timer::PORT_ASSIGNMENT);
        }

        add_virtual_edges(component, rect_dual, graph.num_edges());
        port_assignment(component, rect_dual, pa);
    }

#ifdef DEBUG
    for (size_t i = 0; i < graph.num_edges(); ++i)
        if (!bool(pa[i] & 0b100))
        {
            DEBUG_PRINT("not all edges were port-assigned");
            exit(1);
        }
#endif // DEBUG

    graph_t x_dag, y_dag;
    construct_dag(graph, pa, false, x_dag);
    construct_dag(graph, pa, true,  y_dag);
    delete[] pa;

    coord_t *x_coords = new coord_t[graph.num_vertices()];
    coord_t *y_coords = new coord_t[graph.num_vertices()];
    toposort(x_dag, x_coords);
    toposort(y_dag, y_coords);

    l_drawing_t out_drawing;
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
        out_drawing.push_back({ x_coords[v], y_coords[v] });
    delete[] x_coords;
    delete[] y_coords;

    timer::stop(timer::PORT_ASSIGNMENT);

    return out_drawing;
}
