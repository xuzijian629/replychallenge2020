#include <bits/stdc++.h>  // clang-format off
using namespace std;
using Int = long long;
#define REP_(i, a_, b_, a, b, ...) for (int i = (a), lim_i = (b); i < lim_i; i++)
#define REP(i, ...) REP_((i), __VA_ARGS__, __VA_ARGS__, 0, __VA_ARGS__)
struct SetupIO { SetupIO() { cin.tie(nullptr), ios::sync_with_stdio(false), cout << fixed << setprecision(13); } } setup_io;
#ifndef MY_DEBUG
#define dump(...)
#endif  // clang-format on

const vector<pair<int, int>> DIRECTIONS = {
    {1, 0}, {0, 1},  {-1, 0},  {0, -1},  // 4方向
    {1, 1}, {-1, 1}, {-1, -1}, {1, -1},  // 斜め
    {0, 0},                              // 自身
};
mt19937 rnd(chrono::steady_clock::now().time_since_epoch().count());

constexpr int MAX_SKILL_COUNT = 400;

struct Developer {
    int id;
    int company, bonus;
    bitset<MAX_SKILL_COUNT> skills;
};

struct Manager {
    int id;
    int company, bonus;
};

int num_developer, num_manager;
int W, H;
vector<string> board;
vector<Developer> developers, developers_var;
vector<Manager> managers, managers_var;
vector<pair<int, int>> dev_places, man_places;

// 解を入れる配列
// H * Wでpos[i][j]は-1のとき、空いていてDeveloperが使える。-2のとき、空いていてManagerが使える
// -3のとき、使えない
// 0 <= i < nのとき、i番目のDeveloper
// n <= i < n + mのときi - n番目のManager
int best_score;
vector<vector<int>> ans_pos;
int num_companies;
int num_conn;
// H * Wで、同じ値は同じ連結成分。壁は-1
vector<vector<int>> conn;

inline int work_potential(const Developer& a, const Developer& b) {
    int cap = (a.skills & b.skills).count();
    int cup = (a.skills | b.skills).count();
    return cap * (cup - cap);
}

inline int bonus_potential(const Developer& a, const Developer& b) {
    return a.company == b.company ? a.bonus * b.bonus : 0;
}

inline int bonus_potential(const Developer& a, const Manager& b) {
    return a.company == b.company ? a.bonus * b.bonus : 0;
}

inline int bonus_potential(const Manager& a, const Manager& b) {
    return a.company == b.company ? a.bonus * b.bonus : 0;
}

void initialize_by_valid() {
    sort(developers_var.begin(), developers_var.end(), [](auto& a, auto& b) { return a.company < b.company; });
    sort(managers_var.begin(), managers_var.end(), [](auto& a, auto& b) { return a.company < b.company; });
    vector<pair<int, int>> valid_developer_pos, valid_manager_pos;
    for (int i = 0; i < H; i++) {
        if (i & 1) {
            for (int j = 0; j < W; j++) {
                if (ans_pos[i][j] == -1) valid_developer_pos.emplace_back(i, j);
                if (ans_pos[i][j] == -2) valid_manager_pos.emplace_back(i, j);
            }
        } else {
            for (int j = W - 1; j >= 0; j--) {
                if (ans_pos[i][j] == -1) valid_developer_pos.emplace_back(i, j);
                if (ans_pos[i][j] == -2) valid_manager_pos.emplace_back(i, j);
            }
        }
    }
    for (int i = 0; i < num_developer; i++) {
        if (i < valid_developer_pos.size()) {
            auto [a, b] = valid_developer_pos[i];
            ans_pos[a][b] = developers_var[i].id;
        }
    }
    for (int i = 0; i < num_manager; i++) {
        if (i < valid_manager_pos.size()) {
            auto [a, b] = valid_manager_pos[i];
            ans_pos[a][b] = managers_var[i].id + num_developer;
        }
    }
}

// bonus順ソート追加
// skillもソートしたけどB以外減った
void valid_sort() {
    sort(developers_var.begin(), developers_var.end(), [](auto& a, auto& b) {
        return make_tuple(a.company, -a.bonus, -a.skills.count()) <
               make_tuple(b.company, -b.bonus, -(int)b.skills.count());
    });
    sort(managers_var.begin(), managers_var.end(),
         [](auto& a, auto& b) { return make_tuple(a.company, -a.bonus) < make_tuple(b.company, -b.bonus); });
}

// WPに注目して
void C_sort() {
    // managerはどうでもいい
    sort(managers_var.begin(), managers_var.end(),
         [](auto& a, auto& b) { return make_tuple(a.company, -a.bonus) < make_tuple(b.company, -b.bonus); });

    vector<vector<Developer>> comp_devs(num_companies);
    for (const auto& d : developers_var) {
        comp_devs[d.company].push_back(d);
    }

    vector<Developer> new_developers;
    for (int k = 0; k < num_companies; k++) {
        int N = comp_devs[k].size();
        vector<vector<int>> mat(N, vector<int>(N));
        for (int i = 0; i < N; i++) {
            for (int j = i + 1; j < N; j++) {
                mat[i][j] = work_potential(comp_devs[k][i], comp_devs[k][j]);
                mat[j][i] = mat[i][j];
            }
        }
        // このN頂点完全グラフの最長パスを求めたい。とりあえず雑探索
        int best_score = -1;
        vector<int> best;
        // 始点全探索。あとはgreedy
        for (int i = 0; i < N; i++) {
            int cur_score = 0;
            vector<int> cur = {i};
            set<int> ok;
            for (int j = 0; j < N; j++) {
                ok.insert(j);
            }
            ok.erase(i);
            while (!ok.empty()) {
                int nax = -1;
                int idx = -1;
                for (int j : ok) {
                    if (mat[cur.back()][j] > nax) {
                        nax = mat[cur.back()][j];
                        idx = j;
                    }
                }
                assert(idx != -1);
                cur_score += nax;
                ok.erase(idx);
                cur.push_back(idx);
            }
            if (cur_score > best_score) {
                best_score = cur_score;
                best = cur;
            }
        }
        assert(best.size() == N);
        for (int i = 0; i < N; i++) {
            new_developers.push_back(comp_devs[k][best[i]]);
        }
    }
    swap(developers_var, new_developers);
}

// 同じ会社で連結成分にするのではなく、skillで同じ成分にする
void C_sort_grouped_by_skill() {
    // managerはどうでもいい
    sort(managers_var.begin(), managers_var.end(),
         [](auto& a, auto& b) { return make_tuple(a.company, -a.bonus) < make_tuple(b.company, -b.bonus); });

    set<int> ok;
    for (int i = 0; i < num_developer; i++) {
        ok.insert(i);
    }

    vector<vector<Developer>> skill_devs(MAX_SKILL_COUNT);
    for (int i = 0; i < MAX_SKILL_COUNT; i++) {
        for (const auto& d : developers_var) {
            if (!ok.count(d.id)) continue;
            if (d.skills[i]) {
                skill_devs[i].push_back(d);
                ok.erase(d.id);
            }
        }
    }

    vector<Developer> new_developers;
    for (int k = 0; k < MAX_SKILL_COUNT; k++) {
        int N = skill_devs[k].size();
        vector<vector<int>> mat(N, vector<int>(N));
        for (int i = 0; i < N; i++) {
            for (int j = i + 1; j < N; j++) {
                mat[i][j] = work_potential(skill_devs[k][i], skill_devs[k][j]);
                mat[i][j] += bonus_potential(skill_devs[k][i], skill_devs[k][j]);
                mat[j][i] = mat[i][j];
            }
        }
        // このN頂点完全グラフの最長パスを求めたい。とりあえず雑探索
        int best_score = -1;
        vector<int> best;
        // 始点全探索。あとはgreedy
        for (int i = 0; i < N; i++) {
            int cur_score = 0;
            vector<int> cur = {i};
            set<int> ok;
            for (int j = 0; j < N; j++) {
                ok.insert(j);
            }
            ok.erase(i);
            while (!ok.empty()) {
                int nax = -1;
                int idx = -1;
                for (int j : ok) {
                    if (mat[cur.back()][j] > nax) {
                        nax = mat[cur.back()][j];
                        idx = j;
                    }
                }
                assert(idx != -1);
                cur_score += nax;
                ok.erase(idx);
                cur.push_back(idx);
            }
            if (cur_score > best_score) {
                best_score = cur_score;
                best = cur;
            }
        }
        assert(best.size() == N);
        for (int i = 0; i < N; i++) {
            new_developers.push_back(skill_devs[k][best[i]]);
        }
    }
    swap(developers_var, new_developers);
}

// 同じ連結成分内を蛇行して敷き詰める
void initialize_by_conn() {
    // valid_sort();
    C_sort();
    vector<vector<pair<int, int>>> valid_developer_pos_conn(num_conn), valid_manager_pos_conn(num_conn);
    vector<pair<int, int>> valid_developer_pos, valid_manager_pos;
    for (int i = 0; i < H; i++) {
        if (i & 1) {
            for (int j = 0; j < W; j++) {
                if (ans_pos[i][j] == -1) valid_developer_pos_conn[conn[i][j]].emplace_back(i, j);
                if (ans_pos[i][j] == -2) valid_manager_pos_conn[conn[i][j]].emplace_back(i, j);
            }
        } else {
            for (int j = W - 1; j >= 0; j--) {
                if (ans_pos[i][j] == -1) valid_developer_pos_conn[conn[i][j]].emplace_back(i, j);
                if (ans_pos[i][j] == -2) valid_manager_pos_conn[conn[i][j]].emplace_back(i, j);
            }
        }
    }
    for (int i = 0; i < num_conn; i++) {
        for (auto& e : valid_developer_pos_conn[i]) {
            valid_developer_pos.push_back(e);
        }
        for (auto& e : valid_manager_pos_conn[i]) {
            valid_manager_pos.push_back(e);
        }
    }

    for (int i = 0; i < num_developer; i++) {
        if (i < valid_developer_pos.size()) {
            auto [a, b] = valid_developer_pos[i];
            ans_pos[a][b] = developers_var[i].id;
        }
    }
    for (int i = 0; i < num_manager; i++) {
        if (i < valid_manager_pos.size()) {
            auto [a, b] = valid_manager_pos[i];
            ans_pos[a][b] = managers_var[i].id + num_developer;
        }
    }
}

void print_solution() {
    vector<pair<int, int>> developer_pos(num_developer, {-1, -1}), manager_pos(num_manager, {-1, -1});
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            if (0 <= ans_pos[i][j]) {
                if (ans_pos[i][j] < num_developer) {
                    developer_pos[ans_pos[i][j]] = {i, j};
                } else {
                    manager_pos[ans_pos[i][j] - num_developer] = {i, j};
                }
            }
        }
    }
    for (int i = 0; i < num_developer; i++) {
        if (developer_pos[i].first == -1) {
            cout << 'X' << '\n';
        } else {
            cout << developer_pos[i].second << ' ' << developer_pos[i].first << '\n';
        }
    }
    for (int i = 0; i < num_manager; i++) {
        if (manager_pos[i].first == -1) {
            cout << 'X' << '\n';
        } else {
            cout << manager_pos[i].second << ' ' << manager_pos[i].first << '\n';
        }
    }
}

void init_conn() {
    conn.assign(H, vector<int>(W, -1));
    queue<pair<int, int>> que;
    num_conn = 0;
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            if (board[i][j] != '#' && conn[i][j] == -1) {
                que.emplace(i, j);
                conn[i][j] = num_conn;
                while (!que.empty()) {
                    auto& [x, y] = que.front();
                    que.pop();
                    for (int k = 0; k < 4; k++) {
                        int a = x + dx[k], b = y + dy[k];
                        if (0 <= a && a < H && 0 <= b && b < W && board[a][b] != '#' && conn[a][b] == -1) {
                            conn[a][b] = num_conn;
                            que.emplace(a, b);
                        }
                    }
                }
                num_conn++;
            }
        }
    }
}

int total_potential(const vector<vector<int>>& pos, int i1, int j1, int i2, int j2) {
    if (pos[i1][j1] < 0 || pos[i2][j2] < 0) return 0;
    int ret = 0;
    if (pos[i1][j1] < num_developer && pos[i2][j2] < num_developer) {
        int dev1 = pos[i1][j1], dev2 = pos[i2][j2];
        ret += work_potential(developers[dev1], developers[dev2]);
        if (developers[dev1].company == developers[dev2].company) {
            ret += bonus_potential(developers[dev1], developers[dev2]);
        }
    } else if (pos[i1][j1] < num_developer) {
        int dev1 = pos[i1][j1], man2 = pos[i2][j2] - num_developer;
        if (developers[dev1].company == managers[man2].company) {
            ret += bonus_potential(developers[dev1], managers[man2]);
        }
    } else if (pos[i2][j2] < num_developer) {
        int man1 = pos[i1][j1] - num_developer, dev2 = pos[i2][j2];
        if (managers[man1].company == developers[dev2].company) {
            ret += bonus_potential(developers[dev2], managers[man1]);
        }
    } else {
        int man1 = pos[i1][j1] - num_developer, man2 = pos[i2][j2] - num_developer;
        if (managers[man1].company == managers[man2].company) {
            ret += bonus_potential(managers[man1], managers[man2]);
        }
    }
    return ret;
}

int surrouding_potential(const vector<vector<int>>& pos, int i, int j) {
    int ret = 0;
    REP(k, 4) {
        int i2 = i + DIRECTIONS[k].first;
        int j2 = j + DIRECTIONS[k].second;
        if (i2 >= H || j2 >= W || i2 < 0 || j2 < 0 || pos[i2][j2] < 0) continue;
        ret += total_potential(pos, i, j, i2, j2);
    }
    return ret;
}

int calc_score(vector<vector<int>>& pos) {
    int score = 0;
    REP(i1, H) {
        REP(j1, W) {
            if (pos[i1][j1] < 0) continue;
            REP(k, 2) {
                int i2 = i1 + DIRECTIONS[k].first;
                int j2 = j1 + DIRECTIONS[k].second;
                if (i2 >= H || j2 >= W || pos[i2][j2] < 0) continue;
                int tmp = total_potential(pos, i1, j1, i2, j2);
                if (tmp > 0) dump(i1, j1, i2, j2, tmp);
                score += total_potential(pos, i1, j1, i2, j2);
            }
        }
    }
    return score;
}

void hill_climb(int seconds = 60) {
    auto start = chrono::steady_clock::now();
    while (1) {
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - start).count() > seconds * 1000) {
            break;
        }
        // developer
        pair<int, int> p1 = dev_places[rnd() % dev_places.size()];
        pair<int, int> p2 = dev_places[rnd() % dev_places.size()];
        // とりあえず接してたら差分計算がバグるので保留
        if (abs(p1.first - p2.first) <= 1 || abs(p2.first - p2.second) <= 1) continue;
        int nxt_score = best_score;
        nxt_score -= surrouding_potential(ans_pos, p1.first, p1.second);
        nxt_score -= surrouding_potential(ans_pos, p2.first, p2.second);
        swap(ans_pos[p1.first][p1.second], ans_pos[p2.first][p2.second]);
        nxt_score += surrouding_potential(ans_pos, p1.first, p1.second);
        nxt_score += surrouding_potential(ans_pos, p2.first, p2.second);
        if (nxt_score > best_score) {
            best_score = nxt_score;
            assert(best_score == calc_score(ans_pos));
            cerr << "best_score updated: " << best_score << endl;
        } else {
            swap(ans_pos[p1.first][p1.second], ans_pos[p2.first][p2.second]);
            assert(best_score == calc_score(ans_pos));
        }
        // manager
    }
}

void read_input() {
    cin >> W >> H;
    board.resize(H);
    ans_pos.assign(H, vector<int>(W, -3));
    for (int i = 0; i < H; i++) cin >> board[i];
    int avail_developer_cnt = 0;
    int avail_manager_cnt = 0;
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            if (board[i][j] == '_') {
                ans_pos[i][j] = -1;
                avail_developer_cnt++;
                dev_places.emplace_back(i, j);
            }
            if (board[i][j] == 'M') {
                ans_pos[i][j] = -2;
                avail_manager_cnt++;
                man_places.emplace_back(i, j);
            }
        }
    }
    cerr << "available developer cnt: " << avail_developer_cnt << endl;
    cerr << "available manager cnt: " << avail_manager_cnt << endl;
    int n;
    cin >> n;
    cerr << "input developer cnt: " << n << endl;
    num_developer = n;
    map<string, int> company_map;
    map<string, int> skill_map;
    vector<string> companies;
    vector<vector<string>> skills;
    companies.resize(n);
    skills.resize(n);
    for (int i = 0; i < n; i++) {
        Developer developer;
        cin >> companies[i] >> developer.bonus;
        developer.id = i;
        company_map[companies[i]];
        developers.push_back(developer);
        int k;
        cin >> k;
        skills[i].resize(k);
        for (int j = 0; j < k; j++) {
            cin >> skills[i][j];
            skill_map[skills[i][j]];
        }
    }
    int N = 0;
    for (auto& p : skill_map) {
        p.second = N++;
    }
    cerr << "total skill kind: " << N << endl;
    assert(N <= MAX_SKILL_COUNT);
    for (int i = 0; i < n; i++) {
        for (const string& skill : skills[i]) {
            developers[i].skills[skill_map[skill]] = 1;
        }
    }
    int m;
    cin >> m;
    num_manager = m;
    cerr << "input manager cnt: " << m << endl;
    for (int i = 0; i < m; i++) {
        string s;
        Manager manager;
        manager.id = i;
        cin >> s >> manager.bonus;
        managers.push_back(manager);
        company_map[s];
        companies.push_back(s);
    }
    N = 0;
    for (auto& p : company_map) {
        p.second = N++;
    }
    cerr << "total company kind: " << N << endl;
    num_companies = N;
    for (int i = 0; i < n; i++) {
        developers[i].company = company_map[companies[i]];
    }
    for (int i = 0; i < m; i++) {
        managers[i].company = company_map[companies[i + n]];
    }
    init_conn();
    developers_var = developers;
    managers_var = managers;
}

const char* input_files[] = {"test/a_solar.txt",     "test/b_dream.txt",  "test/c_soup.txt",
                             "test/d_maelstrom.txt", "test/e_igloos.txt", "test/f_glitch.txt"};

const char* output_files[] = {"sol/a.sol", "sol/b.sol", "sol/c.sol", "sol/d.sol", "sol/e.sol", "sol/f.sol"};

void redirect_io(int k) {
    assert(freopen(input_files[k], "r", stdin));
    assert(freopen(output_files[k], "w", stdout));
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);

    int n;
    cin >> n;
    redirect_io(n);
    read_input();

    // initialize_by_valid();
    initialize_by_conn();
    best_score = calc_score(ans_pos);
    cerr << "initial score: " << best_score << endl;
    dump(ans_pos, best_score);
    hill_climb();
    print_solution();
}
