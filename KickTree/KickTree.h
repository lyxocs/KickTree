#ifndef KICKTREE_V0_1_KICKTREE_H
#define KICKTREE_V0_1_KICKTREE_H

#pragma once
#include <iomanip>
#include <iostream>

#include "../ElementaryClasses.h"

using namespace std;
struct KickTreeNode {
    vector<Rule> classifier;
    int nrules;
    int depth;
    bool isLeaf;
    vector<int> opt, bit;

    vector<KickTreeNode*> children;
    KickTreeNode* parent;
    int parentPosition;
    vector<int> left;

    KickTreeNode(const vector<Rule> &rules, int level = 0, bool isleaf = false) {
        classifier = rules;
        nrules = int(rules.size());
        depth = level;
        isLeaf = isleaf;
        left = {0, 0, 0, 0};
        parent = nullptr;
    }
};

class KickTree : public PacketClassifier {
public:
    KickTree(int maxBits = 4, int maxLevel = 4, int binth = 4, int maxTreeNum = 200);
    ~KickTree();
    void ConstructClassifier(const std::vector<Rule>& rules) override;
    int ClassifyAPacket(const Packet& packet) override;
    int ClassifyAPacket(const Packet &packet, uint64_t &Query) override;
    void DeleteRule(const Rule& delete_rule) override;
    void InsertRule(const Rule& insert_rule) override;
    Memory MemSizeBytes() const override;
    size_t NumTables() const override;
    size_t RulesInTable(size_t tableIndex) const override { return 0; }

    int WorstMemAccess();
    int WorstMemAccess(const Packet& packet);

    KickTreeNode* CreateSubKickTree(const vector<Rule> &rules, vector<Rule> &kickedRules) ;
    bool DeleteInSubKickTree(KickTreeNode* root, const Rule& delete_rule);
    bool InsertInSubKickTree(KickTreeNode* root, const Rule& insert_rule);

    static vector<int> GetSelectBit(KickTreeNode *node, vector<int>& opt);
    int ClassifyAPacketInKickTree(KickTreeNode *root, const Packet &p) const;
    void UpdatePriInKickTree(KickTreeNode *root, int currentUpdatePriTreeIndex);

private:
    vector<Rule> classifier;
    int nrules;
    vector<KickTreeNode*> roots;

    int maxBits;
    int maxLevel;
    int binth;
    int maxTreeNum;
    vector<vector<int> > partitionOpt;
    vector<Rule> bigRule;
    vector<int> Maxpri;

    vector<priority_queue<int> > rootPri;

    int currentTreeIndex;
    int currentInsertTreeIndex;

    double rulePercent;
    int nTreeToChangeStrategy;
    int strategyFlag; // 0: no-wildcard     1: rule duplication
};

#endif //KICKTREE_V0_1_KICKTREE_H
