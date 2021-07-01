#include <list>
#include <algorithm>
#include "include/rectangular_dual.hpp"
#include "include/debug_print.hpp"

struct path_t {
    std::vector<vertex_t> contents{};
    std::vector<vertex_t> predecessors{};
};

typedef std::vector<path_t> ordering31_t;

template<typename T>
T pred(T it)
{
    return --it;
}
template<typename T>
T succ(T it)
{
    return ++it;
}

void compute_ordering31(const graph_t &graph, ordering31_t &out_ordering)
{
    std::vector<vertex_t> leg_free, basic_two_leg_centers;
    std::vector<std::vector<vertex_t>> outer(graph.num_vertices());
    std::list<vertex_t> boundary;
    auto *position_in_boundary     = new std::list<vertex_t>::iterator[graph.num_vertices()];
    auto *two_leg_centers          = new size_t[graph.num_vertices()];
    auto *outer_deg_three          = new size_t[graph.num_vertices()];
    auto *leg_free_ix              = new size_t[graph.num_vertices()];
    auto *basic_two_leg_centers_ix = new size_t[graph.num_vertices()];
    auto *degree                   = new size_t[graph.num_vertices()];
    auto *picked                   = new bool[graph.num_vertices()];
    auto *was_two_leg_center       = new bool[graph.num_vertices()];
    size_t num_not_picked = graph.num_vertices();
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
    {
        position_in_boundary[v] = boundary.end();
        two_leg_centers[v]
            = outer_deg_three[v]
            = leg_free_ix[v]
            = basic_two_leg_centers_ix[v] = 0;
        degree[v] = graph.degree(v);
        picked[v]
            = was_two_leg_center[v] = false;
    }
    out_ordering.clear();

    auto is_on_boundary = [&](vertex_t v)
    {
        return position_in_boundary[v] != boundary.end();
    };
    auto is_two_leg_center = [&](vertex_t v)
    {
        if (is_on_boundary(v)) return false;
        if (outer[v].size() > 2) return true;
        if (outer[v].size() < 2) return false;
        /* It enough to check whether the two neighbors are neighbors on the
         * boundary. If there was an edge between them that does not lie on the
         * boundary, it is a chord, which will be avoided. */
        const auto n1_it = position_in_boundary[outer[v][0]];
        const auto n2_it = position_in_boundary[outer[v][1]];
        /* DEBUG_PRINT("checking for 2leg with " << outer[v][0] << ',' << v << ',' << outer[v][1]); */
        if (succ(n1_it) == n2_it || n1_it == succ(n2_it)) return false;
        return true;
    };
    auto check_basic_two_leg_center = [&](vertex_t v)
    {
        /* DEBUG_PRINT("checking whether " << v << " is b2lc"); */
        /* DEBUG_PRINT("outer_deg_three[" << v << "] = " << outer_deg_three[v]); */
        /* DEBUG_PRINT("outer[" << v << "] = "); */
        /* for (auto n : outer[v]) */
        /*     DEBUG_PRINT(' ' << n); */
        const bool was = basic_two_leg_centers_ix[v] < basic_two_leg_centers.size()
            && basic_two_leg_centers[basic_two_leg_centers_ix[v]] == v;
        if (outer_deg_three[v] > 0 && outer_deg_three[v] + 2 == outer[v].size() && is_two_leg_center(v))
        {
            if (!was)
            {
                basic_two_leg_centers_ix[v] = basic_two_leg_centers.size();
                basic_two_leg_centers.push_back(v);
            }
        }
        else if (was)
        {
            basic_two_leg_centers[basic_two_leg_centers_ix[v]] = basic_two_leg_centers.back();
            basic_two_leg_centers_ix[basic_two_leg_centers.back()] = basic_two_leg_centers_ix[v];
            basic_two_leg_centers.pop_back();
        }
    };
    auto check_leg_free = [&](vertex_t v)
    {
        /* DEBUG_PRINT("checking whether " << v << " is leg-free"); */
        /* DEBUG_PRINT("two_leg_centers[" << v << "] = " << two_leg_centers[v]); */
        const bool was = leg_free_ix[v] < leg_free.size()
            && leg_free[leg_free_ix[v]] == v;
        if (two_leg_centers[v] == 0 && is_on_boundary(v))
        {
            if (!was)
            {
                /* DEBUG_PRINT(v << " is now leg-free"); */
                leg_free_ix[v] = leg_free.size();
                leg_free.push_back(v);
            }
        }
        else if (was)
        {
            /* DEBUG_PRINT(v << " is no longer leg-free"); */
            leg_free[leg_free_ix[v]] = leg_free.back();
            leg_free_ix[leg_free.back()] = leg_free_ix[v];
            leg_free.pop_back();
        }
    };
    auto check_two_leg_center = [&](vertex_t v)
    {
        if (is_two_leg_center(v))
        {
            if (!was_two_leg_center[v])
            {
                /* DEBUG_PRINT(v << " is now 2leg-center"); */
                was_two_leg_center[v] = true;
                for (size_t i = 0; i < graph.degree(v); ++i)
                {
                    const vertex_t n = graph.neighbor(v, i);
                    if (two_leg_centers[n]++ == 0)
                        check_leg_free(n);
                }
            }
        }
        else if (was_two_leg_center[v])
        {
            /* DEBUG_PRINT(v << " is no longer 2leg-center"); */
            was_two_leg_center[v] = false;
            graph.for_neighbors(v, [&](vertex_t n)
            {
                if (!picked[n] && --two_leg_centers[n] == 0)
                    check_leg_free(n);
            });
        }
        else
            return;
        check_basic_two_leg_center(v);
    };
    auto decrease_degree = [&](vertex_t v, size_t count = 1)
    {
        /* v will always be on the boundary, so no need to check that */
        if ((degree[v] -= count) == 3)
            graph.for_neighbors(v, [&](vertex_t n)
            {
                if (picked[n])
                    return;
                ++outer_deg_three[n];
                check_basic_two_leg_center(n);
            });
    };

    /* glorious hack to prevent these two from reaching degree 3 */
    degree[graph.outer_face[0]] += 2;
    degree[graph.outer_face[2]] += 2;
    /* set face bounded by graph.outer_face[0,3,2] as outer face */
    boundary = { graph.outer_face[0], graph.outer_face[3], graph.outer_face[2] };
    leg_free.push_back(graph.outer_face[3]);
    ++degree[graph.outer_face[3]];
    decrease_degree(graph.outer_face[3]);
    for (auto it = boundary.begin(); it != boundary.end(); ++it)
    {
        const vertex_t v = *it;
        position_in_boundary[v] = it;
        graph.for_neighbors(v, [&](vertex_t n)
        {
            if (picked[n])
                return;
            outer[n].push_back(v);
            check_two_leg_center(n);
        });
    }

    while (num_not_picked > 3)
    {
#ifdef DEBUG
        DEBUG_PRINT("===================");
        DEBUG_PRINT("boundary:");
        for (auto v : boundary)
            DEBUG_PRINT(' ' << graph.labels[v]);
        DEBUG_PRINT("leg_free:");
        for (auto v : leg_free)
            DEBUG_PRINT(' ' << graph.labels[v]);
        DEBUG_PRINT("basic_two_leg_centers:");
        for (auto v : basic_two_leg_centers)
            DEBUG_PRINT(' ' << graph.labels[v]);
#endif // DEBUG

        if (!leg_free.empty())
        {
            /* pick singleton */
            const vertex_t v = leg_free.back();
            leg_free.pop_back();
            picked[v] = true;
            out_ordering.emplace_back();
            out_ordering.back().contents.push_back(v);
            const auto it = position_in_boundary[v];
            const vertex_t vl = *pred(it);
            const vertex_t vr = *succ(it);
            size_t vl_ix;
            for (vl_ix = 0; graph.neighbor(v, vl_ix) != vl; ++vl_ix)
            {
                /* modify vl_ix */
            }
            out_ordering.back().predecessors.push_back(vl);
            for (size_t i = 1; i < degree[v] - 1; ++i)
            {
                const vertex_t n = graph.neighbor(v, (i + vl_ix) % graph.degree(v));
                out_ordering.back().predecessors.push_back(n);
                boundary.insert(position_in_boundary[v], n);
                position_in_boundary[n] = pred(it);
            }
            out_ordering.back().predecessors.push_back(vr);
            boundary.erase(it);
        }
        else if (!basic_two_leg_centers.empty())
        {
            /* pick fan */
            const vertex_t c = basic_two_leg_centers.back();
            basic_two_leg_centers.pop_back();
            out_ordering.emplace_back();
            vertex_t vl;
            size_t vl_ix = 0;
            vertex_t one_to_the_right = graph.neighbor(c, graph.degree(c) - 1);
            while (true)
            {
                vl = graph.neighbor(c, vl_ix);
                if (is_on_boundary(vl))
                {
                    if (vl == graph.outer_face[0])
                    {
                        break;
                    }
                    if (degree[vl] > 3 && is_on_boundary(one_to_the_right) && vl != graph.outer_face[2])
                    {
                        break;
                    }
                }
                one_to_the_right = vl;
                ++vl_ix;
            }
            auto it = position_in_boundary[vl];
            ++it;
            while (degree[*it] == 3 && it != position_in_boundary[graph.outer_face[2]])
            {
                const vertex_t v = *it;
                picked[v] = true;
                out_ordering.back().contents.push_back(v);
                ++it;
                boundary.erase(position_in_boundary[v]);
            }
            const vertex_t vr = *it;
            boundary.insert(it, c);
            position_in_boundary[c] = --it;
            out_ordering.back().predecessors.push_back(vl);
            out_ordering.back().predecessors.push_back(c);
            out_ordering.back().predecessors.push_back(vr);
        }
        else
            exit(1);

#ifdef DEBUG
        DEBUG_PRINT("contents:");
        for (auto v : out_ordering.back().contents)
            DEBUG_PRINT(' ' << graph.labels[v]);
        DEBUG_PRINT("predecessors:");
        for (auto v : out_ordering.back().predecessors)
            DEBUG_PRINT(' ' << graph.labels[v]);
        DEBUG_PRINT("end");
#endif // DEBUG

        /* bookkeeping */
        num_not_picked -= out_ordering.back().contents.size();
        decrease_degree(out_ordering.back().predecessors.front());
        decrease_degree(out_ordering.back().predecessors.back());
        for (size_t i = 1; i < out_ordering.back().predecessors.size() - 1; ++i)
        {
            const vertex_t n = out_ordering.back().predecessors[i];
            decrease_degree(n, out_ordering.back().contents.size());
            check_two_leg_center(n);
            check_leg_free(n);
            graph.for_neighbors(n, [&](vertex_t m)
            {
                if (picked[n])
                    return;
                outer[m].push_back(n);
                check_two_leg_center(m);
            });
        }
    }

    delete[] position_in_boundary;
    delete[] two_leg_centers;
    delete[] outer_deg_three;
    delete[] leg_free_ix;
    delete[] basic_two_leg_centers_ix;
    delete[] degree;
    delete[] picked;
    delete[] was_two_leg_center;

    std::reverse(out_ordering.begin(), out_ordering.end());
    return;

    out_ordering.clear();
    const vertex_t s = 0, t = 1, u = 2, v = 3, w = 4, x = 5, y = 6, z = 7;
    out_ordering.push_back({ { y, w }, { x, z, t } });
    out_ordering.push_back({ { v }, { y, w, t } });
    out_ordering.push_back({ { u }, { y, v, t } });
    out_ordering.push_back({ { s }, { x, y, u, t } });
}

rectangular_dual_t compute_rect_dual(const graph_t &graph)
{
    ordering31_t ordering;
    compute_ordering31(graph, ordering);

    rectangular_dual_t out_rect_dual(graph.num_vertices());
    coord_t top = 0;
    std::list<coord_t> vertical;
    auto *x_min = new std::list<coord_t>::iterator[graph.num_vertices()];
    auto *x_max = new std::list<coord_t>::iterator[graph.num_vertices()];

    vertical.push_back(0);
    vertical.push_back(0);
    x_min[graph.outer_face[0]] = vertical.begin();
    x_max[graph.outer_face[0]] = --vertical.end();
    out_rect_dual[graph.outer_face[0]].y_min = top;
    vertical.push_back(0);
    x_min[graph.outer_face[1]] = x_min[graph.outer_face[0]];
    ++x_min[graph.outer_face[1]];
    x_max[graph.outer_face[1]] = --vertical.end();
    out_rect_dual[graph.outer_face[1]].y_min = top;
    vertical.push_back(0);
    x_min[graph.outer_face[2]] = x_min[graph.outer_face[1]];
    ++x_min[graph.outer_face[2]];
    x_max[graph.outer_face[2]] = --vertical.end();
    out_rect_dual[graph.outer_face[2]].y_min = top;

    for (const auto &path : ordering)
    {
        ++top;
        for (vertex_t v : path.contents)
            out_rect_dual[v].y_min = top;
        for (vertex_t u : path.predecessors)
            out_rect_dual[u].y_max = top;

        if (path.contents.size() == 1)
        {
            vertex_t v_k = path.contents.front(),
                v_l = path.predecessors.front(),
                v_r = path.predecessors.back();
            x_min[v_k] = x_max[v_l];
            x_max[v_k] = x_min[v_r];
        }
        else
        {
            auto it = x_max[path.predecessors.front()];
            size_t i;
            if (path.contents.empty()) exit(1); // TODO
            for (i = 0; i < path.contents.size() - 1; ++i)
            {
                vertex_t v = path.contents[i];
                x_min[v] = it;
                ++it;
                if (it == x_min[path.predecessors.back()])
                {
                    vertical.insert(it, 0);
                    --it;
                }
                x_max[v] = it;
            }
            x_min[path.contents.back()] = it;
            x_max[path.contents.back()] = x_min[path.predecessors.back()];
        }
    }

    ++top;
    out_rect_dual[graph.outer_face[2]].y_max = top;
    out_rect_dual[graph.outer_face[3]].y_max = top;
    out_rect_dual[graph.outer_face[0]].y_max = top;

    coord_t x = 0;
    for (coord_t &vert : vertical)
        vert = x++;
    for (size_t i = 0; i < out_rect_dual.size(); ++i)
    {
        out_rect_dual[i].x_min = *x_min[i];
        out_rect_dual[i].x_max = *x_max[i];
    }
    delete[] x_min;
    delete[] x_max;

    return out_rect_dual;
}
