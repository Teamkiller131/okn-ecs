#include <doctest/doctest.h>
#include <okn/ecs/events/event_queue.hpp>
#include <okn/ecs/events/event_bus.hpp>
#include <string>

using namespace okn::ecs;

struct SimpleEvent {
    int value = 0;
    std::string message;
};

struct DamageEvent {
    int amount = 0;
    int target_id = 0;
};

TEST_CASE("EventQueue - basic operations") {
    EventQueue<int> queue;

    SUBCASE("new queue is empty") {
        CHECK(queue.empty());
        CHECK(queue.size() == 0);
    }

    SUBCASE("push and pop") {
        queue.push(42);
        CHECK(!queue.empty());
        CHECK(queue.size() == 1);

        int val = 0;
        CHECK(queue.pop(val));
        CHECK(val == 42);
        CHECK(queue.empty());
    }

    SUBCASE("pop from empty returns false") {
        int val = 0;
        CHECK(!queue.pop(val));
    }

    SUBCASE("clear empties queue") {
        queue.push(1);
        queue.push(2);
        queue.push(3);
        CHECK(queue.size() == 3);
        queue.clear();
        CHECK(queue.empty());
        CHECK(queue.size() == 0);
    }

    SUBCASE("multiple push/pop") {
        for (int i = 0; i < 10; ++i) {
            queue.push(i);
        }
        CHECK(queue.size() == 10);

        int val = 0;
        for (int i = 9; i >= 0; --i) {
            CHECK(queue.pop(val));
            CHECK(val == i);
        }
        CHECK(queue.empty());
    }
}

TEST_CASE("EventQueue - struct events") {
    EventQueue<SimpleEvent> queue;

    SUBCASE("push and pop struct") {
        SimpleEvent ev{42, "hello"};
        queue.push(ev);

        SimpleEvent out{};
        CHECK(queue.pop(out));
        CHECK(out.value == 42);
        CHECK(out.message == "hello");
    }
}

TEST_CASE("EventBus - subscribe and publish") {
    EventBus bus;

    SUBCASE("subscribe receives published events") {
        int received = 0;
        bus.subscribe<DamageEvent>([&received](const DamageEvent& ev) {
            received = ev.amount;
        });

        bus.publish(DamageEvent{25, 1});
        CHECK(bus.pending_count() == 1);
        CHECK(received == 0);

        bus.dispatch();
        CHECK(received == 25);
        CHECK(bus.pending_count() == 0);
    }

    SUBCASE("multiple subscribers for same type") {
        int count = 0;
        bus.subscribe<SimpleEvent>([&count](const SimpleEvent&) { count++; });
        bus.subscribe<SimpleEvent>([&count](const SimpleEvent&) { count++; });

        bus.publish(SimpleEvent{0, ""});
        bus.dispatch();
        CHECK(count == 2);
    }

    SUBCASE("different event types are separate") {
        int damage_count = 0;
        int simple_count = 0;

        bus.subscribe<DamageEvent>([&damage_count](const DamageEvent&) { damage_count++; });
        bus.subscribe<SimpleEvent>([&simple_count](const SimpleEvent&) { simple_count++; });

        bus.publish(DamageEvent{10, 1});
        bus.publish(SimpleEvent{42, "test"});
        bus.dispatch();

        CHECK(damage_count == 1);
        CHECK(simple_count == 1);
    }

    SUBCASE("empty dispatch is safe") {
        bus.dispatch();
        CHECK(bus.pending_count() == 0);
    }

    SUBCASE("publish without subscribers is safe") {
        bus.publish(DamageEvent{10, 1});
        bus.dispatch();
    }

    SUBCASE("handler count reflects subscriptions") {
        CHECK(bus.handler_count() == 0);
        bus.subscribe<DamageEvent>([](const DamageEvent&) {});
        CHECK(bus.handler_count() == 1);
        bus.subscribe<SimpleEvent>([](const SimpleEvent&) {});
        CHECK(bus.handler_count() == 2);
    }
}
