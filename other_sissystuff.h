#ifdef __cplusplus
	#include <unordered_map>
	#include <thread> //For the timers/bgThreads

	typedef unsigned char uchar;
	//Stuff for the manipulating of inventory data
	typedef unsigned char *(*Inventory_t)(unsigned char *inventory, unsigned char *player, uint32_t is_creative);
	static Inventory_t Inventory = (Inventory_t) 0x8e768;


	// CANNOT BE STATIC, SHARED BETWEEN .CPP FILES
	unsigned char *get_mcObj();
	void parse_inventory_packet(std::string packetStr, uchar *guid, uchar *callback);
	std::string get_inv_id(std::string serverName);
	void sissyDirCheck();
	bool all_allowed_chars(const std::string& str);
	bool isValidBase64(const std::string& input);
	bool isInvString(std::string inv_str);
	std::vector<std::string> split_str(std::string str, char theChar);

	static void callback_for_mcpisissy(unsigned char *mcpi);
	static uchar *get_inventory();

	static uint64_t get_ms_unixTS();

	static void showInfo(std::string text);
	static void invTickFunction(unsigned char *mcpi);
	static void load_saved_inventories();
	static void save_loaded_inventories();
	static void inventory_to_b64();
	static void b64_to_inventory(std::string inputStr);
	static void file_put_contents(std::string thePath, std::string saveStr); // mimics the php function :)
	static void createDir(const std::string& thePath);
	static void str2inv(std::string inv_str);
	static void file_to_inventory();
	static void inventory_to_file();


	static bool existsDir(const std::string& thePath);
	static bool self_is_server();
	static bool got_raknetInstance();
	static bool on_ext_server();
	static bool isNumeric(const std::string& str);


	static std::string inv2str();
	static std::string randomStr(int length);
	static std::string file_get_contents(std::string fileName); // mimics the php function :P
	static std::string server_name();


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