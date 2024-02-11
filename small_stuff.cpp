#include <filesystem>
#include <regex>
//#include <cctype>
//#include <string>
//#include <iostream>
#include <fstream> //For file reading and writing
//#include <sstream> //For string split function

#include <libreborn/libreborn.h>
#include <symbols/minecraft.h>
#include <mods/misc/misc.h>

#include "other_sissystuff.h"

namespace fs = std::filesystem;
using namespace std; //so that we don't need that darn std:: in front of everything!

std::unordered_map<std::string, std::string> invKeyDict = {};	// Umap holding server name and client_id key

// Check if path+dir exists
bool existsDir(const std::string& thePath) {
    fs::path directory(thePath);
    return fs::exists(directory) && fs::is_directory(directory);
}



// Function for creating the dirs we need for storage
void createDir(const std::string& thePath) {
    fs::path dir(thePath);
    fs::create_directories(dir);
}





// Nice split_str function
vector<string> split_str(string str, char theChar) {
    stringstream ss(str);
    vector<string> ret;
    string tmp = "";
    while (getline(ss, tmp, theChar)) {
        ret.push_back(tmp);
    }
    return ret;
}



// Function to make unix timestamp in ms since epoch.
uint64_t get_ms_unixTS(){
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


/* maybe needed.. hm
void strip(std::string& str) {
  while (!str.empty() && str.back() == '\n') {
    str.pop_back();
  }
}
*/

// Check if a string contains only valid b64 chars
bool isValidBase64(const std::string& input) {
    std::regex base64_regex("[A-Za-z0-9+/]*={0,2}");

    return std::regex_match(input, base64_regex);
}



// Check if a string contains only chars in given charset
bool all_allowed_chars(const std::string& str) {
    std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    return str.find_first_not_of(charset) == std::string::npos;
}


// For saving string to file, named after the php-function
void file_put_contents(std::string thePath, std::string saveStr){
    ofstream outfile;
    outfile.open(thePath, std::ios_base::trunc); // 'app' for append, 'trunc' overwrite
    outfile << saveStr;
}



// For reading the contents of a file, named after the php-function
std::string file_get_contents(std::string file){
    std::ifstream input_file( file );
	std::string resultStr = "";
    if ( ( !input_file.good() ) || ( !input_file.is_open() )  ) { 
        ERR("Error with file %s", file.c_str());
    } else { 
        std::string theLine;
        while (std::getline(input_file, theLine)){
			resultStr += theLine + '\n';
		}
   }
    return resultStr; 
}



// Function that returns a random-ish string of given len.
std::string randomStr(int length) {// Function to generate somewhat random alphanumeric string of requested length.
    static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::string result;
    result.resize(length);

    srand(time(NULL));
    for (int i = 0; i < length; i++) result[i] = charset[rand() % charset.length()];
    return result;
}


// Function for printing some debug info directly to game screen
void showInfo(string text){ 
    unsigned char *gui = get_minecraft() + Minecraft_gui_property_offset;
    misc_add_message(gui, text.c_str() );
}




// Function that determines if on external server or not
bool on_ext_server() {
    if( self_is_server() ) return false;
    bool on_ext_srv = *(bool *) (*(unsigned char **) (get_minecraft() + Minecraft_level_property_offset) + 0x11);
    return on_ext_srv;
}



// Function needed for self_is_server() 
bool got_raknetInstance(){
    return ( *(unsigned char **) (get_minecraft() + Minecraft_rak_net_instance_property_offset) != NULL ) ? true : false;
}



// Function to check if game instance is currently acting as server
bool self_is_server(){
    if(!got_raknetInstance() ) return false; // If got raknetInstance then check RakNetInstance_isServer.
    unsigned char *rak_net_instance = *(unsigned char **) (get_minecraft() + Minecraft_rak_net_instance_property_offset);
    unsigned char *rak_net_instance_vtable = *(unsigned char**) rak_net_instance;
    RakNetInstance_isServer_t RakNetInstance_isServer = *(RakNetInstance_isServer_t *) (rak_net_instance_vtable + RakNetInstance_isServer_vtable_offset);
    return ((*RakNetInstance_isServer)(rak_net_instance)) ? true : false;
}



// Function for sanitizing inventory data received from clients
bool isInvString(std::string inv_str){
    vector<string> lines = split_str(inv_str, '\n');
    if(lines.size() != 36) return false;
    for(int i = 0; i < lines.size(); i++){
        vector<string> data = split_str(lines[i], ':');// split by : to get separate values
        if( data.size() != 3) return false;
		
		for(int i2 = 0; i2 < data.size(); i2++){
			if( !isNumeric(data[i2]) ) return false;
		}
    }
    return true;
}



// Function to check if a string is digits or not
bool isNumeric(const std::string& str) {
  const std::regex pattern("^[0-9]+$");
  return std::regex_match(str, pattern);
}



// Function to make sure all needed dirs for the mod are there
void sissyDirCheck(){
    std::string homePath(home_get());
    
    std::string modPath = homePath;
    modPath.append("/mcpisissy/");
    // Make sure the dir exists
    if(!existsDir(modPath) ) createDir(modPath);
    
    // Make sure we also got receive-dir, fallback saving dir, and keyfile-dir
    std::string recvPath = homePath;
    recvPath.append("/mcpisissy/received_as_server/");    
    if(!existsDir(recvPath) ) createDir(recvPath);
	
    std::string fallbackPath = homePath;
    fallbackPath.append("/mcpisissy/fallback_local_saves/");    
    if(!existsDir(fallbackPath) ) createDir(fallbackPath);
	
	std::string keyfilePath = homePath;
    keyfilePath.append("/mcpisissy/keyfiles/");    
    if(!existsDir(keyfilePath) ) createDir(keyfilePath);
    
}



// Function that reads or creates new ID for given serverName and returns it
std::string get_inv_id(std::string serverName){
	if(serverName == "") ERR("Maybe you are using an older libextrapi.so? Get the newest!");
	
    std::string homePath(home_get());
    std::string returnStr = "";
    std::string file = homePath;
    // Open conf file
    file.append("/mcpisissy/keyfiles/" + serverName + ".conf");
    std::ifstream conf_file(file);

    if (!conf_file.good()) { // Check it, if not exists it will be written
        std::string random_id = randomStr(25);
        std::ofstream file_output(file);
        file_output << "# DO NOT CHANGE THIS!\n";
        file_output << "inv_id=" << random_id << "\n";
        //More conf lines can be added here ^
        file_output.close();
        // Re-Open File
        conf_file = std::ifstream(file);
    }

    // Check that conf file is open
    if (!conf_file.is_open()) {
        ERR("Unable To Open %s", file.c_str());
    }
    // Load data from conf file
    std::string theLine;
    bool conf_OK = false;
    while (std::getline(conf_file, theLine)){
        bool commentLine = ((theLine.rfind("#", 0) == 0)||(theLine.rfind(" ", 0) == 0) ||(theLine.rfind("\n", 0) == 0) ) ? true : false;
        if( theLine.length() > 1 && !commentLine){
            std::vector<std::string> data = split_str(theLine, '=');
            if(data[0] == "inv_id"){
                if( data[1].length() == 25){
                    returnStr = data[1];
                    conf_OK = true;
                }
            } 
        }
    }
    // Close conf file
    conf_file.close();
    if(!conf_OK) {
		ERR("You made errors in conf, pls delete your file '%s'", file.c_str());
	} else return returnStr;
}



// For filename-sanitizing server name, make it safe for filename
std::string serverFilename(const std::string& str) {
	std::string retStr = "";
	for (char c : str) {
		if ( ( c == '/') || (!isalnum(c) && c != '_' && c != '.') ) {
			retStr += '_';
		} else {
			retStr += c;
		}
	}
	return retStr;
}
