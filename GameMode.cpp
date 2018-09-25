#include "GameMode.hpp"
//TODO
#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>


Load< MeshBuffer > meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("paddle-ball.pnc"));
});

Load< GLuint > meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(vertex_color_program->program));
});

Scene::Transform *paddle_transform = nullptr;
Scene::Transform *ball_transform = nullptr;

Scene::Camera *camera = nullptr;

Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;
	//load transform hierarchy:
	ret->load(data_path("paddle-ball.scene"), [](Scene &s, Scene::Transform *t, std::string const &m){
		Scene::Object *obj = s.new_object(t);

		obj->program = vertex_color_program->program;
		obj->program_mvp_mat4  = vertex_color_program->object_to_clip_mat4;
		obj->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		obj->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

		MeshBuffer::Mesh const &mesh = meshes->lookup(m);
		obj->vao = *meshes_for_vertex_color_program;
		obj->start = mesh.start;
		obj->count = mesh.count;
	});

	//look up paddle and ball transforms:
	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		if (t->name == "Ball1") {
			if (ball_transform) throw std::runtime_error("Multiple 'Ball' transforms in scene.");
			ball_transform = t;
		}
	}
	if (!ball_transform) throw std::runtime_error("No 'Ball' transform in scene.");

	//look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");
	return ret;
});

GameMode::GameMode(Client &client_) : client(client_) {
	client.connection.send_raw("h", 1); //send a 'hello' to the server
}

GameMode::~GameMode() {
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.scancode == SDL_SCANCODE_W || evt.key.keysym.scancode == SDL_SCANCODE_UP) {
				if (state.id % 2 == 0 && state.snake1_direction != DOWN) { //if you're player 1
					state.snake1_direction = UP;
				} else if (state.id % 2 == 1 && state.snake1_direction != DOWN) {
					state.snake2_direction = UP;
				}
				return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_S || evt.key.keysym.scancode == SDL_SCANCODE_DOWN) {
				if (state.id % 2 == 0 && state.snake1_direction != UP) { //if you're player 1
					state.snake1_direction = DOWN;
				} else if (state.id % 2 == 1 && state.snake1_direction != UP) {
					state.snake2_direction = DOWN;
				}
				return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_A || evt.key.keysym.scancode == SDL_SCANCODE_LEFT) {
				if (state.id % 2 == 0 && state.snake1_direction != RIGHT) { //if you're player 1
					state.snake1_direction = LEFT;
				} else if (state.id % 2 == 1 && state.snake1_direction != RIGHT) {
					state.snake2_direction = LEFT;
				}
				return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_D || evt.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
				if (state.id % 2 == 0 && state.snake1_direction != LEFT) { //if you're player 1
					state.snake1_direction = RIGHT;
				} else if (state.id % 2 == 1 && state.snake1_direction != LEFT) {
					state.snake2_direction = RIGHT;
				}
				return true;
		}
	}

	return false;
}

void GameMode::update(float elapsed) {
	if (state.gameOver && state.playerOne) {
		std::string message = "PLAYER ONE WINS";
		/**
		float height = 0.06f;
		float width = text_width(message, height);
		draw_text(message, glm::vec2(-0.5f * width,-0.99f), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
		draw_text(message, glm::vec2(-0.5f * width,-1.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		glUseProgram(0);
		**/
		return;
	} else if (state.gameOver && !state.playerOne) {
		std::string message = "PLAYER TWO WINS";
		/**
		float height = 0.06f;
		float width = text_width(message, height);
		draw_text(message, glm::vec2(-0.5f * width,-0.99f), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
		draw_text(message, glm::vec2(-0.5f * width,-1.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		glUseProgram(0);
		**/
		return;
	}

	 else {
		if (state.gameStarted) { //update only if game started
			std::cout << "1" << std::endl;
			state.update(elapsed);
		}

		if (client.connection) {
			std::cout << "2" << std::endl;
			//send game state to server
			ServerState temp_serverstate{};
			temp_serverstate.snake1 = state.snake1;
			temp_serverstate.snake2 = state.snake2;
			temp_serverstate.snake1_direction = state.snake1_direction;
			temp_serverstate.snake2_direction = state.snake2_direction;
			temp_serverstate.apples = state.apples;
			temp_serverstate.gameOver = state.gameOver;
			temp_serverstate.playerOne  = state.playerOne;
			std::cout << "3" << std::endl;
			SerializedState temp_serial{};
			state_to_serialized(&temp_serverstate, &temp_serial, state.id, true);
			std::cout << "4" << std::endl;
			client.connection.send_raw("s", 1);
			client.connection.send_raw(&temp_serial, sizeof(SerializedState));
			std::cout << "5" << std::endl;
		}

		//get state
		client.poll([&](Connection *c, Connection::Event event){
			if (event == Connection::OnOpen) {
				//probably won't get this.
			} else if (event == Connection::OnClose) {
				std::cerr << "Lost connection to server." << std::endl;
			} else { assert(event == Connection::OnRecv); //Recieve bytes from server
				std::cout << "6" << std::endl;
				if (c->recv_buffer[0] == 's') {
					if (c->recv_buffer.size() < 1 + sizeof(SerializedState)) {
						return; //wait for more data
					} else {
						std::cout << "7" << std::endl;
						SerializedState temp_ss{};
						std::cout << "from outside" << &temp_ss << std::endl;
						ServerState temp_serverstate{};
						memcpy(&temp_ss, c->recv_buffer.data() + 1, sizeof(SerializedState));
						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 +sizeof(SerializedState));
						std::cout << "8" << std::endl;
						state.id = serialized_to_state(&temp_ss, &temp_serverstate, true, -5);
						std::cout << "8.5" << std::endl;
						//Save state you recieved
						state.apples = temp_serverstate.apples;
						state.gameOver = temp_serverstate.gameOver;
						state.playerOne = temp_serverstate.playerOne;
						std::cout << "9" << std::endl;
						if (state.id % 2 == 0) { //if you're player one
							state.snake2 = temp_serverstate.snake2;
							state.snake2_direction = temp_serverstate.snake2_direction;
						} else {
							state.snake1 = temp_serverstate.snake1;
							state.snake1_direction = temp_serverstate.snake1_direction;
						}
						std::cout << "10" << std::endl;

					}

				}
			}
	});
}

//TODO render new balls somehow && case for snake
	//copy game state to scene positions:
	/**
	ball_transform->position.x = state.ball.x;
	ball_transform->position.y = state.ball.y;

	paddle_transform->position.x = state.paddle.x;
	paddle_transform->position.y = state.paddle.y;
	**/
}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	camera->aspect = drawable_size.x / float(drawable_size.y);

	glClearColor(0.25f, 0.0f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light positions:
	glUseProgram(vertex_color_program->program);
	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));

	scene->draw(camera);

	GL_ERRORS();
}
