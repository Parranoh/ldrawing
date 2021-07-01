#include <vector>
#include <random>
#include <functional>
#include <iostream>
#include <sstream>
#include <list>

std::vector<bool> random_bitstring(size_t length, size_t weight)
{
    size_t buckets = weight + 1;
    size_t items = length - weight;
    std::vector<size_t> num_zeros_after_one(buckets);

    std::random_device rd;
    std::mt19937 gen(rd());
    auto random_double = std::bind(std::generate_canonical<double, std::numeric_limits<double>::digits, std::mt19937>, gen);
    while (items > 0)
    {
        if (random_double() < double(items) / double(buckets + items - 1))
        {
            ++num_zeros_after_one[buckets - 1];
            --items;
        }
        else
            --buckets;
    }

    std::vector<bool> bitstring;
    bitstring.reserve(length + 1);
    for (size_t z : num_zeros_after_one)
    {
        for (size_t i = 0; i < z; ++i)
            bitstring.push_back(false);
        bitstring.push_back(true);
    }
    bitstring.pop_back();

    return bitstring;
}

std::vector<bool> find_permutation(const std::vector<bool> &in)
{
    int ones3_zeros = 0, value_of_minimum = 3 * in.size();
    size_t index_of_minimum = 0;
    for (size_t i = 0; i < in.size(); ++i)
    {
        ones3_zeros += in[i] ? 3 : -1;
        if (ones3_zeros < value_of_minimum)
        {
            value_of_minimum = ones3_zeros;
            index_of_minimum = i;
        }
    }

    std::vector<bool> out;
    for (size_t i = index_of_minimum + 1; i < in.size(); ++i)
        out.push_back(in[i]);
    for (size_t i = 0; i < index_of_minimum + 1; ++i)
        out.push_back(in[i]);
    return out;
}

struct graph_t {
    struct edge_t {
        size_t tail, head;
        std::list<size_t>::iterator tail_it, head_it;
    };
    std::vector<edge_t> edges;
    std::vector<std::list<size_t>> vertices;
};

graph_t code_to_tree(const std::vector<bool> &bitstring)
{
    const size_t leaf = bitstring.size() / 4 + 1;
    std::vector<size_t> stack;
    std::vector<unsigned char> num_leaves;
    graph_t graph;

    /* add initial vertex */
    stack.push_back(0);
    num_leaves.push_back(0);
    graph.vertices.emplace_back();

    for (auto b : bitstring)
    {
        if (b)
        {
            /* add inner vertex */
            size_t old_top = stack.back();
            size_t new_top = graph.vertices.size();
            size_t new_edge = graph.edges.size();

            graph.vertices.emplace_back();
            graph.vertices[old_top].push_back(new_edge);
            graph.vertices[new_top].push_back(new_edge);
            auto tail_it = --graph.vertices[old_top].end();
            auto head_it = --graph.vertices[new_top].end();
            graph.edges.push_back({ old_top, new_top, tail_it, head_it });

            num_leaves.push_back(0);
            stack.push_back(new_top);
        }
        else
        {
            size_t top = stack.back();
            if (num_leaves[top] < 2)
            {
                /* add leaf */
                size_t new_edge = graph.edges.size();
                graph.vertices[top].push_back(new_edge);
                auto it = -- graph.vertices[top].end();
                graph.edges.push_back({ top, leaf, it, it });

                ++num_leaves[top];
            }
            else
            {
                /* go up */
                stack.pop_back();
            }
        }
    }

    return graph;
}

/* outer face will be (return_value,graph.size()-2,graph.size()-1) */
size_t complete_closure(graph_t &graph)
{
    const size_t leaf = graph.vertices.size();

    std::vector<std::pair<graph_t::edge_t *, bool>> stack;
    auto first_edge = &graph.edges[graph.vertices.front().front()];
    stack.emplace_back(first_edge, false);

    auto next_edge = [&](const std::pair<graph_t::edge_t *, bool> &pair)
    {
        auto [edge, reversed] = pair;
        size_t old_head = reversed ? edge->tail : edge->head;
        if (old_head == leaf)
            return std::pair(edge, !reversed);
        else
        {
            auto it = reversed ? edge->tail_it : edge->head_it;
            ++it;
            if (it == graph.vertices[old_head].end())
                it = graph.vertices[old_head].begin();
            auto next_edge = &graph.edges[*it];
            bool next_reversed = old_head != next_edge->tail;
            return std::pair(next_edge, next_reversed);
        }
    };

    auto is_leaf_edge = [&](const graph_t::edge_t &edge)
    { return edge.tail_it == edge.head_it; };

    auto top_is_admissible_triangle = [&]()
    {
        return stack.size() >= 3
            && !is_leaf_edge(*stack[stack.size() - 3].first)
            && !is_leaf_edge(*stack[stack.size() - 2].first)
            && is_leaf_edge(*stack[stack.size() - 1].first);
    };

    /* total number of edges is 3 * n, so in the end every edges is examined
     * in both directions at least twice (twice around the graph) */
    while (stack.size() < 12 * graph.vertices.size())
    {
        stack.push_back(next_edge(stack.back()));
        while (top_is_admissible_triangle())
        {
            /* local closure */
            auto [e1, e1_reversed] = stack[stack.size() - 3];
            auto [e3, e3_reversed] = stack[stack.size() - 1];
            size_t e3_index = e3 - &graph.edges[0];
            auto e1_tail = e1_reversed ? e1->head : e1->tail;
            const auto &e1_tail_it = e1_reversed ? e1->head_it : e1->tail_it;
            auto &e3_head = e3_reversed ? e3->tail : e3->head;
            auto &e3_head_it = e3_reversed ? e3->tail_it : e3->head_it;
            e3_head = e1_tail;
            e3_head_it = graph.vertices[e1_tail].insert(e1_tail_it, e3_index);

            stack.pop_back();
            stack.pop_back();
            stack.pop_back();
            stack.emplace_back(e3, !e3_reversed);
        }
    }

    /* find v0 */
    auto edge_on_outer_face = stack.back();
    unsigned char consecutive_leaf_edges = 0;
    while (consecutive_leaf_edges < 4)
    {
        edge_on_outer_face = next_edge(edge_on_outer_face);
        if (is_leaf_edge(*edge_on_outer_face.first))
            ++consecutive_leaf_edges;
        else
            consecutive_leaf_edges = 0;
    }
    auto l1 = edge_on_outer_face.first;
    auto v0 = l1->tail;

    size_t v1 = graph.vertices.size();
    graph.vertices.emplace_back();
    size_t v2 = graph.vertices.size();
    graph.vertices.emplace_back();

    while (true)
    {
        size_t edge_index = edge_on_outer_face.first - &graph.edges[0];
        graph.vertices[v1].push_front(edge_index);
        edge_on_outer_face.first->head = v1;
        edge_on_outer_face.first->head_it = graph.vertices[v1].begin();

        /* reverse edge as if we just came from v1 */
        edge_on_outer_face.second = true;
        edge_on_outer_face = next_edge(edge_on_outer_face);
        if (is_leaf_edge(*edge_on_outer_face.first))
            break;
        edge_on_outer_face = next_edge(edge_on_outer_face);
    }
    while (true)
    {
        size_t edge_index = edge_on_outer_face.first - &graph.edges[0];
        graph.vertices[v2].push_front(edge_index);
        edge_on_outer_face.first->head = v2;
        edge_on_outer_face.first->head_it = graph.vertices[v2].begin();

        /* reverse edge as if we just came from v2 */
        edge_on_outer_face.second = true;
        edge_on_outer_face = next_edge(edge_on_outer_face);
        if (edge_on_outer_face.first == l1)
            break;
        edge_on_outer_face = next_edge(edge_on_outer_face);
    }

    /* add edge (v1,v2) */
    size_t e = graph.edges.size();
    graph.vertices[v1].push_front(e);
    graph.vertices[v2].push_front(e);
    graph.edges.push_back({ v1, v2, graph.vertices[v1].begin(), graph.vertices[v2].begin() });

    return v0;
}

void make_bimodal(graph_t &graph)
{
    const size_t s = graph.vertices.size() - 2;
    std::vector<bool> visited(graph.vertices.size(), false);
    std::vector<bool> traversed(graph.edges.size(), false);
    std::vector<bool> oriented(graph.edges.size(), false);
    std::vector<size_t> parent(graph.vertices.size());
    std::vector<size_t> edge_to_parent(graph.vertices.size());
    std::vector<size_t> active_child_edge(graph.vertices.size());
    std::vector<std::vector<size_t>> postponed_back_edges(graph.edges.size());
    std::vector<std::pair<size_t, std::list<size_t>::iterator>> stack = { { s, graph.vertices[s].begin() } };
    visited[s] = true;
    oriented.back() = true;

    auto reverse_edge = [&](size_t edge_index)
    {
        auto &e = graph.edges[edge_index];
        e = { e.head, e.tail, e.head_it, e.tail_it };
    };

    while (!stack.empty())
    {
        auto &[v, it] = stack.back();
        size_t e = *it;
        if (++it == graph.vertices[v].end())
            stack.pop_back();
        bool forward = v == graph.edges[e].tail;
        size_t w = forward ? graph.edges[e].head : graph.edges[e].tail;
        if (!visited[w])
        {
            /* tree edge */
#ifdef DEBUG_PRINT
            std::cerr << "tree edge " << e << std::endl;
#endif // DEBUG_PRINT
            parent[w] = v;
            active_child_edge[v] = e;
            edge_to_parent[w] = e;
            visited[w] = true;
            stack.emplace_back(w, graph.vertices[w].begin());
        }
        else if (!traversed[e])
        {
            /* back edge */
#ifdef DEBUG_PRINT
            std::cerr << "back edge " << e << std::endl;
#endif // DEBUG_PRINT
            if (!forward)
                reverse_edge(e);
            // v is in subtree rooted at x, x is child of w
            size_t e_wx = active_child_edge[w];
            postponed_back_edges[e_wx].push_back(e);
            std::vector<size_t> tree_edges_with_postponed_back_edges;
            if (oriented[e_wx])
                tree_edges_with_postponed_back_edges.push_back(e_wx);
            for (size_t i = 0; i < tree_edges_with_postponed_back_edges.size(); ++i)
            {
                const size_t e_wx = tree_edges_with_postponed_back_edges[i];
                const size_t w = graph.edges[postponed_back_edges[e_wx].front()].head;
                bool away_from_w = w == graph.edges[e_wx].tail;
                for (size_t back_edge : postponed_back_edges[e_wx])
                {
                    if (away_from_w)
                        reverse_edge(back_edge);
                    oriented[back_edge] = true;
                    size_t u = away_from_w ? graph.edges[back_edge].head : graph.edges[back_edge].tail;
                    size_t parent_edge_of_u = edge_to_parent[u];
                    while (!oriented[parent_edge_of_u])
                    {
                        if ((u == graph.edges[parent_edge_of_u].tail) ^ away_from_w)
                            reverse_edge(parent_edge_of_u);
                        oriented[parent_edge_of_u] = true;
                        if (!postponed_back_edges[parent_edge_of_u].empty())
                            tree_edges_with_postponed_back_edges.push_back(parent_edge_of_u);
                        u = parent[u];
                        parent_edge_of_u = edge_to_parent[u];
                    }
                }
                postponed_back_edges[e_wx].clear();
            }
        }
        traversed[e] = true;
    }

#ifdef DEBUG_PRINT
    for (size_t v = 0; v < parent.size(); ++v)
        std::cerr << "parent[" << v << "] = " << parent[v]
            << ", edge_to_parent[" << v << "] = " << edge_to_parent[v] << std::endl;

    for (bool o : oriented)
        if (!o)
            exit(1);
#endif // DEBUG_PRINT
}

void insert_2_cycles(graph_t &graph)
{
    /* find 0-modal vertices */
    std::vector<bool> has_outgoing(graph.vertices.size(), false);
    std::vector<bool> has_incoming(graph.vertices.size(), false);
    for (const auto &e : graph.edges)
    {
        has_outgoing[e.tail] = true;
        has_incoming[e.head] = true;
    }
    std::vector<bool> is_0_modal;
    is_0_modal.reserve(graph.vertices.size());
    for (size_t v = 0; v < graph.vertices.size(); ++v)
        is_0_modal.push_back(!has_incoming[v] || !has_outgoing[v]);

    /* duplicate edges where possible */
    std::random_device rd;
    std::mt19937 gen(rd());
    auto random_double = std::bind(std::generate_canonical<double, std::numeric_limits<double>::digits, std::mt19937>, gen);
    auto random_bool = [&]() -> bool { return random_double() < .5; };

    auto inc = [&](std::list<size_t>::iterator &it, size_t v) -> void
    {
        ++it;
        if (it == graph.vertices[v].end())
            it = graph.vertices[v].begin();
    };
    auto dec = [&](std::list<size_t>::iterator &it, size_t v) -> void
    {
        if (it == graph.vertices[v].begin())
            it = graph.vertices[v].end();
        --it;
    };

    size_t num_edges = graph.edges.size();
    for (size_t i = 0; i < num_edges; ++i)
    {
        const auto &e = graph.edges[i];
        const bool left = false; // right = true
        auto try_adding = [&](bool side) -> bool
        {
            if (!is_0_modal[e.tail])
            {
                auto tail_it = e.tail_it;
                if (side == left)
                    inc(tail_it, e.tail);
                else
                    dec(tail_it, e.tail);
                if (e.tail == graph.edges[*tail_it].tail) // next edge is also outgoing
                    return false;
            }
            if (!is_0_modal[e.head])
            {
                auto head_it = e.head_it;
                if (side == left)
                    dec(head_it, e.head);
                else
                    inc(head_it, e.head);
                if (e.head == graph.edges[*head_it].head) // next edge is also incoming
                    return false;
            }

            // If this point is reached, the edge can be inserted.
            is_0_modal[e.tail] = is_0_modal[e.head] = false;
            size_t new_edge = graph.edges.size();
            auto tail_insertion_point = e.tail_it;
            auto head_insertion_point = e.head_it;
            if (side == left)
                ++tail_insertion_point;
            else
                ++head_insertion_point;
            graph.vertices[e.tail].insert(tail_insertion_point, new_edge);
            graph.vertices[e.head].insert(head_insertion_point, new_edge);
            graph.edges.push_back({ e.head, e.tail, --head_insertion_point, --tail_insertion_point });

#ifdef DEBUG_PRINT
            std::cerr << "added 2-cycle to " << (side ? "right" : "left") << " side of edge " << e.tail+1 << '-' << e.head+1 << std::endl;
#endif // DEBUG_PRINT

            return true;
        };
        bool side = random_bool();
        if (!try_adding(side))
            try_adding(!side);
    }
}

void print_graph(const graph_t &graph, size_t v0)
{
    std::cout << graph.vertices.size() << ' ' << graph.edges.size() << " 3" << std::endl;
    std::cout << v0+1 << ' ' << graph.vertices.size()-1 << ' '
        << graph.vertices.size() << std::endl;
    for (size_t v = 0; v < graph.vertices.size(); ++v)
        std::cout << v+1 << std::endl;
    for (size_t i = 0; i < graph.edges.size(); ++i)
        std::cout << graph.edges[i].tail+1 << ' ' << graph.edges[i].head+1 << std::endl;
    for (size_t v = 0; v < graph.vertices.size(); ++v)
    {
        for (auto e : graph.vertices[v])
            std::cout << e+1 << ' ';
        std::cout << std::endl;
    }
}

void usage(char *name)
{
    std::cerr << "usage: " << name << " [ --2-cycles ] n" << std::endl;
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc < 2)
        usage(argv[0]);
    size_t n;
    if (!(std::istringstream(argv[argc - 1]) >> n))
        usage(argv[0]);
    if (n < 2)
    {
        std::cerr << "n must be at least 2" << std::endl;
        exit(1);
    }
    auto bitstring = random_bitstring(4 * n - 2, n - 1);
    bitstring = find_permutation(bitstring);
#ifdef DEBUG_PRINT
    std::cerr << "generated bitstring: ";
#endif // DEBUG_PRINT
    for (auto b : bitstring)
        std::cerr << (b ? '1' : '0');
    std::cerr << std::endl;

    auto graph = code_to_tree(bitstring);
    size_t v0 = complete_closure(graph);
    make_bimodal(graph);
    if (std::string(argv[1]) == std::string("--2-cycles"))
        insert_2_cycles(graph);
    print_graph(graph, v0);

    return 0;
}
