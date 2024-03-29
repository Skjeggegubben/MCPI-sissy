#include <filesystem>
#include <dirent.h>
#include <fstream> //For file reading and writing

#include <libreborn/libreborn.h>
#include <symbols/minecraft.h>
#include <mods/misc/misc.h>
#include <mods/home/home.h>

#include "custom_packetstuff.h"
#include "other_sissystuff.h"

namespace fs = std::filesystem;
using namespace std; //so that we don't need that darn std:: in front of everything!

// Declarations, declare vars and functions blah blah..
static unsigned char *minecraft;
static string server = "";
static string inventoryString = "";
static string inv_id = "";              // A random string that will be written to conf-file, used for id in coms with server.
static std::string server_motd = "";    // String to help determine server name (for fallback saving to file).

std::unordered_map<std::string, std::string> inventoryDict = {};	// Umap holding user_id + inventory as base64 string.
std::unordered_map<std::string, std::string> inventoryDict_old = {};// Duplicate umap for comparison before saving.
std::unordered_map<std::string, struct bgThread> bgThreads = {};	// Umap holding bgThread.

b64class b64; // Make the class object first, then usage: INFO( b64.enc("BMW") );

// If on server without SISSY?
static bool local_fallback = true;                 // Bool that helps determine if we need to save locally as fallback when on this server.
static bool received_inventory = false;            // Bool to help ensure server doesn't keep sending inventory when we don't want MOAR inventory!
static bool game_ready = false;                    // Bool var that will be set to true once Minecraft_isLevelGenerated returns true, i.e. when game is up.
static uint64_t tickMarkerTS = 0;                  // Marker timestamp, for making interval'ish behaviour instead of running all the code on every tick.
static int sec_counter = 0;                        // Int var for counting seconds between intervals.


// The functions that catches the minecraft obj for us
static void callback_for_mcpisissy(unsigned char *mcpi) { // Runs on every tick, sets the minecraft var.
    minecraft = mcpi; 
    invTickFunction(mcpi); // From here we can call a custom function to run on every tick
}



// Helper function. Returns the minecraft obj. 
unsigned char *get_mcObj() {
    return minecraft;
}



static void invTickFunction(unsigned char *mcpi){     // The function that will run on every tick.
	if (mcpi == NULL) {                     // Ticks start running before we have mcpi, and ofc
        return;                             // without mcpi there is nothing more to do here but return.
    }
    int x = (game_ready) ? 1000 : 500;      // Half sec interval if game isn't actually started yet, then one sec interval.

	if(tickMarkerTS < 1) { 
		tickMarkerTS = get_ms_unixTS();     // Variable could be still NULL, which is also less than 1, then initiate it, make new timestamp.
	}
    uint64_t nowTS = get_ms_unixTS();		// Making a new timestamp on each tick, to compare with marker. Maybe kinda stupid waste of cpu resources?
    if( (nowTS - tickMarkerTS) > x ){	    // If time now, minus time when tickMarkerTS was set, equals more than 1000 millisec ( 1 sec ),
        tickMarkerTS = get_ms_unixTS();		// then just set a new marker timestamp, and below comes the stuff to repeat at ~1 second interval.


        if(!game_ready){    // This code part is to be executed only while game isnt up i.e. level not generated yet. Once it finds that
                            // level is generated, it is time to do important stuff needed at game startup. Afterwards this part gets skipped.
            
            if ((*Minecraft_isLevelGenerated)(mcpi)){   // Got level, minecraft obj, all up and running and all is presumably good to go!
                INFO("[MCPISISSY]: GAME IS READY!");
                local_fallback = true;      // This turns to 'false' if server responds to INV request.
                server = server_name(); 	// In case of fallback saving to file, we need the server name. 
                game_ready = true;          // Set bool to true so this wont be repeated!
                sissyDirCheck();            // Check / create the necessary dir's for the mod!
                sec_counter = 0;            // Counter must be reset to ensure 0!
                received_inventory = false; // Reset bool so we can receive again!
                
                // Make background thread for loading file data either as server or as client.
                bgThread initLoadTimer = bgThread();
                bgThreads["initLoadTimer"] = initLoadTimer;
                
                if(!on_ext_server() ) { // Procedures needed for server at game startup, can be done here:
                    bgThreads["initLoadTimer"].setTimeout([&](std::string str1, std::string str2) {            
                        load_saved_inventories(); // SERVER NEEDS TO LOAD USER'S B64-STRINGS INTO UNORDERED MAP.
                    }, 10, "", "");
                    
                } else { // Procedures needed at game startup as client, can be done here:
                    inv_id = get_inv_id( server );
                    send_inventory_packet("INV " + inv_id, NULL, NULL ); // SEND 1ST PACKET, ASK IF SERVER SUPPORTS SISSY / INV.
                    // In case server doesn't reply, i.e. doesn't have this mod, the fallback is to load/save inventory from file:
                    bgThreads["initLoadTimer"].setTimeout([&](std::string str1, std::string str2) {
                        // If no reply from server, bool local_fallback is still true.
                        if(local_fallback) file_to_inventory();
                        received_inventory = true; // Don't try fooling us later with false inventory strings we didn't ask for!
                    }, 5000, "", ""); // Will be executed 5 sec from now. Server reply should be instant within 1-2s.
                }
                
            } else return;
        } else {
            // THIS PART repeats every sec. after game has started.
            if( !(*Minecraft_isLevelGenerated)(mcpi) ){ // Check if game still up once per sec.
                game_ready = false;
                INFO("[MCPISISSY]: GAME OVER!");
            } else {
                // The game is still running, so here's the spot for in-game interval'ed stuff:
                if(!self_is_server() ){ // Client saves/sends inventory string every 30 sec.
                    sec_counter++;
                    if(sec_counter == 30){
                        if(!local_fallback){ // Send the inventory string in a custom packet.
                            inventory_to_b64(); // Gets inventory to b64 and sends it to server!
                            sec_counter = 0;
                        } else { // Use local file as fallback cause server doesn't support INV packets!
                            inventory_to_file();
                            sec_counter = 0;
                        }
                    }
                } else { // Server side can do a save to file every 60 sec. if any changes.
                    sec_counter++;
                    if(sec_counter == 60){
                        save_loaded_inventories();
                        sec_counter = 0;
                    }
                }
            }   
        }
    }
}



// Parse the string from inventory packet and do different stuff as result.
void parse_inventory_packet(std::string packetStr, uchar *guid, uchar *callback){ 
    // Called from friendly custompackets.cpp each time there's some mail <3
    if(self_is_server() ){
        std::vector<std::string> data = split_str(packetStr, ' ');
        if( (data.size() != 2) && (data.size() != 3) ) return;                      // Malformed shiet! 
        if( (data[0] != "INV") && (data[0] != "GET") && (data[0] != "SET") ) return;// Malformed packet or hackery attempt!
        if( (data[1].length() != 25) || ( !all_allowed_chars(data[1]) ) ) return;   // Hacker basterd gtfo!!
        std::string client_id = data[1];
        
        // At this point, the inv_id is sanitized and command is 'INV', 'GET' or 'SET'
        if(data[0] == "INV"){ // Client connected and wants to know if server supports INV packets!
            if (inventoryDict.find(client_id) == inventoryDict.end()) { // Is not in dict, new client
                //INFO(" *** NEW CLIENT! NO DATA ON THIS ONE!");
                inventoryDict[client_id] = "";
                inventoryDict_old[client_id] = "";
            }
            send_inventory_packet( "INV", guid, callback );
            
        } else if(data[0] == "SET"){ // Client wants server to do the favor of saving his inventory data...
            if( data.size() != 3) return;               // Malformed piece of sheeit! 
            if(!isValidBase64(data[2]) ) return;        // Not a base64 string! Wtf is this hacker trying to do here?
            if( data[2].length() > 1000 ) return;       // This is NOT new free unlimited Dropbox storage for dumb hackrs!
            if(!isInvString( b64.dec(data[2]) ) ) return; // No thanks, only inventory data pls, gtfo!
            inventoryDict[client_id] = data[2];         // Save it to dict and reply 'OK'
            send_inventory_packet( "OK", guid, callback );

        } else { // must be GET then.. Client with given inv_id asks for his inventory string..
            //  If not in dict, must be because client is trying the heckry stuff and not INV first! 
            if (inventoryDict.find(client_id) == inventoryDict.end()) return;
            std::string replyStr = "OK";
            if(inventoryDict[client_id] != "") replyStr = inventoryDict[client_id];
            // If empty, the reply will still be just "OK".
            send_inventory_packet( replyStr, guid, callback );
        }

    } else { // WHEN THIS INSTANCE IS A CLIENT - Expect either 'INV', 'OK', or a base64 string from server.
        if(packetStr == "OK"){
            // Server has accepted packet, no inventory to give, just be happy!
            //INFO("YES, 'OK' WAS RECEIVED!!!");
        }else if(packetStr == "INV"){ // Reply on first packet. Server supports the mod, no need for fallback, yey!
            //INFO(" WOOHOO server has SISSY / INV!");
            local_fallback = false; // The ext. server has sissy / supports INV packet!
            send_inventory_packet("GET " + inv_id, NULL, NULL );
            
        } else {
            //INFO(" *** GOT OTHER THAN 'INV'/'OK'!"); // Expecting a base64-string with inventory only!
            if( !isValidBase64( packetStr ) ) return; // Not a base64 string! Heckery server fukcer hm?
            if( packetStr.length() > 1000 || !isInvString( b64.dec(packetStr) ) ) return; // No thanks gtfo!
            if(!received_inventory) { 
                b64_to_inventory(packetStr); // Valid base64 chars, so decode and load into inventory!
                received_inventory = true; // Set bool var so that we wont accept MOAR than once.
            } else return; // No thanks, pls dear server, don't spam me, I don't want MOAR inventory!
        }
    }
}



// Function for server, loads inventory strings from files
static void load_saved_inventories(){	
    std::string thePath(home_get());
    thePath.append("/mcpisissy/received_as_server/");
    
	struct dirent *entry = nullptr;
    DIR *dp = nullptr;

    dp = opendir(thePath.c_str() );
    if (dp != nullptr) {
        while ((entry = readdir(dp))){
            std::string filenameStr = entry->d_name; 
			if( filenameStr != "." && filenameStr != ".." ){
				std::string pathToFile = thePath + filenameStr;
				std::string inventoryAsB64 = file_get_contents(pathToFile);
                //if( inventoryAsB64.back() == '\n') inventoryAsB64.pop_back(); // Remove nasty linebreak!
				inventoryDict[filenameStr] = inventoryAsB64;
                inventoryDict_old[filenameStr] = inventoryAsB64; // We keep a duplicate to check if saving is needed!
			}
		}
        INFO("[MCPISISSY]: USER INVENTORIES FROM FILES LOADED!");
    }
    closedir(dp);
}



// Function for server, saves inventories from umap to files
static void save_loaded_inventories(){
    bool changed = false;
    std::string thePath(home_get());
    thePath.append("/mcpisissy/received_as_server/");    
    for ( auto& [client_id, client_inventory] : inventoryDict ){ // Check if there is any changes?
        if( inventoryDict_old[client_id] != client_inventory){ // if changes, save to file!
            changed = true;
            file_put_contents(thePath + client_id, client_inventory);
            inventoryDict_old[client_id] = client_inventory;
        }
    }
    //if(changed) INFO("SAVED INVENTORIES TO FILES");
}




// Function to make unix timestamp in ms since epoch.
static uint64_t get_ms_unixTS(){
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}




// Gets inventory, needed for loading stuff into inventory
static uchar *get_inventory() { 
    // Get minecraft and the player
    uchar *player = *(uchar **) ( get_mcObj() + Minecraft_player_property_offset);
    if (player != NULL) { // Get the player inventory
        uchar *inventory = *(uchar **) (player + Player_inventory_property_offset);
        return inventory;
    }
    return NULL; // The player doesn't exist
}



// The function that converts inventory data to base64 string for saving
static void inventory_to_b64(){
    std::string invStr = inv2str();
    if(invStr == "ERROR") return; // Maybe game has crashed? Let's hope never happens!
    if(invStr != inventoryString){ // Only save if there is a change!
        //INFO("SENDING INVENTORY STRING!");
        send_inventory_packet("SET " + inv_id + " " + b64.enc(invStr), NULL, NULL ); 
        inventoryString = invStr;
    } //else showInfo("No inventory change, not saving anything!");
}



// The function that loads inventory from base64 string to items in inventory
static void b64_to_inventory(std::string inputStr){
    // Called once, right after connecting to server if having received b64-string
    std::string inv_str = b64.dec(inputStr);
    str2inv(inv_str);
}



// The fallback to save inventory to local file when server lacks INV support.
static void inventory_to_file(){
    std::string invStr = inv2str();
    if(invStr == "ERROR") return; // Maybe game has crashed? Let's hope never happens!
    if(invStr != inventoryString){ //Only save to file if there is a change!
        std::string fallbackPath(home_get());
        fallbackPath.append("/mcpisissy/fallback_local_saves/" + server + ".inventory");
        file_put_contents(fallbackPath, invStr);
        inventoryString = invStr;
    } //else showInfo("No inventory change, not saving anything!");

}



// The fallback to load inventory from local file when server lacks INV support.
static void file_to_inventory(){
    std::string fallbackPath(home_get());
    fallbackPath.append("/mcpisissy/fallback_local_saves/" + server + ".inventory");
    if( fs::exists(fallbackPath) ) {
        std::string inv_str = file_get_contents(fallbackPath);
        str2inv(inv_str);
    }
}



// Function for getting inventory data into inventory, from string
static void str2inv(std::string inv_str){
    //showInfo("Trying to load inventory!");
    inventoryString = "";
    uchar *inventory = get_inventory();

    vector<string> lines = split_str(inv_str, '\n');
    for(int i = 0; i < lines.size(); i++){
        vector<string> data = split_str(lines[i], ':');// split by : to get separate values
        inventoryString += data[0] +":"+ data[1] +":"+ data[2] + "\n"; //set string so we can compare later for change
        ItemInstance *item_instance = new ItemInstance;
        ALLOC_CHECK(item_instance); //nice but I dunno what this actually does :S reserves memory space?
        item_instance->id = stoi( data[0] );
        item_instance->auxiliary = stoi( data[1] );
        item_instance->count = stoi( data[2] );
        (*FillingContainer_addItem)(inventory, item_instance);
    }
    showInfo("Inventory for this server is loaded!");

}



// Function for retrieving current inventory data as string
static std::string inv2str(){
    string tmpStr = ""; // make new string to put values into
    uchar *inventory = get_inventory(); // make sure that inventory is accessible
    if (inventory == NULL) {
        showInfo("ERROR: suddenly inventory == NULL");
        return "ERROR";
    }
    
    for (int i = 9; i < 45; i++){ // get data from ALL slots
        uchar *inventory_vtable = *(uchar **) inventory;
        FillingContainer_getItem_t FillingContainer_getItem = *(FillingContainer_getItem_t *) (inventory_vtable + FillingContainer_getItem_vtable_offset);
        ItemInstance *inventory_item = (*FillingContainer_getItem)(inventory, i);

        if (inventory_item != NULL) {
            tmpStr += to_string(inventory_item->id) + ":" + to_string(inventory_item->auxiliary) + ":" + to_string(inventory_item->count) + "\n";
        } else tmpStr += "0:0:0\n";
    }
    return tmpStr;
}




// For saving string to file, named after the php-function
static void file_put_contents(std::string thePath, std::string saveStr){
    ofstream outfile;
    outfile.open(thePath, std::ios_base::trunc); // 'app' for append, 'trunc' overwrite
    outfile << saveStr;
}



// For reading the contents of a file, named after the php-function
static std::string file_get_contents(std::string file){
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
    resultStr.pop_back(); // Remove nasty linebreak!
    return resultStr; 
}



// Function that determines if on external server or not
static bool on_ext_server() {
    if( self_is_server() ) return false;
    bool on_ext_srv = *(bool *) (*(unsigned char **) (get_mcObj() + Minecraft_level_property_offset) + 0x11);
    return on_ext_srv;
}



// Function needed for self_is_server() 
static bool got_raknetInstance(){
    return ( *(unsigned char **) (get_mcObj() + Minecraft_rak_net_instance_property_offset) != NULL ) ? true : false;
}



// Function to check if game instance is currently acting as server
static bool self_is_server(){
    if(!got_raknetInstance() ) return false; // If got raknetInstance then check RakNetInstance_isServer.
    unsigned char *rak_net_instance = *(unsigned char **) (get_mcObj() + Minecraft_rak_net_instance_property_offset);
    unsigned char *rak_net_instance_vtable = *(unsigned char**) rak_net_instance;
    RakNetInstance_isServer_t RakNetInstance_isServer = *(RakNetInstance_isServer_t *) (rak_net_instance_vtable + RakNetInstance_isServer_vtable_offset);
    return ((*RakNetInstance_isServer)(rak_net_instance)) ? true : false;
}





// Function for printing some debug info directly to game screen
static void showInfo(string text){ 
    unsigned char *gui = get_mcObj() + Minecraft_gui_property_offset;
    misc_add_message(gui, text.c_str() );
}


// For filename-sanitizing server name, make it safe for filename
static std::string server_name() {
//std::string server_name(const std::string& str) {
	std::string srvStr = get_server_motd();
	std::string retStr = "";
	for (char c : srvStr) {
		if ( ( c == '/') || (!isalnum(c) && c != '_' && c != '.') ) {
			retStr += '_';
		} else {
			retStr += c;
		}
	}
	return retStr;
}


static std::string get_server_motd() {
    return server_motd;
}


static bool Minecraft_joinMP_injection(uchar *self, uchar *server) {
    unsigned char *shared_string = *(unsigned char **) (server + RakNet_RakString_sharedString_property_offset);
    char *c_str = *(char **) (shared_string + RakNet_RakString_SharedString_c_str_property_offset);
    server_motd = c_str;
    return Minecraft_joinMultiplayer(self, server);
}


//The "main" function that starts when you run game
__attribute__((constructor)) static void init() {
    // Patch Minecraft::joinMultiplayer
    overwrite_calls((void *) Minecraft_joinMultiplayer, (void *) Minecraft_joinMP_injection);    
    
    misc_run_on_update(callback_for_mcpisissy);  
}