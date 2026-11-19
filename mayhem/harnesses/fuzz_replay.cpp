// Fuzzes the client's replay (.cor) loading: the bytes are parsed as a GameReplay, exactly as
// TabReplays/MainWindow do with a downloaded or opened replay (GameReplay::ParseFromArray), and a
// replay that parses is fed through the real game logic: a Replay is built from it and every
// GameEventContainer goes through its GameEventHandler, as the replay timeline does. The headless
// zone view mirrors the one in upstream's tests/game/replay_test.cpp (it stands in for the graphics
// side that materializes cards).
//
// Kept across inputs on purpose: the QCoreApplication and the CardDatabase singleton, which is left
// empty (its path provider points at an empty scratch dir and it is never loaded), so no input can
// change it. Each input gets a fresh GameReplay, Replay and view.
#include "fuzz_common.h"
#include <QDir>
#include <QList>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <libcockatrice/card/database/card_database.h>
#include <libcockatrice/card/database/card_database_manager.h>
#include <libcockatrice/game/board/card_state.h>
#include <libcockatrice/game/game_event_handler.h>
#include <libcockatrice/game/player/event_processing_options.h>
#include <libcockatrice/game/player/player_logic.h>
#include <libcockatrice/game/player/player_manager.h>
#include <libcockatrice/game/replay.h>
#include <libcockatrice/game/zones/card_zone_logic.h>
#include <libcockatrice/interfaces/interface_card_database_path_provider.h>
#include <libcockatrice/protocol/pb/game_event_container.pb.h>
#include <libcockatrice/protocol/pb/game_replay.pb.h>
#include <libcockatrice/protocol/pb/serverinfo_card.pb.h>
#include <libcockatrice/utility/card_ref.h>
#include <memory>

namespace
{
class EmptyPathProvider : public ICardDatabasePathProvider
{
public:
    explicit EmptyPathProvider(QString base) : base_(std::move(base))
    {
    }
    QString getCardDatabasePath() const override
    {
        return base_ + QStringLiteral("/cards.xml");
    }
    QString getCustomCardDatabasePath() const override
    {
        return base_ + QStringLiteral("/customsets/");
    }
    QString getTokenDatabasePath() const override
    {
        return base_ + QStringLiteral("/tokens.xml");
    }
    QString getSpoilerCardDatabasePath() const override
    {
        return base_ + QStringLiteral("/spoilers.xml");
    }

private:
    QString base_;
};

class HeadlessZoneView : public QObject
{
public:
    using QObject::QObject;
    ~HeadlessZoneView() override
    {
        qDeleteAll(cards_);
    }
    void attach(PlayerLogic *player)
    {
        for (CardZoneLogic *zone : player->getZones()) {
            connect(zone, &CardZoneLogic::createCardRequested, this,
                    [this, zone](const ServerInfo_Card &info, bool reorganize) { materialize(zone, info, reorganize); });
        }
    }

private:
    void materialize(CardZoneLogic *zone, const ServerInfo_Card &info, bool reorganize)
    {
        const CardRef cardRef{QString::fromStdString(info.name()), QString::fromStdString(info.provider_id())};
        auto *card = new CardState(nullptr, zone, cardRef, info.id());
        card->refreshCardInfo();
        card->clearCounters();
        for (int i = 0; i < info.counter_list_size(); ++i)
            card->insertCounter(info.counter_list(i).id(), info.counter_list(i).value());
        card->setAttacking(info.attacking());
        card->setFaceDown(info.face_down());
        card->setPT(QString::fromStdString(info.pt()));
        card->setAnnotation(QString::fromStdString(info.annotation()));
        card->setColor(QString::fromStdString(info.color()));
        card->setTapped(info.tapped());
        card->setDestroyOnZoneChange(info.destroy_on_zone_change());
        card->setDoesntUntap(info.doesnt_untap());
        zone->addCard(card, reorganize, info.x(), info.y());
        cards_.append(card);
    }
    QList<CardState *> cards_;
};
} // namespace

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    QStandardPaths::setTestModeEnabled(true);
    fuzzQtApp();
    static QTemporaryDir scratch; // empty: the card database stays empty
    static EmptyPathProvider provider(scratch.path());
    CardDatabaseManager::setCardDatabasePathProvider(&provider);
    CardDatabaseManager::getInstance();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    GameReplay replayProto;
    if (!replayProto.ParseFromArray(data, static_cast<int>(size)))
        return 0; // the client rejects a replay that does not parse
    {
        HeadlessZoneView view;
        Replay replay(nullptr, &replayProto, false);
        QObject::connect(replay.getPlayerManager(), &PlayerManager::playerAdded, &view, &HeadlessZoneView::attach);
        for (const GameEventContainer &container : replayProto.event_list())
            replay.getGameEventHandler()->processGameEventContainer(container, nullptr, EventProcessingOptions());
    }
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    return 0;
}
