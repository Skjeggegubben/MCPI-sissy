#ifdef __cplusplus
	#include <unordered_map>
	#include <thread> //For the timers/bgThreads

	typedef unsigned char uchar;
	//Stuff for the manipulating of inventory data
	typedef unsigned char *(*Inventory_t)(unsigned char *inventory, unsigned char *player, uint32_t is_creative);
	static Inventory_t Inventory = (Inventory_t) 0x8e768;


	unsigned char *get_minecraft();
	uint64_t get_ms_unixTS();

	void showInfo(std::string text);
	void tickFunction(unsigned char *mcpi);
	//void conf_file_load();
	void load_saved_inventories();
	void save_loaded_inventories();
	void parse_inventory_packet(std::string packetStr, uchar *guid, uchar *callback);
	void inventory_to_b64();
	void b64_to_inventory(std::string inputStr);
	void file_put_contents(std::string thePath, std::string saveStr); // mimics the php function :)
	void createDir(const std::string& thePath);
	void str2inv(std::string inv_str);
	void file_to_inventory();
	void inventory_to_file();
	void sissyDirCheck();

	bool existsDir(const std::string& thePath);
	bool self_is_server();
	bool got_raknetInstance();
	bool on_ext_server();
	bool all_allowed_chars(const std::string& str);
	bool isValidBase64(const std::string& input);
	bool isInvString(std::string inv_str);
	bool isNumeric(const std::string& str);

	std::string serverFilename(const std::string& str);
	std::string get_inv_id(std::string serverName);
	std::string inv2str();
	std::string randomStr(int length);
	std::string file_get_contents(std::string fileName); // mimics the php function :P

	std::vector<std::string> split_str(std::string str, char theChar);

	extern std::unordered_map<std::string, std::string> inventoryDict; 		// The server's umap of received inventories as b64
	extern std::unordered_map<std::string, std::string> inventoryDict_old; 	// Duplicate umap of received inventories as b64, for comparison

	extern std::unordered_map<std::string, struct bgThread> bgThreads;
	class bgThread {
	public:
		template<typename Function>
		void setTimeout(Function function, int delay, std::string inputStr1, std::string inputStr2) {
			std::thread t([=]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(delay));
				function(inputStr1, inputStr2);
				bgThreads.erase(inputStr1); // Yes, deletes itself from the umap once done!
			});
			t.detach();
		}
	};




	class b64class {
	  public:
		// encode and decode copies from https://stackoverflow.com/a/34571089/18254316
		std::string dec(std::string inputStr){    
			std::string ret;
			std::vector<int> T(256,-1);
			for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

			int val=0, valb=-8;
			for (char c : inputStr) {
				if (T[c] == -1) break;
				val = (val << 6) + T[c];
				valb += 6;
				if (valb >= 0) {
					ret.push_back(char((val>>valb)&0xFF));
					valb -= 8;
				}
			}
			return ret;
		}

		std::string enc(std::string inputStr){
			std::string ret;
			int val = 0, valb = -6;
			for (char c : inputStr) {
				val = (val << 8) + c;
				valb += 8;
				while (valb >= 0) {
					ret.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
					valb -= 6;
				}
			}
			if (valb>-6) ret.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
			while (ret.size()%4) ret.push_back('=');
			return ret;
		}
	};
	/*
	  // EXAMPLE USE:
	  b64class b64; // Make the class object first, then use:
	  INFO( b64.enc("BMW") );
	  INFO( b64.dec("dGVzdA==") );
	*/

	extern "C" {
		char *home_get();

		std::string get_server_name();
		bool in_local_world();
	}

#endif