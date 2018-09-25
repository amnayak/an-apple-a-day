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

	//NOTE: https://stackoverflow.com/questions/14391327/how-to-get-duration-as-int-millis-and-float-seconds-from-chrono
	float MS_IN_TIC = 0.35; //# of seconds in one game tic
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::duration<float> fsec;

	std::random_device r;
  std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
  std::mt19937 eng{seed};
	//TODO: maybe un-hardcode this?? atleast remember to change this
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
	auto last_updated = Time::now();
	auto time_now = Time::now();

	while (1) {

		server.poll([&](Connection *c, Connection::Event e) {
			if (e == Connection::OnOpen) {
				assert(!data.count(c));
				uint32_t new_id = data.size();
				PlayerData p{};
				p.id = new_id;
				p.winner = false;
				data.insert(std::make_pair(c, p));
				assert(data.count(c));
				if (new_id % 2 == 0) { //if player 1, create game state
					ServerState new_state{};
					new_state.snake1.push_back(glm::vec2(1.0f, 1.0f));
					new_state.snake2.push_back(glm::vec2(19.0f, 15.0f));
					assert(new_state.snake1.size() > 0);
					assert(new_state.snake2.size() > 0);
					game_data.insert(std::make_pair(new_id, new_state));
				} else { //if player 2 , begin game
					auto d = game_data.find(new_id - 1);
					d->second.gameStarted = true;
				}
			} else if (e == Connection::OnClose) {
				auto f =  data.find(c);
				uint32_t temp_id = f->second.id;
				auto d = game_data.find((temp_id % 2 == 0) ? temp_id : temp_id - 1);
				d->second.gameOver = true;
				assert(f != data.end());
				assert(d != game_data.end());
				data.erase(f);
				game_data.erase(d);
			} else if (e == Connection::OnRecv) {
				auto f =  data.find(c);
				assert(f != data.end());
				PlayerData &player = f->second;
				if (c->recv_buffer[0] == 'h') {
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
				} else if (c->recv_buffer[0] == 's') { //incoming snake data
					if (c->recv_buffer.size() < 1 + sizeof(SerializedState)) {
						return; //wait for more data
					} else {
						//calculate index into game_data
						uint32_t game_id = (player.id % 2 == 0) ? player.id : player.id - 1;
						auto d = game_data.find(game_id);
						assert(d->second.snake1.size() > 0);
						assert(d->second.snake2.size() > 0);
						// parse server data into game state
							SerializedState temp_ss{};
							memcpy(&temp_ss, c->recv_buffer.data() + 1, sizeof(SerializedState));
							c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(SerializedState));
							serialized_to_state(&temp_ss, &d->second, false, player.id);

							time_now = Time::now();
							fsec fs = time_now - last_updated;
							if (d->second.gameStarted && fs.count() >= MS_IN_TIC ){
								last_updated = Time::now();
							//update snake positions
							//insert a new position into the snake vector
							//if it didn't collide with an apple, remove last element
								d->second.snake1.insert(d->second.snake1.begin(), d->second.snake1[0] + d->second.snake1_direction);
								if ( std::find(d->second.apples.begin(), d->second.apples.end(), d->second.snake1[0]) == d->second.apples.end() ) {
									d->second.snake1.pop_back();
								} else if (d->second.snake1.size() > MAX_SNAKE_SIZE){
									d->second.snake1.pop_back();
								}
								assert(d->second.snake1.size() > 0);

								d->second.snake2.insert(d->second.snake2.begin(), d->second.snake2[0] + d->second.snake2_direction);
								if ( std::find(d->second.apples.begin(), d->second.apples.end(), d->second.snake2[0]) == d->second.apples.end() ) {
									d->second.snake2.pop_back();
								} else if (d->second.snake2.size() > MAX_SNAKE_SIZE){
									d->second.snake2.pop_back();
								}

								assert(d->second.snake2.size() > 0);

							//update game status and apple positions if needed
							glm::vec2 player_head = (player.id % 2 == 0) ? d->second.snake1[0] : d->second.snake2[0];
							//if player head intersects with apple, delete that apple from list
							auto find_apple = std::find(d->second.apples.begin(), d->second.apples.end(), player_head);
							if (find_apple != d->second.apples.end()){
								d->second.apples.erase(find_apple);
							}
							//if player head intersects with any player body, kill player head
							if (player.id % 2 == 0) { //if player 1
								//check if head intersects with other snake's body
								auto find_collision = std::find(d->second.snake2.begin(), d->second.snake2.end(), player_head);
								if (find_collision != d->second.snake2.end()){
									d->second.gameOver = true;
									d->second.playerOne = false;
								}
								//check if head intersects with your own body
								auto find_collision2 = std::find(d->second.snake1.begin() + 1, d->second.snake1.end(), player_head);
								if (find_collision2 != d->second.snake1.end()){
									d->second.gameOver = true;
									d->second.playerOne = false;
								}

							} else {									//if player 2
								//check if head intersects with other snake's body
								auto find_collision = std::find(d->second.snake1.begin(), d->second.snake1.end(), player_head);
								if (find_collision != d->second.snake1.end()){
									d->second.gameOver = true;
									d->second.playerOne = true;
								}
								//check if head intersects with your own body
								auto find_collision2 = std::find(d->second.snake2.begin() + 1, d->second.snake2.end(), player_head);
								if (find_collision2 != d->second.snake2.end()){
									d->second.gameOver = true;
									d->second.playerOne = true;
								}
							}

							//if player head intersects with any wall, kill player head
							if (player_head.x <= 0 || player_head.x >= d->second.width
									|| player_head.y <= 0 || player_head.y >= d->second.height) {
									d->second.gameOver = true;
									d->second.playerOne = (player.id % 2 == 0)? false : true;
							}
							//if # of apples is less than 5, place apples in random locations
							//		except where player bodies are located
							//NOTE: https://stackoverflow.com/questions/18726102/what-difference-between-rand-and-random-functions
							//std::cout << "apple loc:" <<  d->second.apples.size() <<std::endl;
							while (d->second.apples.size() < 5) {
								//generate random location that doesn't collide with other objects in game
								glm::vec2 rand_loc;

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

								d->second.apples.insert(d->second.apples.begin(),rand_loc);
								std::cout << "apple loc:" <<  rand_loc.x << "," << rand_loc.y <<std::endl;
								assert(d->second.apples.size() <= 5);
							}
						}

					}

				} else {
				}
			}

			{
				//TODO potential to reduce data sent over network
				//			by casing by which player current conn is

				//send snake data, apples, and game status of other player
				SerializedState temp_ss{};
				auto f =  data.find(c);
				assert(f != data.end());
				PlayerData &player = f->second;
				//calculate index into game_data
				uint32_t game_id = (player.id % 2 == 0) ? player.id : player.id - 1;
				auto d = game_data.find(game_id);

				state_to_serialized(&d->second, &temp_ss, player.id, false);
				c->send_raw("s", 1);
				c->send_raw(&temp_ss, sizeof(SerializedState));
			}

		});

		}
}
