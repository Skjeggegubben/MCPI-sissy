#ifdef __cplusplus
    // Should be above 0xb7
    #define PACKET_ID 0xb8
    #define ARGS_SIZE 0x4

    typedef unsigned char uchar;
    struct CustomPacket {
        // Stuff from packet (don't mess with this)
        uchar *vtable;
        uchar padding1[12];
        // Args (mess with this)
        int len_arg;
        std::string str_arg;
    };

    // Symbols
    #define PACKET_VTABLE_SIZE 0x14
    static uint32_t Packet_write_vtable_offset = 0x8;
    static uint32_t Packet_read_vtable_offset = 0xc;
    static uint32_t Packet_handle_vtable_offset = 0x10;

    // void RakNet_BitStream_Read_uchar(uchar *i) = 0x45ab0;
    typedef void (*RakNet_BitStream_Read_uchar_t)(uchar *self, uchar *i);
    static RakNet_BitStream_Read_uchar_t RakNet_BitStream_Read_uchar = (RakNet_BitStream_Read_uchar_t) 0x45ab0;

    typedef void (*RakNet_BitStream_Write_uchar_t)(uchar *self, uchar *i);
    static RakNet_BitStream_Write_uchar_t RakNet_BitStream_Write_uchar = (RakNet_BitStream_Write_uchar_t) 0x18448;
    typedef void (*RakNet_BitStream_Write_int_t)(uchar *self, int *i);
    static RakNet_BitStream_Write_int_t RakNet_BitStream_Write_int = (RakNet_BitStream_Write_int_t) 0x18454;
    typedef void (*RakNet_BitStream_Read_int_t)(uchar *self, int *i);
    static RakNet_BitStream_Read_int_t RakNet_BitStream_Read_int = (RakNet_BitStream_Read_int_t) 0x184ec;

    typedef uchar *(*Packet_t)(uchar *self);
    static Packet_t Packet = (Packet_t) 0x6fc18;

    typedef uchar *(*MinecraftPackets_createPacket_t)(int id);
    static uchar *make_custom_packet(int intArg, std::string strArg);

    // This one will send to every player except the one packet came from, i.e. can be used for skin transfer
    //typedef void (*ServerSideNetworkHandler_redistributePacket_t)(uchar *callback, uchar *packet, uchar *guid);
    //static ServerSideNetworkHandler_redistributePacket_t ServerSideNetworkHandler_redistributePacket = (ServerSideNetworkHandler_redistributePacket_t) 0x74b74;

    typedef void (*RakNetInstance_sendTo_t)(uchar *self, uchar *guid, uchar *packet);
    static uint32_t RakNetInstance_sendTo_vtable_offset = 0x3c;

    void send_inventory_packet(std::string strArg, uchar *guid, uchar *callback);

    extern "C" {

        #else
        #define bool _Bool
        #endif

        #ifdef __cplusplus

    }
#endif