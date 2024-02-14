#include <filesystem>
#include <regex>
#include <fstream> //For file reading and writing

#include <libreborn/libreborn.h>
#include <symbols/minecraft.h>
#include <mods/misc/misc.h>

#include "other_sissystuff.h"

namespace fs = std::filesystem;
using namespace std; //so that we don't need that darn std:: in front of everything!

std::unordered_map<std::string, std::string> invKeyDict = {};	// Umap holding server name and client_id key

// Check if path+dir exists
static bool existsDir(const std::string& thePath) {
    fs::path directory(thePath);
    return fs::exists(directory) && fs::is_directory(directory);
}



// Function for creating the dirs we need for storage
static void createDir(const std::string& thePath) {
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




// Function that returns a random-ish string of given len.
static std::string randomStr(int length) {// Function to generate somewhat random alphanumeric string of requested length.
    static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::string result;
    result.resize(length);

    srand(time(NULL));
    for (int i = 0; i < length; i++) result[i] = charset[rand() % charset.length()];
    return result;
}





// Function for sanitizing received inventory data 
bool isInvString(std::string inv_str){
    vector<string> lines = split_str(inv_str, '\n');
    if(lines.size() != 36) return false;
    for(int i = 0; i < lines.size(); i++){
        vector<string> data = split_str(lines[i], ':');// split by : to get separate values
        if( data.size() != 3) return false;
		// Check if every value is number
		for(int i2 = 0; i2 < data.size(); i2++){
			if( !isNumeric(data[i2]) ) return false;
		}
		// Check if number is in range
		if( stoi(data[0]) < 0 || stoi(data[0]) > 456) { INFO("data[0] bugs"); return false; }
		if( stoi(data[1]) < 0 || stoi(data[1]) > 65535) { INFO("data[1] bugs"); return false; }
		if( stoi(data[2]) < 0 || stoi(data[2]) > 255) { INFO("data[2] bugs"); return false; }
    }
    return true;
}



// Function to check if a string is digits or not
static bool isNumeric(const std::string& str) {
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
    //std::string random_id = "";
    
    if (!conf_file.good()) { // Check it, if not exists it will be written
        std::string random_id = randomStr(25);
        //std::static string random_id = randomStr(25);
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



