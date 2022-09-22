#include <iostream>
#include <fstream>
#include <string>
//#include "./OVS/TupleSpaceSearch.h"
#include "ElementaryClasses.h"
#include "./HybridTSS/HybridTSS.h"
#include "./CutTSS/CutTSS.h"
#include "./MultiHyperSplit/MultiHyperSplit.h"
using namespace std;

string ruleFile, packetFile;
FILE *fpr, *fpt;
vector<Rule> rules;
vector<Packet> packets;
std::chrono::time_point<std::chrono::steady_clock> Start, End;
std::chrono::duration<double> elapsed_seconds{};
std::chrono::duration<double,std::milli> elapsed_milliseconds{};
vector<int> randUpdate;
int nInsert, nDelete;
// ofstream fout("result.csv", ios::app);
ofstream fError("ErrorLog.csv", ios::app);

void testPerformance(PacketClassifier *p, ofstream &fout) {
    cout << p->funName() << ":" << endl;
    Start = std::chrono::steady_clock::now();
    p->ConstructClassifier(rules);
    End = std::chrono::steady_clock::now();
    elapsed_milliseconds = End - Start;
    cout << "\tConstruction time: " << elapsed_milliseconds.count() <<" ms " << endl;
    fout << elapsed_milliseconds.count() << ",";
    // Classify
//    if (false) {
        printf("\nClassify Performance:\n");
        std::chrono::duration<double> sumTime(0);
        int matchPri = -1, matchMiss = 0;
        int nPacket = int(packets.size());
        int nRules = int(rules.size());
        vector<int> results(nPacket, -1);
        const int trials = 10;
        for (int i = 0; i < trials; i++) {
            Start = std::chrono::steady_clock::now();
            for (int j = 0; j < nPacket; j++) {
                matchPri = p->ClassifyAPacket(packets[j]);
                results[j] = nRules - 1 - matchPri;
            }
            End = std::chrono::steady_clock::now();
            sumTime += End - Start;
            for (int j = 0; j < nPacket; j++) {
                // cout << packets[j][5] << "\t" << results[j] << endl;
                if (results[j] == nRules || packets[j][5] < results[j]) {
                    cout << rules[packets[j][5]].priority << "\t" << results[j] << "\t" << packets[j][5] << endl;
                    matchMiss ++;
                }
            }
        }
        // fprintf("\t%d packets are classified, %d of them are misclassified\n", nPacket * trials, matchMiss);
        // fout << "\t" << nPacket * trials << " packets are classified, " << matchMiss << " of them are misclassified" << endl;
        // fout << "\tTotal classification time: " << sumTime.count() / trials << " s" << endl;
        // fout << "\tAverage classification time: " << sumTime.count() * 1e6 / (trials * nPacket) << " s" << endl;
        // fout << "\tThroughput: " << 1 / (sumTime.count() * 1e6 / (trials * nPacket)) << " Mpps" << endl;
        printf("\t%d packets are classified, %d of them are misclassified\n", nPacket * trials, matchMiss);
        printf("\tTotal classification time: %f s\n", sumTime.count() / trials);
        printf("\tAverage classification time: %f us\n", sumTime.count() * 1e6 / (trials * nPacket));
        printf("\tThroughput: %f Mpps\n", 1 / (sumTime.count() * 1e6 / (trials * nPacket)));
        p->printInfo(fout);
        if (matchMiss != 0) {
            fout << "error" << ",";
        }
        fout << "," << 1 / (sumTime.count() * 1e6 / (trials * nPacket)) << ",," ;
        // fout << trials * nPacket << "," << nRules << endl;


//        fout << 1 / (sumTime.count() * 1e6 / (trials * nPacket)) << ",";


//    }
    // Update
   if (false) {
        // initial rand_update
        printf("\nUpdate Performance:\n");

        if (randUpdate.empty()) {
            nInsert = nDelete = 0;
            for (int ra = 0; ra < rules.size(); ra ++) {
                // 0 for InsertRule & 1 for DeletRule
                int t = rand() % 2;
                randUpdate.push_back(t);
                t ? nDelete ++ : nInsert++;
            }
        }
        Start = std::chrono::steady_clock::now();
        for (int ra = 0; ra < rules.size(); ra++) {
            randUpdate[ra] ? p->DeleteRule(rules[ra]) : p->InsertRule(rules[ra]);
        }
        End = std::chrono::steady_clock::now();
        elapsed_seconds = End - Start;
        int nrules = static_cast<int>(rules.size());
        printf("\t%d rules update: insert_num = %d delete_num = %d\n", nrules, nInsert, nDelete);
        printf("\tTotal update time: %f s\n", elapsed_seconds.count());
        printf("\tAverage update time: %f us\n", elapsed_seconds.count() * 1e6/nrules);
        printf("\tThroughput: %f Mpps\n", 1/ (elapsed_seconds.count() * 1e6 /nrules));
        printf("-------------------------------\n\n");
//        double MUPS = 1/ (elapsed_seconds.count() * 1e6 / nrules) ;
//        fout << MUPS << ",";


   }
}

int main(int argc, char* argv[]) {
    ofstream fout("result0612.csv", ios::app);
    int rulePri = -1;
    int maxDepth = 8;
    int binth = 8;
    for (int i = 0; i < argc; i++) {
        if (string(argv[i]) == "-r") {
            ruleFile = string(argv[++ i]);
            fpr = fopen(argv[i], "r");
        }
        else if (string(argv[i]) == "-p") {
            packetFile = string(argv[++ i]);
            fpt = fopen(argv[i], "r");
        }
        else if (string(argv[i]) == "-rp") {
            rulePri = atoi(argv[++ i]);
        }
        else if (string(argv[i]) == "-m") {
            maxDepth = atoi(argv[++ i]);
        }
        else if (string(argv[i]) == "-b") {
            binth = atoi(argv[++ i]);
        }
    }
    cout << ruleFile << endl;
    cout << packetFile << endl;
    rules = loadrule(fpr);
    packets = loadpacket(fpt);
    cout << rules.size() << endl;
    cout << packets.size() << endl;

    // ---HybridTSS---Construction---
//    PacketClassifier *HT = new HybridTSS();
    // PacketClassifier *TMO = new TupleMergeOnline(10);
    // testPerformance(TMO, fout);

    // PacketClassifier *TMO20 = new TupleMergeOnline(20);
    // testPerformance(TMO20, fout);

    // PacketClassifier *TMO30 = new TupleMergeOnline(30);
    // testPerformance(TMO30, fout);

    PacketClassifier *TMO40 = new TupleMergeOnline(40);
    testPerformance(TMO40, fout);

    // PacketClassifier *PSTSS = new PriorityTupleSpaceSearch();
    // testPerformance(PSTSS, fout);
    // fout << rules.size() << "," << packets.size() << endl;
    // PacketClassifier *CT = new CutTSS();
    // testPerformance(CT);

    // ---test---
//    cout <<"---------------------" << endl;
    // PacketClassifier *HT = new HybridTSS();
    // testPerformance(HT);
    PacketClassifier *MHS = new MultiHyperSplit(maxDepth, binth);
    testPerformance(MHS, fout);

   fout << endl;

    return 0;
}
