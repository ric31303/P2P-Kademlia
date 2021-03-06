#include "kad_bucket.hpp"

Node::Node(const char* _ip, const char* _port, const SHA_1 _id){
	strcpy(ip, _ip);
	strcpy(port, _port);
	ID = _id;
}

bool Node::operator == (const Node& _a) const {
	return ID == _a.ID;
}

vector<Node> Node::parse(const char* _data, int _len){
	vector<Node> ret;
	char* end = (char*)_data + _len;
	char* pos = (char*)_data;
	// char* tok = 0;
	int n = 0;
	while(pos < end){
		char _ip[IP_size] = "";
		char _port[PORT_size] = "";
		char _key[64] = "";

		if(!(n = strstr(pos, ":") - pos)) return ret;
		strncpy(_ip, pos, n); 	pos += n+1;
		if(!(n = strstr(pos, ":") - pos)) return ret;
		strncpy(_port, pos, n); 	pos += n+1;
		if(pos < end){
			strncpy(_key, pos, 40);	pos += 40+1;
		}else{
			return ret;
		}
		ret.push_back(Node(_ip, _port, SHA_1(SHA_1::to_hash(_key))));
	}
	return ret;
}

void Node::print(){
	printf("(%s:%s:%s)", ip, port, ID.get());
}

K_Buck::K_Buck(int _k){
	k = _k;
}

vector<Node> K_Buck::get(){ 
	return vector<Node>(nodes.begin(), nodes.end());
}

bool K_Buck::insert(const Node& _node){

	if(nodes.size() <= k){

		auto it = find(nodes.begin(), nodes.end(), _node);
		if(it != nodes.end()){
			// already exists, put it to the back
			nodes.remove(_node);
		}
		nodes.push_back(_node);
		return true;
	}else{
		// bucket is full
		Node lru = nodes.front(); nodes.pop_front();
		// ping lru 
		bool ret = 0;
		RPC* rpc = new RPC(lru.ID, "PING", '0', true);
		// rpc->ret = &ret;
		ret = rpc->request();
		if(ret){
			nodes.push_back(lru);
			delete rpc;
			return false;
		}else{
			nodes.push_back(_node);
			delete rpc;
			return true;
		}
	}
}

void K_Buck::print(const char end){
	printf("[");
	for(auto it=nodes.begin(); it!=nodes.end(); ++it){
		if(it!=nodes.begin()){
			printf(", ");
		}
		it->print();
	}
	printf("]%c", end);
}

int DHT::distance(const SHA_1& _a, const SHA_1& _b){
	// unsigned char XOR[SHA_DIGEST_LENGTH] = "";
	unsigned char* a = _a.get_hash();
	unsigned char* b = _b.get_hash();

	unsigned char byte = 0;
	int i = 0;
	for(i = 0; i<SHA_DIGEST_LENGTH; ++i){
		// printf("%02x %02x %02x \n", a[i], b[i], byte);
		if((byte = a[i] ^ b[i])){
			unsigned char x = 1;
			for(int j=0; j<8; ++j){
				if(byte & x){
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
	pthread_mutex_init(&lock , NULL);
	buckets = vector<K_Buck>(161, K_Buck(local_k));

	ID = _key;
	Node local(local_ip, local_port, ID);
	insert(local);

	mkdir(shared_folder, 0777);
	mkdir(download_folder, 0777);
	// initilize the list to share
	
	print_file();
	SHA_1 id(stripp(boot_ip, boot_port));
	Node boot(boot_ip, boot_port, id);
	// insert bootstrap node into the DHT
	insert(boot);

}

void DHT::join(){

	// join the network
	SHA_1 id(stripp(boot_ip, boot_port));
	RPC* rpc = new RPC(id, "FIND_NODE", '0', false);
	rpc->ID = ID;
	rpc->request();
	// delete rpc;
}

void DHT::print_file(){
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (shared_folder)) != NULL) {
	  	/* print all the files and directories within directory */
	  	printf("Shared files :\n");
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

void DHT::ls_file(){
	
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (shared_folder)) != NULL) {
	  	/* print all the files and directories within directory */
	  	while ((ent = readdir (dir)) != NULL) {
	  		if(ent->d_type != DT_DIR){
	  			files[string(SHA_1(ent->d_name).get())] = string(ent->d_name);
	  			// printf ("%s\t", ent->d_name);
	  		}
	  	}
	  	printf("\n");
	  	closedir (dir);
	} else {
	  	/* could not open directory */
	  	perror ("Shared folder not found.");
	}

}

bool DHT::contain(const Node& _node){
	if(nodes.count(string(_node.ID.get()))){
		return true;
	}else{
		return false;
	}
}

void DHT::insert(const Node& _node){

	int d = distance(ID, _node.ID);
	pthread_mutex_lock(&lock);
	if(!contain(_node)){
		if(buckets[d].insert(_node)){
			nodes[string(_node.ID.get())] = _node;
			printf("%08.0f [insert] %s --> bucket[%d]\n", time_stamp(), _node.ID.get(), d);
		}
	}
	pthread_mutex_unlock(&lock);
}
Node DHT::get(const SHA_1& _key){
	string k = string(_key.get());
	pthread_mutex_lock(&lock);
	Node ret;
	if(nodes.count(k)){
		ret = nodes[k];
	}
	pthread_mutex_unlock(&lock);
	return ret;
}

vector<Node> DHT::get_node(const SHA_1& _key){

	vector<Node> ret;
	printf("%08.0f [lookup] %s\n", time_stamp(), _key.get());
	string key = string(_key.get());
	pthread_mutex_lock(&lock);
	if(_key == local_id){
		// local id
		ret.push_back(Node(local_ip, local_port, local_id));

	}
	// if(nodes.count(key)){
	// 	// if target is in the list
	// 	ret.push_back(nodes[key]);
	// }
	if(true){
		// return the closest k nodes
		int d = distance(ID, _key);
		int small_d = d-1;
		while(ret.size() < local_k && d <= 160){
			vector<Node> buck = buckets[d].get();
			for(auto& n : buck){
				ret.push_back(n);
				if(ret.size() >= local_k){
					break;
				}
			}
			d++;
		}
		while(ret.size() < local_k && small_d > 0){
			vector<Node> buck = buckets[small_d].get();
			for(auto& n : buck){
				ret.push_back(n);
				if(ret.size() >= local_k){
					break;
				}
			}
			small_d--;
		}
	}
	pthread_mutex_unlock(&lock);
	return ret;
}

string DHT::get_file(const SHA_1& _key){
	ls_file();
	char fname[File_size] = "";
	if(files.count(string(_key.get()))){
		string ret = files[string(_key.get())];
		
		strcpy(fname, shared_folder);
		strcpy(fname + strlen(fname), ret.c_str());

		if(File(fname, 0)){
			printf("%08.0f [look file] %s\n", time_stamp(), ret.c_str());
			return ret;
		}else{
			files.erase(string(_key.get()));
			// return "";
		}
		
	}
	// printf("File %s not found\n", fname);
	return "";
}

void DHT::add_file(const SHA_1& _key, const char* _name){
	if(!files.count(string(_key.get()))){
		files[string(_key.get())] = string(_name);
		printf("%08.0f [add file] %s\n", time_stamp(), _name);
	}
}

void DHT::print_all(){
	printf("\n[\n");
	for(int i=0; i<buckets.size(); i++){
		printf(" %3d:", i);
		buckets[i].print(' ');
		if(i<buckets.size()-1){
			printf(",\n");
		}
	}
	printf("\n]\n");
}

void DHT::print_buck(int _n){
	buckets[_n].print();
}



