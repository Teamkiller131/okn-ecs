#include <okn/ecs/scheduler/system_graph.hpp>
#include <algorithm>
#include <unordered_set>
#include <deque>
#include <stdexcept>

namespace okn::ecs {

void SystemGraph::add_system(std::unique_ptr<System> system) {
    if (!system) return;
    const auto& name = system->name();
    if (name_index_.contains(name)) {
        return;
    }
    usize idx = systems_.size();
    name_index_[name] = idx;
    systems_.push_back(std::move(system));
}

auto SystemGraph::get_system(const std::string& name) -> System* {
    auto it = name_index_.find(name);
    if (it == name_index_.end()) {
        return nullptr;
    }
    return systems_[it->second].get();
}

auto SystemGraph::resolve_name_index(const std::string& name) const -> usize {
    auto it = name_index_.find(name);
    if (it == name_index_.end()) {
        return static_cast<usize>(-1);
    }
    return it->second;
}

auto SystemGraph::has_conflict(const System& a, const System& b) const -> bool {
    auto a_reads = a.reads();
    auto a_writes = a.writes();
    auto b_reads = b.reads();
    auto b_writes = b.writes();

    for (auto aw : a_writes) {
        for (auto br : b_reads) {
            if (aw == br) return true;
        }
        for (auto bw : b_writes) {
            if (aw == bw) return true;
        }
    }

    for (auto bw : b_writes) {
        for (auto ar : a_reads) {
            if (bw == ar) return true;
        }
    }

    return false;
}

auto SystemGraph::build_execution_order() -> std::vector<System*> {
    const usize n = systems_.size();
    std::vector<std::vector<usize>> adj(n);
    std::vector<usize> in_degree(n, 0);

    for (usize i = 0; i < n; ++i) {
        const auto& sys = systems_[i];
        for (const auto& before_name : sys->before()) {
            usize bi = resolve_name_index(before_name);
            if (bi < n) {
                adj[bi].push_back(i);
                in_degree[i]++;
            }
        }
        for (const auto& after_name : sys->after()) {
            usize ai = resolve_name_index(after_name);
            if (ai < n) {
                adj[i].push_back(ai);
                in_degree[ai]++;
            }
        }
    }

    for (usize i = 0; i < n; ++i) {
        for (usize j = i + 1; j < n; ++j) {
            if (has_conflict(*systems_[i], *systems_[j])) {
                bool i_to_j = false;
                bool j_to_i = false;

                std::vector<bool> visited(n, false);
                std::deque<usize> q;
                q.push_back(i);
                visited[i] = true;
                while (!q.empty()) {
                    usize cur = q.front();
                    q.pop_front();
                    if (cur == j) { i_to_j = true; break; }
                    for (auto next : adj[cur]) {
                        if (!visited[next]) {
                            visited[next] = true;
                            q.push_back(next);
                        }
                    }
                }

                if (!i_to_j) {
                    std::fill(visited.begin(), visited.end(), false);
                    q.clear();
                    q.push_back(j);
                    visited[j] = true;
                    while (!q.empty()) {
                        usize cur = q.front();
                        q.pop_front();
                        if (cur == i) { j_to_i = true; break; }
                        for (auto next : adj[cur]) {
                            if (!visited[next]) {
                                visited[next] = true;
                                q.push_back(next);
                            }
                        }
                    }
                }

                if (!i_to_j && !j_to_i) {
                    adj[i].push_back(j);
                    in_degree[j]++;
                }
            }
        }
    }

    std::deque<usize> q;
    for (usize i = 0; i < n; ++i) {
        if (in_degree[i] == 0) {
            q.push_back(i);
        }
    }

    std::vector<System*> result;
    result.reserve(n);

    while (!q.empty()) {
        usize cur = q.front();
        q.pop_front();
        result.push_back(systems_[cur].get());
        for (auto next : adj[cur]) {
            if (--in_degree[next] == 0) {
                q.push_back(next);
            }
        }
    }

    return result;
}

} // namespace okn::ecs
