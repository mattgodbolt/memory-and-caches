#!/usr/bin/env python

import os, sys, numpy, pylab

from matplotlib.dates import datestr2num

def load(fname):
    dt = [('log2ElemSize', numpy.uint64), ('log2WorkingSet', numpy.uint64), ('numCycles', numpy.uint64), ('cyclesPerOp', numpy.float)]
    return numpy.loadtxt(open(fname, "rb"), delimiter=",", dtype=dt)

def main():
    print sys.argv
    data1 = load(sys.argv[1])
    data1Name = sys.argv[2]
    data2 = load(sys.argv[3])
    data2Name = sys.argv[4]
    elemSize = int(sys.argv[5])
    title = data1Name + " vs " + data2Name + " log2ElemSize=%d" % elemSize
    legend = []
    for data, name in ((data1, data1Name), (data2, data2Name)):
        elemData = data[data['log2ElemSize'] == elemSize]
        pylab.plot(elemData['log2WorkingSet'], elemData['cyclesPerOp'], linestyle='-', marker='x')
        legend.append(name)
    pylab.title(title)
    pylab.xlabel("log2 working set size")
    pylab.ylabel("cycles per operation")
    pylab.grid(True)
    pylab.legend(legend, shadow=True, fancybox=True, loc='upper left')
    fig = pylab.gcf()
    fig.set_size_inches(12, 8)
    fig.savefig("myfig.svg", dpi=300)
    pylab.show()

if __name__ == "__main__":
    main()
