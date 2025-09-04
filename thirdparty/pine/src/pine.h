#pragma once

#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <thread>

#ifdef _WIN32
#define read_portable(a, b, c) (recv(a, b, c, 0))
#define write_portable(a, b, c) (send(a, b, c, 0))
#define close_portable(a) (closesocket(a))
#include <windows.h>
#else
#define read_portable(a, b, c) (read(a, b, c))
#define write_portable(a, b, c) (write(a, b, c))
#define close_portable(a) (close(a))
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

/**
 * The PINE API. @n
 * This is the client side implementation of the PINE protocol. @n
 * It allows for a three
 * way communication between the emulated game, the emulator and an external
 * tool, using the external tool as a relay for all communication. @n
 * It is a socket based IPC that is _very_ fast. @n
 *
 * If you want to draw comparisons you can think of this as an equivalent of the
 * BizHawk LUA API, although with the logic out of the core and in an external
 * tool. @n
 * While BizHawk would run a lua script at each frame in the core of the
 * emulator we opt instead to keep the entire logic out of the emulator to make
 * it more easily extensible, more portable, require less code and be more
 * performant. @n
 *
 * Event based commands such as ExecuteOnFrameEnd (not yet implemented) can thus
 * be a blocking socket event, which is then noticed by our API, executes out
 * IPC commands and then tells the game to resume. Thanks to the speed of the
 * IPC even complex events can be outsourced from the emulator, thus keeping
 * the main codebase lean and minimal. @n
 *
 * If you are reading the documentation, feel free to browse through the class
 * that targets your preferred emulator.
 *
 * Have fun! @n
 * -Gauvain "GovanifY" Roussel-Tarbouriech, 2020
 */
namespace PINE {

#ifdef DEBUG

void hexdump(void *ptr, int buflen) {
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;
    for (i = 0; i < buflen; i += 16) {
        printf("%06x: ", i);
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%02x ", buf[i + j]);
            else
                printf("   ");
        printf(" ");
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        printf("\n");
    }
}

#endif

class Shared {
    // allow test suite to poke internals
  protected:
    /**
     * IPC Slot identifier. @n
     * Used by the IPC to identify concurrent sessions.
     */
    uint16_t slot;

#if defined(_WIN32) || defined(DOXYGEN)
    /**
     * Socket handler. @n
     * On windows it uses the type SOCKET, on linux int.
     */
    SOCKET sock;
#else
    int sock;
#endif

    /**
     * Socket initialization state. @n
     * True if initialized, false if not or if impossible to connect to.
     */
    bool sock_state = false;

#if !defined(_WIN32) || defined(DOXYGEN)
    /**
     * Unix socket name. @n
     * The name of the unix socket used on platforms with unix socket support.
     * @n Currently everything except Windows.
     */
    std::string SOCKET_NAME;
#endif

    /**
     * Maximum memory used by an IPC message request.
     * Equivalent to 50,000 Write64 requests.
     * @see MAX_IPC_RETURN_SIZE
     * @see MAX_BATCH_REPLY_COUNT
     */
#define MAX_IPC_SIZE 650000

    /**
     * Maximum memory used by an IPC message reply.
     * Equivalent to 50,000 Read64 replies.
     * @see MAX_IPC_SIZE
     * @see MAX_BATCH_REPLY_COUNT
     */
#define MAX_IPC_RETURN_SIZE 450000

    /**
     * Maximum number of commands sent in a batch message.
     * @see MAX_IPC_RETURN_SIZE
     * @see MAX_IPC_SIZE
     */
#define MAX_BATCH_REPLY_COUNT 50000

    /**
     * IPC return buffer. @n
     * A preallocated buffer used to store all IPC replies.
     * @see ipc_buffer
     * @see MAX_IPC_RETURN_SIZE
     */
    char *ret_buffer;

    /**
     * IPC messages buffer. @n
     * A preallocated buffer used to store all IPC messages.
     * @see ret_buffer
     * @see MAX_IPC_SIZE
     */
    char *ipc_buffer;

    /**
     * Length of the batch IPC request. @n
     * This is used when chaining multiple IPC commands in one go to store the
     * current size of the packet chain.
     * @see IPCCommand
     * @see MAX_IPC_SIZE
     */
    unsigned int batch_len = 0;

    /**
     * Length of the reply of the batch IPC request. @n
     * This is used when chaining multiple IPC commands in one go
     * to store the length of the reply of the IPC message.
     * @see IPCCommand
     * @see MAX_IPC_RETURN_SIZE
     */
    unsigned int reply_len = 0;

    /**
     * Whether a batch command reply needs relocation. @n
     * This automatically sets up the reply size to be the
     * maximum possible since we cannot anticipate how much
     * resources this will take.
     * @see IPCCommand
     * @see MAX_IPC_RETURN_SIZE
     */
    bool needs_reloc = false;

    /**
     * Number of IPC messages of the batch IPC request. @n
     * This is used when chaining multiple IPC commands in one go
     * to store the number of IPC messages chained together.
     * @see IPCCommand
     * @see MAX_BATCH_REPLY_COUNT
     */
    unsigned int arg_cnt = 0;

    /**
     * Position of the batch arguments. @n
     * This is used when chaining multiple IPC commands in one go.
     * Stores the location of each message reply in the buffer
     * sent by FinalizeBatch.
     * @see FinalizeBatch
     * @see IPCCommand
     * @see MAX_BATCH_REPLY_COUNT
     */
    unsigned int *batch_arg_place;

    /**
     * Sets the state of the batch command building. @n
     * This is used when chaining multiple IPC commands in one go. @n
     * As we cannot build multiple batch IPC commands at the same time because
     * of state keeping issues we block the initialization of another batch
     * request until the other ends.
     */
    std::mutex batch_blocking;

    /**
     * Sets the state of the IPC message building. @n
     * As we cannot build multiple batch IPC commands at the same time because
     * of state keeping issues we block the initialization of another message
     * request until the other ends.
     */
    std::mutex ipc_blocking;

    /**
     * IPC result codes. @n
     * A list of possible result codes the IPC can send back. @n
     * Each one of them is what we call an "opcode" or "tag" and is the
     * first byte sent by the IPC to differentiate between results.
     */
    enum IPCResult : unsigned char {
        IPC_OK = 0,     /**< IPC command successfully completed. */
        IPC_FAIL = 0xFF /**< IPC command failed to complete. */
    };

    /**
     * Converts an uint to an char* in little endian.
     * @param res_array The array to modify.
     * @param res The value to convert.
     * @param i When to insert it into the array
     * @return res_array
     */
    template <typename T>
    static auto ToArray(char *res_array, T res, int i) -> char * {
        memcpy((res_array + i), (char *)&res, sizeof(T));
        return res_array;
    }

    /**
     * Converts a char* to an uint in little endian.
     * @param arr The array to convert.
     * @param i When to load it from the array.
     * @return The converted value.
     */
    template <typename T>
    static auto FromArray(char *arr, int i) -> T {
        return *(T *)(arr + i);
    }

    /**
     * Ensures a batch IPC message isn't too big.
     * @param command_size Additional size required for the message.
     * @param reply_size Additional size required for the reply.
     */
    auto BatchSafetyChecks(int command_size, int reply_size = 0) -> bool {
        // we do not really care about wasting cycles when building batch
        // packets, so let's just do sanity checks for the sake of it.
        // TODO: go back when clang has implemented C++20 [[unlikely]]
        return ((batch_len + command_size) >= MAX_IPC_SIZE ||
                (reply_len + reply_size) >= MAX_IPC_RETURN_SIZE ||
                arg_cnt + 1 >= MAX_BATCH_REPLY_COUNT);
    }

    /**
     * Initializes the socket IPC connection with the server. @n
     * @see sock
     * @see sock_state
     */
    auto InitSocket() -> void {
#ifdef _WIN32
        struct sockaddr_in server;

        sock = socket(AF_INET, SOCK_STREAM, 0);

        // Prepare the sockaddr_in structure
        server.sin_family = AF_INET;
        // localhost only
        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_port = htons(slot);

        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
            close_portable(sock);
            sock_state = false;
            return;
        }

#else
        struct sockaddr_un server;

        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        server.sun_family = AF_UNIX;
        strcpy(server.sun_path, SOCKET_NAME.c_str());

        if (connect(sock, (struct sockaddr *)&server,
                    sizeof(struct sockaddr_un)) < 0) {
            close_portable(sock);
            sock_state = false;
            return;
        }
#endif
        sock_state = true;
    }

  public:
    /**
     * IPC Command messages opcodes. @n
     * A list of possible operations possible by the IPC. @n
     * Each one of them is what we call an "opcode" and is the first
     * byte sent by the IPC to differentiate between commands.
     */
    enum IPCCommand : unsigned char {
        MsgRead8 = 0,           /**< Read 8 bit value to memory. */
        MsgRead16 = 1,          /**< Read 16 bit value to memory. */
        MsgRead32 = 2,          /**< Read 32 bit value to memory. */
        MsgRead64 = 3,          /**< Read 64 bit value to memory. */
        MsgWrite8 = 4,          /**< Write 8 bit value to memory. */
        MsgWrite16 = 5,         /**< Write 16 bit value to memory. */
        MsgWrite32 = 6,         /**< Write 32 bit value to memory. */
        MsgWrite64 = 7,         /**< Write 64 bit value to memory. */
        MsgVersion = 8,         /**< Returns the emulator version. */
        MsgSaveState = 9,       /**< Saves a savestate. */
        MsgLoadState = 0xA,     /**< Loads a savestate. */
        MsgTitle = 0xB,         /**< Returns the game title. */
        MsgID = 0xC,            /**< Returns the game ID. */
        MsgUUID = 0xD,          /**< Returns the game UUID. */
        MsgGameVersion = 0xE,   /**< Returns the game verion. */
        MsgStatus = 0xF,        /**< Returns the emulator status. */
        MsgUnimplemented = 0xFF /**< Unimplemented IPC message. */
    };

    /**
     * Emulator status enum. @n
     * A list of possible emulator statuses. @n
     */
    enum EmuStatus : uint32_t {
        Running = 0,  /**< Game is running */
        Paused = 1,   /**< Game is paused */
        Shutdown = 2, /**< Game is shutdown */
    };

  protected:
    /**
     * Internal function for savestate IPC messages. @n
     * On error throws an IPCStatus. @n
     * Format: XX YY @n
     * Legend: XX = IPC Tag, YY = Slot.
     * @see IPCCommand
     * @see IPCStatus
     * @param slot The savestate slot to use.
     * @param T Flag to enable batch processing or not.
     * @param Y IPCCommand to use.
     * @return If in batch mode the IPC message otherwise void.
     */
    template <IPCCommand Y, bool T = false>
    auto EmuState(uint8_t slot) {
        // easiest way to get tag into a constexpr is a lambda, necessary for
        // GetReply
        // batch mode
        if constexpr (T) {
            if (BatchSafetyChecks(2)) {
                SetError(OutOfMemory);
                return (char *)0;
            }
            char *cmd = &ipc_buffer[batch_len];
            cmd[0] = Y;
            cmd[1] = slot;
            batch_len += 2;
            arg_cnt += 1;
            return cmd;
        } else {
            // we are already locked in batch mode
            std::lock_guard<std::mutex> lock(ipc_blocking);
            ToArray(ipc_buffer, 4 + 2, 0);
            ipc_buffer[4] = Y;
            ipc_buffer[5] = slot;
            SendCommand(IPCBuffer{ 4 + 1 + 1, ipc_buffer },
                        IPCBuffer{ 4 + 1, ret_buffer });
            return;
        }
    }

    /**
     * Internal function for IPC messages returning strings. @n
     * On error throws an IPCStatus. @n
     * Format: XX @n
     * Legend: XX = IPC Tag.
     * Return: ZZ * 256
     * Legend: ZZ = text.
     * @see IPCCommand
     * @see IPCStatus
     * @param T Flag to enable batch processing or not.
     * @param Y IPCCommand to use.
     * @return If in batch mode the IPC message otherwise void.
     */
    template <IPCCommand Y, bool T = false>
    auto StringCommands() {
        // batch mode
        if constexpr (T) {
            // reply is automatically set to max because of reloc, so let's not
            // check that
            if (BatchSafetyChecks(1, 4)) {
                SetError(OutOfMemory);
                return (char *)0;
            }
            char *cmd = &ipc_buffer[batch_len];
            cmd[0] = Y;
            batch_len += 1;
            // MSB is used as a flag to warn pine that the reply is a VLE!
            batch_arg_place[arg_cnt] = (reply_len | 0x80000000);
            reply_len += 4;
            needs_reloc = true;
            arg_cnt += 1;
            return cmd;
        } else {
            // we are already locked in batch mode
            std::lock_guard<std::mutex> lock(ipc_blocking);
            ToArray(ipc_buffer, 4 + 1, 0);
            ipc_buffer[4] = Y;
            SendCommand(IPCBuffer{ 4 + 1, ipc_buffer },
                        IPCBuffer{ MAX_IPC_RETURN_SIZE, ret_buffer });
            return GetReply<Y>(ret_buffer, 5);
        }
    }

  public:
    /**
     * IPC message buffer. @n
     * A list of all needed fields to store an IPC message.
     */
    struct IPCBuffer {
        int size;     /**< Size of the buffer. */
        char *buffer; /**< Buffer. */
        // do NOT specify a destructor to free buffer as we reuse the same
        // buffer to avoid the cost of mallocs; We specify destructors upon need
        // on structures that makes a copy of this, eg BatchCommand.
    };

    /**
     * IPC batch message fields. @n
     * A list of all needed fields to send a batch IPC message command and
     * retrieve their result.
     */
    struct BatchCommand {
        IPCBuffer ipc_message;          /**< IPC message fields. */
        IPCBuffer ipc_return;           /**< IPC return fields. */
        unsigned int *return_locations; /**< Location of arguments in IPC return
                                           fields. */
        unsigned int msg_size;          /**< Number of IPC messages. */
        bool reloc; /**< Whether the message needs relocation. */

        // C bindings handle manually the freeing of such resources.
#ifndef C_FFI
        /**
         * BatchCommand Destructor.
         */
        ~BatchCommand() {
            delete[] ipc_message.buffer;
            delete[] ipc_return.buffer;
            delete[] return_locations;
        }
#endif
    };

    /**
     * Result code of the IPC operation. @n
     * A list of result codes that should be returned, or thrown, depending
     * on the state of the result of an IPC command.
     */
    enum IPCStatus : unsigned int {
        Success = 0,       /**< IPC command successfully completed. */
        Fail = 1,          /**< IPC command failed to execute. */
        OutOfMemory = 2,   /**< IPC command too big to send. */
        NoConnection = 3,  /**< Cannot connect to the IPC socket. */
        Unimplemented = 4, /**< Unimplemented IPC command. */
        Unknown = 5        /**< Unknown status. */
    };

  protected:
    /**
     * Formats an IPC buffer. @n
     * Creates a new buffer with IPC opcode set and first address argument
     * currently used for memory IPC commands.
     * @param size The size of the array to allocate.
     * @param address The address to put as an argument of the IPC command.
     * @param command The IPC message tag(opcode).
     * @param T false to make a full packet or true for a part of one, used for
     * batch commands.
     * @see IPCCommand
     * @return The IPC buffer.
     */
    template <bool T = false>
    auto FormatBeginning(char *cmd, uint32_t address, IPCCommand command,
                         uint32_t size = 0) -> char * {
        if constexpr (T) {
            cmd[0] = (unsigned char)command;
            return ToArray(cmd, address, 1);
        } else {
            ToArray(cmd, size, 0);
            cmd[4] = (unsigned char)command;
            return ToArray(cmd, address, 5);
        }
    }

#if defined(C_FFI) || defined(DOXYGEN)
    /**
     * Error number. @n
     * Result code of the last executed function. @n
     * Used instead of exceptions on the C library bindings.
     */
    IPCStatus ipc_errno = Success;
#endif

    /**
     * Sets the error code for the last operation. @n
     * On C++, throws an exception, on C, sets the error code. @n
     * @param err The error to set.
     */
    auto SetError(IPCStatus err) -> void {
#ifdef C_FFI
        ipc_errno = err;
#else
        throw err;
#endif
    }

  public:
#if defined(C_FFI) || defined(DOXYGEN)
    /**
     * Gets the last error code set. @n
     * Only for C bindings.
     */
    auto GetError() -> IPCStatus {
        IPCStatus copy = ipc_errno;
        ipc_errno = Success;
        return copy;
    }
#endif

    /**
     * Returns the reply of an IPC command. @n
     * Throws an IPCStatus if there is no reply to read.
     * @param cmd A char array containing the IPC return buffer OR a
     * BatchCommand.
     * @param place An integer specifying where the argument is
     * in the buffer OR which function to read the reply of in
     * the case of a BatchCommand.
     * @return The reply, variable type. Refer to the documentation of the
     * standard function. Ownership of datastreams is also passed down to you,
     * so don't forget to read carefully the documentation and see if you need
     * to free anything!
     * @see IPCResult
     * @see IPCBuffer
     */
    template <IPCCommand T, typename Y>
    auto GetReply(const Y &cmd, int place) {
        [[maybe_unused]] char *buf;
        [[maybe_unused]] int loc;
        if constexpr (std::is_same<Y, BatchCommand>::value) {
            buf = cmd.ipc_return.buffer;
            loc = cmd.return_locations[place];
        } else {
            buf = cmd;
            loc = place;
        }
        if constexpr (T == MsgRead8)
            return FromArray<uint8_t>(buf, loc);
        else if constexpr (T == MsgRead16)
            return FromArray<uint16_t>(buf, loc);
        else if constexpr (T == MsgRead32)
            return FromArray<uint32_t>(buf, loc);
        else if constexpr (T == MsgRead64)
            return FromArray<uint64_t>(buf, loc);
        else if constexpr (T == MsgStatus)
            return FromArray<EmuStatus>(buf, loc);
        else if constexpr (T == MsgVersion || T == MsgID || T == MsgTitle ||
                           T == MsgUUID || T == MsgGameVersion) {
            uint32_t size = FromArray<uint32_t>(buf, loc);
            char *datastream = new char[size];
            memcpy(datastream, &buf[loc + 4], size);
            return datastream;
        } else {
            SetError(Unimplemented);
            return;
        }
    }

    /**
     * Sends an IPC command to the emulator. @n
     * Fails if the IPC cannot be sent or if the emulator returns IPC_FAIL.
     * Throws an IPCStatus on failure.
     * @param cmd An IPCBuffer containing the IPC command size and buffer OR a
     * BatchCommand.
     * @param rt An IPCBuffer containing the IPC return size and buffer.
     * @see IPCResult
     * @see IPCBuffer
     */
    template <typename T>
    auto SendCommand(const T &cmd, const T &rt = T()) -> void {
        IPCBuffer command;
        IPCBuffer ret;

        if constexpr (std::is_same<T, BatchCommand>::value) {
            command = cmd.ipc_message;
            ret = cmd.ipc_return;
        } else {
            command = cmd;
            ret = rt;
        }

        if (!sock_state) {
            InitSocket();
        }

        if (write_portable(sock, command.buffer, command.size) < 0) {
            // if our write failed, assume the socket connection cannot be
            // established
            close_portable(sock);
            sock_state = false;
            SetError(NoConnection);
            return;
        }

#ifdef DEBUG
        printf("packet sent:\n");
        hexdump(command.buffer, command.size);
#endif

        // either int or ssize_t depending on the platform, so we have to
        // use a bunch of auto
        auto receive_length = 0;
        auto end_length = 4;

        // while we haven't received the entire packet, maybe due to
        // socket datagram splittage, we continue to read
        while (receive_length < end_length) {
            auto tmp_length = read_portable(sock, &ret.buffer[receive_length],
                                            ret.size - receive_length);
            // we close the connection if an error happens
            if (tmp_length <= 0) {
                receive_length = 0;
                break;
            }

            receive_length += tmp_length;

            // if we got at least the final size then update
            if (end_length == 4 && receive_length >= 4) {
                end_length = FromArray<uint32_t>(ret.buffer, 0);
                if (end_length > MAX_IPC_SIZE) {
                    receive_length = 0;
                    break;
                }
            }
        }
#ifdef DEBUG
        printf("reply received:\n");
        hexdump(ret.buffer, receive_length);
#endif
        if (receive_length == 0) {
            SetError(Fail);
            return;
        }

        if ((unsigned char)ret.buffer[4] == IPC_FAIL) {
            SetError(Fail);
            return;
        }

        // batch commands are a bit more complex than you'd expect: some replies
        // are VLE, so we need to relocate accordingly all future replies by an
        // offset to ensure GetReply points to the correct buffer location.
        // We can do it in an O(n) way by storing the global relocation offset
        // and applying it to all future commands in one go instead of doing it
        // in an O(n^2) and updating the list every time we encounter an offset
        // update.
        // why not just assume a standard size instead of going through the pain
        // of relocating everything in the protocol? math is cheap, io isn't.
        if constexpr (std::is_same<T, BatchCommand>::value) {
            if (cmd.reloc) {
                unsigned int reloc_add = 0;
                for (unsigned int i = 0; i < cmd.msg_size; i++) {
                    cmd.return_locations[i] += reloc_add;
                    if ((cmd.return_locations[i] & 0x80000000) != 0) {
                        cmd.return_locations[i] =
                            (cmd.return_locations[i] & ~0x80000000);
                        reloc_add += FromArray<uint32_t>(
                            ret.buffer, (cmd.return_locations[i]));
                    }
                }
            }
        }
    }

    /**
     * Initializes a batch command IPC message.  @n
     * Batch IPC messages are preferred when dealing with a lot of IPC
     * messages in a quick fashion. They are _very_ fast, 1000
     * Write<uint8_t> are as fast as one Read<uint32_t> in non-batch mode,
     * give or take, which is about 100µs. @n You'll have to build the IPC
     * messages in advance, with this function and FinalizeBatch, and will
     * have to send the command yourself, along with dealing with the
     * extraction of return values, if need there is. It is a little bit
     * less convenient than the standard IPC but has, at the very least, a
     * 1000x speedup on big commands.
     * @see batch_blocking
     * @see batch_len
     * @see reply_len
     * @see arg_cnt
     * @see FinalizeBatch
     */
    auto InitializeBatch() -> void {
        batch_blocking.lock();
        ipc_blocking.lock();
        // 0-3 = header size, 4 = opcode
        batch_len = 4;
        reply_len = 5;
        needs_reloc = false;
        arg_cnt = 0;
    }

    /**
     * Finalizes a batch command IPC message. @n
     * WARNING: You will ALWAYS have to call a FinalizeBatch, even on
     * exceptions, once an InitializeBatch has been called overthise the
     * class will deadlock.
     * @return A BatchCommand with:
     *         * The IPCBuffer of the message.
     *         * The IPCBuffer of the return.
     *         * The argument location in the reply buffer.
     * @see batch_blocking
     * @see batch_len
     * @see reply_len
     * @see arg_cnt
     * @see InitializeBatch
     * @see IPCBuffer
     * @see BatchCommand
     */
    auto FinalizeBatch() -> BatchCommand {
        // save size in IPC message header.
        ToArray<uint32_t>(ipc_buffer, batch_len, 0);

        // we copy our arrays to unblock the IPC class.
        uint16_t bl = batch_len;
        int rl = needs_reloc ? MAX_IPC_RETURN_SIZE : reply_len;
        char *c_cmd = new char[batch_len];
        memcpy(c_cmd, ipc_buffer, batch_len * sizeof(char));
        char *c_ret = new char[rl];
        memcpy(c_ret, ret_buffer, rl * sizeof(char));
        unsigned int *arg_place = new unsigned int[arg_cnt];
        memcpy(arg_place, batch_arg_place, arg_cnt * sizeof(unsigned int));

        // we unblock the mutex
        batch_blocking.unlock();
        ipc_blocking.unlock();

        // MultiCommand is done!
        return BatchCommand{ IPCBuffer{ bl, c_cmd }, IPCBuffer{ rl, c_ret },
                             arg_place, arg_cnt, needs_reloc };
    }

    /**
     * Reads a value from the emulator's memory. @n
     * On error throws an IPCStatus. @n
     * Format: XX YY YY YY YY @n
     * Legend: XX = IPC Tag, YY = Address. @n
     * Return: (ZZ*??) @n
     * Legend: ZZ = Value read.
     * @see IPCCommand
     * @see IPCStatus
     * @see GetReply
     * @param address The address to read.
     * @param T Flag to enable batch processing or not.
     * @param Y The type of the variable to read (eg uint8_t).
     * @return The value read in memory. If in batch mode the IPC message.
     */
    template <typename Y, bool T = false>
    auto Read(uint32_t address) {

        // easiest way to get tag into a constexpr is a lambda, necessary
        // for GetReply
        constexpr IPCCommand tag = []() -> IPCCommand {
            switch (sizeof(Y)) {
                case 1:
                    return MsgRead8;
                case 2:
                    return MsgRead16;
                case 4:
                    return MsgRead32;
                case 8:
                    return MsgRead64;
                default:
                    return MsgUnimplemented;
            }
        }();
        if constexpr (tag == MsgUnimplemented) {
            SetError(Unimplemented);
            return;
        }

        // batch mode
        if constexpr (T) {
            if (BatchSafetyChecks(5, sizeof(Y))) {
                SetError(OutOfMemory);
                return (char *)0;
            }
            char *cmd =
                FormatBeginning<true>(&ipc_buffer[batch_len], address, tag);
            batch_len += 5;
            batch_arg_place[arg_cnt] = reply_len;
            reply_len += sizeof(Y);
            arg_cnt += 1;
            return cmd;
        } else {
            // we are already locked in batch mode
            std::lock_guard<std::mutex> lock(ipc_blocking);
            IPCBuffer cmd =
                IPCBuffer{ 4 + 5,
                           FormatBeginning(ipc_buffer, address, tag, 4 + 5) };
            IPCBuffer ret = IPCBuffer{ 1 + sizeof(Y) + 4, ret_buffer };
            SendCommand(cmd, ret);
            return GetReply<tag>(ret_buffer, 5);
        }
    }

    /**
     * Writes a value to the emulator's game memory. @n
     * On error throws an IPCStatus. @n
     * Format: XX YY YY YY YY (ZZ*??) @n
     * Legend: XX = IPC Tag, YY = Address, ZZ = Value.
     * @see IPCCommand
     * @see IPCStatus
     * @param address The address to write to.
     * @param value The value to write.
     * @param T Flag to enable batch processing or not.
     * @param Y The type of the variable to write (eg uint8_t).
     * @return If in batch mode the IPC message otherwise void.
     */
    template <typename Y, bool T = false>
    auto Write(uint32_t address, Y value) {

        // easiest way to get tag into a constexpr is a lambda, necessary
        // for GetReply
        constexpr IPCCommand tag = []() -> IPCCommand {
            switch (sizeof(Y)) {
                case 1:
                    return MsgWrite8;
                case 2:
                    return MsgWrite16;
                case 4:
                    return MsgWrite32;
                case 8:
                    return MsgWrite64;
                default:
                    return MsgUnimplemented;
            }
        }();
        if constexpr (tag == MsgUnimplemented) {
            SetError(Unimplemented);
            return;
        }

        // batch mode
        if constexpr (T) {
            if (BatchSafetyChecks(5 + sizeof(Y))) {
                SetError(OutOfMemory);
                return (char *)0;
            }
            char *cmd = ToArray<Y>(
                FormatBeginning<true>(&ipc_buffer[batch_len], address, tag),
                value, 5);
            batch_len += 5 + sizeof(Y);
            arg_cnt += 1;
            return cmd;
        } else {
            // we are already locked in batch mode
            std::lock_guard<std::mutex> lock(ipc_blocking);
            int size = 4 + 5 + sizeof(Y);
            char *cmd = ToArray(FormatBeginning(ipc_buffer, address, tag, size),
                                value, 4 + 5);
            SendCommand(IPCBuffer{ size, cmd }, IPCBuffer{ 1 + 4, ret_buffer });
            return;
        }
    }

    /**
     * Retrieves the emulator's version. @n
     * On error throws an IPCStatus. @n
     * Format: XX @n
     * Legend: XX = IPC Tag. @n
     * Return: YY YY YY YY (ZZ*??) @n
     * Legend: YY = string size, ZZ = version string.
     * @see IPCCommand
     * @see IPCStatus
     * @param T Flag to enable batch processing or not.
     * @return If in batch mode the IPC message otherwise the version
     * string. @n
     * /!\ If not in batch mode this function passes ownership of a
     * datastream to you, do not forget to free it!
     */
    template <bool T = false>
    auto Version() {
        constexpr IPCCommand tag = MsgVersion;
        return StringCommands<tag, T>();
    }

    /**
     * Retrieves emulator status. @n
     * On error throws an IPCStatus. @n
     * Format: XX @n
     * Legend: XX = IPC Tag. @n
     * Return: ZZ ZZ ZZ ZZ @n
     * Legend: ZZ = emulator status.
     * @see IPCCommand
     * @see IPCStatus
     * @param T Flag to enable batch processing or not.
     * @return If in batch mode the IPC message otherwise the emulator
     * status.
     */
    template <bool T = false>
    auto Status() {
        constexpr IPCCommand tag = MsgStatus;
        // batch mode
        if constexpr (T) {
            if (BatchSafetyChecks(1, 4)) {
                SetError(OutOfMemory);
                return (char *)0;
            }
            char *cmd = &ipc_buffer[batch_len];
            cmd[0] = tag;
            batch_len += 1;
            batch_arg_place[arg_cnt] = reply_len;
            reply_len += 4;
            arg_cnt += 1;
            return cmd;
        } else {
            // we are already locked in batch mode
            std::lock_guard<std::mutex> lock(ipc_blocking);
            ToArray(ipc_buffer, 4 + 1, 0);
            ipc_buffer[4] = tag;
            SendCommand(IPCBuffer{ 4 + 1, ipc_buffer },
                        IPCBuffer{ 4 + 1 + 4, ret_buffer });
            return GetReply<tag>(ret_buffer, 5);
        }
    }

    /**
     * Retrieves the game title. @n
     * On error throws an IPCStatus. @n
     * Format: XX @n
     * Legend: XX = IPC Tag. @n
     * Return: YY YY YY YY (ZZ*??) @n
     * Legend: YY = string size, ZZ = title string.
     * @see IPCCommand
     * @see IPCStatus
     * @param T Flag to enable batch processing or not.
     * @return If in batch mode the IPC message otherwise the version
     * string. @n
     * /!\ If not in batch mode this function passes ownership of a
     * datastream to you, do not forget to free it!
     */
    template <bool T = false>
    auto GetGameTitle() {
        constexpr IPCCommand tag = MsgTitle;
        return StringCommands<tag, T>();
    }

    /**
     * Retrieves the game ID. @n
     * On error throws an IPCStatus. @n
     * Format: XX @n
     * Legend: XX = IPC Tag. @n
     * Return: YY YY YY YY (ZZ*??) @n
     * Legend: YY = string size, ZZ = ID string.
     * @see IPCCommand
     * @see IPCStatus
     * @param T Flag to enable batch processing or not.
     * @return If in batch mode the IPC message otherwise the version
     * string. @n
     * /!\ If not in batch mode this function passes ownership of a
     * datastream to you, do not forget to free it!
     */
    template <bool T = false>
    auto GetGameID() {
        constexpr IPCCommand tag = MsgID;
        return StringCommands<tag, T>();
    }

    /**
     * Retrieves the game UUID. @n
     * On error throws an IPCStatus. @n
     * Format: XX @n
     * Legend: XX = IPC Tag. @n
     * Return: YY YY YY YY (ZZ*??) @n
     * Legend: YY = string size, ZZ = UUID string.
     * @see IPCCommand
     * @see IPCStatus
     * @param T Flag to enable batch processing or not.
     * @return If in batch mode the IPC message otherwise the version
     * string. @n
     * /!\ If not in batch mode this function passes ownership of a
     * datastream to you, do not forget to free it!
     */
    template <bool T = false>
    auto GetGameUUID() {
        constexpr IPCCommand tag = MsgUUID;
        return StringCommands<tag, T>();
    }

    /**
     * Retrieves the game version. @n
     * On error throws an IPCStatus. @n
     * Format: XX @n
     * Legend: XX = IPC Tag. @n
     * Return: YY YY YY YY (ZZ*??) @n
     * Legend: YY = string size, ZZ = game version string.
     * @see IPCCommand
     * @see IPCStatus
     * @param T Flag to enable batch processing or not.
     * @return If in batch mode the IPC message otherwise the game version
     * string. @n
     * /!\ If not in batch mode this function passes ownership of a
     * datastream to you, do not forget to free it!
     */
    template <bool T = false>
    auto GetGameVersion() {
        constexpr IPCCommand tag = MsgGameVersion;
        return StringCommands<tag, T>();
    }

    /**
     * Asks the emulator to save a savestate. @n
     * On error throws an IPCStatus. @n
     * Format: XX YY @n
     * Legend: XX = IPC Tag, YY = Slot.
     * @see IPCCommand
     * @see IPCStatus
     * @param slot The savestate slot to use.
     * @param T Flag to enable batch processing or not.
     * @return If in batch mode the IPC message otherwise void.
     */
    template <bool T = false>
    auto SaveState(uint8_t slot) {
        constexpr IPCCommand tag = MsgSaveState;
        return EmuState<tag, T>(slot);
    }

    /**
     * Asks the emulator to load a savestate. @n
     * On error throws an IPCStatus. @n
     * Format: XX YY @n
     * Legend: XX = IPC Tag, YY = Slot.
     * @see IPCCommand
     * @see IPCStatus
     * @param slot The savestate slot to use.
     * @param T Flag to enable batch processing or not.
     * @return If in batch mode the IPC message otherwise void.
     */
    template <bool T = false>
    auto LoadState(uint8_t slot) {
        constexpr IPCCommand tag = MsgLoadState;
        return EmuState<tag, T>(slot);
    }

    /**
     * Shared Initializer.
     * @param slot Slot to use for this IPC session.
     * @param emulator_name Emulator name to use for this IPC session.
     * @param default_slot Whether this is the default slot for the emulator
     * or not.
     * @see slot
     */
    Shared(const unsigned int slot, const std::string emulator_name,
           const bool default_slot) {
        // some basic input sanitization
        if (slot > 65536) {
            SetError(NoConnection);
            return;
        }
        this->slot = slot;
#ifdef _WIN32
        // We initialize winsock.
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#else
        char *runtime_dir = nullptr;
#ifdef __APPLE__
        runtime_dir = std::getenv("TMPDIR");
#else
        runtime_dir = std::getenv("XDG_RUNTIME_DIR");
#endif
        // fallback in case macOS or other OSes don't implement the XDG base
        // spec
        if (runtime_dir == nullptr)
            SOCKET_NAME = "/tmp/" + emulator_name + ".sock";
        else {
            SOCKET_NAME = runtime_dir;
            SOCKET_NAME += "/" + emulator_name + ".sock";
        }

        if (!default_slot) {
            SOCKET_NAME += "." + std::to_string(slot);
        }
#endif
        // we allocate once buffers to not have to do mallocs for each IPC
        // request, as malloc is expansive when we optimize for µs.
        ret_buffer = new char[MAX_IPC_RETURN_SIZE];
        ipc_buffer = new char[MAX_IPC_SIZE];
        batch_arg_place = new unsigned int[MAX_BATCH_REPLY_COUNT];
        InitSocket();
    }

    /**
     * Shared Destructor.
     */
    virtual ~Shared() {
        // We clean up winsock.
#ifdef _WIN32
        WSACleanup();
#endif
        delete[] ret_buffer;
        delete[] ipc_buffer;
        delete[] batch_arg_place;
    }
};

class PCSX2 : public Shared {
  public:
    /**
     * PCSX2 session Initializer with a specified slot.
     * @param slot Slot to use for this IPC session.
     * @see slot
     */
    PCSX2(const unsigned int slot = 0)
        : Shared((slot == 0) ? 28011 : slot, "pcsx2", (slot == 0)){};
};

class RPCS3 : public Shared {
  public:
    /**
     * RPCS3 session Initializer with a specified slot.
     * @param slot Slot to use for this IPC session.
     * @see slot
     */
    RPCS3(const unsigned int slot = 0)
        : Shared((slot == 0) ? 28012 : slot, "rpcs3", (slot == 0)){};
};

class DuckStation : public Shared {
  public:
    /**
     * DuckStation session Initializer with a specified slot.
     * @param slot Slot to use for this IPC session.
     * @see slot
     */
    DuckStation(const unsigned int slot = 0)
        : Shared((slot == 0) ? 28011 : slot, "duckstation", (slot == 0)){};

    auto GetGameVersion() {
        SetError(Unimplemented);
        return;
    }
};

}; // namespace PINE
