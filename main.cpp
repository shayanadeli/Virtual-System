#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

using namespace std;

const int SECTOR_SIZE = 64;

class Disk {
public:
    vector<string> sectors;
    int total_sectors;

    Disk(int mgb) {
        long long total_bytes = (long long)mgb * 1024 * 1024;
        total_sectors = total_bytes / SECTOR_SIZE;
        sectors.resize(total_sectors, string(SECTOR_SIZE, '\0'));

        cout << "Disk installed: " << mgb << " Megabytes = "
             << total_sectors << " sectors of " << SECTOR_SIZE << " bytes" << endl << endl;
    }
};

class Node {
public:
    string name;
    bool is_folder;
    Node* parent;
    vector<Node*> children;
    vector<int> allocated_sectors;
    int byte_size;

    Node(string n, bool folder, Node* p = nullptr)
        : name(n), is_folder(folder), parent(p), byte_size(0) {}

    string get_path() {
        if (parent == nullptr) return "/";
        if (parent->parent == nullptr) return "/" + name;
        return parent->get_path() + "/" + name;
    }

    ~Node() {
        for (Node* ch : children) delete ch;
    }
};

class FileSystem {
private:
    Disk disk;
    Node* root;
    Node* current;

    void sort_children(Node* node) {
        sort(node->children.begin(), node->children.end(),
             [](Node* a, Node* b) { return a->name < b->name; });
    }

    Node* resolve_path(string path) {
        if (path.empty()) return current;

        bool absolute = (path[0] == '/');
        if (absolute && path.size() > 1) path = path.substr(1);

        vector<string> parts;
        string part = "";
        for (char c : path) {
            if (c == '/') {
                if (!part.empty()) parts.push_back(part);
                part = "";
            } else {
                part += c;
            }
        }
        if (!part.empty()) parts.push_back(part);

        Node* cur = absolute ? root : current;
        for (string comp : parts) {
            if (comp.empty() || comp == ".") continue;
            if (comp == "..") {
                if (cur->parent) cur = cur->parent;
                continue;
            }

            bool found = false;
            for (Node* ch : cur->children) {
                if (ch->name == comp) {
                    cur = ch;
                    found = true;
                    break;
                }
            }
            if (!found) return nullptr;
        }
        return cur;
    }

    bool get_parent_and_name(string path, Node*& parent, string& name) {
        if (path.empty()) return false;

        bool absolute = (path[0] == '/');
        if (absolute) path = path.substr(1);

        vector<string> parts;
        string part = "";
        for (char c : path) {
            if (c == '/') {
                if (!part.empty()) parts.push_back(part);
                part = "";
            } else part += c;
        }
        if (!part.empty()) parts.push_back(part);

        if (parts.empty()) return false;

        Node* cur = absolute ? root : current;
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            string comp = parts[i];
            if (comp.empty() || comp == ".") continue;
            if (comp == "..") {
                if (cur->parent) cur = cur->parent;
                continue;
            }

            bool found = false;
            for (Node* ch : cur->children) {
                if (ch->name == comp) {
                    cur = ch;
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }

        parent = cur;
        name = parts.back();
        if (name.empty() || name == "." || name == "..") return false;
        return true;
    }

    bool valid_name(const string& n) {
        if (n.empty()) return false;
        for (char c : n) {
            if (!isalnum(c) && c != '-') return false;
        }
        return true;
    }

    Node* find_child(Node* par, const string& n) {
        for (Node* ch : par->children) {
            if (ch->name == n) return ch;
        }
        return nullptr;
    }

    vector<int> allocate_sectors(int needed) {
        vector<int> res;
        for (int i = 0; i < disk.total_sectors && (int)res.size() < needed; ++i) {
            if (disk.sectors[i] == string(SECTOR_SIZE, '\0')) {
                res.push_back(i);
            }
        }
        return res;
    }

    void free_sectors(Node* node) {
        if (!node->is_folder) {
            for (int s : node->allocated_sectors) {
                disk.sectors[s] = string(SECTOR_SIZE, '\0');
            }
            node->allocated_sectors.clear();
            node->byte_size = 0;
        }
    }

    void delete_recursive(Node* node) {
        if (node->is_folder) {
            for (Node* ch : node->children) {
                delete_recursive(ch);
            }
        }
        free_sectors(node);
    }

    string read_content(Node* file) {
        if (file->is_folder) return "";
        string content;
        for (int s : file->allocated_sectors) {
            content += disk.sectors[s];
        }
        if ((int)content.size() > file->byte_size) {
            content.resize(file->byte_size);
        }
        return content;
    }

    bool write_content(Node* file, const string& content) {
        free_sectors(file);

        int needed = (content.size() + SECTOR_SIZE - 1) / SECTOR_SIZE;
        vector<int> secs = allocate_sectors(needed);
        if ((int)secs.size() < needed) return false;

        file->allocated_sectors = secs;
        file->byte_size = content.size();

        int pos = 0;
        for (int s : secs) {
            string chunk = content.substr(pos, SECTOR_SIZE);
            chunk.resize(SECTOR_SIZE, '\0');
            disk.sectors[s] = chunk;
            pos += SECTOR_SIZE;
        }
        return true;
    }

    void make_folder(Node* par, const string& n) {
        if (!valid_name(n)) {
            cout << "mkdir: invalid name" << endl;
            return;
        }
        if (find_child(par, n)) {
            cout << "mkdir: cannot create directory '" << n << "': File exists" << endl;
            return;
        }
        par->children.push_back(new Node(n, true, par));
    }

    bool make_file(Node* par, const string& n, const string& content = "") {
        if (!valid_name(n)) {
            cout << "put: invalid name" << endl;
            return false;
        }
        if (find_child(par, n)) {
            cout << "put: file exists" << endl;
            return false;
        }
        Node* new_file = new Node(n, false, par);
        par->children.push_back(new_file);
        if (!content.empty()) {
            if (!write_content(new_file, content)) {
                cout << "put: no space left on disk" << endl;
                auto it = find(par->children.begin(), par->children.end(), new_file);
                par->children.erase(it);
                delete new_file;
                return false;
            }
        }
        return true;
    }

    void copy_recursive(Node* src, Node* dest_par, const string& new_name) {
        if (src->is_folder) {
            Node* new_dir = new Node(new_name, true, dest_par);
            dest_par->children.push_back(new_dir);
            for (Node* ch : src->children) {
                copy_recursive(ch, new_dir, ch->name);
            }
        } else {
            string cont = read_content(src);
            make_file(dest_par, new_name, cont);
        }
    }

    void collect_files(Node* node, vector<pair<Node*, string>>& all_files) {
        for (Node* ch : node->children) {
            if (ch->is_folder) {
                collect_files(ch, all_files);
            } else {
                all_files.push_back({ch, read_content(ch)});
            }
        }
    }

    void do_defrag() {
        vector<pair<Node*, string>> all_files;
        collect_files(root, all_files);

        for (auto& sec : disk.sectors) sec = string(SECTOR_SIZE, '\0');

        int next = 0;
        for (auto& p : all_files) {
            Node* f = p.first;
            string cont = p.second;
            int needed = (cont.size() + SECTOR_SIZE - 1) / SECTOR_SIZE;
            vector<int> new_secs;
            for (int i = 0; i < needed; ++i) new_secs.push_back(next++);
            f->allocated_sectors = new_secs;
            f->byte_size = cont.size();

            int pos = 0;
            for (int s : new_secs) {
                string chunk = cont.substr(pos, SECTOR_SIZE);
                chunk.resize(SECTOR_SIZE, '\0');
                disk.sectors[s] = chunk;
                pos += SECTOR_SIZE;
            }
        }
        cout << "defrag: completed" << endl;
    }

public:
    FileSystem(int mgb) : disk(mgb) {
        root = new Node("", true);
        Node* home = new Node("home", true, root);
        root->children.push_back(home);
        current = home;

        cout << "File system is ready!" << endl
             << "You are in /home" << endl
             << "Type 'exit' to quit" << endl << endl;
    }

    ~FileSystem() { delete root; }

    void run() {
        string line;
        while (true) {
            cout << current->get_path() << "> ";
            getline(cin, line);

            size_t startpos = 0;
            while (startpos < line.size() && line[startpos] == ' ') startpos++;
            line = line.substr(startpos);

            if (line.empty()) continue;

            size_t space_pos = line.find(' ');
            string command = (space_pos == string::npos) ? line : line.substr(0, space_pos);
            string args = (space_pos == string::npos) ? "" : line.substr(space_pos + 1);

            startpos = 0;
            while (startpos < args.size() && args[startpos] == ' ') startpos++;
            args = args.substr(startpos);

            if (command == "exit") {
                cout << "Bye bye!" << endl;
                break;
            }
            else if (command == "pwd") {
                cout << current->get_path() << endl;
            }
            else if (command == "ls") {
                Node* target = current;
                if (!args.empty()) {
                    target = resolve_path(args);
                    if (!target || !target->is_folder) {
                        cout << "ls: no such directory" << endl;
                        continue;
                    }
                }
                sort_children(target);
                for (Node* ch : target->children) {
                    cout << ch->name;
                    if (ch->is_folder) cout << "/";
                    cout << endl;
                }
            }
            else if (command == "mkdir") {
                if (args.empty()) {
                    cout << "mkdir: missing operand" << endl;
                    continue;
                }
                Node* par;
                string folder_name;
                if (!get_parent_and_name(args, par, folder_name)) {
                    cout << "mkdir: invalid path" << endl;
                    continue;
                }
                make_folder(par, folder_name);
            }
            else if (command == "cd") {
                string target_path = args.empty() ? "/home" : args;
                Node* new_dir = resolve_path(target_path);
                if (!new_dir || !new_dir->is_folder) {
                    cout << "cd: no such directory: " << target_path << endl;
                    continue;
                }
                current = new_dir;
            }
            else if (command == "rm") {
                if (args.empty()) {
                    cout << "rm: missing operand" << endl;
                    continue;
                }
                Node* target = resolve_path(args);
                if (!target) {
                    cout << "rm: no such file or directory" << endl;
                    continue;
                }
                if (target->parent == nullptr) {
                    cout << "rm: cannot remove root" << endl;
                    continue;
                }
                delete_recursive(target);
                Node* par = target->parent;
                auto it = find(par->children.begin(), par->children.end(), target);
                if (it != par->children.end()) par->children.erase(it);
                delete target;
            }
            else if (command == "cp" || command == "mv") {
                if (args.empty()) {
                    cout << command << ": missing operands" << endl;
                    continue;
                }
                size_t second_space = args.find(' ');
                if (second_space == string::npos) {
                    cout << command << ": missing destination" << endl;
                    continue;
                }
                string src_path = args.substr(0, second_space);
                string dest_path = args.substr(second_space + 1);

                startpos = 0;
                while (startpos < dest_path.size() && dest_path[startpos] == ' ') startpos++;
                dest_path = dest_path.substr(startpos);

                Node* src = resolve_path(src_path);
                if (!src) {
                    cout << command << ": no such source" << endl;
                    continue;
                }

                Node* dest = resolve_path(dest_path);
                Node* dest_parent;
                string dest_name = src->name;

                if (dest) {
                    if (!dest->is_folder) {
                        cout << command << ": destination not a directory" << endl;
                        continue;
                    }
                    dest_parent = dest;
                } else {
                    if (!get_parent_and_name(dest_path, dest_parent, dest_name)) {
                        cout << command << ": invalid destination" << endl;
                        continue;
                    }
                }

                if (find_child(dest_parent, dest_name)) {
                    cout << command << ": file exists" << endl;
                    continue;
                }

                if (command == "cp") {
                    copy_recursive(src, dest_parent, dest_name);
                } else {
                    if (src->parent == nullptr) {
                        cout << "mv: cannot move root" << endl;
                        continue;
                    }
                    Node* old_parent = src->parent;
                    auto it = find(old_parent->children.begin(), old_parent->children.end(), src);
                    old_parent->children.erase(it);
                    src->parent = dest_parent;
                    src->name = dest_name;
                    dest_parent->children.push_back(src);
                }
            }
            else if (command == "put") {
                if (args.empty()) {
                    cout << "put: missing file name" << endl;
                    continue;
                }
                string real_path = args;
                string virt_dir = "";
                size_t dir_pos = args.find(' ');
                if (dir_pos != string::npos) {
                    real_path = args.substr(0, dir_pos);
                    virt_dir = args.substr(dir_pos + 1);
                    startpos = 0;
                    while (startpos < virt_dir.size() && virt_dir[startpos] == ' ') startpos++;
                    virt_dir = virt_dir.substr(startpos);
                }

                Node* target_dir = current;
                if (!virt_dir.empty()) {
                    target_dir = resolve_path(virt_dir);
                    if (!target_dir || !target_dir->is_folder) {
                        cout << "put: invalid directory" << endl;
                        continue;
                    }
                }

                ifstream real_file(real_path);
                if (!real_file) {
                    cout << "put: no such real file" << endl;
                    continue;
                }

                string content;
                string ln;
                while (getline(real_file, ln)) {
                    content += ln + "\n";
                }

                string file_name = real_path;
                size_t last_slash = real_path.find_last_of("/\\");
                if (last_slash != string::npos) {
                    file_name = real_path.substr(last_slash + 1);
                }

                make_file(target_dir, file_name, content);
            }
            else if (command == "get") {
                if (args.empty()) {
                    cout << "get: missing file" << endl;
                    continue;
                }
                Node* file = resolve_path(args);
                if (!file || file->is_folder) {
                    cout << "get: not a file" << endl;
                    continue;
                }
                string content = read_content(file);
                ofstream out(file->name);
                if (!out) {
                    cout << "get: cannot create file" << endl;
                    continue;
                }
                out << content;
                cout << "get: saved as '" << file->name << "'" << endl;
            }
            else if (command == "defrag") {
                do_defrag();
            }
            else {
                cout << "Unknown command: " << command << endl;
            }
        }
    }
};

int main() {
    int size;
    cout << "Enter Disk size (MB): ";
    cin >> size;
    cin.ignore();

    FileSystem fs(size);
    fs.run();

    return 0;
}