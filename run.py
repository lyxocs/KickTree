import os
import sys


ruleType = ['acl','ipc','fw']
ruleName = [str(i + 1) + '_' for i in range(5)]
ruleSize = ['1k', '10k', '100k']
dir = './../ruleslwj/'

binth = range(4, 11)
depth = range(4, 11)
bits = range(2, 6)


tot = 0
for bit in bits:
    for b in binth:
        for d in depth:
            print()
            print()
            print()
            print('bit: {}    binth: {}    maxDepth: {}'.format(bit, b, d))
            for t in ruleType:
                for s in ruleSize:
                    for r in ruleName:
                        rule2 = t + r + s
                        dir_file = os.path.join(dir, rule2)

                        if not os.path.isfile(dir_file):
                            print('Not exist')
                            continue


                        outputFile = './Result/KickTree' + '_b' + str(b) + '_bit' + str(bit) + '_l' + str(d) + '.csv'
                        order = './main -r ' + dir + rule2 +' -p ' + dir + rule2 + '_trace -b ' + str(b) + ' -bit ' + str(bit) + ' -t 128 ' + '-l ' + str(d)
                        print(order)
                        os.system(order)
                        tot += 1