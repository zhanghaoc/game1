#include "PlayMode.hpp"

// for asset loading
#include "read_write_chunk.hpp"

// for load_png()
#include "load_save_png.hpp"

// for data_path()
#include "data_path.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <cstdio>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

PlayMode::PlayMode() {
	map  = std::vector<std::vector<int>>(30, std::vector<int>(32, green));
	doorMap = std::vector<std::vector<door>>(30, std::vector<door>(32, {false, false, false, false}));
	triggers = std::vector<trigger>();
	triggerMap = std::vector<std::vector<int>>(30, std::vector<int>(32, 0));
	player_at = glm::uvec2(3, 4);

	// initialize all the tile to zero
	for (auto i = 0; i < 16 * 16; ++i) {
		ppu.tile_table[i].bit0 = {
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
		};
		ppu.tile_table[i].bit1 = {
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
			0b00000000,
		};
	}

	namespace fs = std::filesystem;
	fs::path p = data_path("");
	std::string workDir = p.parent_path().parent_path().string();
	std::string filePath = workDir + "/assets/data.bin";


	std::fstream fileStream1(filePath, std::fstream::in | std::fstream::out | std::ios::binary);
	if(!fileStream1) {
      	std::cout << "Cannot open file! at " << filePath << std::endl;
      	// exit(1);
   	}

	for (uint32_t y = 0; y < HEIGHT; ++y) {
		read_chunk<int>(fileStream1, "map0", &map[y]);		
	}
	for (uint32_t y = 0; y < HEIGHT; ++y) {
		read_chunk<door>(fileStream1, "door", &doorMap[y]);		
	}
	
	fileStream1.read(reinterpret_cast<char*>(&ppu.palette_table), sizeof(ppu.palette_table));
	fileStream1.read(reinterpret_cast<char*>(&ppu.tile_table), sizeof(ppu.tile_table));

	size_t num = 0;
	fileStream1.read(reinterpret_cast< char *>(&num), sizeof(num));
	for (unsigned int i = 0; i < num; ++i) {
		trigger t;
		fileStream1.read(reinterpret_cast< char *>(&t.x), sizeof(t.x));
		fileStream1.read(reinterpret_cast< char *>(&t.y), sizeof(t.y));
		fileStream1.read(reinterpret_cast< char *>(&t.color), sizeof(t.color));
		fileStream1.read(reinterpret_cast< char *>(&t.id), sizeof(t.id));
		fileStream1.read(reinterpret_cast< char *>(&t.dir), sizeof(t.dir));
		size_t blockNum;
		fileStream1.read(reinterpret_cast< char *>(&blockNum), sizeof(blockNum));
		t.blocks.resize(blockNum);
		read_chunk<block>(fileStream1, "trig", &t.blocks);
		triggers.push_back(t);
	}

	fileStream1.close();
	
	auto glmuvec2Equal = [=](const glm::u8vec4 v1, const glm::u8vec4 v2) {
		return v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2] && v1[3] == v2[3];
	};

	std::string portalImgPath = workDir + "/assets/portal.png";
	
	glm::uvec2 size = glm::uvec2(128, 96);
	auto pngArray = std::vector< glm::u8vec4 >(size.x * size.y);
	load_png(portalImgPath, &size, &pngArray, LowerLeftOrigin);
	unsigned int tileCnt = 0;

	// 128 * 96 -> 16 * 12 -> 0~191
	for (unsigned int y = 0; y < size.y; ++y) {
		for (unsigned int x = 0; x < size.x; ++x) {
			tileCnt = (y / 8) * 16 + (x / 8);
			// &= set to 0, |= set to 1
			if (glmuvec2Equal(pngArray[y*size.x + x],ppu.palette_table[portal_img_palette][0])) {
				ppu.tile_table[tileCnt].bit0[y % 8] &= 0b11111111 ^ (1 << (x % 8));
				ppu.tile_table[tileCnt].bit1[y % 8] &= 0b11111111 ^ (1 << (x % 8));
			}
			if (glmuvec2Equal(pngArray[y*size.x + x],ppu.palette_table[portal_img_palette][1])) {
				ppu.tile_table[tileCnt].bit0[y % 8] |= 1 << (x % 8);
				ppu.tile_table[tileCnt].bit1[y % 8] &= 0b11111111 ^ (1 << (x % 8));
			}
			if (glmuvec2Equal(pngArray[y*size.x + x],ppu.palette_table[portal_img_palette][2])) {
				ppu.tile_table[tileCnt].bit0[y % 8] &= 0b11111111 ^ (1 << (x % 8));
				ppu.tile_table[tileCnt].bit1[y % 8] |= 1 << (x % 8);
			}
			if (glmuvec2Equal(pngArray[y*size.x + x],ppu.palette_table[portal_img_palette][3])) {
				ppu.tile_table[tileCnt].bit0[y % 8] |= 1 << (x % 8);
				ppu.tile_table[tileCnt].bit1[y % 8] |= 1 << (x % 8);
			}
		}
	}
	
	for (uint32_t y = 0; y < HEIGHT; ++y) {
		for (uint32_t x = 0; x < WIDTH; ++x) {
			ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[map[y][x]];
		}
	}

	// due to the limitation of sprites number in ppu, we have to draw the doors as background
	for (uint32_t y = 0; y < HEIGHT; ++y) {
		for (uint32_t x = 0; x < WIDTH; ++x) {
			if (doorMap[y][x].left) {
				if (map[y][x] == red) {
					ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[red_left];
				} else {
					ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[green_left];
				}
				
			}
			if (doorMap[y][x].right) {
				if (map[y][x] == red) {
					ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[red_right];
				} else {
					ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[green_right];
				}
			}
			if (doorMap[y][x].up) {
				if (map[y][x] == red) {
					ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[red_up];
				} else {
					ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[green_up];
				}
			}
			if (doorMap[y][x].down) {
				if (map[y][x] == red) {
					ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[red_down];
				} else {
					ppu.background[x+PPU466::BackgroundWidth*y] = colorIndex[green_down];
				}
			}

		}
	}

	// red flag
	ppu.sprites[1].x = 28 * 8;
	ppu.sprites[1].y = 19 * 8;
	ppu.sprites[1].index =flag;
	ppu.sprites[1].attributes = 7;

	// purple flag
	ppu.sprites[2].x = 8 * 8;
	ppu.sprites[2].y = 17 * 8;
	ppu.sprites[2].index = flag;
	ppu.sprites[2].attributes = 6;

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	last_input_time += elapsed;
	if (last_input_time < cooldown_time) 
		return;
	last_input_time = 0;

	if (left.pressed) {
		unsigned int newx = (player_at.x - 1 + WIDTH) % WIDTH;
		if (map[player_at.y][player_at.x] != map[player_at.y][newx] && !doorMap[player_at.y][player_at.x].left ) {
			// cannot go through wall unless there is a door
			;
		} else if(triggerMap[player_at.y][newx] > 0) {
			// cannot go through trigger block
			;
		} else {
			player_at.x -= 1;
		}
	}
	if (right.pressed) {
		unsigned int newx = (player_at.x + 1 + WIDTH) % WIDTH;
		if (map[player_at.y][player_at.x] != map[player_at.y][newx] && !doorMap[player_at.y][player_at.x].right ) {
			;
		} else if (triggerMap[player_at.y][newx] > 0) {
			;
		} else {
			player_at.x += 1;
		}
	}
	player_at.x = (player_at.x + WIDTH) % WIDTH;

	if (down.pressed) {
		unsigned int newy = (player_at.y - 1 + HEIGHT) % HEIGHT;
		if (map[player_at.y][player_at.x] != map[newy][player_at.x] && !doorMap[player_at.y][player_at.x].down) {
			;
		} else if (triggerMap[newy][player_at.x] > 0) {
			;
		} else {
			player_at.y -= 1;
		}
	}
	if (up.pressed) {
		unsigned int newy = (player_at.y + 1 + HEIGHT) % HEIGHT;
		if (map[player_at.y][player_at.x] != map[newy][player_at.x] && !doorMap[player_at.y][player_at.x].up) {
			;
		} else if (triggerMap[newy][player_at.x] > 0) {
			;
		} else {
			player_at.y += 1;
		}
	}
	player_at.y = (player_at.y + HEIGHT) % HEIGHT;


	struct hitResult {
		int hitBlockID;
		unsigned int distance;
	};

	auto moving = [=](movingBlockGroup blockgroup) {
		hitResult hit= hitResult();
		hit.distance = 0;
		hit.hitBlockID = 1000;
		while (hit.distance < WIDTH + 10) {
			bool breakFlag = false;
			for (auto &b : blockgroup.blocks) {
				int newx = b.x;
				int newy = b.y;
				if (blockgroup.dir == dir_right) {
					newx += 1;
				} else if (blockgroup.dir == dir_left) {
					newx -= 1;
				} else if (blockgroup.dir == dir_up) {
					newy += 1;
				} else if (blockgroup.dir == dir_down) {
					newy -= 1;
				}
				newx = (newx + WIDTH) % WIDTH;
				newy = (newy + HEIGHT) % HEIGHT;
				// block may trigger another trigger, but the situation is not considered currently
				// currently the block will stop if it hits another trigger.
				bool flag = false;
				for (auto id : blockgroup.ids) {
					if (id == triggerMap[newy][newx]) {
						flag = true;
						break;
					}
				}
				if (map[b.y][b.x] == map[newy][newx] && 
					(triggerMap[newy][newx] == 0 || flag)) {
					b.x = newx;
					b.y = newy;
				} else {
					hit.hitBlockID = std::min(hit.hitBlockID, abs(triggerMap[newy][newx]));
					breakFlag = true;
				}
			}
			if (breakFlag) {
				return hit;
			}
				
			hit.distance += 1;
		}
		// std::printf("No obstable! should not reach here!\n");
		return hit;
	};

	if (triggerMap[player_at.y][player_at.x] < 0) {
		int triggerID = -1 * triggerMap[player_at.y][player_at.x];
		// std::printf("triggerID: %d\n", triggerID);
		movingBlockGroup blockgroup = movingBlockGroup();
		blockgroup.dir = triggers[triggerID - 1].dir;
		blockgroup.blocks.clear();
		blockgroup.ids.clear();
		blockgroup.ids.push_back(triggerID);
		if (triggerID == 7)
			blockgroup.ids.push_back(6);
		for (auto id: blockgroup.ids) {
			for (auto b: triggers[id - 1].blocks) {
				blockgroup.blocks.push_back(b);
			}
		}
		
		hitResult hit = hitResult();
		hit.distance = 1;
		
		while (hit.distance != 0 || hit.hitBlockID != 0) {
			// if return distance < 0, then the blockgroup is blocked by another block
			hit = moving(blockgroup);
						
			for (auto id : blockgroup.ids) {
				for (auto &b : triggers[id-1].blocks) {
					if (blockgroup.dir == dir_right) {
						b.x += hit.distance;
					} else if (blockgroup.dir == dir_left) {
						b.x -= hit.distance;
					} else if (blockgroup.dir == dir_up) {
						b.y += hit.distance;
					} else if (blockgroup.dir == dir_down) {
						b.y -= hit.distance;
					}
					b.x = (b.x + WIDTH) % WIDTH;
					b.y = (b.y + HEIGHT) % HEIGHT;
				}
			}
			// the blockgroup is blocked by a wall
			if (hit.hitBlockID == 0) {
				break;
			}
			blockgroup.ids.push_back(hit.hitBlockID);
			for (auto b : triggers[hit.hitBlockID - 1].blocks) {
				blockgroup.blocks.push_back(b);
			}
		}
		
	}

	if (player_at.x == 28 && player_at.y == 19) {
		// draw right portal image
		uint8_t graphicCnt = 0;
		for (uint32_t y = 1; y < 13; ++y) {
			for (uint32_t x = 16; x <= 31; ++x) {
				if (x > 23)
					ppu.background[x + PPU466::BackgroundWidth*y] = graphicCnt + (6 << 8);
				graphicCnt++;
			} 
		}
	}
	if (player_at.x == 8 && player_at.y == 17) {
		// draw left portal image
		uint8_t graphicCnt = 0;
		for (uint32_t y = 1; y < 13; ++y) {
			for (uint32_t x = 16; x <= 31; ++x) {
				if (x <= 23)
					ppu.background[x + PPU466::BackgroundWidth*y] = graphicCnt + (6 << 8);
				graphicCnt++;
			} 
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}


void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	// background color is black so if feels like there is a wall
	ppu.background_color = glm::u8vec4(0x00, 0x00, 0x00, 0xff);

	//player sprite, put player at the top
	ppu.sprites[63].x = int8_t(player_at.x * 8);
	ppu.sprites[63].y = int8_t(player_at.y * 8);
	ppu.sprites[63].index = player_shape;
	ppu.sprites[63].attributes = player_palette;

	auto drawElem = [=](int x, int y, uint8_t type, uint8_t color) {
		// in case of too many sprites
		if (sprites_index >= 63) return;
		ppu.sprites[sprites_index].x = int8_t(x * 8);
		ppu.sprites[sprites_index].y = int8_t(y * 8);
		ppu.sprites[sprites_index].index = type;
		ppu.sprites[sprites_index].attributes = color;
		sprites_index++;
	};

	sprites_index = 3;
	
	// draw trigger and block
	triggerMap = std::vector<std::vector<int>>(HEIGHT, std::vector<int>(WIDTH, 0));
	for (auto tri : triggers) {
		if (tri.dir != dir_none)
			drawElem(tri.x, tri.y, trigger_heart, tri.color);
		triggerMap[tri.y][tri.x] = -tri.id;
		for (auto b: tri.blocks) {
			triggerMap[b.y][b.x] = tri.id;
			drawElem(b.x, b.y, b.block_type, b.color);
		}
	}
	
	//--- actually draw ---
	ppu.draw(drawable_size);
}
