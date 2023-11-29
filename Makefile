server:main.o http_parser.o web_server.o utils.o sort_list_timer.o
	g++ main.o http_parser.o web_server.o utils.o sort_list_timer.o -o server 
main.o:main.cpp
	g++ -c main.cpp -o main.o
http_parser.o:http_parser.cpp
	g++ -c http_parser.cpp -o http_parser.o
web_server.o:web_server.cpp
	g++ -c web_server.cpp -o web_server.o
utils.o:utils.cpp
	g++ -c utils.cpp -o utils.o
sort_list_timer.o:sort_list_timer.cpp
	g++ -c sort_list_timer.cpp -o sort_list_timer.o
.PHONY:clean
clean:
	rm -rf main.o http_parser.o web_server.o server utils.o sort_list_timer.o
