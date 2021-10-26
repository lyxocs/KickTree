//
// Created by xixixi on 2021/9/3.
//

#include "KickTree.h"

KickTree::KickTree(int maxBits, int maxLevel, int binth, int maxTreeNum) {
    this->maxBits = maxBits;
    this->maxLevel = maxLevel;
    this->binth = binth;
    this->maxTreeNum = maxTreeNum;

    this->nTreeToChangeStrategy = 10;
    this->rulePercent = 0.2;
    this->strategyFlag = 0;

    partitionOpt.resize(5);
    for (int i = -1; i < 4; i++) {
        partitionOpt[i + 1].push_back(i);
    }

    while (partitionOpt[0].size() < maxBits) {
        auto temp = partitionOpt;
        partitionOpt.clear();
        for (auto iter : temp) {
            for (int i = iter.back(); i < 4; i++) {
                vector<int> tmp = iter;
                tmp.push_back(i);
                partitionOpt.push_back(tmp);
            }
        }
    }
}

KickTree::~KickTree() = default;

void KickTree::ConstructClassifier(const vector<Rule> &rules) {
    this->classifier = rules;
    this->nrules = int(rules.size());
    vector<Rule> currRules = rules;
    vector<Rule> kickedRules;
    int treeNum = 0;
    while (!currRules.empty()) {
        kickedRules.clear();
        Maxpri.push_back(-1);

        priority_queue<int> rulePri;
        rootPri.push_back(rulePri);

        if(currRules.size() < 0.2 * nrules) {
            strategyFlag = 1;
            cout<<"Rule duplication"<<endl;
        } else {
            strategyFlag = 0;
        }

        KickTreeNode *node = CreateSubKickTree(currRules, kickedRules);
        cout << "Tree #" << setw(3) << treeNum++ << "    contains rule:" << setw(7)
             << currRules.size() - kickedRules.size() << "\tKicked Rules: " << kickedRules.size() << endl;
        roots.push_back(node);

        if(kickedRules.empty()) {
            break;
        }
        SortRules(kickedRules);
        currRules.clear();
        currRules.push_back(kickedRules.front());
        for(int i = 1; i < kickedRules.size(); i++) {
            if(kickedRules[i].priority != currRules.back().priority) {
                currRules.push_back(kickedRules[i]);
            }
        }

        if (roots.size() > maxTreeNum) {
            bigRule = currRules;
            break;
        }
    }

    if (!bigRule.empty()) {
        Maxpri.push_back(bigRule[0].priority);
    }
}

int KickTree::ClassifyAPacket(const Packet &packet) {
    int matchPri = -1;
    for (int i = 0; i < roots.size(); i++) {
        if (matchPri < Maxpri[i]) {
            matchPri = max(matchPri, ClassifyAPacketInKickTree(roots[i], packet));
        }
    }
    int i = 0;
    while (i < bigRule.size() && matchPri < bigRule[i].priority) {
        for (const Rule &rule : bigRule) {
            matchPri = max(matchPri, rule.priority);
        }
    }
    if (!bigRule.empty() && matchPri < bigRule[0].priority) {
        for (const Rule &rule : bigRule) {
            if (rule.MatchesPacket(packet)) {
                matchPri = max(matchPri, rule.priority);
            }
        }
    }
    return matchPri;
}

int KickTree::ClassifyAPacket(const Packet &packet, uint64_t &Query) {
    return 0;
}

void KickTree::DeleteRule(const Rule &delete_rule) {
    for (int i = 0; i < roots.size(); i++) {
        if (delete_rule.priority < Maxpri[i]) {
            if(DeleteInSubKickTree(roots[i], delete_rule)) {
                return;
            }
        }
    }
    // cout<<"Not Delete"<<endl;
}

void KickTree::InsertRule(const Rule &insert_rule) {

    Rule kickedRule = insert_rule;
    for (int i = 0; i < roots.size(); i++) {
        if (InsertInSubKickTree(roots[i], insert_rule)) {
            if(insert_rule.priority > Maxpri[i]) {
                Maxpri[i] = insert_rule.priority;
            }
            return;
        }
    }
    // cout<<"New Tree"<<endl;
    vector<Rule> newTreeRule = {insert_rule};
    auto *newRoot = new KickTreeNode(newTreeRule, 0, true);
    roots.push_back(newRoot);
    Maxpri.push_back(insert_rule.priority);

}

Memory KickTree::MemSizeBytes() const {
    int nNodeCount = 0, nRuleCount = 0, nPTRCount = 0;
    Memory totMemory = 0;
    for(auto root : roots) {
        queue<KickTreeNode*> que;
        que.push(root);
        while(!que.empty()) {
            KickTreeNode *node = que.front();
            que.pop();
            nNodeCount ++;
            if(node->isLeaf) {
                nRuleCount += node->nrules;
                continue;
            }
            nPTRCount += int(node->children.size());
            for(auto iter : node->children) {
                if(iter) {
                    que.push(iter);
                }
            }
        }
    }
    totMemory = nNodeCount * NODE_SIZE + nRuleCount * PTR_SIZE + nPTRCount * PTR_SIZE;
    return totMemory;
}

size_t KickTree::NumTables() const {
    return roots.size();
}

KickTreeNode *KickTree::CreateSubKickTree(const vector<Rule> &rules, vector<Rule> &kickedRules) {
    auto *root = new KickTreeNode(rules, 1, false);
    queue<KickTreeNode *> que;
    que.push(root);
    int nNode = int(que.size());
    while (!que.empty()) {
        KickTreeNode *node = que.front();

        que.pop();
        nNode--;

        if (node->depth == maxLevel || node->nrules <= binth) {
            node->isLeaf = true;
            while (!node->classifier.empty() && node->nrules > binth) {
                kickedRules.push_back(node->classifier.back());
                node->classifier.pop_back();
                node->nrules--;
            }
            Maxpri.back() = max(node->classifier[0].priority, Maxpri.back());
            for (const Rule &rule : node->classifier) {
                rootPri.back().push(rule.priority);
            }
            continue;
        }

        int Min = node->nrules, minKicked = node->nrules;
        vector<int> bestOpt = partitionOpt[0];
        vector<int> bestBit = GetSelectBit(node, partitionOpt[0]);

        for (auto opt : partitionOpt) {
            vector<int> subnRules((1 << maxBits), 0);
            int nKickedRules = 0;
            vector<int> bit = GetSelectBit(node, opt);
            for(const Rule &rule : node->classifier) {
                vector<int> loc = {0};
                for(int i =0; i < maxBits; i++) {
                    if(opt[i] == -1) {
                        continue;
                    }
                    for(auto &iter : loc) {
                        iter <<= 1;
                    }
                    int t = rule.Getbit(opt[i], bit[i]);
                    switch(t) {
                        case -1: {
                            int locSize = int(loc.size());
                            for(int locIndex = 0; locIndex < locSize; locIndex ++) {
                                loc.push_back(loc[locIndex] + 1);
                            }
                            break;
                        }
                        case 0:
                            break;
                        case 1:
                            for(auto &iter : loc) {
                                iter ++;
                            }
                            break;
                        default:
                            cout<<"Wrong in CreateSubKickTree(211)"<<endl;
                            exit(-1);
                    }
                }
                if((strategyFlag == 1 && loc.size() > 2) || (strategyFlag == 0 && loc.size() > 1)) {
                    // kick
                    nKickedRules++;
                } else {
                    for(auto iter : loc) {
                        subnRules[iter] ++;
                    }
                }
            }
            int maxRule = 0;
            for(auto iter : subnRules) {
                maxRule = max(maxRule, iter + nKickedRules);
            }
            if (minKicked < Min || minKicked == Min && maxRule <= minKicked) {
                Min = maxRule;
                minKicked = nKickedRules;
                bestOpt = opt;
                bestBit = bit;
            }
        }

        vector<int> breakOpt(maxBits, -1);
        if (bestOpt == breakOpt) {
            node->isLeaf = true;
            while (!node->classifier.empty() && node->nrules > binth) {
                kickedRules.push_back(node->classifier.back());
                node->classifier.pop_back();
                node->nrules--;
            }
            Maxpri.back() = max(node->classifier[0].priority, Maxpri.back());
            for (const Rule &rule : node->classifier) {
                rootPri.back().push(rule.priority);
            }
            continue;
        }

        node->opt = bestOpt;
        node->bit = bestBit;

        vector<vector<Rule> > childRule(1 << maxBits);
        for (const Rule &rule : node->classifier) {
            vector<int> loc = {0};
            bool kickFlag = false;
            for (int i = 0; i < maxBits; i++) {

                if (bestOpt[i] == -1 || bestBit[i] == -1) {
                    continue;
                }
                for(auto &iter : loc) {
                    iter <<= 1;
                }

                switch(rule.Getbit(bestOpt[i], bestBit[i])) {
                    case -1: {
                        int locSize = int(loc.size());
                        for(int locIndex = 0; locIndex < locSize; locIndex ++) {
                            loc.push_back(loc[locIndex] + 1);
                        }
                        break;
                    }
                    case 0: {
                        break;
                    }
                    case 1: {
                        for(auto &iter : loc) {
                            iter ++;
                        }
                        break;
                    }
                    default: {
                        cout<<"Wrong in CreateSubKickTree(211)"<<endl;
                        exit(-1);
                    }
                }
            }
            if ((strategyFlag == 1 && loc.size() > 2) || (strategyFlag == 0 && loc.size() > 1)) {
                kickedRules.push_back(rule);
                break;
            } else {
                for(auto iter : loc) {
                    childRule[iter].push_back(rule);
                }
            }
        }


        vector<int> subNodeLeft = node->left;
        for (int i = 0; i < maxBits; i++) {
            if (bestOpt[i] == -1) {
                continue;
            }
            subNodeLeft[bestOpt[i]] = bestBit[i];
        }

        node->children.resize((1 << maxBits), nullptr);
        for (int i = 0; i < childRule.size(); i++) {
            if (!childRule[i].empty()) {
                node->children[i] = new KickTreeNode(childRule[i], node->depth + 1, false);
                node->children[i]->left = subNodeLeft;
                node->children[i]->parent = node;
                que.push(node->children[i]);
            }
        }
        if (nNode == 0) {
            nNode = int(que.size());
        }
    }
    return root;
}

vector<int> KickTree::GetSelectBit(KickTreeNode *node, vector<int> &opt) {
    vector<int> left = node->left;
    vector<int> bit;
    for (int i : opt) {
        if (i == -1) {
            bit.push_back(-1);
            continue;
        }
        while (true) {
            bool oneFlag = false, zeroFlag = false, wildcardFlag = true;
            int field = i, bitIndex = left[i];
            for (const Rule &rule : node->classifier) {
                int t = rule.Getbit(field, bitIndex);
                if (t == -1) {
                    continue;
                } else {
                    wildcardFlag = false;
                    if (t == 1) {
                        oneFlag = true;
                    } else {
                        zeroFlag = true;
                    }
                }
                if (oneFlag && zeroFlag) {
                    break;
                }
            }
            if (oneFlag && zeroFlag) {
                break;
            }
            if (wildcardFlag) {
                opt[i] = -1;
                break;
            }
            left[i]++;
        }
        bit.push_back(left[i]++);
    }
    return bit;
}

int KickTree::ClassifyAPacketInKickTree(KickTreeNode *root, const Packet &p) const {
    KickTreeNode *node = root;
    vector<int> maxMask = {31, 31, 15, 15, 7};
    while (node && !node->isLeaf) {
        int loc = 0;
        for (int i = 0; i < maxBits; i++) {
            loc <<= 1;
            if (node->opt[i] == -1) {
                continue;
            }
            if (p[node->opt[i]] & (1 << (maxMask[node->opt[i]] - node->bit[i]))) {
                loc++;
            }
        }
        node = node->children[loc];
    }
    if (node && node->isLeaf) {
        for (const Rule &rule : node->classifier) {
            if (rule.MatchesPacket(p)) {
                return rule.priority;
            }
        }
    }
    return -1;
}

bool KickTree::DeleteInSubKickTree(KickTreeNode *root, const Rule &delete_rule) {
    KickTreeNode *node = root;
    vector<int> maxMask = {31, 31, 15, 15, 7};
    while (node && !node->isLeaf) {
        int loc = 0;
        for (int i = 0; i < maxBits; i++) {
            loc <<= 1;
            int field = node->opt[i], bit = node->bit[i];
            if (field == -1) {
                continue;
            }
            switch (delete_rule.Getbit(field, bit)) {
                case -1:
                    return false;
                case 0:
                    break;
                case 1:
                    loc++;
                    break;
                default:
                    cout << "Worng: getbit in delete rule" << endl;
                    exit(-3);
            }
        }
        node = node->children[loc];
    }
    if (node && node->isLeaf) {
        auto iter = lower_bound(node->classifier.begin(), node->classifier.end(), delete_rule);
        if (iter == node->classifier.end()) {
            return false;
        }
        node->classifier.erase(iter);
        node->nrules--;
        return true;
    }
    return false;
}

bool KickTree::InsertInSubKickTree(KickTreeNode *root, const Rule &insert_rule) {
    KickTreeNode *node = root;

    while (node && !node->isLeaf) {
        int loc = 0;
        for (int i = 0; i < maxBits; i++) {
            loc <<= 1;
            if (node->opt[i] == -1) {
                continue;
            }
            int field = node->opt[i], bit = node->bit[i];
            if (field == -1) {
                continue;
            }
            switch (insert_rule.Getbit(field, bit)) {
                case -1:
                    return false;
                case 0:
                    break;
                case 1:
                    loc += 1;
                    break;
                default:
                    cout << "Worng: getbit in insert rule" << endl;
                    exit(-3);
            }
        }
        if (!node->children[loc]) {
            vector<Rule> newTreeRule = {insert_rule};
            node->children[loc] = new KickTreeNode(newTreeRule, node->depth + 1, true);
            return true;
        } else {
            node = node->children[loc];
        }
    }

    if (node && node->isLeaf) {
        if (node->nrules < binth) {
            auto iter = lower_bound(node->classifier.begin(), node->classifier.end(), insert_rule);
            node->classifier.insert(iter, insert_rule);
            node->nrules ++;
            return true;
        } else if(node->depth == maxLevel) {
            return false;
        } else {
            vector<Rule> rules = node->classifier;
            auto iter = lower_bound(rules.begin(), rules.end(), insert_rule);
            rules.insert(iter, insert_rule);
            auto *node_temp = new KickTreeNode(rules, node->depth, false);
            node_temp->left = node->left;

            int Min = int(rules.size()), minKicked = int(rules.size());
            vector<int> bestOpt = partitionOpt[0];
            vector<int> bestBit = GetSelectBit(node_temp, partitionOpt[0]);

            for (auto opt : partitionOpt) {
                vector<int> subnRules((1 << maxBits), 0);
                int nKickedRules = 0;
                vector<int> bit = GetSelectBit(node_temp, opt);
                for (const Rule &rule : node_temp->classifier) {
                    int loc = 0;
                    bool kickFlag = false;
                    for (int i = 0; i < maxBits; i++) {
                        loc <<= 1;
                        if (opt[i] == -1) {
                            continue;
                        }
                        int t = rule.Getbit(opt[i], bit[i]);
                        if (t == -1) {
                            kickFlag = true;
                            break;
                        } else if (t == 1) {
                            loc++;
                        }
                    }
                    if (kickFlag) {
                        nKickedRules++;
                    } else {
                        subnRules[loc]++;
                    }
                }
                if(nKickedRules) {
                    continue;
                }

                int maxRule = 0;
                for (int i : subnRules) {
                    maxRule = max(i + nKickedRules, maxRule);
                }
                if (maxRule < Min || (maxRule == Min && nKickedRules <= minKicked)) {
                    Min = maxRule;
                    minKicked = nKickedRules;
                    bestOpt = opt;
                    bestBit = bit;
                }
            }

            vector<int> breakOpt(maxBits, -1);
            if(bestOpt == breakOpt || minKicked != 0) {
                return false;
            }
            node->opt = bestOpt;
            node->bit = bestBit;
            vector<vector<Rule> > childRule(1 << maxBits);

            for(const Rule &rule : rules) {
                int loc = 0;
                bool kickFlag = false;
                for(int i = 0; i < maxBits; i++) {
                    loc <<= 1;
                    if(bestOpt[i] == -1) {
                        continue;
                    }
                    int t = rule.Getbit(bestOpt[i], bestBit[i]);
                    if(t == -1) {
                        kickFlag = true;
                        break;
                    } else {
                        loc += t;
                    }
                }
                if(kickFlag) {
                    return false;
                } else {
                    childRule[loc].push_back(rule);
                }
            }

            vector<int> subNodeLeft = node->left;
            for(int i = 0; i < maxBits; i++) {
                if(bestOpt[i] == -1) {
                    continue;
                }
                subNodeLeft[bestOpt[i]] = bestBit[i];
            }

            node->isLeaf = false;
            node->children.resize((1 << maxBits), nullptr);
            for(int i = 0; i < childRule.size(); i++) {
                if(!childRule[i].empty()) {
                    node->children[i] = new KickTreeNode(childRule[i], node->depth + 1, true);
                    node->children[i]->left = subNodeLeft;
                    node->children[i]->parent = node;
                }
            }
            return true;
        }
    }

    return false;

}

void KickTree::UpdatePriInKickTree(KickTreeNode *root, int currentUpdatePriTreeIndex) {
    queue<KickTreeNode *> que;
    Maxpri[currentUpdatePriTreeIndex] = -1;
    que.push(root);
    while (!que.empty()) {
        auto node = que.front();
        que.pop();
        if (node->isLeaf) {
            for (const Rule &rule : node->classifier) {
                Maxpri[currentUpdatePriTreeIndex] = max(Maxpri[currentUpdatePriTreeIndex], rule.priority);
            }
            continue;
        }
        for (auto iter : node->children) {
            if (iter) {
                que.push(iter);
            }
        }
    }

}

int KickTree::WorstMemAccess() {
    int WorstAccess = 0;
    for(auto root : roots) {
        queue<KickTreeNode*> que;
        que.push(root);
        while(!que.empty()) {
            KickTreeNode *node = que.front();
            que.pop();
            if(node->isLeaf) {
                WorstAccess = max(WorstAccess, node->depth + node->nrules);
            }
            for(auto iter : node->children) {
                if(iter) {
                    que.push(iter);
                }
            }
        }
    }
    return WorstAccess;
}

int KickTree::WorstMemAccess(const Packet &packet) {
    int memAccess = 0;
    for(auto root : roots) {
        KickTreeNode *node = root;
        vector<int> maxMask = {31, 31, 15, 15, 7};
        while(node && !node->isLeaf) {
            int loc = 0;
            for(int i = 0; i < maxBits; i++) {
                loc <<= 1;
                if(node->opt[i] == -1) {
                    continue;
                }
                if(packet[node->opt[i]] & (1 << (maxMask[node->opt[i]] - node->bit[i]))) {
                    loc ++;
                }
            }
            node = node->children[loc];
        }
        if(node && node->isLeaf) {
            for(int i = 0; i < node->classifier.size(); i++) {
                if(node->classifier[i].MatchesPacket(packet)) {
                    memAccess = max(memAccess, node->depth + i + 1);
                }
            }
        }
    }
    return memAccess;
}

