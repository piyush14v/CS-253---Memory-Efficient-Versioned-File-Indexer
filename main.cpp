#include <bits/stdc++.h>
using namespace std;
using namespace std::chrono;

template <typename K, typename V>
class SimpleMap {
public:
    unordered_map<K,V> data;

    void insert(const K& key, const V& input){
        data[key] += input;
    }

    V get(const K& key){
        if(data.find(key)==data.end()) return 0;
        return data[key];
    }
};

class BufferedFileReader {
private:
    ifstream file;
    size_t bufferSize;
    vector<char> buffer;

public:
    BufferedFileReader(const string& path, size_t bufSize){
        file.open(path);
        if(!file) throw runtime_error("Cannot open file");

        bufferSize = bufSize;
        buffer.resize(bufferSize);
    }

    bool readChunk(string &out){
        if(file.eof()) return false;

        file.read(buffer.data(), bufferSize);
        streamsize bytes = file.gcount();

        if(bytes<=0) return false;

        out.assign(buffer.data(), bytes);
        return true;
    }
};

class Tokenizer {
private:
    string leftover;

public:

    static string toLower(string s){
        for(char &c : s) c = tolower(c);
        return s;
    }

    vector<string> tokenize(const string& chunk){
        vector<string> tokens;
        string word = leftover;

        for(char c : chunk){
            if(isalnum(c)){
                word += c;
            }
            else{
                if(!word.empty()){
                    tokens.push_back(toLower(word));
                    word.clear();
                }
            }
        }

        leftover = word;
        return tokens;
    }

    string flush(){
        string t = leftover;
        leftover.clear();
        return t;
    }
};

class VersionedIndexer {

private:
    unordered_map<string, SimpleMap<string,int>>versions;

public:

    void buildIndex(const string& version,
                    const string& filepath,
                    size_t bufferSize)
    {
        BufferedFileReader reader(filepath, bufferSize);
        Tokenizer tokenizer;

        string chunk;

        while(reader.readChunk(chunk)){
            vector<string> tokens = tokenizer.tokenize(chunk);

            for(auto &w : tokens){
                versions[version].insert(w,1);
            }
        }

        string last = tokenizer.flush();
        if(!last.empty())
            versions[version].insert(last,1);
    }

    int getWordCount(const string& version,const string& word){
    string w = word;
    for(char &c : w) c = tolower(c);
    return versions[version].get(w);
}
    vector<pair<string,int>>topK(const string& version,int tertiary){

        vector<pair<string,int>>v;

        for(auto &p : versions[version].data)
            v.push_back(p);

        sort(v.begin(),v.end(),
            [](auto &a,auto &b){
                return a.second > b.second;
            });

        if(v.size()>tertiary) v.resize(tertiary);

        return v;
    }
};

class Query {
public:
    virtual void execute() = 0;
    virtual ~Query(){}
};

class WordQuery : public Query {

    VersionedIndexer &indexer;
    string version;
    string word;

public:

    WordQuery(VersionedIndexer& idx,string v,string w)
        : indexer(idx),version(v),word(w){}

    void execute() override{
        int count = indexer.getWordCount(version,word);

        cout<< "Version: "<< version<< endl;
        cout<< "Word: "<< word<< endl;
        cout<< "Frequency: "<< count<< endl;
    }
};

class TopKQuery : public Query {

    VersionedIndexer &indexer;
    string version;
    int tertiary;

public:

    TopKQuery(VersionedIndexer& idx,string v,int K)
        : indexer(idx),version(v),tertiary(K){}

    void execute() override{

        auto result = indexer.topK(version,tertiary);

        cout<< "Version: "<< version<< endl;
        cout<< "Top "<< tertiary<< " words:"<< endl;

        for(auto &p : result){
            cout<< p.first<< " -> "<< p.second<< endl;
        }
    }
};

class DiffQuery : public Query {

    VersionedIndexer &indexer;
    string v1,v2;
    string word;

public:

    DiffQuery(VersionedIndexer& idx,string a,string b,string w)
        : indexer(idx),v1(a),v2(b),word(w){}

    void execute() override{

        int c1 = indexer.getWordCount(v1,word);
        int c2 = indexer.getWordCount(v2,word);

        cout<< "Version1: "<< v1<< " Count: "<< c1<< endl;
        cout<< "Version2: "<< v2<< " Count: "<< c2<< endl;
        cout<< "Difference: "<< (c2-c1)<< endl;
    }
};

string getArg(int argc,char* argv[],string key){
    for(int counter=1;counter<argc;counter++)
        if(string(argv[counter])==key)
            return argv[counter+1];
    return "";
}

int getArg(int argc,char* argv[],string key,int defaultVal){
    for(int counter=1;counter<argc;counter++)
        if(string(argv[counter])==key)
            return stoi(argv[counter+1]);
    return defaultVal;
}

int main(int argc,char* argv[])
{
    try{

        auto start = high_resolution_clock::now();

        string query = getArg(argc,argv,"--query");
        int bufferKB = getArg(argc,argv,"--buffer",512);

        if(bufferKB <256 || bufferKB>1024)
            throw runtime_error("Buffer size must be 256-1024 KB");

        size_t bufferBytes = bufferKB*1024;

        VersionedIndexer indexer;

        Query* q = nullptr;

        if(query=="word")
        {
            string file = getArg(argc,argv,"--file");
            string version = getArg(argc,argv,"--version");
            string word = getArg(argc,argv,"--word");

            indexer.buildIndex(version,file,bufferBytes);

            q = new WordQuery(indexer,version,word);
        }

        else if(query=="top")
        {
            string file = getArg(argc,argv,"--file");
            string version = getArg(argc,argv,"--version");
            int tertiary = getArg(argc,argv,"--top",10);

            indexer.buildIndex(version,file,bufferBytes);

            q = new TopKQuery(indexer,version,tertiary);
        }

        else if(query=="diff")
        {
            string file1 = getArg(argc,argv,"--file1");
            string file2 = getArg(argc,argv,"--file2");

            string v1 = getArg(argc,argv,"--version1");
            string v2 = getArg(argc,argv,"--version2");

            string word = getArg(argc,argv,"--word");

            indexer.buildIndex(v1,file1,bufferBytes);
            indexer.buildIndex(v2,file2,bufferBytes);

            q = new DiffQuery(indexer,v1,v2,word);
        }

        else{
            throw runtime_error("Invalid query type");
        }

        q->execute();

        auto end = high_resolution_clock::now();
        double time =
            duration<double>(end-start).count();

        cout<< "Buffer Size: "<< bufferKB<< " KB"<< endl;
        cout<< "Execution Time: "<< time<< " seconds"<< endl;

        delete q;
    }

    catch(exception &e){
        cout<< "Error: "<< e.what()<< endl;
    }

    return 0;
}
