#include <vector>
#include "include/cyclic_list.hpp"
#include "include/graph.hpp"
#include "include/decompose.hpp"
#include "include/debug_print.hpp"

struct triangle_t {
    vertex_t u, v, w;
    size_t e_uv, e_vw, e_wu;
};

std::vector<triangle_t> list_separating_triangles(const graph_t &graph)
{
    std::vector<triangle_t> out_triangles;

    /* sort vertices by degree */
    std::vector<std::vector<vertex_t>> buckets(graph.num_vertices());
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
        buckets[graph.degree(v)].push_back(v);
    std::vector<vertex_t> vertices_by_degree;
    for (const auto &bucket : buckets)
        for (vertex_t v : bucket)
            vertices_by_degree.push_back(v);

    bool *marked  = new bool[graph.num_vertices()];
    bool *visited = new bool[graph.num_vertices()];
    size_t *e_v_ = new size_t[graph.num_vertices()];
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
        marked[v] = visited[v] = false;
    for (size_t i = graph.num_vertices() - 1; i >= 2; --i)
    {
        const vertex_t v = vertices_by_degree[i];
        visited[v] = true;
        graph.for_neighbors(v, [&](vertex_t w, size_t e_vw)
        {
            marked[w] = true;
            e_v_[w] = e_vw;
        });
        graph.for_neighbors(v, [&](vertex_t u, size_t e_uv)
        {
            if (!visited[u])
                graph.for_neighbors(u, [&](vertex_t w, size_t e_uw)
                {
                    if (!visited[w] && marked[w])
                    {
                        /* found triangle */
                        const size_t ix_of_w_at_u = graph.neighbor_index(u, e_uw);
                        const size_t ix_of_v_at_u = graph.neighbor_index(u, e_uv);
                        if ((graph.degree(u) + ix_of_v_at_u - ix_of_w_at_u + 1) % graph.degree(u) > 2) // |ix_of_v_at_u - ix_of_w_at_u| <= 1 (mod deg(u))
                        {
                            /* is separating */
                            out_triangles.push_back({ u, v, w, e_uv, e_v_[w], e_uw });
                        }
                    }
                });
            marked[u] = false;
        });
        visited[v] = true;
    }
    delete[] marked;
    delete[] visited;
    delete[] e_v_;

    return out_triangles;
}

four_block_tree_t build_four_block_tree(const graph_t &graph)
{
    const size_t infinity = graph.num_vertices();
    const vertex_t root = graph.outer_face.front();
    std::vector<size_t> height(graph.num_vertices());
    std::vector<size_t> lowpoint(graph.num_edges());
    std::vector<size_t> distance_from_tree_edge(graph.num_edges());
    std::vector<size_t> parent(graph.num_edges());
    std::vector<size_t> parent_edge(graph.num_vertices());
    std::vector<bool> back_edge(graph.num_edges());
    std::vector<unsigned char> return_side(graph.num_edges()); // 0b01 = right, 0b10 = left, 0b11 = both
    height[root] = 0;
    size_t max_height = height[root];
    size_t max_degree = graph.degree(root);
    {
        /* first DFS */
        std::vector<size_t> lowpoint_v = { infinity };
        std::vector<size_t> distance_from_tree_edge_v = { 0 };
        std::vector<unsigned char> return_side_v = { false };
        std::vector<size_t> active_child_edge(graph.num_vertices());
        std::vector<bool> visited(graph.num_vertices(), false);
        std::vector<bool> traversed(graph.num_edges(), false);
        std::vector<std::pair<vertex_t, size_t>> stack = { { root, 0 } };
        visited[root] = true;
        {
            size_t i = 0;
            while (graph.neighbor(root, i) != graph.outer_face[1])
                ++i;
            parent_edge[root] = graph.vertices[root][i];
            DEBUG_PRINT("parent_edge[" << root << "] = " << parent_edge[root]);
        }

        while (!stack.empty())
        {
            auto &[v, i] = stack.back();
            if (i == graph.degree(v))
            {
                stack.pop_back();
                if (!stack.empty())
                {
                    lowpoint[parent_edge[v]] = lowpoint_v.back();
                    distance_from_tree_edge[parent_edge[v]] = distance_from_tree_edge_v.back();
                    return_side[parent_edge[v]] = return_side_v.back();

                    lowpoint_v.pop_back();
                    distance_from_tree_edge_v.pop_back();
                    return_side_v.pop_back();

                    if (lowpoint[parent_edge[v]] < lowpoint_v.back())
                    {
                        lowpoint_v.back() = lowpoint[parent_edge[v]];
                        distance_from_tree_edge_v.back() = distance_from_tree_edge[parent_edge[v]];
                        return_side_v.back() = return_side[parent_edge[v]];
                    }
                    else if (lowpoint[parent_edge[v]] == lowpoint_v.back())
                        return_side_v.back() |= return_side[parent_edge[v]];
                }
            }
            else
            {
                const size_t e = graph.vertices[v][i];
                const size_t w = graph.neighbor(v, i);
                ++i;
                if (!visited[w])
                {
                    /* tree edge */
                    height[w] = height[v] + 1;
                    if (height[w] > max_height)
                        max_height = height[w];
                    DEBUG_PRINT("found tree edge " << e);

                    visited[w] = true;
                    traversed[e] = true;
                    active_child_edge[v] = e;
                    parent[e] = v;
                    stack.emplace_back(w, 0);
                    parent_edge[w] = e;
                    lowpoint_v.push_back(infinity);
                    distance_from_tree_edge_v.emplace_back();
                    return_side_v.emplace_back();
                    back_edge[e] = false;
                    if (graph.degree(w) > max_degree)
                        max_degree = graph.degree(w);
                }
                else if (!traversed[e])
                {
                    /* back edge */
                    traversed[e] = true;
                    lowpoint[e] = height[w];
                    parent[e] = v;
                    back_edge[e] = true;
                    {
                        const size_t c = active_child_edge[w];
                        const size_t p = parent_edge[w];
                        const size_t index_of_e = w == graph.edges[e].tail ? graph.edges[e].index_at_tail : graph.edges[e].index_at_head;
                        const size_t index_of_c = w == graph.edges[c].tail ? graph.edges[c].index_at_tail : graph.edges[c].index_at_head;
                        const size_t index_of_p = w == graph.edges[p].tail ? graph.edges[p].index_at_tail : graph.edges[p].index_at_head;
                        const size_t dist_e_c = (graph.degree(w) + index_of_e - index_of_c) % graph.degree(w);
                        const size_t dist_p_c = (graph.degree(w) + index_of_p - index_of_c) % graph.degree(w);
                        bool right = dist_e_c < dist_p_c;
                        if (right)
                        {
                            return_side[e] = 0b01;
                            distance_from_tree_edge[e] = dist_e_c;
                        }
                        else
                        {
                            return_side[e] = 0b10;
                            distance_from_tree_edge[e] = graph.degree(w) - dist_e_c;
                        }
                    }
                    if (lowpoint[e] < lowpoint_v.back())
                    {
                        lowpoint_v.back() = lowpoint[e];
                        distance_from_tree_edge_v.back() = distance_from_tree_edge[e];
                        return_side_v.back() = return_side[e];
                    }
                    else if (lowpoint[e] == lowpoint_v.back())
                        return_side_v.back() |= return_side[e];
                    DEBUG_PRINT("found back edge " << e);
                }
            }
        }
    }

    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
        DEBUG_PRINT("height[" << graph.labels[v] << "] = " << height[v]);
    for (size_t e = 0; e < graph.num_edges(); ++e)
        DEBUG_PRINT("lowpoint[" << e << "] = " << lowpoint[e] << "\tdistance_from_tree_edge["
                << e << "] = " << distance_from_tree_edge[e] << "\tside[" << e << "] = "
                << (return_side[e] & 0b10 ? "left" : "") << (return_side[e] & 0b01 ? "right" : ""));

    std::vector<std::vector<size_t>> edge_order(graph.num_vertices());
    {
        /* find order of edges for each vertex */
        std::vector<std::vector<size_t>> edges_by_dist_from_tree_edge(max_degree);
        for (size_t e = 0; e < graph.num_edges(); ++e)
            edges_by_dist_from_tree_edge[distance_from_tree_edge[e]].push_back(e);

        std::vector<std::vector<size_t>> edges_by_lowpoint(max_height);
        for (const auto &vec : edges_by_dist_from_tree_edge)
            for (auto e : vec)
                edges_by_lowpoint[lowpoint[e]].push_back(e);

        std::vector<std::vector<size_t>> left_edges_by_parent(graph.num_vertices());
        std::vector<std::vector<size_t>> right_edges_by_parent(graph.num_vertices());
        std::vector<std::vector<size_t>> leftright_edges_by_parent(graph.num_vertices());
        for (auto it = edges_by_lowpoint.crbegin(); it != edges_by_lowpoint.crend(); ++it)
            for (auto e : *it)
                switch (return_side[e])
                {
                    case 0b10:
                        left_edges_by_parent[parent[e]].push_back(e);
                        break;
                    case 0b01:
                        right_edges_by_parent[parent[e]].push_back(e);
                        break;
                    case 0b11:
                        leftright_edges_by_parent[parent[e]].push_back(e);
                        break;
                    default:
                        exit(1);
                }

        for (vertex_t v = 0; v < graph.num_vertices(); ++v)
        {
            size_t i_left = 0, i_right = 0;
            while (i_left < left_edges_by_parent[v].size() && i_right < right_edges_by_parent[v].size())
            {
                const size_t left_edge = left_edges_by_parent[v][i_left];
                const size_t right_edge = right_edges_by_parent[v][i_right];
                if (lowpoint[left_edge] > lowpoint[right_edge]
                        || (lowpoint[left_edge] == lowpoint[right_edge]
                            && back_edge[left_edge]))
                {
                    edge_order[v].push_back(left_edge);
                    ++i_left;
                }
                else
                {
                    edge_order[v].push_back(right_edge);
                    ++i_right;
                }
            }
            for ( ; i_left < left_edges_by_parent[v].size(); ++i_left)
                edge_order[v].push_back(left_edges_by_parent[v][i_left]);
            for ( ; i_right < right_edges_by_parent[v].size(); ++i_right)
                edge_order[v].push_back(right_edges_by_parent[v][i_right]);
            for (auto e : leftright_edges_by_parent[v])
                edge_order[v].push_back(e);
        }
    }

#ifdef DEBUG
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
    {
        DEBUG_PRINT(graph.labels[v]);
        for (size_t e : edge_order[v])
            DEBUG_PRINT('\t' << e);
    }
#endif // DEBUG

    auto separating_triangles = list_separating_triangles(graph);

    std::vector<size_t> triangle_order;
    {
        /* find order of separating triangles */
        std::vector<std::vector<size_t>> triangles_by_edge(graph.num_edges());
        for (size_t t = 0; t < separating_triangles.size(); ++t)
        {
            triangles_by_edge[separating_triangles[t].e_uv].push_back(t);
            triangles_by_edge[separating_triangles[t].e_vw].push_back(t);
            triangles_by_edge[separating_triangles[t].e_wu].push_back(t);
        }
        std::vector<unsigned char> edges_found(separating_triangles.size(), 0);
        std::vector<std::pair<vertex_t, size_t>> dfs2_stack = { { root, 0 } };
        while (!dfs2_stack.empty())
        {
            auto &[v, edge_ix] = dfs2_stack.back();
            if (edge_ix < edge_order[v].size())
            {
                const auto e = edge_order[v][edge_ix];
                ++edge_ix;

                DEBUG_PRINT("traversing " << e);
                std::vector<size_t> current_triangles;
                for (auto t : triangles_by_edge[e])
                    if (++edges_found[t] == 3)
                        current_triangles.push_back(t);
                const size_t w = v == graph.edges[e].tail ? graph.edges[e].head : graph.edges[e].tail;
                if (!current_triangles.empty())
                {
                    DEBUG_PRINT("found " << current_triangles.size() << " triangles");
                    std::vector<size_t> dist_from_e(current_triangles.size());
                    size_t max_dist = 0;
                    const size_t p = parent_edge[w];
                    const size_t index_of_p = w == graph.edges[p].tail ? graph.edges[p].index_at_tail : graph.edges[p].index_at_head;
                    const size_t index_of_e = w == graph.edges[e].tail ? graph.edges[e].index_at_tail : graph.edges[e].index_at_head;
                    for (size_t i = 0; i < current_triangles.size(); ++i)
                    {
                        const auto &t = separating_triangles[current_triangles[i]];
                        const size_t u = t.u + t.v + t.w - v - w;
                        size_t index_of_wu;
                        if (w == graph.edges[t.e_uv].tail && u == graph.edges[t.e_uv].head)
                            index_of_wu = graph.edges[t.e_uv].index_at_tail;
                        else if (w == graph.edges[t.e_uv].head && u == graph.edges[t.e_uv].tail)
                            index_of_wu = graph.edges[t.e_uv].index_at_head;
                        else if (w == graph.edges[t.e_vw].tail && u == graph.edges[t.e_vw].head)
                            index_of_wu = graph.edges[t.e_vw].index_at_tail;
                        else if (w == graph.edges[t.e_vw].head && u == graph.edges[t.e_vw].tail)
                            index_of_wu = graph.edges[t.e_vw].index_at_head;
                        else if (w == graph.edges[t.e_wu].tail && u == graph.edges[t.e_wu].head)
                            index_of_wu = graph.edges[t.e_wu].index_at_tail;
                        else // if (w == graph.edges[t.e_wu].head && u == graph.edges[t.e_wu].tail)
                            index_of_wu = graph.edges[t.e_wu].index_at_head;
                        dist_from_e[i] = (graph.degree(w) + index_of_wu - index_of_e) % graph.degree(w);
                        const size_t dist_p_e = (graph.degree(w) + index_of_p - index_of_e) % graph.degree(w);
                        bool left = dist_from_e[i] >= dist_p_e && dist_p_e != 0;
                        if (left)
                            dist_from_e[i] = graph.degree(w) - dist_from_e[i];
                        if (dist_from_e[i] > max_dist)
                            max_dist = dist_from_e[i];
                    }
                    for (size_t i = 0; i < current_triangles.size(); ++i)
                        DEBUG_PRINT("triangle " << graph.labels[separating_triangles[current_triangles[i]].u] << '-'
                                << graph.labels[separating_triangles[current_triangles[i]].v] << '-'
                                << graph.labels[separating_triangles[current_triangles[i]].w] << " has distance " << dist_from_e[i]);
                    std::vector<std::vector<size_t>> triangles_by_dist_from_e(max_dist + 1);
                    for (size_t i = 0; i < current_triangles.size(); ++i)
                        triangles_by_dist_from_e[dist_from_e[i]].push_back(current_triangles[i]);
                    for (auto &ts : triangles_by_dist_from_e)
                        for (auto t : ts)
                            triangle_order.push_back(t);
                }
                if (!back_edge[e])
                    dfs2_stack.emplace_back(w, 0);
            }
            else
                dfs2_stack.pop_back();
        }
    }

    struct mut_edge_t {
        size_t tail, head;
        cyclic_list<size_t>::iterator tail_it, head_it;
    };
    std::vector<mut_edge_t> mut_edges;
    std::vector<cyclic_list<size_t>> adjacency_list(graph.num_vertices());
    std::vector<size_t> original_edge(graph.num_edges() + 3 * separating_triangles.size());
    std::vector<size_t> original_vertex;
    original_vertex.reserve(graph.num_vertices() + 3 * separating_triangles.size());
    for (vertex_t v = 0; v < graph.num_vertices(); ++v)
        original_vertex.push_back(v);
    const size_t virtual_edge = original_edge.size();
    {
        /* split graph along separating triangles */
        mut_edges.reserve(original_edge.size());
        for (const auto &e : graph.edges)
            mut_edges.push_back({ e.tail, e.head, {}, {} });
        for (vertex_t v = 0; v < graph.num_vertices(); ++v)
            for (size_t e : graph.vertices[v])
            {
                auto it = adjacency_list[v].push_back(e);
                if (v == mut_edges[e].tail)
                    mut_edges[e].tail_it = it;
                else
                    mut_edges[e].head_it = it;
            }
        for (size_t e = 0; e < graph.num_edges(); ++e)
            original_edge[e] = e;

        for (auto i : triangle_order)
        {
            const auto &t = separating_triangles[i];

            const bool uv_reversed = t.u == mut_edges[t.e_uv].head;
            const bool vw_reversed = t.v == mut_edges[t.e_vw].head;
            const bool wu_reversed = t.w == mut_edges[t.e_wu].head;
            auto it_uv = uv_reversed ? mut_edges[t.e_uv].head_it : mut_edges[t.e_uv].tail_it;
            auto it_vu = uv_reversed ? mut_edges[t.e_uv].tail_it : mut_edges[t.e_uv].head_it;
            auto it_vw = vw_reversed ? mut_edges[t.e_vw].head_it : mut_edges[t.e_vw].tail_it;
            auto it_wv = vw_reversed ? mut_edges[t.e_vw].tail_it : mut_edges[t.e_vw].head_it;
            auto it_wu = wu_reversed ? mut_edges[t.e_wu].head_it : mut_edges[t.e_wu].tail_it;
            auto it_uw = wu_reversed ? mut_edges[t.e_wu].tail_it : mut_edges[t.e_wu].head_it;

            vertex_t first_found = t.u;
            if (height[t.v] < height[t.u])
                first_found = t.v;
            if (height[t.w] < height[first_found])
                first_found = t.w;
            auto find_outside = [&](vertex_t u, size_t e_uv, size_t e_wu, bool uv_reversed, bool wu_reversed) -> bool
            {
                const size_t e_up = parent_edge[u];
                const size_t index_up = u == graph.edges[e_up].tail ? graph.edges[e_up].index_at_tail : graph.edges[e_up].index_at_head;
                const size_t index_uv = uv_reversed ? graph.edges[e_uv].index_at_head : graph.edges[e_uv].index_at_tail;
                const size_t index_uw = wu_reversed ? graph.edges[e_wu].index_at_tail : graph.edges[e_wu].index_at_head;
                const size_t dist_wp = (graph.degree(u) + index_up - index_uw) % graph.degree(u);
                const size_t dist_wv = (graph.degree(u) + index_uv - index_uw) % graph.degree(u);
                return dist_wv < dist_wp || dist_wp == 0; // true iff uvw is the clockwise order of the triangle
            };
            bool uvw_reversed;
            if (t.u == first_found)
                uvw_reversed = find_outside(t.u, t.e_uv, t.e_wu, uv_reversed, wu_reversed);
            else if (t.v == first_found)
                uvw_reversed = find_outside(t.v, t.e_vw, t.e_uv, vw_reversed, uv_reversed);
            else // if (t.w == first_found)
                uvw_reversed = find_outside(t.w, t.e_wu, t.e_vw, wu_reversed, vw_reversed);

            const vertex_t u_ = adjacency_list.size();
            adjacency_list.emplace_back();
            const vertex_t v_ = adjacency_list.size();
            adjacency_list.emplace_back();
            const vertex_t w_ = adjacency_list.size();
            adjacency_list.emplace_back();
            const size_t e_uv_ = mut_edges.size();
            mut_edges.push_back({ graph.edges[t.e_uv].tail, graph.edges[t.e_uv].head, {}, {} });
            const size_t e_vw_ = mut_edges.size();
            mut_edges.push_back({ graph.edges[t.e_vw].tail, graph.edges[t.e_vw].head, {}, {} });
            const size_t e_wu_ = mut_edges.size();
            mut_edges.push_back({ graph.edges[t.e_wu].tail, graph.edges[t.e_wu].head, {}, {} });

            original_edge[e_uv_] = t.e_uv;
            original_edge[e_vw_] = t.e_vw;
            original_edge[e_wu_] = t.e_wu;

            if (!uvw_reversed)
            {
                adjacency_list[u_].splice(adjacency_list[u_].begin(), adjacency_list[t.u], it_uv, it_uw);
                adjacency_list[v_].splice(adjacency_list[v_].begin(), adjacency_list[t.v], it_vw, it_vu);
                adjacency_list[w_].splice(adjacency_list[w_].begin(), adjacency_list[t.w], it_wu, it_wv);
                adjacency_list[u_].front() = e_uv_;
                adjacency_list[u_].back() = e_wu_;
                adjacency_list[v_].front() = e_vw_;
                adjacency_list[v_].back() = e_uv_;
                adjacency_list[w_].front() = e_wu_;
                adjacency_list[w_].back() = e_vw_;
                mut_edges[e_uv_].tail_it = uv_reversed ? --adjacency_list[v_].end() : adjacency_list[u_].begin();
                mut_edges[e_uv_].head_it = uv_reversed ? adjacency_list[u_].begin() : --adjacency_list[v_].end();
                mut_edges[e_vw_].tail_it = vw_reversed ? --adjacency_list[w_].end() : adjacency_list[v_].begin();
                mut_edges[e_vw_].head_it = vw_reversed ? adjacency_list[v_].begin() : --adjacency_list[w_].end();
                mut_edges[e_wu_].tail_it = wu_reversed ? --adjacency_list[u_].end() : adjacency_list[w_].begin();
                mut_edges[e_wu_].head_it = wu_reversed ? adjacency_list[w_].begin() : --adjacency_list[u_].end();
            }
            else
            {
                adjacency_list[u_].splice(adjacency_list[u_].begin(), adjacency_list[t.u], it_uw, it_uv);
                adjacency_list[v_].splice(adjacency_list[v_].begin(), adjacency_list[t.v], it_vu, it_vw);
                adjacency_list[w_].splice(adjacency_list[w_].begin(), adjacency_list[t.w], it_wv, it_wu);
                adjacency_list[u_].front() = e_wu_;
                adjacency_list[u_].back() = e_uv_;
                adjacency_list[v_].front() = e_uv_;
                adjacency_list[v_].back() = e_vw_;
                adjacency_list[w_].front() = e_vw_;
                adjacency_list[w_].back() = e_wu_;
                mut_edges[e_uv_].tail_it = uv_reversed ? adjacency_list[v_].begin() : --adjacency_list[u_].end();
                mut_edges[e_uv_].head_it = uv_reversed ? --adjacency_list[u_].end() : adjacency_list[v_].begin();
                mut_edges[e_vw_].tail_it = vw_reversed ? adjacency_list[w_].begin() : --adjacency_list[v_].end();
                mut_edges[e_vw_].head_it = vw_reversed ? --adjacency_list[v_].end() : adjacency_list[w_].begin();
                mut_edges[e_wu_].tail_it = wu_reversed ? adjacency_list[u_].begin() : --adjacency_list[w_].end();
                mut_edges[e_wu_].head_it = wu_reversed ? --adjacency_list[w_].end() : adjacency_list[u_].begin();
            }

            auto fix_edges = [&](vertex_t x, vertex_t x_) -> void
            {
                for (size_t e : adjacency_list[x_])
                    if (e != virtual_edge)
                        (x == mut_edges[e].tail ? mut_edges[e].tail : mut_edges[e].head) = x_;
            };
            fix_edges(t.u, u_);
            fix_edges(t.v, v_);
            fix_edges(t.w, w_);

            /* add virtual edge if there is a pincer */
            auto is_pincer = [&](vertex_t u_) -> bool
            {
                const bool first_outgoing = u_ == mut_edges[adjacency_list[u_].front()].tail;
                const bool last_outgoing  = u_ == mut_edges[adjacency_list[u_].back()].tail;
                if (first_outgoing != last_outgoing)
                    return false;
                for (size_t e : adjacency_list[u_])
                    if (e == virtual_edge || first_outgoing != (u_ == mut_edges[e].tail))
                        return true;
                return false;
            };
            if (is_pincer(u_))
                adjacency_list[t.u].insert(uvw_reversed ? it_uv : it_uw, virtual_edge);
            if (is_pincer(v_))
                adjacency_list[t.v].insert(uvw_reversed ? it_vw : it_vu, virtual_edge);
            if (is_pincer(w_))
                adjacency_list[t.w].insert(uvw_reversed ? it_wu : it_wv, virtual_edge);

            DEBUG_PRINT("splitting at triangle " << graph.labels[t.u] << '-'
                    << graph.labels[t.v] << '-' << graph.labels[t.w]
                    << (uvw_reversed ? "; it will be reversed" : ""));

            original_vertex.push_back(t.u);
            original_vertex.push_back(t.v);
            original_vertex.push_back(t.w);

            separating_triangles[i].u = u_;
            separating_triangles[i].v = v_;
            separating_triangles[i].w = w_;

            if (uvw_reversed)
                std::swap(separating_triangles[i].v, separating_triangles[i].w);
        }
    }

    {
        /* permute adjacency list of graph.outer_face[0-2] */
        auto permute_list = [&](vertex_t u, vertex_t v) -> void
        {
            auto it = adjacency_list[u].begin();
            while (*it == virtual_edge || mut_edges[*it].tail + mut_edges[*it].head != u + v)
                ++it;
            adjacency_list[u].reset_head(it);
        };
        permute_list(graph.outer_face[0], graph.outer_face[1]);
        permute_list(graph.outer_face[1], graph.outer_face[2]);
        permute_list(graph.outer_face[2], graph.outer_face[0]);
    }

#ifdef DEBUG
    for (vertex_t v = 0; v < adjacency_list.size(); ++v)
    {
        DEBUG_PRINT(v);
        for (auto e : adjacency_list[v])
            DEBUG_PRINT('\t' << e);
    }
#endif // DEBUG

    four_block_tree_t result;
    {
        /* find 4-connected components */
        triangle_order.push_back(separating_triangles.size());
        separating_triangles.push_back({ graph.outer_face[0], graph.outer_face[1], graph.outer_face[2], {}, {}, {} });

        std::vector<bool> visited(adjacency_list.size(), false);
        std::vector<bool> traversed(mut_edges.size(), false);
        std::vector<size_t> mapped_vertex(adjacency_list.size());
        std::vector<size_t> mapped_edge(mut_edges.size());
        for (auto it = triangle_order.crbegin(); it != triangle_order.crend(); ++it)
        {
            size_t i = *it;
            result.emplace_back();

            const vertex_t bfs_root = separating_triangles[i].u;
            std::vector<vertex_t> bfs_queue = { bfs_root };
            DEBUG_PRINT("finding component with vertex " << bfs_root);
            for (size_t v_ = 0; v_ < bfs_queue.size(); ++v_)
            {
                const vertex_t v = bfs_queue[v_];
                for (size_t e : adjacency_list[v])
                    if (e != virtual_edge && !traversed[e])
                    {
                        DEBUG_PRINT("found edge " << e);
                        traversed[e] = true;
                        mapped_edge[e] = result.back().original_edge.size();
                        result.back().original_edge.push_back(e);
                        const vertex_t w = v == mut_edges[e].tail ? mut_edges[e].head : mut_edges[e].tail;
                        if (!visited[w])
                        {
                            DEBUG_PRINT("found vertex " << w);
                            visited[w] = true;
                            mapped_vertex[w] = bfs_queue.size();
                            bfs_queue.push_back(w);
                        }
                    }
            }

            result.back().outer_face = { mapped_vertex[separating_triangles[i].u], mapped_vertex[separating_triangles[i].v], mapped_vertex[separating_triangles[i].w] };
            result.back().edges.reserve(result.back().original_edge.size());
            result.back().vertices.reserve(bfs_queue.size());
            result.back().designated_face.resize(bfs_queue.size(), 0);

            for (size_t &e : result.back().original_edge)
            {
                result.back().edges.push_back({ mapped_vertex[mut_edges[e].tail], mapped_vertex[mut_edges[e].head] });
                e = original_edge[e];
            }
            for (vertex_t v_ = 0; v_ < bfs_queue.size(); ++v_)
            {
                const vertex_t v = bfs_queue[v_];
                result.back().labels.push_back(graph.labels[original_vertex[v]]);
                result.back().vertices.emplace_back();
                for (size_t e : adjacency_list[v])
                {
                    if (e == virtual_edge)
                        result.back().designated_face[v_] = result.back().vertices.back().size() + 1;
                    else
                    {
                        const size_t e_ = mapped_edge[e];
                        if (v_ == result.back().edges[e_].tail)
                            result.back().edges[e_].index_at_tail = result.back().vertices.back().size();
                        else
                            result.back().edges[e_].index_at_head = result.back().vertices.back().size();
                        result.back().vertices.back().push_back(e_);
                    }
                }
                DEBUG_PRINT("vertex " << v << " has designated face " << result.back().designated_face[v_]);
            }
#ifdef DEBUG
            DEBUG_PRINT("component:");
            DEBUG_PRINT("\touter face:");
            for (vertex_t v : result.back().outer_face)
                DEBUG_PRINT("\t\t" << v);
            DEBUG_PRINT("\tedges:");
            for (size_t e_ = 0; e_ < result.back().num_edges(); ++e_)
            {
                const auto &e = result.back().edges[e_];
                DEBUG_PRINT("\t\t" << e_ << ": " << e.tail << '-' << e.head << " (" << e.index_at_tail << ',' << e.index_at_head << ") was " << result.back().original_edge[e_]);
            }
            DEBUG_PRINT("\tvertices:");
            for (vertex_t v_ = 0; v_ < result.back().num_vertices(); ++v_)
            {
                DEBUG_PRINT("\t\t" << v_);
                for (size_t e_ : result.back().vertices[v_])
                    DEBUG_PRINT("\t\t\t" << e_);
            }
#endif // DEBUG
        }
    }

    return result;
}
