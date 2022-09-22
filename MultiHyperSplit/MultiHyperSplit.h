#include <vector>

#include "./../ElementaryClasses.h"

using namespace std;
struct HSnode {
    uint32_t dim, depth, threshold;
    bool isLeaf;
    vector<HSnode*> children;
    int nRules;
    vector<Rule> rules;
    int maxPri;
    HSnode(const vector<Rule> &r) {
        rules = r;
        isLeaf = false;
        children.resize(2, nullptr);
        nRules = r.size();
        if (!r.empty()) {
            maxPri = r[0].priority;
        }
    }
    HSnode (int _dim, int _depth, int thre, bool leaf, const vector<Rule> &r) {
        dim = _dim;
        depth = _depth;
        isLeaf = leaf;
        rules = r;
        children.resize(2, nullptr);
        nRules = int(rules.size());
        threshold = thre;
    }
};

using namespace std;
class MultiHyperSplit : public PacketClassifier {
public:
    MultiHyperSplit();
    MultiHyperSplit(int rP);
    MultiHyperSplit(int md, int b);
    virtual void ConstructClassifier(const vector<Rule> &rules);
    virtual int ClassifyAPacket(const Packet &packet);
    virtual void DeleteRule(const Rule &rule);
    virtual void InsertRule(const Rule &rule);
    virtual Memory MemSizeBytes() const;
    virtual int MemoryAccess() const;
    virtual size_t NumTables() const;
    virtual size_t RulesInTable(size_t tableIndex) const;

    virtual string funName() {
        return "Derived clas: MultiHyperSplit";
    }

    virtual void printInfo(std::ofstream &fout) {
        fout << roots.size() << ",";
        for (int i = 0; i < roots.size(); i ++) {
            fout << roots[i]->nRules << ",";
        }
        // fout << "base function, ";
    }

    HSnode* ConstructHSTree(const vector<Rule> &rules, vector<Rule> &res);


private:

vector<Rule> testRules;

    int maxDepth;
    int binth;
    vector<HSnode*> roots;
    int rulePri;


};