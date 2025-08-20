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
using namespace std;

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
        output << "ACCEPTED" << endl;
    }
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


//parser

struct Token
{
    string type;
    string value;


};
vector<Token> tokens;
int posCurrentToken=0;

void loadTokens(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Cannot open token file!" << endl;
        return;
    }

    tokens.clear();
    string type, value;
    while (file >> type >> value) {
        if (type == "ACCEPTED") break; 
        tokens.push_back({type, value});
    }
    file.close();
}



Token CurrentToken()
{
    if(posCurrentToken < tokens.size())
    {
        return tokens[posCurrentToken];
    }
    return {"$ ","$"};
}
void match(string  expected)
{
    if(CurrentToken().type == expected)
    {
        cout<<"matched "<<expected<<"\n";
        posCurrentToken++;
    }
    else
    {
        cout<< "Syntax Error: Expected token '" << expected << "' but found '" << CurrentToken().type 
             << "' with value '" << CurrentToken().value << "'" << endl;
        
    }

}
void parseJson();
void parseObject();
void parseObjectT();
void parseMembers();
void parseMembersT();
void parseMember();
void parseArray();
void parseArrayT();
void parseValues();
void parseValuesT();
void parseValue();

void parseString();
void parseNumber();
void parseBoolean();
void parseNull();

void parseJson()
{
    if(CurrentToken().type =="{")
    {
        parseObject();
    }
    else if(CurrentToken().type =="[")
    {
        parseArray();
    }
    else
    {
        cout<<"Error ,Json must start with { or [";
        exit(1);
    }
}
void parseObject()
{
    match("{");
    parseObjectT();
}

void parseObjectT()
{
    if(CurrentToken().type== "}")
    {
        match("}");
    }
    else if(CurrentToken().type== "\"")
    {
        parseMembers();
        match("}");
    }
    else{
        cout<<"unexpected token must be } or \" "<<"\n";
    }
}


void parseMembers()
{
    parseMember();
    parseMembersT();
}
void parseMembersT()
{
    if(CurrentToken().type== ",")
    {
        match(",");
        parseMember();
        parseMembersT();
    }
    else{}

}
void parseMember()
{
    parseString();
    match(":");
    parseValue();
}

void parseArray()
{
    match("[");
    parseArrayT();
}
void parseArrayT()
{
    if(CurrentToken().type== "]")
    {
        match("]");
    }
    else
    {
        parseValues();
        match("]");
    }

}
void parseValues()
{
    parseValue();
    parseValuesT();
}
void parseValuesT()
{
    if(CurrentToken().type== ",")
    {
        match(",");
        parseValue();
        parseValuesT();
    }
    else{}

}

void parseValue()
{
    if(CurrentToken().type=="\"")
    {
        parseString();
    }
    else if(CurrentToken().type=="number")
    {
        parseNumber();
    }
    else if(CurrentToken().type=="{")
    {
        parseObject();
    }
    else if(CurrentToken().type=="[")
    {
        parseArray();
    }
    else if(CurrentToken().type=="null")
    {
        parseNull();
    }
    else if(CurrentToken().type=="true" || CurrentToken().type=="false" )
    {
        parseBoolean();
    }
    else{
        cout<<"error token dont match";
    }
}
void parseString()
{
    match("\"");
    match("string");
    match("\"");
}
void parseNumber()
{
    match("number");
}
void parseBoolean()
{
    if(CurrentToken().type=="true")
    {
        match("true");
    }
    else if(CurrentToken().type=="false")
    {
        match("false");
    }

}
void parseNull()
{
    match("null");
}




int main() {
    vector<TokenRule> rules = loadTokenRules("tokens.txt");
    scanInput("json.txt", rules, "scanner_output.txt");

    


    readGrammar("grammar.txt");
    computeFirst();
    computeFollow();
    writeSetToFile("first.txt", firstSets, "firsts");
    writeSetToFile("follow.txt", followSets, "follows");
    cout << "FIRST and FOLLOW sets written to first.txt and follow.txt." << endl;

    ParsingTable tab1("first.txt", "follow.txt", "grammar.txt");
    tab1.build_parsing_table("parse_table.txt");
    cout << "parsing table is done!" << endl;
    cout<<"\n\n\n";

    loadTokens("scanner_output.txt");
    parseJson();
    if (CurrentToken().type=="$") 
    {
        cout << "VALID JSON! All tokens matched." << endl;
    }
    else
    {
        cout << "INVALID JSON!  unexpected token." << endl;

    }

    system("pause");
    return 0;
}
