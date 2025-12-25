#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;
enum class CommandType { C_ARITHMETIC, C_PUSH, C_POP, C_UNKNOWN };
struct Command {
    CommandType type = CommandType::C_UNKNOWN;
    string arg1;
    int arg2 = 0;
    bool valid = false;
};
string trimLine(const string &line) {
    string s = line;
    size_t commentPos = s.find("//");
    if (commentPos != string::npos) s = s.substr(0, commentPos);
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start])) ++start;
    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1])) --end;
    return s.substr(start, end - start);
}
string baseName(const string &path) {
    string s = path;
    size_t pos = s.find_last_of("/\\");
    if (pos != string::npos) s = s.substr(pos + 1);
    if (s.size() >= 3 && s.substr(s.size() - 3) == ".vm")
        s = s.substr(0, s.size() - 3);
    return s;
}
class CodeWriter {
    ofstream out;
    string basename;
    int labelCounter = 0;
    string newLabel(const string &prefix) {
        return prefix + to_string(labelCounter++);
    }
    void writeLine(const string &s) { out << s << '\n'; }
    void pushD() {
        writeLine("@SP");
        writeLine("A=M");
        writeLine("M=D");
        writeLine("@SP");
        writeLine("M=M+1");
    }
    void popToD() {
        writeLine("@SP");
        writeLine("AM=M-1");
        writeLine("D=M");
    }
public:
    CodeWriter(const string &outFile, const string &inBase) : basename(inBase) {
        out.open(outFile);
    }
    bool good() const { return out.is_open(); }
    void writePush(const string &segment, int index) {
        if (segment == "constant") {
            writeLine("@" + to_string(index));
            writeLine("D=A");
            pushD();
        } else if (segment == "local" || segment == "argument" ||
                   segment == "this" || segment == "that") {
            string base;
            if (segment == "local") base = "LCL";
            if (segment == "argument") base = "ARG";
            if (segment == "this") base = "THIS";
            if (segment == "that") base = "THAT";
            writeLine("@" + to_string(index));
            writeLine("D=A");
            writeLine("@" + base);
            writeLine("A=M+D");
            writeLine("D=M");
            pushD();
        } else if (segment == "temp") {
            writeLine("@" + to_string(5 + index));
            writeLine("D=M");
            pushD();
        } else if (segment == "pointer") {
            writeLine(index == 0 ? "@THIS" : "@THAT");
            writeLine("D=M");
            pushD();
        } else if (segment == "static") {
            string name = basename + "." + to_string(index);
            writeLine("@" + name);
            writeLine("D=M");
            pushD();
        }
    }
    void writePop(const string &segment, int index) {
        if (segment == "constant") return;
        if (segment == "local" || segment == "argument" ||
            segment == "this" || segment == "that") {
            string base;
            if (segment == "local") base = "LCL";
            if (segment == "argument") base = "ARG";
            if (segment == "this") base = "THIS";
            if (segment == "that") base = "THAT";
            writeLine("@" + to_string(index));
            writeLine("D=A");
            writeLine("@" + base);
            writeLine("D=M+D");
            writeLine("@R13");
            writeLine("M=D");
            popToD();
            writeLine("@R13");
            writeLine("A=M");
            writeLine("M=D");
        } else if (segment == "temp") {
            popToD();
            writeLine("@" + to_string(5 + index));
            writeLine("M=D");
        } else if (segment == "pointer") {
            popToD();
            writeLine(index == 0 ? "@THIS" : "@THAT");
            writeLine("M=D");
        } else if (segment == "static") {
            popToD();
            string name = basename + "." + to_string(index);
            writeLine("@" + name);
            writeLine("M=D");
        }
    }
    void writeArithmetic(const string &cmd) {
        if (cmd == "add" || cmd == "sub" || cmd == "and" || cmd == "or") {
            popToD();
            writeLine("@SP");
            writeLine("A=M-1");
            if (cmd == "add") writeLine("M=M+D");
            if (cmd == "sub") writeLine("M=M-D");
            if (cmd == "and") writeLine("M=M&D");
            if (cmd == "or")  writeLine("M=M|D");
        } else if (cmd == "neg" || cmd == "not") {
            writeLine("@SP");
            writeLine("A=M-1");
            if (cmd == "neg") writeLine("M=-M");
            else writeLine("M=!M");
        } else if (cmd == "eq" || cmd == "gt" || cmd == "lt") {
            popToD();
            writeLine("@SP");
            writeLine("A=M-1");
            writeLine("D=M-D");
            string trueLabel = newLabel("TRUE");
            string endLabel  = newLabel("END");
            writeLine("@" + trueLabel);
            if (cmd == "eq") writeLine("D;JEQ");
            if (cmd == "gt") writeLine("D;JGT");
            if (cmd == "lt") writeLine("D;JLT");
            writeLine("@SP");
            writeLine("A=M-1");
            writeLine("M=0");
            writeLine("@" + endLabel);
            writeLine("0;JMP");
            writeLine("(" + trueLabel + ")");
            writeLine("@SP");
            writeLine("A=M-1");
            writeLine("M=-1");
            writeLine("(" + endLabel + ")");
        }
    }
};
class Parser {
    ifstream in;
public:
    Parser(const string &filename) { in.open(filename); }
    bool good() const { return in.is_open(); }
    bool advance(Command &cmd) {
        string line;
        while (getline(in, line)) {
            string s = trimLine(line);
            if (s.empty()) continue;
            vector<string> tokens;
            string token;
            for (size_t i = 0, j = 0; i <= s.size(); i++) {
                if (i == s.size() || isspace((unsigned char)s[i])) {
                    if (i > j) tokens.push_back(s.substr(j, i - j));
                    j = i + 1;
                }
            }
            if (tokens.empty()) continue;
            if (tokens[0] == "push" && tokens.size() == 3) {
                cmd.type = CommandType::C_PUSH;
                cmd.arg1 = tokens[1];
                cmd.arg2 = stoi(tokens[2]);
                cmd.valid = true;
                return true;
            } else if (tokens[0] == "pop" && tokens.size() == 3) {
                cmd.type = CommandType::C_POP;
                cmd.arg1 = tokens[1];
                cmd.arg2 = stoi(tokens[2]);
                cmd.valid = true;
                return true;
            } else if (tokens.size() == 1) {
                string op = tokens[0];
                if (op == "add" || op == "sub" || op == "neg" ||
                    op == "eq" || op == "gt" || op == "lt" ||
                    op == "and" || op == "or" || op == "not") {
                    cmd.type = CommandType::C_ARITHMETIC;
                    cmd.arg1 = op;
                    cmd.valid = true;
                    return true;
                }
            }
        }
        return false;
    }
};
int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Usage: VMTranslator InputFile.vm\n";
        return 1;
    }
    string infile = argv[1];
    if (infile.size() < 4 || infile.substr(infile.size() - 3) != ".vm") {
        cerr << "Input must be a .vm file\n";
        return 1;
    }
    string name = baseName(infile);
    string outfile = infile.substr(0, infile.size() - 3) + ".asm";
    Parser parser(infile);
    if (!parser.good()) {
        cerr << "Failed to open " << infile << "\n";
        return 1;
    }
    CodeWriter writer(outfile, name);
    if (!writer.good()) {
        cerr << "Failed to open output " << outfile << "\n";
        return 1;
    }
    Command cmd;
    while (parser.advance(cmd)) {
        if (!cmd.valid) continue;
        if (cmd.type == CommandType::C_ARITHMETIC) writer.writeArithmetic(cmd.arg1);
        else if (cmd.type == CommandType::C_PUSH) writer.writePush(cmd.arg1, cmd.arg2);
        else if (cmd.type == CommandType::C_POP) writer.writePop(cmd.arg1, cmd.arg2);
        cmd = Command();
    }
    cout << "Wrote " << outfile << "\n";
    return 0;
}
