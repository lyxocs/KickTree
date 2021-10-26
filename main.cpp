#include <iostream>
#include <stdio.h>
#include <fstream>
#include <cstring>
#include <map>
#include "./KickTree/KickTree.h"
#include "./KickTree/Tools.h"


using namespace std;

FILE *fpr = fopen("./../ipc_1k", "r");           // ruleset file
FILE *fpt = fopen("./../ipc_1k_trace", "r");           //  trace file
string ruleName;
int binth = 4;
int maxTree = 32;
int maxBits = 4;
int maxLevel = 4;

//string fileName = "./ipc_1k";
map<int, int> pri_id;

int rand_update[MAXRULES];

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  loadrule(FILE *fp)
 *  Description:  load rules from rule file
 * =====================================================================================
 */
vector<Rule> loadrule(FILE *fp, std::map<int, int> &pri_id) {
    unsigned int tmp;
    unsigned sip1, sip2, sip3, sip4, smask;
    unsigned dip1, dip2, dip3, dip4, dmask;
    unsigned sport1, sport2;
    unsigned dport1, dport2;
    unsigned protocal, protocol_mask;
    unsigned ht, htmask;
    int number_rule = 0; //number of rules

    std::vector<Rule> rule;

    while (true) {

        Rule r;
        std::array<Point, 2> points{};
        if (fscanf(fp, "@%d.%d.%d.%d/%d\t%d.%d.%d.%d/%d\t%d : %d\t%d : %d\t%x/%x\t%x/%x\n",
                   &sip1, &sip2, &sip3, &sip4, &smask, &dip1, &dip2, &dip3, &dip4, &dmask, &sport1, &sport2,
                   &dport1, &dport2, &protocal, &protocol_mask, &ht, &htmask) != 18)
            break;

        r.prefix_length[0] = smask;
        r.prefix_length[1] = dmask;

        if (smask == 0) {
            points[0] = 0;
            points[1] = 0xFFFFFFFF;
        } else if (smask > 0 && smask <= 8) {
            tmp = sip1 << 24;
            points[0] = tmp;
            points[1] = points[0] + (1 << (32 - smask)) - 1;
        } else if (smask > 8 && smask <= 16) {
            tmp = sip1 << 24;
            tmp += sip2 << 16;
            points[0] = tmp;
            points[1] = points[0] + (1 << (32 - smask)) - 1;
        } else if (smask > 16 && smask <= 24) {
            tmp = sip1 << 24;
            tmp += sip2 << 16;
            tmp += sip3 << 8;
            points[0] = tmp;
            points[1] = points[0] + (1 << (32 - smask)) - 1;
        } else if (smask > 24 && smask <= 32) {
            tmp = sip1 << 24;
            tmp += sip2 << 16;
            tmp += sip3 << 8;
            tmp += sip4;
            points[0] = tmp;
            points[1] = points[0] + (1 << (32 - smask)) - 1;
        } else {
            printf("Src IP length exceeds 32\n");
            exit(-1);
        }
        r.range[0] = points;

        if (dmask == 0) {
            points[0] = 0;
            points[1] = 0xFFFFFFFF;
        } else if (dmask > 0 && dmask <= 8) {
            tmp = dip1 << 24;
            points[0] = tmp;
            points[1] = points[0] + (1 << (32 - dmask)) - 1;
        } else if (dmask > 8 && dmask <= 16) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            points[0] = tmp;
            points[1] = points[0] + (1 << (32 - dmask)) - 1;
        } else if (dmask > 16 && dmask <= 24) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            tmp += dip3 << 8;
            points[0] = tmp;
            points[1] = points[0] + (1 << (32 - dmask)) - 1;
        } else if (dmask > 24 && dmask <= 32) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            tmp += dip3 << 8;
            tmp += dip4;
            points[0] = tmp;
            points[1] = points[0] + (1 << (32 - dmask)) - 1;
        } else {
            printf("Dest IP length exceeds 32\n");
            exit(-1);
        }
        r.range[1] = points;

        points[0] = sport1;
        points[1] = sport2;
        r.range[2] = points;

        points[0] = dport1;
        points[1] = dport2;
        r.range[3] = points;

        for (int i = 15; i >= 0; i--) {
            unsigned int Bit = 1 << i;
            unsigned sp = sport1 ^ sport2;
            if (sp & Bit) {
                break;
            }
            r.prefix_length[2]++;
        }

        for (int i = 15; i >= 0; i--) {
            unsigned int Bit = 1 << i;
            unsigned dp = dport1 ^ dport2;
            if (dp & Bit) {
                break;
            }
            r.prefix_length[3]++;
        }

        if (protocol_mask == 0xFF) {
            points[0] = protocal;
            points[1] = protocal;
        } else if (protocol_mask == 0) {
            points[0] = 0;
            points[1] = 0xFF;
        } else {
            printf("Protocol mask error\n");
            exit(-1);
        }
        r.range[4] = points;
        r.prefix_length[4] = protocol_mask;
        r.id = number_rule;

        rule.push_back(r);
        number_rule++;
    }

    //printf("the number of rules = %d\n", number_rule);
    int max_pri = number_rule - 1;
    for (int i = 0; i < number_rule; i++) {
        rule[i].priority = max_pri - i;
        pri_id[rule[i].priority] = rule[i].id;
        // pri_id.insert(std::pair<int,int>(rule[i].priority,rule[i].id));
        // printf("%u: %u:%u %u:%u %u:%u %u:%u %u:%u %d\n", i,
        //   rule[i].range[0][0], rule[i].range[0][1],
        //   rule[i].range[1][0], rule[i].range[1][1],
        //   rule[i].range[2][0], rule[i].range[2][1],
        //   rule[i].range[3][0], rule[i].range[3][1],
        //   rule[i].range[4][0], rule[i].range[4][1],
        //   rule[i].priority);
    }
    pri_id.insert(std::pair<int, int>(-1, -1));
    return rule;
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  loadpacket(FILE *fp)
 *  Description:  load packets from trace file generated from ClassBench[INFOCOM2005]
 * =====================================================================================
 */
std::vector<Packet> loadpacket(FILE *fp) {
    unsigned int header[MAXDIMENSIONS];
    unsigned int proto_mask, fid;
    int number_pkt = 0; //number of packets
    std::vector<Packet> packets;
    while (true) {
        if (fscanf(fp, "%u %u %d %d %d %u %d\n", &header[0], &header[1], &header[2], &header[3], &header[4],
                   &proto_mask, &fid) == Null)
            break;
        Packet p;
        p.push_back(header[0]);
        p.push_back(header[1]);
        p.push_back(header[2]);
        p.push_back(header[3]);
        p.push_back(header[4]);
        p.push_back(fid);

        packets.push_back(p);
        number_pkt++;
    }

    /*printf("the number of packets = %d\n", number_pkt);
    for(int i=0;i<number_pkt;i++){
        printf("%u: %u %u %u %u %u %u\n", i,
               packets[i][0],
               packets[i][1],
               packets[i][2],
               packets[i][3],
               packets[i][4],
               packets[i][5]);
			   }*/

    return packets;
}

int main(int argc, char *argv[]) {

    for (int idx = 1; idx < argc; idx++) {
        if (strcmp(argv[idx], "-r") == 0) {
            const char *ruleFileName = argv[++idx];
            cout<<ruleFileName<<endl;
            ruleName = ruleFileName;
            fpr = fopen(ruleFileName, "r");
        } else if (strcmp(argv[idx], "-b") == 0) {
            binth = atoi(argv[++idx]);
        } else if (strcmp(argv[idx], "-bit") == 0) {
            maxBits = atoi(argv[++idx]);
        } else if (strcmp(argv[idx], "-t") == 0) {
            maxTree = atoi(argv[++idx]);
        } else if (strcmp(argv[idx], "-l") == 0) {
            maxLevel = atoi(argv[++idx]);
        } else if (strcmp(argv[idx], "-p") == 0) {
            const char *packetFileName = argv[++idx];
            cout<<packetFileName<<endl;
            fpt = fopen(packetFileName, "r");
        } else if (strcmp(argv[idx], "-h") == 0) {
            cout<<"./main [-r ruleFile][-b binth][-bit maxbit][-t maxTreenum][-l maxTreeDepth]"<<endl;
            cout<<"e.g., ./main -r acl_10k -b 4 -bit 4 -t 16 -l 10 "<<endl;
            exit(-2);
        }
    }
    cout << "binth: " << binth << endl;
    cout << "maxBits: " << maxBits << endl;
    cout << "maxTree: " << maxTree << endl;
    cout << "maxLevel: " << maxLevel << endl;
    vector<Rule> rule;
    vector<Packet> packets;
    vector<int> results;
    int number_rule = 0;

    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::duration<double> elapsed_seconds{};
    std::chrono::duration<double, std::milli> elapsed_milliseconds{};

    if (fpr != nullptr) {

        rule = loadrule(fpr, pri_id);
        number_rule = int(rule.size());
        cout<<number_rule<<endl;
//---KickTree---Construction---
        printf("KickTree\n");
        start = std::chrono::steady_clock::now();
        KickTree KT(maxBits, maxLevel, binth, maxTree);
        KT.ConstructClassifier(rule);
        end = std::chrono::steady_clock::now();
        elapsed_milliseconds = end - start;
        printf("\tConstruction time: %f ms\n", elapsed_milliseconds.count());
        printf("\tTotal Memory Size: %d(KB)\n", KT.MemSizeBytes() / 1024);
        packets = loadpacket(fpt);
        int number_pkt = int(packets.size());
        const int trials = 10;
        printf("\tTotal packets (run %d times circularly): %ld\n", trials, packets.size()*trials);
        int match_miss = 0;
        vector<int> matchid(number_pkt, -1);
        int matchPri = -1;

//---KickTree---Classification---

//        cout<<number_rule - KT.ClassifyAPacket(packets[0]) - 1<<endl;
        if(false) {
            printf("KickTree\n");
            std::chrono::duration<double> sum_timeKT(0);
            for(int i = 0; i < trials; i++) {
                start = std::chrono::steady_clock::now();
                for(int j = 0; j < number_pkt; j++) {
//                cout<<j<<endl;
                    matchid[j] = number_rule - 1 - KT.ClassifyAPacket(packets[j]);
                }
                end = std::chrono::steady_clock::now();
                elapsed_seconds = end - start;
                sum_timeKT += elapsed_seconds;
                for(int j = 0; j < number_pkt; j++) {
                    if(matchid[j] == -1 || matchid[j] > packets[j][5]) {
                        match_miss++;
                        cout<<matchid[j]<<"  "<<packets[j][5]<<endl;
                    }
                }
            }
            printf("\t%d packets are classified, %d of them are misclassified\n", number_pkt * trials, match_miss);
            printf("\tTotal classification time: %f s\n", sum_timeKT.count() / trials);
            printf("\tAverage classification time: %f us\n", sum_timeKT.count() * 1e6 / (trials * packets.size()));
            printf("\tThroughput: %f Mpps\n", 1 / (sum_timeKT.count() * 1e6 / (trials * packets.size())));
        }


        if(false) {
            printf("\n**************** Update ****************\n");
            srand((unsigned)time(NULL));
            for(int ra=0;ra<MAXRULES;ra++){ //1000000
                rand_update[ra] = rand()%2; //0:insert 1:delete
            }
            int insert_num = 0, delete_num = 0;
            int number_update = min(number_rule,MAXRULES);
            printf("\tThe number of updated rules = %d\n", number_update);

            printf("KickTree\n");
            insert_num = 0, delete_num = 0;
            start = std::chrono::steady_clock::now();
            for(int ra=0;ra<number_update;ra++){
//                cout<<ra<<"\t"<<rand_update[ra]<<endl;
                if(rand_update[ra] == 0)//0:insert
                {
                    KT.InsertRule(rule[ra]);
                    insert_num++;
                } else{//1:delete

                    KT.DeleteRule(rule[ra]);
                    delete_num++;
                }
            }
            end = std::chrono::steady_clock::now();
            elapsed_seconds = end - start;
            printf("\t%d rules update: insert_num = %d delete_num = %d\n", number_update,insert_num,delete_num);
            printf("\tTotal update time: %f s\n", elapsed_seconds.count());
            printf("\tAverage update time: %f us\n", elapsed_seconds.count()*1e6/number_update);
            printf("\tThroughput: %f Mpps\n", 1/ (elapsed_seconds.count() * 1e6 /number_update));
            fstream fout("Result_" + to_string(maxBits) + ".csv", ios::app);
            fout<<ruleName<<","<<binth<<","<<maxLevel<<","<<elapsed_seconds.count()*1e6/number_update<<","<< 1/ (elapsed_seconds.count() * 1e6 /number_update)<<endl;
            fout.close();

        }
    }



    return 0;
}
