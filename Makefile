all: match matchstation

match: match.cc
	g++ -g -Wall -O3 -o match match.cc

matchstation: matchstation.cc
	g++ -g -Wall -O3 -o matchstation matchstation.cc
