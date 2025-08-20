#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <algorithm>
#include <stack>
#include <cctype>
using namespace std;

bool valid = true;

// Scanner
struct TokenRule {
    string name;
    regex pattern;
};
vector<TokenRule> loadTokenRules(const string& filename) {
    vector<TokenRule> rules;
    ifstream file(filename);
    string name, pattern;

    while (file >> name) {
        file >> ws;
        getline(file, pattern);
        rules.push_back({name, regex(pattern)});
    }

    return rules;
}

void scanInput(const string& inputFilename, const vector<TokenRule>& rules, const string& outputFilename) {
    ifstream input(inputFilename);
    ofstream output(outputFilename);
    
    if (!input) {
        cerr << "Error: Cannot open input file " << inputFilename << endl;
        valid = false;
        return;
    }
    
    if (!output) {
        cerr << "Error: Cannot create output file " << outputFilename << endl;
        valid = false;
        return;
    }
    
    string line;
    int lineNum = 1;
    bool errorFound = false;

    while (getline(input, line)) {
        size_t i = 0;
        while (i < line.size()) {
            bool matched = false;
            for (const auto& rule : rules) {
                smatch match;
                string substr = line.substr(i);
                if (regex_search(substr, match, rule.pattern) && match.position() == 0) {
                    if (rule.name != "WHITESPACE") {
                        output << rule.name << " " << match.str() << endl;
                    }
                    i += match.length();
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                output << "ERROR: Unknown token at line " << lineNum << " near: " << line[i] << endl;
                errorFound = true;
                break;
            }
        }
        if (errorFound) break;
        lineNum++;
    }

    if (!errorFound) {
        cout << "ACCEPTED" << endl;
    } else {
        valid = false;
    }
    
    input.close();
    output.close();
}

// create first and follow files
map<string, vector<vector<string>>> grammarr;
map<string, set<string>> firstSets, followSets;
string startSymbol;
bool isNonTerminal(const string &symbol) {
    return isupper(symbol[0]);
}
void readGrammar(const string &filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Cannot open " << filename << endl;
        return; // or exit(1)
    }
    string line;
    int lineCount = 0;

    while (getline(file, line)) {
        lineCount++;
        if (lineCount <= 3 || line.empty()) continue;
        size_t tabPos = line.find('\t');
        if (tabPos == string::npos) continue;

        string lhs = line.substr(0, tabPos);
        lhs.erase(remove(lhs.begin(), lhs.end(), ' '), lhs.end());

        if (startSymbol.empty()) startSymbol = lhs;

        string rhs = line.substr(tabPos + 1);
        istringstream prodStream(rhs);
        string prod;
        while (getline(prodStream, prod, '|')) {
            istringstream symbolStream(prod);
            vector<string> production;
            string symbol;
            while (symbolStream >> symbol) {
                production.push_back(symbol);
            }
            grammarr[lhs].push_back(production);
        }
    }
}

set<string> first(const string &symbol);
set<string> first(const vector<string> &symbols) {
    set<string> result;
    for (const string &symbol : symbols) {
        set<string> temp = first(symbol);
        result.insert(temp.begin(), temp.end());
        if (temp.find("e") == temp.end()) break;
        else if (symbol == symbols.back()) result.insert("e");
    }
    return result;
}
set<string> first(const string &symbol) {
    if (!isNonTerminal(symbol)) return {symbol};

    if (!firstSets[symbol].empty()) return firstSets[symbol];

    for (const auto &prod : grammarr[symbol]) {
        set<string> temp = first(prod);
        firstSets[symbol].insert(temp.begin(), temp.end());
    }

    return firstSets[symbol];
}
void computeFirst() {
    for (const auto &entry : grammarr) {
        const string &nonTerminal = entry.first;
        first(nonTerminal);
    }
}
void computeFollow() {
    followSets[startSymbol].insert("$");

    bool updated = true;
    while (updated) {
        updated = false;

        for (const auto &rule : grammarr) {
            const string &head = rule.first;

            for (const auto &prod : rule.second) {
                for (size_t i = 0; i < prod.size(); ++i) {
                    const string &B = prod[i];
                    if (!isNonTerminal(B)) continue;

                    if (i + 1 < prod.size()) {
                        vector<string> beta(prod.begin() + i + 1, prod.end());
                        set<string> firstBeta = first(beta);

                        size_t before = followSets[B].size();
                        for (const string &sym : firstBeta) {
                            if (sym != "e") followSets[B].insert(sym);
                        }

                        if (firstBeta.count("e")) {
                            followSets[B].insert(followSets[head].begin(), followSets[head].end());
                        }

                        if (followSets[B].size() > before) updated = true;
                    } else {
                        size_t before = followSets[B].size();
                        followSets[B].insert(followSets[head].begin(), followSets[head].end());
                        if (followSets[B].size() > before) updated = true;
                    }
                }
            }
        }
    }
}
void writeSetToFile(const string &filename, const map<string, set<string>> &sets, const string &label) {
    ofstream out(filename);
    for (const auto &entry : sets) {
        out << entry.first << " ";
        for (const auto &sym : entry.second) {
            out << sym << " ";
        }
        out << endl;
    }
}

// parsing table
struct Grammar {
    vector<string> terminals;
    vector<string> nonterminals;
    string startsymbol;
    map<string, vector<string>> production_rules;
};
struct Predictive_table {
    string nonterminal;
    string first;
    string rule;
};
class ParsingTable {
private:
    map<string, vector<string>> first;
    map<string, vector<string>> follow;
    Grammar grammar;
    vector<Predictive_table> predictive_table;

public:
    ParsingTable(string fst, string flo, string grm) {
        cout << "Creating object..." << endl;
        read_first(fst);
        for (const auto& [nonterminal, symbols] : first) {
            cout << "First of " << nonterminal << ": ";
            for (const string& sym : symbols) cout << sym << " ";
            cout << endl;
        }

        read_follow(flo);
        for (const auto& [nonterminal, symbols] : follow) {
            cout << "Follow of " << nonterminal << ": ";
            for (const string& sym : symbols) cout << sym << " ";
            cout << endl;
        }

        read_grammar(grm);
        for (const auto& [nonterminal, symbols] : grammar.production_rules) {
            cout << "rule of " << nonterminal << ": ";
            for (const string& sym : symbols) cout << sym << " ";
            cout << endl;
        }
    }

    void read_first(string fst) {
        ifstream infirst(fst);
        if (!infirst) {
            cerr << "Error opening first.txt!" << endl;
            return;
        }

        string line;
        while (getline(infirst, line)) {
            stringstream s(line);
            string nonterminal;
            s >> nonterminal;
            string symbol;
            vector<string> symbols;
            while (s>>symbol) {
                symbols.push_back(remove_spaces(symbol));
            }
            first[remove_spaces(nonterminal)] = symbols;
        }
        infirst.close();
    }

    void read_follow(string flo) {
        ifstream infollow(flo);
        if (!infollow) {
            cerr << "Error opening follow.txt!" << endl;
            return;
        }

        string line;
        while (getline(infollow, line)) {
            stringstream s(line);
            string nonterminal;
            s >> nonterminal;
            string symbol;
            vector<string> symbols;
            while (s>>symbol) {
                symbols.push_back(remove_spaces(symbol));
            }
            follow[remove_spaces(nonterminal)] = symbols;
        }
        infollow.close();
    }

    void read_grammar(string grm) {
        ifstream ingrammar(grm);
        if (!ingrammar) {
            cerr << "Error opening grammar.txt!" << endl;
            return;
        }

        string line;
        if (getline(ingrammar, line)) {
            stringstream s(line);
            string terminal;
            while (s>>terminal) {
                grammar.terminals.push_back(remove_spaces(terminal));
            }
        }

        if (getline(ingrammar, line)) {
            stringstream s(line);
            string nonterminal;
            while (s>>nonterminal) {
                grammar.nonterminals.push_back(remove_spaces(nonterminal));
            }
        }

        if (getline(ingrammar, line)) {
            grammar.startsymbol = remove_spaces(line);
        }

        while (getline(ingrammar, line)) {
            stringstream s(line);
            string nonterminal;
            s >> nonterminal;
            string rule;
            vector<string> rules;
            while (getline(s, rule, '|')) {
                rules.push_back(trim(rule));
            }
            grammar.production_rules[remove_spaces(nonterminal)] = rules;
        }
        ingrammar.close();
    }

    void build_parsing_table(string output) {
        ofstream predfile(output);
        if (!predfile) {
            cerr << "Error opening output file!" << endl;
            return;
        }
        cout << "Starting to build parsing table..." << endl;

        for (const auto& [nonterminal, rules] : grammar.production_rules) {
            for (const string& rule : rules) {
                vector<string> symbols = split_rule(rule);
                if (symbols.empty()) {
                    continue;
                }

                string symbol = symbols[0];
                if (find(grammar.terminals.begin(), grammar.terminals.end(), symbol) != grammar.terminals.end()) {
                    predictive_table.push_back({ nonterminal, symbol, rule });
                    cout << "Building entry for: " << nonterminal << " -> " << rule << endl;
                }
                else if (find(grammar.nonterminals.begin(), grammar.nonterminals.end(), symbol) != grammar.nonterminals.end()) {
                    for (const string& fst : first[symbol]) {
                        if (fst != "e") {
                            predictive_table.push_back({ nonterminal, fst, rule });
                            cout << "Building entry for: " << nonterminal << " -> " << rule << endl;
                        }
                        else {
                            for (const string& flo : follow[symbol]) {
                                predictive_table.push_back({ nonterminal, flo, rule });
                                cout << "Building entry for: " << nonterminal << " -> " << rule << endl;
                            }
                        }
                    }
                }
                else if (symbol == "e") {
                    for (const string& flo : follow[nonterminal]) {
                        predictive_table.push_back({ nonterminal, flo, rule });
                        cout << "Building entry for: " << nonterminal << " -> " << rule << endl;
                    }
                }
            }
        }
        predfile << grammar.startsymbol << endl;
        for (const auto& entry : predictive_table) {
            predfile << entry.nonterminal << " " << entry.first << "\t" << entry.rule << endl;
            cout << "Writing: " << entry.nonterminal << "," << entry.first << "\t" << entry.rule << endl;
        }

        cout << "Writing to file completed." << endl;
        predfile.close();
    }

    vector<string> split_rule(const string& rule) {
        stringstream ss(rule);
        string sym;
        vector<string> result;
        while (ss >> sym) result.push_back(remove_spaces(sym));
        return result;
    }

    string remove_spaces(const string& str) {
        string result = "";
        for (char c : str) {
            if (c != ' ') result += c;
        }
        return result;
    }

    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t");
        if (first == string::npos)
            return "";

        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }
};

// parser LL-1
class LL1_parser{
    private:
    Grammar grammar;
    stack<string> tokens, temp_stack;
    string startsymbol="";
    vector<Predictive_table> predictive_table;

    public:
    LL1_parser(string inp, string inpre, string grm){
        read_input(inp);
        read_predictive_table(inpre);
        read_grammar(grm);
    }
    void read_input(string inp){
        ifstream in(inp);
        if (!in) {
            cerr << "error" << endl;
            return;
        }
        string line;
        vector<string> tokens1;
        while(getline(in,line)){
            stringstream s(line);
            string token;
            s>>token;
            tokens1.push_back(remove_spaces(token));
        }
        tokens.push("$");
        for (int i = tokens1.size() - 1; i >= 0; --i){
            cout << "Pushed token: " << remove_spaces(tokens1[i]) << endl;
            tokens.push(remove_spaces(tokens1[i]));
        }
        cout << "tokens stack" << endl;
        print_stack(tokens);
        in.close();
    }
    void read_predictive_table(string inpre){
        ifstream inpredictivetable(inpre);
        if (!inpredictivetable) {
            cerr << "error" << endl;
            return;
        }
        string line;
        set<pair<string, string>> rules_check;
        if(getline(inpredictivetable,line)){
            startsymbol = remove_spaces(line);
            cout << "Start symbol: " << startsymbol << endl;
        }
        while(getline(inpredictivetable,line)){
            stringstream ss(line);
            string nonterminal,first,rule;
            ss>>nonterminal>>first;
            getline(ss,rule);
            nonterminal = remove_spaces(nonterminal);
            first = remove_spaces(first);
            if (rules_check.count({nonterminal, first})) {
                cerr << "Error: Conflict in parse table at (" << nonterminal << "," << first << ")" << endl;
                exit(1);
            }
            rules_check.insert({nonterminal, first});
            predictive_table.push_back({nonterminal, first, rule});
        }
        inpredictivetable.close();
    }
    void read_grammar(string grm) {
        ifstream ingrammar(grm);
        if (!ingrammar) {
            cerr << "Error opening grammar.txt!" << endl;
            return;
        }

        string line;
        if (getline(ingrammar, line)) {
            stringstream s(line);
            string terminal;
            while (s>>terminal) {
                grammar.terminals.push_back(remove_spaces(terminal));
            }
        }

        if (getline(ingrammar, line)) {
            stringstream s(line);
            string nonterminal;
            while (s>>nonterminal) {
                grammar.nonterminals.push_back(remove_spaces(nonterminal));
            }
        }

        if (getline(ingrammar, line)) {
            grammar.startsymbol = remove_spaces(line);
        }

        while (getline(ingrammar, line)) {
            stringstream s(line);
            string nonterminal;
            s >> nonterminal;
            string rule;
            vector<string> rules;
            while (getline(s, rule, '|')) {
                rules.push_back(trim(rule));
            }
            grammar.production_rules[remove_spaces(nonterminal)] = rules;
        }
        ingrammar.close();
    }
    void check_parser(string outp){
        ofstream out(outp);
        if (!out) {
            cerr << "error" << endl;
            return;
        }
        temp_stack.push("$");
        temp_stack.push(startsymbol);
        while(temp_stack.top()!="$"){
            string top_r=temp_stack.top();
            string top_i=tokens.top();
            cout << "Top of stack: " << top_r << ", Current token: " << top_i << endl;

            if(is_terminal(top_r)){
                if(top_r==top_i){
                    temp_stack.pop();
                    tokens.pop();
                } else{
                    return;
                }
            } else if(is_nonterminal(top_r)){
                string rule;
                string nonterm = top_r;
                rule = get_rule(nonterm,top_i);
                if(rule!=""){
                    temp_stack.pop();
                    vector<string> srules=split_rule(rule);
                    for (int i = srules.size() - 1; i >= 0; --i){
                        temp_stack.push(remove_spaces(srules[i]));
                    }
                } else{
                    return;
                }
                print_stack(tokens);
                print_stack(temp_stack);
            } else if(top_r=="e"){
                temp_stack.pop();
                print_stack(tokens);
                print_stack(temp_stack);
            } else {
                return;
            }
        }
        if(tokens.top()=="$"&&temp_stack.top()=="$"){
            out<<"accepted!!";
            cout <<"accepted!!"<<endl;
        } else {
            valid = false;
            out<<"not accepted!!";
            cout<<"not accepted!!"<<endl;
        }
        out.close();
    }
    bool is_terminal(string term){
        for(string t : grammar.terminals){
            if(term == t){
                return true;
            }
        }
        return false;
    }
    bool is_nonterminal(string nterm){
        for(string t : grammar.nonterminals){
            if(nterm == t){
                return true;
            }
        }
        return false;
    }
    string get_rule(string nonterminal,string terminal){
        for(Predictive_table t : predictive_table){
            if(t.nonterminal==nonterminal&&t.first==terminal){
                cout << "Matched Rule: " << t.rule << " for " << nonterminal << "," << terminal << endl;
                return t.rule;
            }
        }
        cout << "No rule found for: " << nonterminal << "," << terminal << endl;
        return "";
    }
    vector<string> split_rule(const string& rule) {
        stringstream ss(rule);
        string sym;
        vector<string> result;
        while (ss >> sym) result.push_back(sym);
        return result;
    }
    string remove_spaces(const string& str) {
        string result = "";
        for (char c : str) {
            if (c != ' ')
                result += c;
        }
        return result;
    }
    void print_stack(stack<string> s) {
        vector<string> temp;
        while (!s.empty()) {
            temp.push_back(s.top());
            s.pop();
        }
        for (int i = temp.size()-1; i >= 0; --i) {
            cout << temp[i] << " ";
        }
        cout << endl;
    }
    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t");
        if (first == string::npos)
            return "";

        size_t last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }
};



// JSON file to XML file
string indent(int level) {
    return string(level * 2, ' ');
}
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos || end == string::npos) return "";
    return s.substr(start, end - start + 1);
}
string parseJSONtoXML(stringstream& ss, int level = 0, const string& currentTag = "root");
string parseValue(stringstream& ss, int level, const string& tag);
string parseString(stringstream& ss) {
    string result;
    char ch;
    while (ss.get(ch)) {
        if (ch == '"') break;
        result += ch;
    }
    return result;
}
string parseObject(stringstream& ss, int level, const string& tag) {
    string xml = indent(level) + "<" + tag + ">\n";
    string key, value;
    char ch;

    while (ss >> ch) {
        if (ch == '"') {
            key = parseString(ss);

            // skip colon
            while (ss >> ch && ch != ':');

            xml += parseValue(ss, level + 1, key);
        } else if (ch == '}') {
            break;
        }
    }

    xml += indent(level) + "</" + tag + ">\n";
    return xml;
}
string parseArray(stringstream& ss, int level, const string& tag) {
    string xml;
    char ch;

    while (ss >> ch) {
        if (ch == ']') break;
        ss.putback(ch);

        xml += indent(level) + "<" + tag + ">\n";
        xml += parseValue(ss, level + 1, "item");
        xml += indent(level) + "</" + tag + ">\n";

        ss >> ch;
        if (ch != ',' && ch != ']') ss.putback(ch);
        if (ch == ']') break;
    }

    return xml;
}
string parseValue(stringstream& ss, int level, const string& tag) {
    char ch;
    while (ss >> ch) {
        if (ch == '"') {
            string val = parseString(ss);
            return indent(level) + "<" + tag + ">" + val + "</" + tag + ">\n";
        } else if (isdigit(ch) || ch == '-' || ch == '+') {
            string num(1, ch);
            while (ss.peek() != EOF && (isdigit(ss.peek()) || ss.peek() == '.')) {
                num += ss.get();
            }
            return indent(level) + "<" + tag + ">" + num + "</" + tag + ">\n";
        } else if (ch == 't') { // true
            ss.ignore(3);
            return indent(level) + "<" + tag + ">true</" + tag + ">\n";
        } else if (ch == 'f') { // false
            ss.ignore(4);
            return indent(level) + "<" + tag + ">false</" + tag + ">\n";
        } else if (ch == 'n') { // null
            ss.ignore(3);
            return indent(level) + "<" + tag + "/>\n";
        } else if (ch == '{') {
            return parseObject(ss, level, tag);
        } else if (ch == '[') {
            return parseArray(ss, level, tag);
        }
    }
    return "";
}
string parseJSONtoXML(stringstream& ss, int level, const string& currentTag) {
    string xml;
    char ch;

    while (ss >> ch) {
        if (ch == '{') {
            xml += parseObject(ss, level, currentTag);
        } else if (ch == '[') {
            xml += parseArray(ss, level, currentTag);
        }
    }
    return xml;
}



int main(int argc, char* argv[]) {
    string inputFile = "json.text";
    
    if (argc > 1) { 
        inputFile = argv[1];
    }

    vector<string> requiredFiles = {"tokens.txt", "grammar.txt"};
    for (const string& file : requiredFiles) {
        ifstream check(file);
        if (!check) {
            cerr << "Error: Required file '" << file << "' not found!" << endl;
            return 1;
        }
        check.close();
    }

    ifstream checkInput(inputFile);
    if (!checkInput) {
        cerr << "Error: Input file '" << inputFile << "' not found!" << endl;
        return 1;
    }
    checkInput.close();

    vector<TokenRule> rules = loadTokenRules("tokens.txt");
    scanInput(inputFile, rules, "scanner_output.txt");

    readGrammar("grammar.txt");
    computeFirst();
    computeFollow();
    writeSetToFile("first.txt", firstSets, "firsts");
    writeSetToFile("follow.txt", followSets, "follows");
    cout << "FIRST and FOLLOW sets written to first.txt and follow.txt." << endl;

    ParsingTable tab1("first.txt", "follow.txt", "grammar.txt");
    tab1.build_parsing_table("parse_table.txt");
    cout << "parsing table is done!" << endl;

    LL1_parser pars("scanner_output.txt","parse_table.txt","grammar.txt");
    pars.check_parser("output.txt");
    
    if (valid) {
        ifstream input(inputFile);
        if (!input) {
            cerr << "Error: " << inputFile << " not found\n";
            return 1;
        }

        stringstream buffer;
        buffer << input.rdbuf();
        input.close();

        string xml = parseJSONtoXML(buffer, 0, "root");

        cout << xml;

        ofstream output("xml.txt");
        output << xml;
        output.close();
    }
    return 0;
}