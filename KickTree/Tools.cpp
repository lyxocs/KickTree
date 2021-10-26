//
// Created by lbx on 2021/9/5.
//

#include "Tools.h"

void Tools::LevelTravse(KickTreeNode *root) {
    queue<KickTreeNode*> que;
    que.push(root);
    int nNode = int(que.size());

    vector<int> MaxTuple = {32, 32, 16, 16, 8};
    while(!que.empty()) {
        auto node = que.front();
        que.pop();
        nNode--;

        for(auto iter : node->children) {
            if(iter) {
                que.push(iter);
            }
        }
        if(nNode == 0) {
            nNode = int(que.size());

        }
        if(node->isLeaf) {
            for(int i = 0; i < node->left.size(); i++) {
                cout<<"depth: "<<node->depth<<"\t";
                for(auto iter : node->left) {
                    cout<<iter<<"  ";
                }
                cout<<endl;
                MaxTuple[i] = min(node->left[i], MaxTuple[i]);
            }
        }
    }
    for(auto iter : MaxTuple) {
        cout<<iter<<" ";
    }
    cout<<endl;

}
