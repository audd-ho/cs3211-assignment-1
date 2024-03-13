#include "instrument.hpp"
#include "io.hpp"
#include "engine.hpp"

#include <vector>

BSOrderList::BSOrderList(){
    OrderNode *initial_dummy_node = new OrderNode{};
    frontNode = initial_dummy_node;
    lastNode = initial_dummy_node;
    /*
    same_action_type.store(false, std::memory_order_relaxed);
    diff_action_type.store(false, std::memory_order_relaxed);
    */
}
BSOrderList::~BSOrderList(){
    OrderNode* cur_node = frontNode;
    while (cur_node != nullptr){
        OrderNode* prev_node = cur_node;
        cur_node = cur_node->next;
        delete(prev_node);
    } // need free self? huh? idkkkk
}

void BSOrderList::sema_increment_diff(){
    int sema_old_val = diff_action_type.load();
    while (true){
        int sema_new_val = sema_old_val+1;
        if (diff_action_type.compare_exchange_weak(sema_old_val, sema_new_val))
        {if (sema_old_val >= 0) {return;}}
    }
}
void BSOrderList::sema_decrement_diff(){
    int sema_old_val = diff_action_type.load();
    while (true){
        int sema_new_val = sema_old_val-1;
        if (diff_action_type.compare_exchange_weak(sema_old_val, sema_new_val))
        {return;}
    }
}
void BSOrderList::lock_diff(){
    int pre_lock_val = 0;
    int locked_val = -1;
    while (true){
        if (diff_action_type.compare_exchange_weak(pre_lock_val, locked_val))
        {return;}
        pre_lock_val = 0;
    }
}
void BSOrderList::unlock_diff(){ // only one that can increment it so technically no need loop etc(?)
    // unlocked to 0
    diff_action_type.store(0);
    /*
    int locked_val = -1;
    int post_unlocked_val = 0;
    while (true){
        if (diff_action_type.compare_exchange_weak(locked_val, post_unlocked_val))
        {return;}
        locked_val = -1;
    }
    */
}

void BSOrderList::spin_lock_same(){
    bool pre_lock_val = false;
    bool locked_val = true;
    while(true){
        if (same_action_type.compare_exchange_weak(pre_lock_val,locked_val))
        {return;}
        pre_lock_val = false;
    }
}
void BSOrderList::spin_unlock_same(){
    same_action_type.store(false);
}

int BSOrderList::try_execute(ClientCommand &input_order){
    // chance of deadlock?
    /*
    same_action_type.compare_exchange_weak( false, true);
    while (diff_action_type.load(std::memory_order_acquire));
    diff_action_type.store(true, std::memory_order_release);
    */

    // deadlock over this so i wanted to prevent by removing hold and wait, using computer networks probability method of collision avoidance!!
    // BUT!! that is congestion solution, deadlock should just prevent it!! by having a predefined ordering!!!

    //lock_diff(); 
    spin_lock_same();
    

    //OrderNode* prev_node = nullptr;
    OrderNode* cur_node = frontNode;

    std::vector<OrderNode*> executable_orders;
    //check if available
    // price then 
    switch (input_order.type){
        case (input_buy): // means this is try executing on a sell order list
        {
            while(cur_node->next != nullptr){
                if (cur_node->od.price <= input_order.price){
                    executable_orders.push_back(cur_node);
                }
                //prev_node = cur_node;
                cur_node = cur_node->next;
            }
            while (executable_orders.size() > 0){
                uint32_t lowest_price = input_order.price+1;
                int lowest_price_index = -1;
                int cur_index = 0;
                //OrderNode* lowest_price_order_ptr = nullptr;
                for (auto i = executable_orders.begin(); i != executable_orders.end(); ++i){
                    if ((*i)->od.price < lowest_price){
                        //lowest_price_order_ptr = nullptr;
                        lowest_price_index = cur_index;
                        lowest_price = (*i)->od.price;
                    }
                    cur_index++;
                }
                //Output::OrderDeleted(lowest_price_index, false, lowest_price);
                // remove from vector and also remove from LL or update reduce etc
                OrderNode* lowest_price_order_node = executable_orders[lowest_price_index];
                executable_orders.erase(executable_orders.begin()+lowest_price_index); 
                if (input_order.count > lowest_price_order_node->od.count){
                    // take out from LL
                    if (lowest_price_order_node->prev == nullptr) {frontNode = lowest_price_order_node->next;}
                    else {lowest_price_order_node->prev->next = lowest_price_order_node->next;}
                    lowest_price_order_node->next->prev = lowest_price_order_node->prev;
                    Output::OrderExecuted(lowest_price_order_node->od.order_id, input_order.order_id, lowest_price_order_node->od.executed_count, lowest_price_order_node->od.price, lowest_price_order_node->od.count, getCurrentTimestamp());
                    input_order.count = input_order.count-lowest_price_order_node->od.count;
                    continue;
                }
                else { // <=
                    Output::OrderExecuted(lowest_price_order_node->od.order_id, input_order.order_id, lowest_price_order_node->od.executed_count, lowest_price_order_node->od.price, input_order.count, getCurrentTimestamp());
                    lowest_price_order_node->od.count = lowest_price_order_node->od.count - input_order.count;
                    // cleared along with order list resting order
                    if (lowest_price_order_node->od.count == 0){ // take out from LL if count == 0
                        if (lowest_price_order_node->prev == nullptr) {frontNode = lowest_price_order_node->next;}
                        else {lowest_price_order_node->prev->next = lowest_price_order_node->next;}
                        lowest_price_order_node->next->prev = lowest_price_order_node->prev;
                    }
                    // cleared only active jsut came in order
                    else {(lowest_price_order_node->od.executed_count)++;} // else, only increment executed count and reduce count left
                    spin_unlock_same();
                    unlock_diff();
                    return 0; // successfully finished the active order that just came in
                }
            }
            spin_unlock_same();
            unlock_diff();
            return -1; // didnt manage to execute at all, so need add to order list as resting order
        }
        case (input_sell):
        {
            while(cur_node->next != nullptr){
                if (cur_node->od.price >= input_order.price){
                    executable_orders.push_back(cur_node);
                }
                //prev_node = cur_node;
                cur_node = cur_node->next;
            }
            while (executable_orders.size() > 0){
                uint32_t highest_price = input_order.price-1;
                int highest_price_index = -1;
                int cur_index = 0;
                //OrderNode* lowest_price_order_ptr = nullptr;
                for (auto i = executable_orders.begin(); i != executable_orders.end(); ++i){
                    if ((*i)->od.price > highest_price){
                        //lowest_price_order_ptr = nullptr;
                        highest_price_index = cur_index;
                        highest_price = (*i)->od.price;
                    }
                    cur_index++;
                }
                // remove from vector and also remove from LL or update reduce etc
                OrderNode* highest_price_order_node = executable_orders[highest_price_index];
                executable_orders.erase(executable_orders.begin()+highest_price_index); 
                if (input_order.count > highest_price_order_node->od.count){
                    // take out from LL
                    if (highest_price_order_node->prev == nullptr) {frontNode = highest_price_order_node->next;}
                    else {highest_price_order_node->prev->next = highest_price_order_node->next;}
                    highest_price_order_node->next->prev = highest_price_order_node->prev;
                    Output::OrderExecuted(highest_price_order_node->od.order_id, input_order.order_id, highest_price_order_node->od.executed_count, highest_price_order_node->od.price, highest_price_order_node->od.count, getCurrentTimestamp());
                    input_order.count = input_order.count-highest_price_order_node->od.count;
                    continue;
                }
                else { // <=
                    Output::OrderExecuted(highest_price_order_node->od.order_id, input_order.order_id, highest_price_order_node->od.executed_count, highest_price_order_node->od.price, input_order.count, getCurrentTimestamp());
                    highest_price_order_node->od.count = highest_price_order_node->od.count - input_order.count;
                    // cleared along with order list resting order
                    if (highest_price_order_node->od.count == 0){ // take out from LL if count == 0
                        if (highest_price_order_node->prev == nullptr) {frontNode = highest_price_order_node->next;}
                        else {highest_price_order_node->prev->next = highest_price_order_node->next;}
                        highest_price_order_node->next->prev = highest_price_order_node->prev;
                    }
                    // cleared only active jsut came in order
                    else {(highest_price_order_node->od.executed_count)++;} // else, only increment executed count and reduce count left
                    spin_unlock_same();
                    unlock_diff();
                    return 0; // successfully finished the active order that just came in
                }
            }
            spin_unlock_same();
            unlock_diff();
            return -1; // didnt manage to execute at all, so need add to order list as resting order
        }
        default:
            printf("error, idk: %d",input_order.order_id);
            spin_unlock_same();
            unlock_diff();
            return -10;
    }

    /*
    same_action_type.store(false, std::memory_order_release);
    diff_action_type.store(false, std::memory_order_release);
    */
}
void BSOrderList::add_order(ClientCommand &input_order){
    //std::scoped_lock lock(diff_action_type); // cant add while doing opposite action type
    // can write 2 at once, but the add dummy node has to lock for a bit

    // reached here means incremented semaphore! and old sema_value is >= 0, so can access either cos 0 or alr got someone accessing!
    OrderNode* node_to_work_with; 
    {
        std::scoped_lock lock(addOrder_mutex); // lock to add new node, then others can add also now
        node_to_work_with = lastNode;
        lastNode = new OrderNode{};
        node_to_work_with->next = lastNode;
        lastNode->prev = node_to_work_with;
        Output::OrderAdded(input_order.order_id, input_order.instrument, input_order.price, input_order.count, input_order.type == input_sell, getCurrentTimestamp());
        //fetch_add_time(some)
    }
    node_to_work_with->od.executed_count = 1;
    node_to_work_with->od.count = input_order.count;
    node_to_work_with->od.order_id = input_order.order_id;
    node_to_work_with->od.price = input_order.price;
}

bool BSOrderList::try_cancel(u_int32_t order_id){
    //std::scoped_lock lock(same_action_type, diff_action_type);
    
    // pre-defined ordering to prevent deadlock!!
    lock_diff();
    spin_lock_same();
    
    OrderNode* cur_node = frontNode;
    //bool dummy_present = (frontNode->next == nullptr);
    //Output::OrderDeleted(666,dummy_present, 666);
    while (cur_node->next != nullptr){
        //Output::OrderDeleted(1212,false,121212);
        if (cur_node->od.order_id == order_id){
            //Output::OrderDeleted(1212,false,121212);
            if (cur_node->prev == nullptr) {frontNode=cur_node->next;}
            else {cur_node->prev->next = cur_node->next;}
            cur_node->next->prev = cur_node->prev;
            Output::OrderDeleted(order_id, true, getCurrentTimestamp());
            spin_unlock_same();
            unlock_diff();
            return true;
        }
        cur_node = cur_node->next;
    }
    spin_unlock_same();
    unlock_diff();
    return false; // not able to cancel cos no have, rejected the cancel
}


InstrumentOrder::InstrumentOrder(): buy_order_list{}, sell_order_list{}{
    //timing.store(0, std::memory_order_relaxed);
}

InstrumentOrder::~InstrumentOrder(){} // need free or delete the lists? cos never "new" them... ??


// buy order lock before sell order lock, using pre-defined locking order to prevent deadlock!!, and unlock from within try_execute cos i cheeky... heh
// ALSO!! the lock is diff in buy and sell action, see that buy is locked first but locked differently
// sell is locked 2nd but also locked differently since the type of locking matters to enable better concurrency!! (-1 from 0) or (+1 from >=0) to enable more concurrency cos can write somewhat more concurrently!!


void InstrumentOrder::buy_action(ClientCommand input){
    // prevent buy order diff sema from dropping to <0 [read/execute state]
    // keep it at >0 [write/append state]
    ///buy_order_list.sema_increment_diff(); // may need to append to buy order list later
    buy_order_list.lock_diff();

    sell_order_list.lock_diff();
    int buying_result = sell_order_list.try_execute(input);
    if (buying_result == 0){ // succeed and done!
        ///buy_order_list.sema_decrement_diff(); // no need append so decrement diff sema, let others use or just mark its absence/no need anymore
        buy_order_list.unlock_diff();
        return;
    }
    // fail to execute or rather, fail to fully execute, got remainder
    buy_order_list.add_order(input);
    ///buy_order_list.sema_decrement_diff(); // finish appending to buy order list
    buy_order_list.unlock_diff();
    return;
}

void InstrumentOrder::sell_action(ClientCommand input){
    buy_order_list.lock_diff();
    ///sell_order_list.sema_increment_diff();
    sell_order_list.lock_diff();
    int selling_result = buy_order_list.try_execute(input);
    if (selling_result == 0){ // succeed and done!
        ///sell_order_list.sema_decrement_diff();
        sell_order_list.unlock_diff();
        return;
    }
    // fail to execute or rather, fail to fully execute, got remainder
    sell_order_list.add_order(input);
    ///sell_order_list.sema_decrement_diff();
    sell_order_list.unlock_diff();
    return;
}

int InstrumentOrder::cancel_action(ClientCommand input){
    if (sell_order_list.try_cancel(input.order_id)) {return 0;}
    if (buy_order_list.try_cancel(input.order_id)) {return 0;}
    //Output::OrderDeleted(99,false,123);
    return -1;
}



// no need right actually?
InstrumentsList::InstrumentsList(){
    //std::map<std::string,InstrumentOrder> instrument_map;
    ////std::atomic<bool> accessible;
    //std::mutex access_mutex; 
}

InstrumentsList::~InstrumentsList(){
    for (auto iter = instrument_map.begin(); iter != instrument_map.end(); iter++){
        delete(iter->second);
    }
}

std::vector<std::string> InstrumentsList::get_names(){
    std::vector<std::string> to_return_name_vector;
    {
        std::scoped_lock lock(vector_name_mutex);
        to_return_name_vector = instrument_names;
    }
    return to_return_name_vector;
}

InstrumentOrder& InstrumentsList::get_instrument_order(std::string instrument){
    std::scoped_lock lock(access_mutex); // maybe can pull down or remove or combine with vector mutex...!!??

    if (instrument_map.find(instrument) == instrument_map.end()){
        std::scoped_lock lock(vector_name_mutex);
        instrument_map[instrument] = new InstrumentOrder{};
        instrument_names.push_back(instrument);
        //count++;
        }
    //if (count == 2) {Output::OrderDeleted(676, false, 676);}
    return *(instrument_map[instrument]);
}