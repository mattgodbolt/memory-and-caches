memory: memory.cpp
	$(CXX) -O3 -o memory memory.cpp

OUT_DIR=results/$(shell hostname)

.PHONY: dirs
dirs:
	mkdir -p ${OUT_DIR}
	lscpu > ${OUT_DIR}/lscpu.txt

.PHONY: measure-all
measure-all: measure measure-huge

.PHONY: measure
measure: measure-read measure-write

.PHONY: measure-huge
measure-huge: measure-huge-read measure-huge-write

.PHONY: measure-huge-write
measure-huge-write: dirs memory
	./memory huge random write > ${OUT_DIR}/hp.rmw.rand.csv
	./memory huge sequential write > ${OUT_DIR}/hp.rmw.seq.csv

.PHONY: measure-huge-read
measure-huge-read: dirs memory
	./memory huge random read > ${OUT_DIR}/hp.readonly.rand.csv
	./memory huge sequential read > ${OUT_DIR}/hp.readonly.seq.csv


.PHONY: measure-write
measure-write: dirs memory
	./memory normal sequential write > ${OUT_DIR}/nohp.rmw.seq.csv
	./memory normal random write > ${OUT_DIR}/nohp.rmw.rand.csv

.PHONY: measure-read
measure-read: dirs memory
	./memory normal sequential read > ${OUT_DIR}/nohp.readonly.seq.csv
	./memory normal random read > ${OUT_DIR}/nohp.readonly.rand.csv
