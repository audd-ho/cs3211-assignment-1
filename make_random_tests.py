import random;
import string

def choose_action():
    actions_type = ["B", "S", "C"]
    return actions_type[random.randint(0,2)]

def make_random_test_in():
    lines_to_be_written = []
    num_clients = random.randint(1,50)
    lines_to_be_written.append(str(num_clients))
    lines_to_be_written.append("o")

    instruments_set = set()
    num_instruments = random.randint(10,100)
    while(len(instruments_set) < num_instruments): ## or actually do in order like AAAA AAAB AAAC etc etc or 8 chars etc but yeaaa lazy...
        instruments_set.add(random.choice(string.ascii_uppercase) + random.choice(string.ascii_uppercase) + random.choice(string.ascii_uppercase) + random.choice(string.ascii_uppercase))
    instruments_list = list(instruments_set)

    num_actions = random.randint(10,100)
    action_num = 0 ## can think of as order ID, but then wont be full consecutive cos cancel dh order id so rather another variable bah!!
    order_id = 0
    client_orderID = {x:[] for x in range(num_clients)}
    while (action_num < num_actions):
        client_thread = random.randint(0,(num_clients-1))
        action = choose_action()
        if action == "C":
            num_of_orderIDs = len(client_orderID[client_thread])
            if num_of_orderIDs == 0:
                continue
            chosen_order_id = client_orderID[client_thread][random.randint(0,(num_of_orderIDs-1))]
            lines_to_be_written.append(f"{client_thread} C {chosen_order_id}")
            action_num += 1
        lines_to_be_written.append(f"{client_thread} {action} {order_id} {random.choice(instruments_list)} {random.randint(1000,9999)} {random.randint(100, 9999)}")
        order_id += 1
        action_num += 1
    lines_to_be_written.append("x")
    with open("random-test.in", "w") as rt_f:
        rt_f.writelines(line + '\n' for line in lines_to_be_written)

make_random_test_in()