#include "Connection.hpp"
#include "Game.hpp"

#include <iostream>
#include <set>
#include <chrono>
#include <random>
#include <unordered_map>

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	std::random_device r;
  std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
  std::mt19937 eng{seed};
	//TODO: maybe un-hardcode this?? atleast remember to change this pls
	std::uniform_int_distribution<> dist_x(1,19);
	std::uniform_int_distribution<> dist_y(1,15);

	Server server(argv[1]);

	//Player id=0 is paired with Player with id=1, 2 with 3, etc..
	struct PlayerData {
		uint32_t id;
		bool winner;
	};

	//Match data
	std::unordered_map< Connection *, PlayerData > data;

	//only even-numbered ids are associated with data to save space
	std::unordered_map< uint32_t, ServerState > game_data;

	while (1) {
		//TODO handle waiting for other client
		//TODO handle when one client quits/wins

		server.poll([&](Connection *c, Connection::Event e) {
			if (e == Connection::OnOpen) {
				std::cout << "1" << std::endl;
				assert(!data.count(c));
				uint32_t new_id = data.size();
				PlayerData p{};
				p.id = new_id;
				p.winner = false;
				data.insert(std::make_pair(c, p));
				assert(data.count(c));
				std::cout << "1.1 " << c <<std::endl;
				if (new_id % 2 == 0) { //if player 1, create game state
					game_data.insert(std::make_pair(new_id, ServerState{}));
				} else { //if player 2 , begin game
					auto d = game_data.find(new_id - 1);
					d->second.gameStarted = true;
				}
				std::cout << c << ": Opened a conn with id " << new_id << std::endl;
			} else if (e == Connection::OnClose) {
				std::cout << "2" << std::endl;
				auto f =  data.find(c);
				uint32_t temp_id = f->second.id;
				auto d = game_data.find((temp_id % 2 == 0) ? temp_id : temp_id - 1);
				assert(f != data.end());
				assert(d != game_data.end());
				data.erase(f);
				game_data.erase(d);
			} else if (e == Connection::OnRecv) {
				std::cerr << "3" << std::endl;
				auto f =  data.find(c);
				assert(f != data.end());
				PlayerData &player = f->second;
				if (c->recv_buffer[0] == 'h') {
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
					std::cout << c << ": Got hello from id " << player.id << std::endl;
					std::cout << "4" << std::endl;
				} else if (c->recv_buffer[0] == 's') { //incoming snake data
					std::cout << "5" << std::endl;
					if (c->recv_buffer.size() < 1 + sizeof(SerializedState)) {
						return; //wait for more data
					} else {
						std::cout << "6" << std::endl;
						//calculate index into game_data
						uint32_t game_id = (player.id % 2 == 0) ? player.id : player.id - 1;
						auto d = game_data.find(game_id);
						// parse server data into game state
							std::cout << "7" << std::endl;
							SerializedState temp_ss{};
							memcpy(&temp_ss, c->recv_buffer.data() + 1, sizeof(SerializedState));
							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(SerializedState));
							serialized_to_state(&temp_ss, &d->second, false, player.id);
							std::cout << "8" << std::endl;
							//update game status and apple positions if needed
							glm::vec2 player_head = (player.id % 2 == 0) ? d->second.snake1[0] : d->second.snake2[0];
							//if player head intersects with apple, delete that apple from list
							auto find_apple = std::find(d->second.apples.begin(), d->second.apples.end(), player_head);
							if (find_apple != d->second.apples.end()){
								d->second.apples.erase(find_apple);
							}
							std::cout << "9" << std::endl;
							//if player head intersects with any player body, kill player head
							if (player.id % 2 == 0) { //if player 1
								//check if head intersects with other snake's body
								auto find_collision = std::find(d->second.snake2.begin(), d->second.snake2.end(), player_head);
								if (find_collision != d->second.snake2.end()){
									d->second.gameOver = true;
									d->second.playerOne = false;
								}
								std::cout << "10" << std::endl;
								//check if head intersects with your own body
								auto find_collision2 = std::find(d->second.snake1.begin() + 1, d->second.snake1.end(), player_head);
								if (find_collision2 != d->second.snake1.end()){
									d->second.gameOver = true;
									d->second.playerOne = false;
								}
								std::cout << "11" << std::endl;

							} else {									//if player 2
								//check if head intersects with other snake's body
								auto find_collision = std::find(d->second.snake1.begin(), d->second.snake1.end(), player_head);
								if (find_collision != d->second.snake1.end()){
									d->second.gameOver = true;
									d->second.playerOne = true;
								}
								std::cout << "12" << std::endl;
								//check if head intersects with your own body
								auto find_collision2 = std::find(d->second.snake2.begin() + 1, d->second.snake2.end(), player_head);
								if (find_collision2 != d->second.snake2.end()){
									d->second.gameOver = true;
									d->second.playerOne = true;
								}
							}
							std::cout << "13" << std::endl;

							//if player head intersects with any wall, kill player head
							if (player_head.x <= 0 || player_head.x >= d->second.width
									|| player_head.y <= 0 || player_head.y >= d->second.height) {
									d->second.gameOver = true;
									d->second.playerOne = (player.id % 2 == 0)? false : true;
							}
							std::cout << "14" << std::endl;
							//if # of apples is less than 5, place apples in random locations
							//		except where player bodies are located
							//NOTE: https://stackoverflow.com/questions/18726102/what-difference-between-rand-and-random-functions
							while (d->second.apples.size() < 5) {
								//generate random location that doesn't collide with other objects in game
								glm::vec2 rand_loc;
								std::cout << "15" << std::endl;

								rand_loc = glm::vec2(float(dist_x(eng)), float(dist_y(eng)));;
								auto find_in_apples = std::find(d->second.apples.begin(), d->second.apples.end(), rand_loc) != d->second.apples.end();
								auto find_in_snake1 = std::find(d->second.snake1.begin(), d->second.snake1.end(), rand_loc) != d->second.snake1.end();
								auto find_in_snake2 = std::find(d->second.snake2.begin(), d->second.snake2.end(), rand_loc) != d->second.snake2.end();
								while (find_in_apples || find_in_snake2 || find_in_snake1) {
									rand_loc = glm::vec2(float(dist_x(eng)), float(dist_y(eng)));;
									find_in_apples = std::find(d->second.apples.begin(), d->second.apples.end(), rand_loc) != d->second.apples.end();
									find_in_snake1 = std::find(d->second.snake1.begin(), d->second.snake1.end(), rand_loc) != d->second.snake1.end();
									find_in_snake2 = std::find(d->second.snake2.begin(), d->second.snake2.end(), rand_loc) != d->second.snake2.end();
								}

								d->second.apples.emplace_back(rand_loc);
								std::cout << "apple loc:" <<  rand_loc.x << "," << rand_loc.y <<std::endl;

							}

					}

				} else {
					std::cout << c << ": Got weird data from id " << player.id << std::endl;
				}
			}

			{
				//TODO potential to reduce data sent over network
				//			by casing by which player current conn is

				//send snake data, apples, and game status of other player
				std::cout << "16 " << c << std::endl;
				SerializedState temp_ss{};
				auto f =  data.find(c);
				assert(f != data.end());
				PlayerData &player = f->second;
				//calculate index into game_data
				std::cout << "17.0 " << c << std::endl;
				uint32_t game_id = (player.id % 2 == 0) ? player.id : player.id - 1;
				std::cout << "17.4" << std::endl;
				auto d = game_data.find(game_id);
				std::cout << "17.5" << std::endl;

				state_to_serialized(&d->second, &temp_ss, player.id, false);
				std::cout << "17.6" << std::endl;
				c->send_raw("s", 1);
				std::cout << "17.7" << std::endl;
				c->send_raw(&temp_ss, sizeof(SerializedState));
				std::cout << "18" << std::endl;
			}

		});

		}
}
