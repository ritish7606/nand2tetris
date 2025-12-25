#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
using namespace std;
string baseSegment(const string &seg) {
    if (seg == "local") return "LCL";
    if (seg == "argument") return "ARG";
    if (seg == "this") return "THIS";
    if (seg == "that") return "THAT";
    return "";
}
string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == string::npos) ? "" : s.substr(a, b - a + 1);
}
string fileBase(const string &path) {
    size_t slash = path.find_last_of("/\\");
    size_t dot = path.find_last_of('.');
    size_t start = (slash == string::npos) ? 0 : slash + 1;
    size_t end = (dot == string::npos) ? path.size() : dot;
    return path.substr(start, end - start);
}
string pushPop(const string &cmd, const string &seg, int index, const string &file) {
    ostringstream o;
    if (cmd == "push") {
        if (seg == "constant") o << "@" << index << "\nD=A\n";
        else if (seg == "temp") o << "@" << (5 + index) << "\nD=M\n";
        else if (seg == "pointer") o << "@" << (index == 0 ? "THIS" : "THAT") << "\nD=M\n";
        else if (seg == "static") o << "@" << file << "." << index << "\nD=M\n";
        else o << "@" << baseSegment(seg) << "\nD=M\n@" << index << "\nA=A+D\nD=M\n";
        o << "@SP\nA=M\nM=D\n@SP\nM=M+1\n";
    } else {
        if (seg == "static") o << "@SP\nAM=M-1\nD=M\n@" << file << "." << index << "\nM=D\n";
        else if (seg == "temp") o << "@SP\nAM=M-1\nD=M\n@" << (5 + index) << "\nM=D\n";
        else if (seg == "pointer") o << "@SP\nAM=M-1\nD=M\n@" << (index == 0 ? "THIS" : "THAT") << "\nM=D\n";
        else {
            o << "@" << baseSegment(seg) << "\nD=M\n@" << index << "\nD=A+D\n@R13\nM=D\n";
            o << "@SP\nAM=M-1\nD=M\n@R13\nA=M\nM=D\n";
        }
    }
    return o.str();
}
string arithmetic(const string &cmd) {
    static int lbl = 0;
    ostringstream o;
    if (cmd == "add" || cmd == "sub" || cmd == "and" || cmd == "or") {
        o << "@SP\nAM=M-1\nD=M\nA=A-1\n";
        if (cmd == "add") o << "M=M+D\n";
        if (cmd == "sub") o << "M=M-D\n";
        if (cmd == "and") o << "M=M&D\n";
        if (cmd == "or")  o << "M=M|D\n";
    } else if (cmd == "neg" || cmd == "not") {
        o << "@SP\nA=M-1\n";
        if (cmd == "neg") o << "M=-M\n";
        if (cmd == "not") o << "M=!M\n";
    } else if (cmd == "eq" || cmd == "gt" || cmd == "lt") {
        string T = "TRUE_" + to_string(lbl);
        string E = "END_" + to_string(lbl++);
        o << "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n";
        o << "@" << T << "\n";
        if (cmd == "eq") o << "D;JEQ\n";
        if (cmd == "gt") o << "D;JGT\n";
        if (cmd == "lt") o << "D;JLT\n";
        o << "@SP\nA=M-1\nM=0\n@" << E << "\n0;JMP\n";
        o << "(" << T << ")\n@SP\nA=M-1\nM=-1\n(" << E << ")\n";
    }
    return o.str();
}
string labelName(const string &func, const string &lbl) {
    return func.empty() ? lbl : func + "$" + lbl;
}

string makeLabel(const string &func, const string &lbl) {
    return "(" + labelName(func, lbl) + ")\n";
}

string makeIf(const string &func, const string &lbl) {
    return "@SP\nAM=M-1\nD=M\n@" + labelName(func, lbl) + "\nD;JNE\n";
}

string makeGoto(const string &func, const string &lbl) {
    return "@" + labelName(func, lbl) + "\n0;JMP\n";
}
string makeFunction(const string &f, int vars) {
    ostringstream o;
    o << "(" << f << ")\n";
    for (int i = 0; i < vars; i++)
        o << "@0\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";
    return o.str();
}
string makeCall(const string &f, int args) {
    static int id = 0;
    string ret = "RET_" + to_string(id++);
    ostringstream o;
    o << "@" << ret << "\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";
    for (string s : {"LCL", "ARG", "THIS", "THAT"})
        o << "@" << s << "\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n";
    o << "@SP\nD=M\n@" << (args + 5) << "\nD=D-A\n@ARG\nM=D\n";
    o << "@SP\nD=M\n@LCL\nM=D\n";
    o << "@" << f << "\n0;JMP\n(" << ret << ")\n";
    return o.str();
}
string makeReturn() {
    ostringstream o;
    o << "@LCL\nD=M\n@R13\nM=D\n";
    o << "@5\nA=D-A\nD=M\n@R14\nM=D\n";
    o << "@SP\nAM=M-1\nD=M\n@ARG\nA=M\nM=D\n";
    o << "@ARG\nD=M+1\n@SP\nM=D\n";
    o << "@R13\nD=M\n@1\nA=D-A\nD=M\n@THAT\nM=D\n";
    o << "@R13\nD=M\n@2\nA=D-A\nD=M\n@THIS\nM=D\n";
    o << "@R13\nD=M\n@3\nA=D-A\nD=M\n@ARG\nM=D\n";
    o << "@R13\nD=M\n@4\nA=D-A\nD=M\n@LCL\nM=D\n";
    o << "@R14\nA=M\n0;JMP\n";
    return o.str();
}
string bootstrap() {
    ostringstream o;
    o << "@256\nD=A\n@SP\nM=D\n";
    o << makeCall("Sys.init", 0);
    return o.str();
}
void translate(const string &file, ofstream &out, string &currentFunc) {
    ifstream in(file);
    if (!in.is_open()) {
        cerr << "Could not open " << file << "\n";
        return;
    }
    string line;
    string fname = fileBase(file);
    while (getline(in, line)) {
        size_t cmt = line.find("//");
        if (cmt != string::npos) line = line.substr(0, cmt);
        line = trim(line);
        if (line.empty()) continue;

        stringstream ss(line);
        string cmd; ss >> cmd;

        if (cmd == "push" || cmd == "pop") {
            string seg; int idx;
            ss >> seg >> idx;
            out << pushPop(cmd, seg, idx, fname);
        } else if (cmd == "label") {
            string lbl; ss >> lbl;
            out << makeLabel(currentFunc, lbl);
        } else if (cmd == "goto") {
            string lbl; ss >> lbl;
            out << makeGoto(currentFunc, lbl);
        } else if (cmd == "if-goto") {
            string lbl; ss >> lbl;
            out << makeIf(currentFunc, lbl);
        } else if (cmd == "function") {
            string f; int vars;
            ss >> f >> vars;
            currentFunc = f;
            out << makeFunction(f, vars);
        } else if (cmd == "call") {
            string f; int args;
            ss >> f >> args;
            out << makeCall(f, args);
        } else if (cmd == "return") {
            out << makeReturn();
        } else {
            out << arithmetic(cmd);
        }
    }
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <file or directory>\n";
        return 1;
    }
    string input = argv[1];
    vector<string> files;
    string output;
    if (input.find(".vm") != string::npos) {
        files.push_back(input);
        output = fileBase(input) + ".asm";
    } else {
#ifdef _WIN32
        system(("dir /b " + input + "\\*.vm > vmtemp.txt").c_str());
#else
        system(("ls " + input + "/*.vm > vmtemp.txt").c_str());
#endif
        ifstream list("vmtemp.txt");
        string f;
        while (getline(list, f)) {
            if (!f.empty()) files.push_back(f);
        }
        list.close();
        remove("vmtemp.txt");
        if (files.empty()) {
            cerr << "No VM files found.\n";
            return 1;
        }
        output = input.substr(input.find_last_of("/\\") + 1) + ".asm";
    }
    ofstream out(output);
    if (!out.is_open()) {
        cerr << "Failed to open output file.\n";
        return 1;
    }
    if (input.find(".vm") == string::npos) out << bootstrap();
    string currentFunc = "";
    for (auto &f : files) translate(f, out, currentFunc);
    cout << "Created " << output << "\n";
    return 0;
}
