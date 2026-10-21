CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic -pthread -std=c11 -g
LDFLAGS ?= -pthread -lm

BIN := bin
RES := results
SRC := src
RUNS := 5

.PHONY: help all demo demo-mandelbrot demo-nbody demo-rwlock bench fmt fmt-check docker-demo clean

all: $(BIN)/mandelbrot $(BIN)/nbody $(BIN)/llist_pthread $(BIN)/llist_custom

$(BIN)/mandelbrot: $(SRC)/mandelbrot.c $(SRC)/main_mandelbrot.c $(SRC)/mandelbrot.h
	$(CC) $(CFLAGS) -I$(SRC) -o $@ $(SRC)/mandelbrot.c $(SRC)/main_mandelbrot.c $(LDFLAGS)

$(BIN)/nbody: $(SRC)/nbody.c $(SRC)/main_nbody.c $(SRC)/nbody.h
	$(CC) $(CFLAGS) -I$(SRC) -o $@ $(SRC)/nbody.c $(SRC)/main_nbody.c $(LDFLAGS)

$(BIN)/llist_pthread: $(SRC)/llist.c $(SRC)/main_llist.c $(SRC)/my_rwlock.c $(SRC)/llist.h $(SRC)/my_rwlock.h
	$(CC) $(CFLAGS) -I$(SRC) -DUSE_PTHREAD_RWLOCK -o $@ $(SRC)/llist.c $(SRC)/main_llist.c $(SRC)/my_rwlock.c $(LDFLAGS)

$(BIN)/llist_custom: $(SRC)/llist.c $(SRC)/main_llist.c $(SRC)/my_rwlock.c $(SRC)/llist.h $(SRC)/my_rwlock.h
	$(CC) $(CFLAGS) -I$(SRC) -o $@ $(SRC)/llist.c $(SRC)/main_llist.c $(SRC)/my_rwlock.c $(LDFLAGS)

demo-mandelbrot: $(BIN)/mandelbrot
	@mkdir -p $(RES)
	./$(BIN)/mandelbrot 4 10000
	@wc -l $(RES)/mandelbrot.csv
	@echo "-> $(RES)/mandelbrot.csv"

demo-nbody: $(BIN)/nbody
	@mkdir -p $(RES)
	./$(BIN)/nbody 1.0 data/bodies.txt
	@wc -l $(RES)/nbody.csv
	@echo "-> $(RES)/nbody.csv"

demo-rwlock: $(BIN)/llist_pthread $(BIN)/llist_custom
	@mkdir -p $(RES)
	@echo "--- pthread rwlock ---"
	./$(BIN)/llist_pthread 4 1 0.8
	@echo "--- custom rwlock ---"
	./$(BIN)/llist_custom 4 1 0.8

demo: demo-mandelbrot demo-nbody demo-rwlock

bench: all
	@mkdir -p $(RES)
	@echo "task,threads,unit,min,mean" > $(RES)/bench.csv
	@for t in 1 2 4 8; do \
		printf "mandelbrot,%s,ms," $$t >> $(RES)/bench.csv; \
		for i in $$(seq $(RUNS)); do \
			s=$$(date +%s%3N); ./$(BIN)/mandelbrot $$t 200000 >/dev/null 2>&1; e=$$(date +%s%3N); echo $$((e-s)); \
		done | awk 'BEGIN{min=1e18} {if($$1<min)min=$$1; sum+=$$1} END{printf "%d,%.1f\n", min, sum/NR}' >> $(RES)/bench.csv; \
	done
	@for t in 1 2 4 8; do \
		$(CC) $(CFLAGS) -I$(SRC) -DNBODY_NTHREADS=$$t -o $(BIN)/nbody_t$$t $(SRC)/nbody.c $(SRC)/main_nbody.c $(LDFLAGS); \
		printf "nbody,%s,ms," $$t >> $(RES)/bench.csv; \
		for i in $$(seq $(RUNS)); do \
			s=$$(date +%s%3N); ./$(BIN)/nbody_t$$t 1.0 data/bodies300.txt >/dev/null 2>&1; e=$$(date +%s%3N); echo $$((e-s)); \
		done | awk 'BEGIN{min=1e18} {if($$1<min)min=$$1; sum+=$$1} END{printf "%d,%.1f\n", min, sum/NR}' >> $(RES)/bench.csv; \
		rm -f $(BIN)/nbody_t$$t; \
	done
	@for t in 1 2 4 8; do \
		for impl in pthread custom; do \
			printf "llist_%s,%s,ops/s," $$impl $$t >> $(RES)/bench.csv; \
			for i in $$(seq $(RUNS)); do \
				./$(BIN)/llist_$$impl $$t 2 0.8 | awk '{printf "%.1f\n", $$1/$$4}'; \
			done | awk 'BEGIN{min=1e18} {if($$1<min)min=$$1; sum+=$$1} END{printf "%.0f,%.0f\n", min, sum/NR}' >> $(RES)/bench.csv; \
		done; \
	done
	@cat $(RES)/bench.csv

fmt:
	clang-format -i $(SRC)/*.c $(SRC)/*.h

fmt-check:
	clang-format --dry-run -Werror $(SRC)/*.c $(SRC)/*.h

docker-demo:
	docker build -t pthreads-lab1 .
	docker run --rm -v $(PWD)/$(RES):/app/$(RES) pthreads-lab1 make demo

clean:
	rm -f $(BIN)/* $(RES)/* core core.* $(SRC)/*.o
