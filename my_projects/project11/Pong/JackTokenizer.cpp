#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

class Tokenizer {
private:
    vector<string> alltokens;
    vector<string> keywords = {
        "class","constructor","function","method","field","static","var",
        "int","char","boolean","void","true","false","null","this",
        "let","do","if","else","while","return"
    };
    
    vector<char> symbols = {
        '{','}','(',')','[',']','.',',',';','+','-','*','/','&','|','<','>','=','~'
    };

    bool isKeyword(const string& token) {
        return find(keywords.begin(), keywords.end(), token) != keywords.end();
    }

    bool isSymbol(char ch) {
        return find(symbols.begin(), symbols.end(), ch) != symbols.end();
    }

    bool isIntegerConstant(const string& token) {
        if (!all_of(token.begin(), token.end(), ::isdigit)) return false;
        int value = stoi(token);
        return value >= 0 && value <= 32767;
    }

    bool isStringConstant(const string& token) {
        return token.size() >= 2 && token.front() == '"' && token.back() == '"';
    }

    vector<string> removeComments(const string& fname);
    vector<string> tokenizeLine(const string& line);

public:
    string build(const string& filename);
    vector<string> getAllTokens() { return alltokens; }
};

vector<string> Tokenizer::removeComments(const string& fname) {
    vector<string> lines;
    ifstream infile(fname);
    string line;
    bool inBlockComment = false;

    while (getline(infile, line)) {
        string processedLine;
        for (size_t i = 0; i < line.size(); i++) {

            if (!inBlockComment && i + 1 < line.size() && line[i] == '/' && line[i+1] == '/') break;
            else if (!inBlockComment && i + 1 < line.size() && line[i] == '/' && line[i+1] == '*') {
                inBlockComment = true; i++;
            }
            else if (inBlockComment && i + 1 < line.size() && line[i] == '*' && line[i+1] == '/') {
                inBlockComment = false; i++;
            }
            else if (!inBlockComment) processedLine += line[i];
        }
        if (!processedLine.empty()) lines.push_back(processedLine);
    }
    return lines;
}

vector<string> Tokenizer::tokenizeLine(const string& line) {
    vector<string> tokens;
    string token;
    bool inString = false;

    for (char ch : line) {
        if (ch == '"') {
            if (inString) {
                token += ch;
                tokens.push_back(token);
                token.clear();
                inString = false;
            } else {
                if (!token.empty()) tokens.push_back(token);
                token = ch;
                inString = true;
            }
        }
        else if (inString) token += ch;
        else if (isspace(ch)) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        }
        else if (isSymbol(ch)) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            tokens.push_back(string(1, ch));
        }
        else token += ch;
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

string Tokenizer::build(const string& filename) {
    vector<string> lines = removeComments(filename);
    string outfile = filename.substr(0, filename.find_last_of('.')) + "_myT.xml";
    
    ofstream ofs(outfile);
    ofs << "<tokens>\n";

    for (const string& line : lines) {
        vector<string> tokens = tokenizeLine(line);
        for (const string& token : tokens) {

            string str = "";
            if (isKeyword(token)) {
                str += "<keyword> " + token + " </keyword>\n";
            } 
            else if (token.size() == 1 && isSymbol(token[0])) {
                char ch = token[0];
                str += "<symbol> ";
                if (ch == '<') str += "&lt;";
                else if (ch == '>') str += "&gt;";
                else if (ch == '&') str += "&amp;";
                else str += ch;
                str += " </symbol>\n";
            } 
            else if (isIntegerConstant(token)) str += "<integerConstant> " + token + " </integerConstant>\n";
            else if (isStringConstant(token)) str += "<stringConstant> " + token.substr(1, token.size()-2) + " </stringConstant>\n";
            else str += "<identifier> " + token + " </identifier>\n";

            ofs << str;
            alltokens.push_back(token);
        }
    }
    ofs << "</tokens>\n";
    return outfile;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: JackAnalyzer <filename or directory>\n";
        return 0;
    }

    string path = argv[1];
    vector<string> Jackfiles;

    if (fs::is_directory(path)) {
        for (auto& entry : fs::directory_iterator(path)) {
            if (entry.path().extension() == ".jack") {
                Jackfiles.push_back(entry.path().string());
            }
        }
    } else {
        Jackfiles.push_back(path);
    }

    for (string& fname : Jackfiles) {
        Tokenizer tokenizer;
        string output = tokenizer.build(fname);
        cout << "Tokenized " << fname << " -> " << output << "\n";
    }

    return 0;
}
