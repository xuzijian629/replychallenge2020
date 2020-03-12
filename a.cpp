#include <bits/stdc++.h>
using namespace std;

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
vector<Developer> developers;
vector<Manager> managers;

// 解を入れる配列
// H * Wでpos[i][j]は-1のとき、空いていてDeveloperが使える。-2のとき、空いていてManagerが使える
// -3のとき、使えない
// 0 <= i < nのとき、i番目のDeveloper
// n <= i < n + mのときi - n番目のManager
vector<vector<int>> pos;
int num_conn;
// H * Wで、同じ値は同じ連結成分。壁は-1
vector<vector<int>> conn;

void initialize_by_valid() {
    sort(developers.begin(), developers.end(), [](auto& a, auto& b) { return a.company < b.company; });
    sort(managers.begin(), managers.end(), [](auto& a, auto& b) { return a.company < b.company; });
    vector<pair<int, int>> valid_developer_pos, valid_manager_pos;
    for (int i = 0; i < H; i++) {
        if (i & 1) {
            for (int j = 0; j < W; j++) {
                if (pos[i][j] == -1) valid_developer_pos.emplace_back(i, j);
                if (pos[i][j] == -2) valid_manager_pos.emplace_back(i, j);
            }
        } else {
            for (int j = W - 1; j >= 0; j--) {
                if (pos[i][j] == -1) valid_developer_pos.emplace_back(i, j);
                if (pos[i][j] == -2) valid_manager_pos.emplace_back(i, j);
            }
        }
    }
    for (int i = 0; i < num_developer; i++) {
        if (i < valid_developer_pos.size()) {
            auto [a, b] = valid_developer_pos[i];
            pos[a][b] = developers[i].id;
        }
    }
    for (int i = 0; i < num_manager; i++) {
        if (i < valid_manager_pos.size()) {
            auto [a, b] = valid_manager_pos[i];
            pos[a][b] = managers[i].id + num_developer;
        }
    }
}

// 同じ連結成分内を蛇行して敷き詰める
// bonus順ソート追加
// skillもソートしたけどB以外減った
void initialize_by_conn() {
    sort(developers.begin(), developers.end(), [](auto& a, auto& b) {
        return make_tuple(a.company, -a.bonus, -a.skills.count()) <
               make_tuple(b.company, -b.bonus, -(int)b.skills.count());
    });
    sort(managers.begin(), managers.end(),
         [](auto& a, auto& b) { return make_tuple(a.company, -a.bonus) < make_tuple(b.company, -b.bonus); });
    vector<vector<pair<int, int>>> valid_developer_pos_conn(num_conn), valid_manager_pos_conn(num_conn);
    vector<pair<int, int>> valid_developer_pos, valid_manager_pos;
    for (int i = 0; i < H; i++) {
        if (i & 1) {
            for (int j = 0; j < W; j++) {
                if (pos[i][j] == -1) valid_developer_pos_conn[conn[i][j]].emplace_back(i, j);
                if (pos[i][j] == -2) valid_manager_pos_conn[conn[i][j]].emplace_back(i, j);
            }
        } else {
            for (int j = W - 1; j >= 0; j--) {
                if (pos[i][j] == -1) valid_developer_pos_conn[conn[i][j]].emplace_back(i, j);
                if (pos[i][j] == -2) valid_manager_pos_conn[conn[i][j]].emplace_back(i, j);
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
            pos[a][b] = developers[i].id;
        }
    }
    for (int i = 0; i < num_manager; i++) {
        if (i < valid_manager_pos.size()) {
            auto [a, b] = valid_manager_pos[i];
            pos[a][b] = managers[i].id + num_developer;
        }
    }
}

void print_solution() {
    vector<pair<int, int>> developer_pos(num_developer, {-1, -1}), manager_pos(num_manager, {-1, -1});
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            if (0 <= pos[i][j]) {
                if (pos[i][j] < num_developer) {
                    developer_pos[pos[i][j]] = {i, j};
                } else {
                    manager_pos[pos[i][j] - num_developer] = {i, j};
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

void read_input() {
    cin >> W >> H;
    board.resize(H);
    pos.assign(H, vector<int>(W, -3));
    for (int i = 0; i < H; i++) cin >> board[i];
    int avail_developer_cnt = 0;
    int avail_manager_cnt = 0;
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            if (board[i][j] == '_') {
                pos[i][j] = -1;
                avail_developer_cnt++;
            }
            if (board[i][j] == 'M') {
                pos[i][j] = -2;
                avail_manager_cnt++;
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
    for (int i = 0; i < n; i++) {
        developers[i].company = company_map[companies[i]];
    }
    for (int i = 0; i < m; i++) {
        managers[i].company = company_map[companies[i + n]];
    }
    init_conn();
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
    print_solution();
}
