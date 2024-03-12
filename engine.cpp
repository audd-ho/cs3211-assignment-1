#include <iostream>
#include <thread>
#include <vector>

#include "io.hpp"
#include "engine.hpp"
#include "instrument.hpp"

void Engine::accept(ClientConnection connection)
{
	auto thread = std::thread(&Engine::connection_thread, this, std::move(connection));
	thread.detach();
}

InstrumentsList* order_book_ptr = new InstrumentsList{};

void Engine::connection_thread(ClientConnection connection)
{	
	while(true)
	{
		ClientCommand input {};
		switch(connection.readInput(input))
		{
			case ReadResult::Error: SyncCerr {} << "Error reading input" << std::endl;
			case ReadResult::EndOfFile: return;
			case ReadResult::Success: break;
		}

		// Functions for printing output actions in the prescribed format are
		// provided in the Output class:
		
		switch(input.type)
		{
			case input_cancel: {
				//printf("Canceled order! %d\n", input.order_id);
				//Output::OrderDeleted(99,false,123);
				int cancelled = -1;
				std::vector<std::string> vector_instruments = order_book_ptr->get_names();
				for (auto i_names = vector_instruments.begin(); i_names != vector_instruments.end(); ++i_names)
				{
					InstrumentOrder& instrument_OB = order_book_ptr->get_instrument_order((*i_names));
					cancelled = instrument_OB.cancel_action(input);
					//Output::OrderAdded(cancelled,(*i_names).c_str(), 666,666,666,666);
					if (cancelled == 0) {break;}
				}
				if (cancelled == -1) {Output::OrderDeleted(input.order_id, false, getCurrentTimestamp());}
				/*
				SyncCerr {} << "Got cancel: ID: " << input.order_id << std::endl;

				// Remember to take timestamp at the appropriate time, or compute
				// an appropriate timestamp!
				auto output_time = getCurrentTimestamp();
				Output::OrderDeleted(input.order_id, true, output_time);
				*/
				break;
			}

			case input_buy: {
				InstrumentOrder& instrument_OB = order_book_ptr->get_instrument_order(input.instrument);
				instrument_OB.buy_action(input);
				break;
			}

			case input_sell: {
				InstrumentOrder& instrument_OB = order_book_ptr->get_instrument_order(input.instrument);
				instrument_OB.sell_action(input);
				break;
			}

			default: {
				SyncCerr {}
				    << "Got order: " << static_cast<char>(input.type) << " " << input.instrument << " x " << input.count << " @ "
				    << input.price << " ID: " << input.order_id << std::endl;

				// Remember to take timestamp at the appropriate time, or compute
				// an appropriate timestamp!
				auto output_time = getCurrentTimestamp();
				Output::OrderAdded(input.order_id, input.instrument, input.price, input.count, input.type == input_sell,
				    output_time);
				break;
			}
		}
		/*
		// Additionally:

		// Remember to take timestamp at the appropriate time, or compute
		// an appropriate timestamp!
		intmax_t output_time = getCurrentTimestamp();

		// Check the parameter names in `io.hpp`.
		Output::OrderExecuted(123, 124, 1, 2000, 10, output_time);
		*/
	}
}
