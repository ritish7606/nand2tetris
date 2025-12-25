#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <filesystem>
#include <cctype>
using namespace std;
namespace fs = std::filesystem;

enum class Type { Keyword, Symbol, Identifier, Integer, String };

static bool sym(char c) {
    static const string s = "{}()[].,;+-*/&|<>=~";
    return s.find(c) != string::npos;
}

static bool keyword(const string &x) {
    static const unordered_set<string> kw = {
        "class","constructor","function","method","field","static","var",
        "int","char","boolean","void","true","false","null","this",
        "let","do","if","else","while","return"
    };
    return kw.find(x) != kw.end();
}

static string xml(const string &v) {
    if (v == "<") return "&lt;";
    if (v == ">") return "&gt;";
    if (v == "&") return "&amp;";
    return v;
}

class Tokenizer {
    vector<string> toks;

    static string fileText(const string &name) {
        ifstream f(name);
        if (!f.is_open()) throw runtime_error("open fail: " + name);
        stringstream ss; ss << f.rdbuf();
        return ss.str();
    }

    static string strip(const string &txt) {
        string res;
        bool blk = false, line = false;
        for (size_t i = 0; i < txt.size(); ++i) {
            if (blk) {
                if (txt[i] == '*' && i + 1 < txt.size() && txt[i + 1] == '/') {
                    blk = false; ++i;
                }
            } else if (line) {
                if (txt[i] == '\n') { line = false; res += '\n'; }
            } else {
                if (txt[i] == '/' && i + 1 < txt.size() && txt[i + 1] == '/') {
                    line = true; ++i;
                } else if (txt[i] == '/' && i + 1 < txt.size() && txt[i + 1] == '*') {
                    blk = true; ++i;
                } else res += txt[i];
            }
        }
        return res;
    }

    static Type cat(const string &t) {
        if (keyword(t)) return Type::Keyword;
        if (t.size() == 1 && sym(t[0])) return Type::Symbol;
        if (t.size() > 1 && t.front() == '"' && t.back() == '"') return Type::String;
        if (all_of(t.begin(), t.end(), ::isdigit)) return Type::Integer;
        return Type::Identifier;
    }

    void split(const string &src) {
        size_t i = 0;
        while (i < src.size()) {
            if (isspace(static_cast<unsigned char>(src[i]))) { ++i; continue; }
            if (sym(src[i])) { toks.push_back(string(1, src[i++])); continue; }
            if (src[i] == '"') {
                size_t j = i + 1;
                while (j < src.size() && src[j] != '"') ++j;
                toks.push_back(src.substr(i, j - i + 1));
                i = j + 1;
                continue;
            }
            if (isdigit(static_cast<unsigned char>(src[i]))) {
                size_t j = i;
                while (j < src.size() && isdigit(static_cast<unsigned char>(src[j]))) ++j;
                toks.push_back(src.substr(i, j - i));
                i = j;
                continue;
            }
            size_t j = i;
            while (j < src.size() && !isspace(static_cast<unsigned char>(src[j])) && !sym(src[j])) ++j;
            toks.push_back(src.substr(i, j - i));
            i = j;
        }
    }

public:
    explicit Tokenizer(const string &input) {
        string c = fileText(input);
        string noC = strip(c);
        split(noC);
    }

    void output(const string &file) {
        fs::path p(file);
        string outName = (p.parent_path() / (p.stem().string() + "_Tokens.xml")).string();
        ofstream o(outName);
        if (!o.is_open()) throw runtime_error("cannot write: " + outName);
        o << "<tokens>\n";
        for (const auto &x : toks) {
            Type t = cat(x);
            switch (t) {
                case Type::Keyword: o << "<keyword> " << x << " </keyword>\n"; break;
                case Type::Symbol: o << "<symbol> " << xml(x) << " </symbol>\n"; break;
                case Type::Identifier: o << "<identifier> " << x << " </identifier>\n"; break;
                case Type::Integer: o << "<integerConstant> " << x << " </integerConstant>\n"; break;
                case Type::String: o << "<stringConstant> " << x.substr(1, x.size()-2) << " </stringConstant>\n"; break;
            }
        }
        o << "</tokens>\n";
        cout << "Saved: " << outName << endl;
    }
};

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: tokenizer <file or folder>\n";
        return 1;
    }
    string arg = argv[1];
    vector<string> files;
    if (fs::is_directory(arg)) {
        for (auto &f : fs::directory_iterator(arg))
            if (f.path().extension() == ".jack")
                files.push_back(f.path().string());
    } else files.push_back(arg);

    for (auto &f : files) {
        try {
            cout << "Reading: " << f << endl;
            Tokenizer T(f);
            T.output(f);
        } catch (const exception &e) {
            cerr << "Error: " << e.what() << endl;
        }
    }
}
