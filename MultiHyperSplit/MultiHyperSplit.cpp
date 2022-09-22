# include "MultiHyperSplit.h"

// #define DEBUG


MultiHyperSplit::MultiHyperSplit() {
    maxDepth = 8;
    binth = 8;
}

MultiHyperSplit::MultiHyperSplit(int rP) {
    maxDepth = 8;
    binth = 8;
    rulePri = rP;
}

MultiHyperSplit::MultiHyperSplit(int md, int b) {
    maxDepth = md;
    binth = b;
}

void MultiHyperSplit::ConstructClassifier(const vector<Rule> &rules) {
    vector<Rule> r = rules;
    testRules = r;
    vector<Rule> tmp;
    while (!r.empty()) {
        cout << roots.size() << "#";
        roots.push_back(ConstructHSTree(ref(r), ref(tmp)));
        roots.back()->rules = r;
        roots.back()->nRules = r.size();
        roots.back()->maxPri = r[0].priority;
        r = move(tmp);
        cout << "\t" << roots.back()->nRules << "\t" << r.size() << endl;
        SortRules(r);
    }
    cout << "\troots size:" << roots.size() << endl;
    sort(roots.begin(), roots.end(), [](HSnode *r1, HSnode *r2) {
        return r1->maxPri > r2->maxPri; 
    });
}

int MultiHyperSplit::ClassifyAPacket(const Packet &packet) {
    int matchPri = -1;
    for (HSnode *root : roots) {
        
    #ifdef DEBUG
        if (testRules[packet[5]].priority == rulePri) {
            cout << "next tree" << endl;
        }
    #endif
        if (root->maxPri < matchPri) {
            return matchPri;
            // break;
        }
        HSnode *node = root;
        while (node && !node->isLeaf) {
            if (packet[node->dim] <= node->threshold) {
                
            #ifdef DEBUG
                if (testRules[packet[5]].priority == rulePri) {
                    cout << "left\t" << packet[node->dim] << "\t" << node->threshold << endl;
                }
            #endif
                node = node->children[0];
            } else {
            #ifdef DEBUG
                if (testRules[packet[5]].priority == rulePri) {
                    cout << "right\t" << packet[node->dim] << "\t" << node->threshold << endl;
                }
            #endif
                node = node->children[1];
            }
        }
        // if (!node || node->maxPri < matchPri) {
        //     continue;
        // }
        if (!node) {
            continue;
        }
        for (Rule &r : node->rules) {
            if (r.MatchesPacket(ref(packet))) {
                matchPri = max(matchPri, r.priority);
                break;
            }
        }
    }
    return matchPri;
}

void MultiHyperSplit::DeleteRule(const Rule &rule) {

}

void MultiHyperSplit::InsertRule(const Rule &rule) {

}

Memory MultiHyperSplit::MemSizeBytes() const {
    return 0;
}

int MultiHyperSplit::MemoryAccess() const {

    return 0;
}

size_t MultiHyperSplit::NumTables() const {
    return roots.size();

}

size_t MultiHyperSplit::RulesInTable(size_t tableIndex) const {

    return 0;

}


// Constrcut HyperSplit Tree based rules, return the root node, and put the rest duplicated rules into res
HSnode* MultiHyperSplit::ConstructHSTree(const vector<Rule> &rules, vector<Rule> &res) {
    HSnode *root = new HSnode(ref(rules));
    root->depth = 1;
    queue<HSnode*> que;
    que.push(root);
    while (!que.empty()) {
        HSnode *node = que.front();
        que.pop();
        if (node->nRules <= binth) {
            node->isLeaf = true;
            continue;
        }
        if (node->depth == maxDepth) {
            while (node->rules.size() > binth) {
                res.push_back(node->rules.back());
                node->rules.pop_back();
            }
            node->isLeaf = true;
            continue;
        }
        
        // Select different segmeng point
        vector<vector<uint32_t> > diffSegpt(MAXDIMENSIONS);
        vector<vector<uint32_t> > segPoint(MAXDIMENSIONS, vector<uint32_t>(2 * rules.size(), 0));
        for (int d = 0; d < MAXDIMENSIONS; d ++) {
            
            // Select all segPoint
            for (int i = 0; i < node->rules.size(); i ++) {
                segPoint[d][2 * i] = node->rules[i].range[d][LowDim];
                segPoint[d][2 * i + 1] = node->rules[i].range[d][HighDim]; 
            }
            // cout << "size: " << segPoint[d].size() << endl;
            sort(segPoint[d].begin(), segPoint[d].end());

            // Select distinct segPoint
            diffSegpt[d].push_back(segPoint[d][0]);
            for (int i = 1; i < segPoint[d].size(); i ++) {
                if (segPoint[d][i] != diffSegpt[d].back()) {
                    diffSegpt[d].push_back(segPoint[d][i]);
                    // cout << "back: " << diffSegpt[d].back() << endl;
                }
            }
        }
        
        // Construct Monotonic stack(Maybe not)
        vector<vector<uint32_t> > dp(MAXDIMENSIONS);
        vector<vector<uint32_t> > kickRules(MAXDIMENSIONS);
        for (int d = 0; d < MAXDIMENSIONS; d ++) {
            dp[d].resize(diffSegpt[d].size());
            kickRules[d].resize(diffSegpt[d].size());
        }
        for (const Rule &r : node->rules) {
            for (int d = 0; d < MAXDIMENSIONS; d ++) {
                int lowIndex = lower_bound(diffSegpt[d].begin(), diffSegpt[d].end(),  r.range[d][LowDim]) - diffSegpt[d].begin();
                int highIndex = lower_bound(diffSegpt[d].begin(), diffSegpt[d].end(), r.range[d][HighDim]) - diffSegpt[d].begin();
                if (r.range[d][LowDim] != diffSegpt[d][lowIndex]) {
                    cout << "Low not exits" << endl;
                    cout << "dim: " << d << "\trule:" << r.id << "\t" << r.range[d][LowDim] << "\t" << diffSegpt[d][lowIndex] << endl; 
                    exit(-1);
                }
                if (r.range[d][HighDim] != diffSegpt[d][highIndex]) {
                    cout << "High not exits" << endl;
                    cout << "dim: " << d << "\trule:" << r.id << "\t" << r.range[d][HighDim] << "\t" << diffSegpt[d][highIndex] << endl; 
                    exit(-1);
                }
                dp[d][highIndex] ++;
                kickRules[d][lowIndex] ++;
                kickRules[d][highIndex] --;
            }            
        }


        // Select dim & thre
        uint32_t dim = 0, thre = 0, maxSegment = 0;
        for (int d = 0; d < MAXDIMENSIONS; d ++) {
            int nLeftRules = 0, nRightRules = 0, nKickRules = 0;
            for (int i = 0; i < diffSegpt[d].size(); i ++) {
                nLeftRules += dp[d][i];
                nKickRules += kickRules[d][i];
                nRightRules = node->rules.size() - nLeftRules - nKickRules;
                int t = min(nLeftRules, nRightRules);
                if (t > maxSegment) {
                    maxSegment = t;
                    dim = d;
                    thre = diffSegpt[d][i];
                }
            }
        }

        // Cannot not segmeng ant more
        if (maxSegment == 0) {
            while (node->rules.size() > binth) {
                res.push_back(node->rules.back());
                node->rules.pop_back();
            }
            node->nRules = node->rules.size();
            node->isLeaf = true;
            continue;
        }
        
        // Split into children
        node->dim = dim;
        node->threshold = thre;

        vector<Rule> leftRules, rightRules;

        int nKicked = 0;
        for (Rule &r : node->rules) {
            if (r.range[dim][LowDim] <= thre && r.range[dim][HighDim] > thre) {
                
            #ifdef DEBUG
                if (r.priority == rulePri) {
                    cout << "kick to next" << endl;
                }
            #endif
                res.push_back(r);
                nKicked ++;
            } else if (r.range[dim][HighDim] <= thre) {
                
            #ifdef DEBUG
                if (r.priority == rulePri) {
                    cout << "left\t" << r.range[dim][LowDim] << "\t" << r.range[dim][HighDim] << "\t" << thre << endl;
                }
            #endif
                leftRules.push_back(r);
            } else {
            #ifdef DEBUG
                if (r.priority == rulePri) {
                    cout << "right\t" << r.range[dim][LowDim] << "\t" << r.range[dim][HighDim] << "\t" << thre << endl;
                }
            #endif
                rightRules.push_back(r);
            }
        }
        // cout << "left: " << leftRules.size() << "\tright: " << rightRules.size() << "\tmaxSeg: " << maxSegment << "\tkicked: " << nKicked  << endl;
        node->rules.clear();
        if (!leftRules.empty()) {
            node->children[0] = new HSnode(move(leftRules));
            node->children[0]->depth = node->depth + 1;
            que.push(node->children[0]);
        }

        if (!rightRules.empty()) {
            node->children[1] = new HSnode(move(rightRules));
            node->children[1]->depth = node->depth + 1;
            que.push(node->children[1]);
        }
    }
    return root;
}