#include "kad_bucket.hpp"

Node::Node(const char* _ip, const char* _port, const SHA_1 _id){
	strcpy(ip, _ip);
	strcpy(port, _port);
	ID = _id;
}

bool Node::operator == (const Node& _a) const {
	return ID == _a.ID;
}

K_Buck::K_Buck(int _k){
	k = _k;
}

vector<Node> K_Buck::get(){ 
	return vector<Node>(list.begin(), list.end());
}

bool K_Buck::insert(const Node& _node){

	if(list.size() <= k){

		auto it = find(list.begin(), list.end(), _node);
		if(it != list.end()){
			// already exists, put it to the back
			list.remove(_node);
		}
		list.push_back(_node);
		return true;
	}else{
		// bucket is full
		Node lru = list.front(); list.pop_front();
		// ping lru 
		return false;
	}
}

int DHT::distance(const SHA_1& _a, const SHA_1& _b){
	// unsigned char XOR[SHA_DIGEST_LENGTH] = "";
	unsigned char* a = _a.get_hash();
	unsigned char* b = _b.get_hash();

	unsigned char byte = 0;
	int i = 0;
	for(i = 0; i<SHA_DIGEST_LENGTH; ++i){
		if((byte = a[i] ^ b[i])){
			unsigned char x = 1;
			for(int j=0; j<8; ++j){
				if(byte ^ x){
					return (SHA_DIGEST_LENGTH - i)*8 - j;
				}
				x <<= 1;
			}
		}
	}
	return 0;
}

int DHT::distance(const SHA_1& _key){

	unsigned char XOR[SHA_DIGEST_LENGTH] = "";
	unsigned char* a = ID.get_hash();
	unsigned char* b = _key.get_hash();

	unsigned char byte = 0;
	int i = 0;
	for(i = 0; i<SHA_DIGEST_LENGTH; ++i){
		if((byte = a[i] ^ b[i])){
			unsigned char x = 1;
			for(int j=0; j<8; ++j){
				if(byte ^ x){
					return (SHA_DIGEST_LENGTH - i)*8 - j;
				}
				x <<= 1;
			}
		}
	}
	return 0;
}

DHT::DHT(const SHA_1& _key){
	// initialize the DHT
	ID = _key;
	buckets = vector<K_Buck>(161, K_Buck(local_k));

	mkdir(shared_folder, 0777);
	mkdir(download_folder, 0777);
	// initilize the list to share
	ls_file();
	SHA_1 id(stripp(boot_ip, boot_port));
	Node boot(boot_ip, boot_port, id);
	// insert bootstrap node into the DHT
	insert(boot);

}

void DHT::join(){

	// join the network
	SHA_1 id(stripp(boot_ip, boot_port));
	RPC* rpc = new RPC(id, "FIND_NODE", '0');
	rpc->ID = ID;
	rpc->request();
}

void DHT::ls_file(){
	printf("Shared files :\n");
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (shared_folder)) != NULL) {
	  	/* print all the files and directories within directory */
	  	while ((ent = readdir (dir)) != NULL) {
	  		if(ent->d_type != DT_DIR){
	  			printf ("%s\t", ent->d_name);
	  		}
	  	}
	  	printf("\n");
	  	closedir (dir);
	} else {
	  	/* could not open directory */
	  	perror ("Shared folder not found.");
	}

}

void DHT::insert(const Node& _node){

	int d = distance(ID, _node.ID);
	if(buckets[d].insert(_node)){
		nodes[string(_node.ID.get())] = _node;
		printf("[insert] %s --> bucket[%d]\n", _node.ID.get(), d);
	}
}

vector<Node> DHT::get_node(const SHA_1& _key){
	vector<Node> list;
	printf("[lookup] %s\n", _key.get());
	string key = string(_key.get());
	if(_key == local_id){
		// local id
		list.push_back(Node(local_ip, local_port, local_id));
	}else if(nodes.count(key)){
		// if target is in the list
		list.push_back(nodes[key]);
	}else{
		// return the closest k nodes
		int d = distance(ID, _key);
		while(list.size() < local_k && d <= 160){
			vector<Node> buck = buckets[d].get();
			for(auto& n : buck){
				list.push_back(n);
				if(list.size() >= local_k){
					break;
				}
			}
			d++;
		}
	}

	return list;
}

string DHT::get_file(const SHA_1& _key){
	return "";
}







