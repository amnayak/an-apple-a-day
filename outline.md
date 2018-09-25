**GAME LOBBY**
- track player count
- first player that joins is placed (player 1), display text "WAITING FOR PLAYER 2"
- when second player joins, immediately start
- BONUS: if a third player tries to join, disconnect them/prevent them from joining
- BONUS: if a player suddenly quits, the other player is the winner

**SETUP**

Spawn 5 apples in random locations, except upper-right and lower-left
Place both players, set their directions

**GAME LOOP**

0)if # of apples is less than 5, place apples in random locations except where player bodies are located
1) make player step, render new locations
2) if player head intersects with apple, increase player size
3) if player head intersects with any player body, kill player head
4) if player head intersects with any wall, kill player head

**DATA STRUCTURES**

Directions {
  LEFT: (-1, 0)
  RIGHT: (1, 0)
  UP: (0, 1)
  DOWN: (0, -1)
}

Snake {
  Vector of x,y pairs that represent the squares of the body, with the first entry being the head
  Vector direction, representing the direction of the next step snake will take
}

Apples {
  Vector of x,y pairs that represents locations of apples
}

Winner {
  bool gameOver
  bool playerOne //true if player 1 is the winner
}

**PROCESSING PLAYER STEPS**

WASD/LRUD
--> On key press, update player snake's "direction" IFF it's not the exact opposite of the current dir
    e.g. if current dir is LEFT, you cannot set to RIGHT
--> You can check this easily since opposites are \*-1 of each other
--> Take a step on each update()
--> if you hit a wall or player body, immediately end game
--> if you hit an apple, emplace_front the position of the apple
--> if you don't hit an apple, emplace_front the new position and delete the last entry

**CLIENT CODE**

- client is in charge of:
  - it's own snake position and broadcasting it
  - stores the other snake position as well

**SERVER CODE**

- server is in charge of:
  - receiving both snake positions and sending it to other player
  - generating apple locations and giving it to both players
  - generating win/loss conditions and giving it to both players

**ASSETS**

- modeling snakes is complicated since they bend, turn around corners
- for now, snakes and apples are balls
- if there's time, color 'em
- if there's even more time insert an apple mesh and snake meshes

**SERVER-CLIENT COMMUNICATION**

1) Server starts up, waits for clients
2) When a client connects, receive 'h', send back client ID
3) If an odd-numbered client (Player 2) connects, send "gameStarted" message to both clients
4) On receiving SerializedData from client, send back updated SerializedData to client

1) When client starts up, send 'h', receive ID
2) Wait until gameStarted == true
3) On sending SerializedData to server, receive data from server
