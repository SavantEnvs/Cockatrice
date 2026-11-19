// Fuzzes the server-side protocol handler and game logic (Server_ProtocolHandler::processCommandContainer,
// Server_Game, Server_AbstractPlayer: dump zone, move card, arrows, counters, concede, ...) through the
// public LocalServer interface the client uses for local games.
//
// Per input a FRESH LocalServer with two connected clients is built, so nothing carries over between
// inputs: both clients log in, join the lobby room, client 0 creates game 1, client 1 joins it, both
// select a small fixed deck and ready up, so the game is started and the zones hold cards. The global
// RNG is re-created with a fixed seed per input so shuffles are deterministic.
//
// Fuzz input: a sequence of records [selector:1][len:2 BE][CommandContainer protobuf bytes:len].
// The low bit of the selector picks which of the two clients sends the command. A record whose bytes
// do not parse as a CommandContainer is skipped (the network layer drops such frames the same way).
#include "fuzz_common.h"
#include <QCoreApplication>
#include <QEvent>
#include <libcockatrice/network/server/local/local_server.h>
#include <libcockatrice/network/server/local/local_server_interface.h>
#include <libcockatrice/protocol/pb/command_deck_select.pb.h>
#include <libcockatrice/protocol/pb/command_ready_start.pb.h>
#include <libcockatrice/protocol/pb/commands.pb.h>
#include <libcockatrice/protocol/pb/room_commands.pb.h>
#include <libcockatrice/protocol/pb/session_commands.pb.h>
#include <libcockatrice/rng/rng_sfmt.h>

extern RNG_Abstract *rng;
RNG_Abstract *rng = nullptr;

static const char kDeck[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                            "<cockatrice_deck version=\"1\"><deckname>fuzz</deckname>"
                            "<zone name=\"main\"><card number=\"8\" name=\"Island\"/>"
                            "<card number=\"4\" name=\"Grizzly Bears\"/></zone>"
                            "<zone name=\"side\"><card number=\"2\" name=\"Forest\"/></zone>"
                            "</cockatrice_deck>";

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    fuzzQtApp();
    return 0;
}

static void send(LocalServerInterface *c, const CommandContainer &cc)
{
    c->itemFromClient(cc);
}

static void setup(LocalServerInterface *c[2])
{
    for (int i = 0; i < 2; ++i) {
        CommandContainer login;
        auto *l = login.add_session_command()->MutableExtension(Command_Login::ext);
        l->set_user_name(i == 0 ? "alice" : "bob");
        send(c[i], login);
        CommandContainer join;
        join.add_session_command()->MutableExtension(Command_JoinRoom::ext)->set_room_id(0);
        send(c[i], join);
    }
    CommandContainer create;
    create.set_room_id(0);
    auto *cg = create.add_room_command()->MutableExtension(Command_CreateGame::ext);
    cg->set_description("fuzz");
    cg->set_max_players(2);
    send(c[0], create);
    CommandContainer joinGame;
    joinGame.set_room_id(0);
    joinGame.add_room_command()->MutableExtension(Command_JoinGame::ext)->set_game_id(1);
    send(c[1], joinGame);
    for (int i = 0; i < 2; ++i) {
        CommandContainer deck;
        deck.set_game_id(1);
        deck.add_game_command()->MutableExtension(Command_DeckSelect::ext)->set_deck(kDeck);
        send(c[i], deck);
    }
    for (int i = 0; i < 2; ++i) {
        CommandContainer ready;
        ready.set_game_id(1);
        ready.add_game_command()->MutableExtension(Command_ReadyStart::ext)->set_ready(true);
        send(c[i], ready);
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    delete rng;
    rng = new RNG_SFMT(0x5eed5eedULL);
    {
        LocalServer server;
        LocalServerInterface *clients[2] = {server.newConnection(), server.newConnection()};
        setup(clients);
        size_t pos = 0;
        while (pos + 3 <= size) {
            const uint8_t selector = data[pos];
            const size_t len = (size_t(data[pos + 1]) << 8) | data[pos + 2];
            pos += 3;
            if (len > size - pos)
                break;
            CommandContainer cc;
            if (cc.ParseFromArray(data + pos, static_cast<int>(len)))
                send(clients[selector & 1], cc);
            pos += len;
            QCoreApplication::sendPostedEvents();
        }
    }
    // Flush deleteLater() of the protocol handlers/games torn down with the server.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return 0;
}
