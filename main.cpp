// Author: Yu-Cheng Chen, Yu-Chan Huang
// Date: 5/19/2020
// Contact: ychen22@scu.edu
// CEON 317 Distributed System
// P2P-Kademlia

#include "kad_util.hpp"
#include "kad_bucket.hpp"

// ==========================================================================================
// global objects
// ==========================================================================================
RPC_Manager rpc_mng;
DHT dht;
Server_socket server;


void cmd_handle(const char* _cmd);

int main(int argc, char* argv[]){
	
	//get command line arguments
	for(int i=1; i<argc; i++){
		string arg = string(argv[i]);
		if(arg == "-h" || arg == "-help"){
			help();
			return 0;
		}
		if(arg == "-c" || arg == "-config"){
			if(i + 1 < argc){
				strcpy(config_file, argv[++i]);
			}else{ 
				std::cerr << "-config option requires one argument." << std::endl;
				return 1;
			}  
		}
	}
	printf("==================================================\n");
	printf("            Welcome to P2P-Kademlia\n");
	printf("==================================================\n");
	//print GMT time
	char gtime[30] = "";
	print_time(gtime);
	printf("%s\n", gtime);
	
	// get config file content
	if(!get_config(config_file)){
		return 1;
	}

	// create udp server
	pthread_t server_ID = 0;
	pthread_create(&server_ID, NULL, serverThread, NULL);
	usleep(100000);

	// printf("-----------------------------------\n");
	printf("Waiting for command:\n");
	char cmd[1024] = "";
	
	while(RUNNING){
		cin.getline(cmd, 1024);
		if(!strcmp(cmd, "exit")){
			RUNNING = false;
			// pthread_join(server_ID, NULL);
			break;
		}else{
			cmd_handle(cmd);
		}
		usleep(500);
	}

	printf("\n\ndone.\n");
	return 0;
}

void cmd_handle(const char* _cmd){
	char* pos = (char*)_cmd;
	char* tok = 0;
	int n = 0;
	tok = strstr(pos, " ");
	if(!tok){
		if(!strcmp(_cmd, "ls")){
			dht.ls_file();
		}else{

		}
	}else{
		n = tok - pos;
		char cmd[1024] = "";
		strncpy(cmd, pos, n); 	pos += n+1;
		if(!strcmp(cmd, "ping")){
			// SHA_1 id(pos);
			RPC* node = new RPC(pos, "PING", '0');
			node->request();
		}
	}
}






