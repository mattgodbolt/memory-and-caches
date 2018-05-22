#!/usr/bin/env python

import os, sys, numpy, pylab

from matplotlib.dates import datestr2num

def load(fname):
    #dt = numpy.dtype({'names': ('log2ElemSize', 'log2WorkingSet', 'numCycles', 'cyclesPerOp')})
    dt = [('log2ElemSize', numpy.uint64), ('log2WorkingSet', numpy.uint64), ('numCycles', numpy.uint64), ('cyclesPerOp', numpy.float)]
    return numpy.loadtxt(open(fname, "rb"), delimiter=",", dtype=dt)

def main():
    data = load(sys.argv[1])
    title = "untitled"
    if len(sys.argv) > 2: title = sys.argv[2]
    legend = []
    for elemSize in numpy.unique(data['log2ElemSize']):
        elemData = data[data['log2ElemSize'] == elemSize]
        pylab.plot(elemData['log2WorkingSet'], elemData['cyclesPerOp'], linestyle='-')
        legend.append("%db" % (1<<int(elemSize)))
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
