#ifndef INSTRUMENT_HPP
#define INSTRUMENT_HPP

#include <map>
#include <string>
#include <atomic>
#include <mutex>
#include "io.hpp"

class BSOrderList
{
    struct OrderData
    {
        uint32_t order_id; //?? // maybe also need person who made order... the thread or client? another member field?
        uint32_t price;
        uint32_t count;
        uint32_t executed_count;
    };
    struct OrderNode
    {
        OrderNode *prev = nullptr;
        OrderNode *next = nullptr;
        OrderData od{};
    };
    std::atomic<bool> same_action_type; // buy dont restrict buy, but other actions restrict buy
    //std::mutex same_action_type;
    OrderNode* frontNode;
    OrderNode* lastNode; // dummy ah
    std::mutex addOrder_mutex; // just for adding a new dummy node, so can still add multiple at once, just a slighr delay cos need make new node but can concurrently fill multiple dummy nodes with data at once?
    void spin_lock_same();
    void spin_unlock_same();
public:
    BSOrderList(); // constructor
    ~BSOrderList(); // destructor
    // >=0 can add order tgt, then finish adding then decrement, till 0, then at 0 can then be decremented if want to lock for execution => -1 value to prevent others(diff type) from taking/using and entering the order list
    std::atomic<int> diff_action_type; // buy check buy list will restrict sell list though
    // 0 being neutral ground sorta!
    // >=0
    void sema_increment_diff();
    void sema_decrement_diff();
    // <=0
    void lock_diff();
    void unlock_diff();
    //std::mutex diff_action_type;
    int try_execute(ClientCommand &input_order); // -1 if fail and int of remaining count left if succeed
    void add_order(ClientCommand &input_order);
    bool try_cancel(int order_id);
};

struct InstrumentOrder {
public:
    InstrumentOrder();
    ~InstrumentOrder();
    void buy_action(ClientCommand input); // count, price, id, details
    void sell_action(ClientCommand input);
    void cancel_action(ClientCommand input);
private:
    BSOrderList buy_order_list;
    BSOrderList sell_order_list;
    std::atomic<int> timing;
    //void fetch_add_time(); // to see
};

struct InstrumentsList
{
public:
    InstrumentsList();
    ~InstrumentsList();
    InstrumentOrder& get_instrument_order(std::string instrument);
private:
    std::map<std::string,InstrumentOrder*> instrument_map;
    std::mutex access_mutex;
};


#endif