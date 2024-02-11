#include <libreborn/libreborn.h>
#include <symbols/minecraft.h>
#include <mods/chat/chat.h>
#include <mods/misc/misc.h>

#include "custom_packetstuff.h"
#include "other_sissystuff.h"



void send_inventory_packet(std::string strArg, uchar *guid, uchar *callback){// Neat function for just shipping off a string in a custom packet
	int intArg = strArg.size(); // Need to get the string length so we can feed it as arg along with the string
    uchar *packet = make_custom_packet(intArg, strArg);	// to the make_custom_packet function :) 
    uchar *rak_net_instance = *(uchar **) ( get_minecraft() + Minecraft_rak_net_instance_property_offset); // This and next lines just sends it.. 
    if(guid == NULL){
		RakNetInstance_send_t RakNetInstance_send = *(RakNetInstance_send_t *) (*(uchar **) rak_net_instance + RakNetInstance_send_vtable_offset);
    	RakNetInstance_send(rak_net_instance, packet); // And poof the custom packet is sent :)
	} else {
		//server instance is sending packet back to client
		// wrong -> ServerSideNetworkHandler_redistributePacket(callback, packet, guid);
		RakNetInstance_sendTo_t RakNetInstance_sendTo = *(RakNetInstance_sendTo_t *) (*(uchar **) rak_net_instance + RakNetInstance_sendTo_vtable_offset);
        RakNetInstance_sendTo(rak_net_instance, guid, packet);
	}
}



static void CustomPacket_write(CustomPacket *self, uchar *bit_stream) {
    uchar id = PACKET_ID;
    RakNet_BitStream_Write_uchar(bit_stream, &id);			// Send the packet id
    // Write the args
	RakNet_BitStream_Write_int(bit_stream, &self->len_arg ); // Send the length of string as integer..
	const char *str = self->str_arg.c_str();				// Make nice const char array of the crappy string that compiler didnt like
	for (int i = 0; i < self->len_arg; i++) {					// Next loop through each char in the array
		RakNet_BitStream_Write_uchar(bit_stream, (uchar*) &str[i]); // .. and send each char converted to uchar!
	}	
}

static void CustomPacket_read(CustomPacket *self, uchar *bit_stream) { // Read the args (the id is already read)
    RakNet_BitStream_Read_int(bit_stream, &self->len_arg); // Need the len to know how many times to loop
    int size = self->len_arg; 			// Now we know the size of the string that is to be extracted from packet!
    std::string str(size, 0); 			// Create an output string to gather decoded chars into
    for (int i = 0; i < size; i++) { 	// Loop through each char and decode from uchar back to ascii char..
        RakNet_BitStream_Read_uchar(bit_stream, (uchar *) &str.c_str()[i]);
    }
	self->str_arg = str; // Finally set the decoded value to the str_arg variable so it is ready to be handled!
}

static void CustomPacket_handle(CustomPacket *self, uchar *guid, uchar *callback) {
    // Handle it, you can also use args here
    //INFO("The len_arg is: %i", self->len_arg);
	//INFO("The str_arg is: %s", self->str_arg.c_str() );
	parse_inventory_packet(self->str_arg, guid, callback);
}

static uchar *get_custom_packet_vtable() {
    static uchar *vtable = NULL;
    if (vtable == NULL) {
        // Init
        vtable = (uchar *) ::operator new(PACKET_VTABLE_SIZE);
        memcpy((void *) vtable, (void *) 0x1024d8, PACKET_VTABLE_SIZE);
        ALLOC_CHECK(vtable);

        // Modify
        *(uchar **) (vtable + Packet_write_vtable_offset) = (uchar *) CustomPacket_write;
        *(uchar **) (vtable + Packet_read_vtable_offset) = (uchar *) CustomPacket_read;
        *(uchar **) (vtable + Packet_handle_vtable_offset) = (uchar *) CustomPacket_handle;
    }
    return vtable;
}

static uchar *make_custom_packet(int intArg, std::string strArg) {
    CustomPacket *p = new CustomPacket;
    ALLOC_CHECK(p);
    Packet((uchar *) p);
    uchar *vtable = get_custom_packet_vtable();
    p->vtable = vtable;
    p->len_arg = intArg;
	p->str_arg = strArg;
    return (uchar *) p;
}




// Handle making the packet on the receiving end
static MinecraftPackets_createPacket_t MinecraftPackets_createPacket_original = NULL;
static uchar *MinecraftPackets_createPacket_injection(int id) {
    if (id == PACKET_ID) {
        // Args don't need to be set, they will be right after this call
        return make_custom_packet(0, ""); // But dear Bigjango, I had to add arg "" since I added str arg, compiler whined about it!
    }
    return MinecraftPackets_createPacket_original(id);
}

__attribute__((constructor)) static void init() {
    MinecraftPackets_createPacket_original = (MinecraftPackets_createPacket_t) extract_from_bl_instruction((uchar *) 0x740a0);
    overwrite_calls((void *) MinecraftPackets_createPacket_original, (void *) MinecraftPackets_createPacket_injection);
}

/*
// Just a way to send the packet, example of sending chat input in a packet
HOOK(chat_handle_packet_send, void, (uchar *minecraft, uchar *chatpacket)) {
    char *msg = *(char **) (chatpacket + ChatPacket_message_property_offset);;
    std::string strArg = msg;
	send_inventory_packet(strArg, NULL, NULL);

    // Orignal
    ensure_chat_handle_packet_send();
    real_chat_handle_packet_send(minecraft, chatpacket);
}*/