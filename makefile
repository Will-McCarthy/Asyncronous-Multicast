# CSCI 4780 Spring 2021 Project 3
# Jisoo Kim, Will McCarthy, Tyler Scalzo

GCC=g++
FLAGS=-pthread
CLTPROG=participant
SRVPROG=coordinator

$(CLTPROG): participant.o
	$(GCC) -o $(CLTPROG) participant.o ${FLAGS}

$(SRVPROG): coordinator.o
	$(GCC) -o $(SRVPROG) coordinator.o ${FLAGS}

participant.o: participant.cpp
	$(GCC) -c participant.cpp ${FLAGS}

coordinator.o: coordinator.cpp
	$(GCC) -c coordinator.cpp ${FLAGS}

all: $(CLTPROG) $(SRVPROG)

clean:
	rm -rf $(CLTPROG) $(SRVPROG) *.o