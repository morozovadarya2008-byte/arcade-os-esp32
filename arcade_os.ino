#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <string.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int SDA_PIN = 5;
const int SCL_PIN = 6;
const int BUTTON_LEFT = 2;
const int BUTTON_RIGHT = 3;

enum GameState { MENU, GAME_PILLOWS, GAME_FLAPPY, GAME_BREAKOUT, GAME_PONG, GAME_MAZE, GAME_HILL, GAME_DOOM };
GameState currentState = MENU;

int menuSelection = 0;
int menuOldSelection = 0;
unsigned long menuAnimStart = 0;
bool menuAnimating = false;
const unsigned long MENU_ANIM_DURATION = 250;
const int MENU_CENTER_Y = 24;

int globalScore = 0;
bool gameOver = false;

const unsigned char PROGMEM android_bmp[] = {
  0x0c, 0x30, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe,
  0x6d, 0xb6, 0x7f, 0xfe, 0x7f, 0xfe, 0x3f, 0xfc,
  0x1f, 0xf8, 0x1f, 0xf8, 0x13, 0xc8, 0x13, 0xc8
};
const unsigned char PROGMEM bird_bmp[] = {
  0x03, 0xe0, 0x0f, 0x10, 0x1d, 0x30, 0x3f, 0xf0,
  0x7f, 0xf0, 0x7c, 0x00, 0x3f, 0xe0, 0x1f, 0xc0,
  0x07, 0x00
};
// 8x12 силуэт врага
const unsigned char PROGMEM enemy_sprite[] = {
  0x3C, 0x7E, 0x7E, 0xFF, 0xFF, 0xFF,
  0xDB, 0xDB, 0xDB, 0xDB, 0x66, 0x66
};

// ================= ИГРА 1: ПОДУШКИ =================
const int playerWidth = 16;
const int playerHeight = 12;
int playerX = 0;
const int playerY = SCREEN_HEIGHT - playerHeight - 2;
int playerSpeed = 3;
const int pillowWidth = 10;
const int pillowHeight = 6;
int pillowX = 0;
int pillowY = 0;
const int pillowSpeed = 2;

// ================= ИГРА 2: FLAPPY =================
float birdY = 25.0;
float birdVelocity = 0.0;
float gravity = 0.4;
float jumpStrength = -2.5;
const int birdWidth = 16;
const int birdHeight = 9;
const int birdX = 25;
int pipeX = SCREEN_WIDTH;
int pipeGap = 26;
int pipeWidth = 12;
int pipeUpperHeight = 20;
bool scoredForThisPipe = false;

// ================= ИГРА 3: АРКАНОИД =================
const int paddleWidth = 20;
const int paddleHeight = 4;
const int ballSize = 3;
int paddleX = 54;
const int paddleY = SCREEN_HEIGHT - 6;
float ballX = 64;
float ballY = SCREEN_HEIGHT - paddleHeight - 5;
float ballVx = 1.5, ballVy = -2.0;
int brickRows = 4;
int brickCols = 8;
int brickWidth = 14;
int brickHeight = 6;
int currentLevel = 0;
int maxLevels = 3;
int bricks[4][8];

// ================= ИГРА 4: PONG =================
const int paddlePongW = 4;
const int paddlePongH = 20;
int playerPaddleY = (SCREEN_HEIGHT - paddlePongH) / 2;
float botPaddleY = (SCREEN_HEIGHT - paddlePongH) / 2.0;
float botTargetOffset = 0;
const int playerPaddleX = 4;
const int botPaddleX = SCREEN_WIDTH - 4 - paddlePongW;
float ballPongX = SCREEN_WIDTH / 2;
float ballPongY = SCREEN_HEIGHT / 2;
float ballPongVx = -2.0;
float ballPongVy = 1.4;
int playerScore = 0;
int botScore = 0;
const int winScore = 5;
const float botSpeed = 0.9;

// ================= ИГРА 5: 3D ЛАБИРИНТ =================
#define MAZE_W 7
#define MAZE_H 7
#define GRID_W (MAZE_W * 2 + 1)
#define GRID_H (MAZE_H * 2 + 1)
#define MINIMAP_SIZE 34
#define MINIMAP_CELL 2
#define MINIMAP_PAD 1
byte mazeGrid[GRID_H][GRID_W];
float mazePosX = 1.5;
float mazePosY = 1.5;
float mazeAngle = 0.0;
int exitGridX = 0, exitGridY = 0;
unsigned long mazeWinTime = 0;
const float MAZE_FOV = PI / 3.0;
const float MAZE_MOVE_SPEED = 0.14;
const float MAZE_ROT_SPEED = 0.10;

float zBuffer[SCREEN_WIDTH];

// ================= ИГРА 6: HILL CLIMB =================
float hw1x, hw1y, hw1vx, hw1vy;
float hw2x, hw2y, hw2vx, hw2vy;
float hillDistance = 0, hillMaxDistance = 0;
float hillFuel = 100.0;
const float HILL_FUEL_MAX = 100.0;
float hillFuelNextX = 400.0;
const float FUEL_CAN_VALUE = 50.0;
const char* hillGameOverMsg = "CRASH!";
const float HW_DIST = 12.0;
const float HW_RADIUS = 3.0;
const float HW_GRAV = 0.20;
const float HW_ACCEL = 0.35;
const float HW_REST = 0.10;
const float HW_FRICTION = 0.98;
const float HW_MAX_V = 7.0;

float hillTerrainY(float x) {
  return 40.0 + 5.0 * sin(x * 0.040) + 2.5 * sin(x * 0.110 + 1.3) + 1.2 * sin(x * 0.270 + 2.1);
}

// ================= ИГРА 7: DOOM =================
#define DOOM_MAX_ENEMIES 8
struct DoomEnemy { float x, y; int hp; int state; unsigned long lastAtk; unsigned long hitTime; };
DoomEnemy doomEnemies[DOOM_MAX_ENEMIES];
int doomEnemyCount = 0;
int doomAlive = 0;
float doomPX, doomPY, doomAngle;
float doomHP;
int doomKills;
int doomWave;
unsigned long doomShootTime = 0;
const float DOOM_MOVE_SPEED = 0.10;
const float DOOM_ROT_SPEED = 0.10;
const float DOOM_FOV = PI / 3.0;

// ================= ВСПОМОГАТЕЛЬНЫЕ =================
void waitForButtonRelease() {
  unsigned long start = millis();
  while ((digitalRead(BUTTON_LEFT) == LOW || digitalRead(BUTTON_RIGHT) == LOW) && millis() - start < 2000) delay(10);
  delay(100);
}

void spawnPillow() {
  int attempts = 0;
  do {
    pillowX = random(0, SCREEN_WIDTH - pillowWidth);
    pillowY = 0;
    attempts++;
    if (attempts > 100) break;
  } while (pillowX + pillowWidth > playerX && pillowX < playerX + playerWidth);
}

void resetPillowsGame() {
  waitForButtonRelease();
  globalScore = 0;
  playerX = (SCREEN_WIDTH - playerWidth) / 2;
  spawnPillow();
  gameOver = false;
}

void resetFlappyGame() {
  waitForButtonRelease();
  globalScore = 0;
  birdY = 25.0;
  birdVelocity = 0.0;
  pipeX = SCREEN_WIDTH;
  int minH = 10, maxH = SCREEN_HEIGHT - pipeGap - 10;
  if (maxH < minH) maxH = minH + 1;
  pipeUpperHeight = random(minH, maxH);
  scoredForThisPipe = false;
  gameOver = false;
}

void loadLevel(int level) {
  for (int r = 0; r < brickRows; r++)
    for (int c = 0; c < brickCols; c++)
      bricks[r][c] = 1;
  if (level == 1) {
    for (int r = 0; r < brickRows; r++)
      for (int c = 0; c < brickCols; c++)
        if ((r + c) % 2 == 0) bricks[r][c] = 0;
  } else if (level == 2) {
    for (int r = 0; r < brickRows; r++)
      for (int c = 0; c < brickCols; c++)
        if (c < 2 || c > brickCols - 3) bricks[r][c] = 0;
  }
}

void resetBreakoutGame() {
  waitForButtonRelease();
  globalScore = 0;
  currentLevel = 0;
  loadLevel(currentLevel);
  paddleX = (SCREEN_WIDTH - paddleWidth) / 2;
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT - paddleHeight - 5;
  ballVx = 1.5;
  ballVy = -2.0;
  gameOver = false;
}

void resetPongGame() {
  waitForButtonRelease();
  playerScore = 0;
  botScore = 0;
  playerPaddleY = (SCREEN_HEIGHT - paddlePongH) / 2;
  botPaddleY = (SCREEN_HEIGHT - paddlePongH) / 2.0;
  botTargetOffset = (float)random(-4, 5);
  ballPongX = SCREEN_WIDTH / 2;
  ballPongY = SCREEN_HEIGHT / 2;
  ballPongVx = -2.0;
  ballPongVy = (random(0, 2) == 0) ? 1.4 : -1.4;
  gameOver = false;
}

void resetHillClimbGame() {
  waitForButtonRelease();
  hw1x = -HW_DIST / 2;
  hw2x =  HW_DIST / 2;
  hw1y = hillTerrainY(hw1x) - HW_RADIUS;
  hw2y = hillTerrainY(hw2x) - HW_RADIUS;
  hw1vx = hw1vy = 0;
  hw2vx = hw2vy = 0;
  hillDistance = 0;
  hillMaxDistance = 0;
  hillFuel = HILL_FUEL_MAX;
  hillFuelNextX = 400.0;
  hillGameOverMsg = "CRASH!";
  gameOver = false;
}

void drawGameOverScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.print("GAME OVER");
  display.setTextSize(1);
  display.setCursor(35, 32);
  display.print("Score: ");
  display.print(globalScore);
  display.setCursor(5, 48);
  display.print("L: Restart");
  display.setCursor(5, 56);
  display.print("R: Main Menu");
}

// ================= ГЕНЕРАЦИЯ ЛАБИРИНТА =================
void generateMaze() {
  for (int y = 0; y < GRID_H; y++)
    for (int x = 0; x < GRID_W; x++)
      mazeGrid[y][x] = 1;
  byte visited[MAZE_H][MAZE_W];
  memset(visited, 0, sizeof(visited));
  int stackX[MAZE_W * MAZE_H], stackY[MAZE_W * MAZE_H];
  int stackSize = 0;
  int cx = 0, cy = 0;
  visited[cy][cx] = 1;
  mazeGrid[cy * 2 + 1][cx * 2 + 1] = 0;
  stackX[stackSize] = cx; stackY[stackSize] = cy; stackSize++;
  while (stackSize > 0) {
    cx = stackX[stackSize - 1]; cy = stackY[stackSize - 1];
    int neighbors[4][2]; int nc = 0;
    if (cy > 0 && !visited[cy - 1][cx])           { neighbors[nc][0] = cx;     neighbors[nc][1] = cy - 1; nc++; }
    if (cy < MAZE_H - 1 && !visited[cy + 1][cx])  { neighbors[nc][0] = cx;     neighbors[nc][1] = cy + 1; nc++; }
    if (cx > 0 && !visited[cy][cx - 1])           { neighbors[nc][0] = cx - 1; neighbors[nc][1] = cy;     nc++; }
    if (cx < MAZE_W - 1 && !visited[cy][cx + 1])  { neighbors[nc][0] = cx + 1; neighbors[nc][1] = cy;     nc++; }
    if (nc == 0) { stackSize--; continue; }
    int choice = random(nc);
    int nx = neighbors[choice][0], ny = neighbors[choice][1];
    mazeGrid[cy * 2 + 1 + (ny - cy)][cx * 2 + 1 + (nx - cx)] = 0;
    mazeGrid[ny * 2 + 1][nx * 2 + 1] = 0;
    visited[ny][nx] = 1;
    stackX[stackSize] = nx; stackY[stackSize] = ny; stackSize++;
  }
  exitGridX = (MAZE_W - 1) * 2 + 1;
  exitGridY = (MAZE_H - 1) * 2 + 1;
}

void resetMazeGame() {
  waitForButtonRelease();
  generateMaze();
  mazePosX = 1.5; mazePosY = 1.5; mazeAngle = 0.0;
  mazeWinTime = 0;
  gameOver = false;
}

void castRay(float px, float py, float angle, float &dist, bool &side) {
  float rdx = cos(angle), rdy = sin(angle);
  int mapX = (int)px, mapY = (int)py;
  float ddx = (rdx == 0) ? 1e30 : fabs(1.0 / rdx);
  float ddy = (rdy == 0) ? 1e30 : fabs(1.0 / rdy);
  int stepX, stepY; float sdx, sdy;
  if (rdx < 0) { stepX = -1; sdx = (px - mapX) * ddx; } else { stepX = 1; sdx = (mapX + 1.0 - px) * ddx; }
  if (rdy < 0) { stepY = -1; sdy = (py - mapY) * ddy; } else { stepY = 1; sdy = (mapY + 1.0 - py) * ddy; }
  side = false;
  int maxSteps = 64;
  while (maxSteps-- > 0) {
    if (sdx < sdy) { sdx += ddx; mapX += stepX; side = false; }
    else           { sdy += ddy; mapY += stepY; side = true; }
    if (mapX < 0 || mapX >= GRID_W || mapY < 0 || mapY >= GRID_H) { dist = 100.0; return; }
    if (mazeGrid[mapY][mapX] == 1) break;
  }
  dist = side ? (sdy - ddy) : (sdx - ddx);
  if (dist < 0.05) dist = 0.05;
}

void renderWallsWithZBuffer(float px, float py, float angle, float fov) {
  int horizon = SCREEN_HEIGHT / 2;
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    float a = angle - fov / 2.0 + fov * x / (float)SCREEN_WIDTH;
    float dist; bool side;
    castRay(px, py, a, dist, side);
    float perp = dist * cos(a - angle);
    if (perp < 0.05) perp = 0.05;
    zBuffer[x] = perp;
    int lineH = (int)(SCREEN_HEIGHT / perp);
    if (lineH > SCREEN_HEIGHT * 2) lineH = SCREEN_HEIGHT * 2;
    int drawStart = horizon - lineH / 2;
    int drawEnd = horizon + lineH / 2;
    if (drawStart < 0) drawStart = 0;
    if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;
    if (side || perp > 4.5) {
      for (int y = drawStart; y <= drawEnd; y++) if (((x + y) & 1) == 0) display.drawPixel(x, y, SSD1306_WHITE);
    } else if (perp > 2.0) {
      for (int y = drawStart; y <= drawEnd; y++) if (((x + y) % 3) == 0) display.drawPixel(x, y, SSD1306_WHITE);
    } else {
      display.drawFastVLine(x, drawStart, drawEnd - drawStart + 1, SSD1306_WHITE);
    }
  }
}

void renderMaze3D() { renderWallsWithZBuffer(mazePosX, mazePosY, mazeAngle, MAZE_FOV); }

void renderMinimap() {
  int mmX = SCREEN_WIDTH - MINIMAP_SIZE, mmY = 0;
  display.fillRect(mmX, mmY, MINIMAP_SIZE, MINIMAP_SIZE, SSD1306_BLACK);
  display.drawRect(mmX, mmY, MINIMAP_SIZE - 1, MINIMAP_SIZE - 1, SSD1306_WHITE);
  for (int y = 0; y < GRID_H; y++)
    for (int x = 0; x < GRID_W; x++)
      if (mazeGrid[y][x] == 1)
        display.fillRect(mmX + MINIMAP_PAD + x * MINIMAP_CELL, mmY + MINIMAP_PAD + y * MINIMAP_CELL, MINIMAP_CELL, MINIMAP_CELL, SSD1306_WHITE);
  int exPx = mmX + MINIMAP_PAD + exitGridX * MINIMAP_CELL;
  int eyPx = mmY + MINIMAP_PAD + exitGridY * MINIMAP_CELL;
  display.fillRect(exPx - 1, eyPx - 1, MINIMAP_CELL + 2, MINIMAP_CELL + 2, SSD1306_BLACK);
  bool blink = ((millis() / 300) & 1) == 0;
  if (blink) {
    int cx = exPx + MINIMAP_CELL / 2, cy = eyPx + MINIMAP_CELL / 2;
    display.drawPixel(cx, cy, SSD1306_WHITE);
    display.drawPixel(cx - 1, cy, SSD1306_WHITE);
    display.drawPixel(cx + 1, cy, SSD1306_WHITE);
    display.drawPixel(cx, cy - 1, SSD1306_WHITE);
    display.drawPixel(cx, cy + 1, SSD1306_WHITE);
  }
  int ppx = mmX + MINIMAP_PAD + (int)(mazePosX * MINIMAP_CELL);
  int ppy = mmY + MINIMAP_PAD + (int)(mazePosY * MINIMAP_CELL);
  int dx = ppx + (int)(cos(mazeAngle) * 3), dy = ppy + (int)(sin(mazeAngle) * 3);
  display.drawLine(ppx, ppy, dx, dy, SSD1306_WHITE);
  display.drawPixel(ppx, ppy, SSD1306_WHITE);
}

void mazeTryMove(float dx, float dy) {
  const float r = 0.25;
  float tx = mazePosX + dx + (dx > 0 ? r : -r);
  int gx = (int)tx, gy = (int)mazePosY;
  if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H && mazeGrid[gy][gx] == 0) mazePosX += dx;
  float ty = mazePosY + dy + (dy > 0 ? r : -r);
  gx = (int)mazePosX; gy = (int)ty;
  if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H && mazeGrid[gy][gx] == 0) mazePosY += dy;
}

// ================= ЛАБИРИНТ =================
void updateMazeGame() {
  bool curL = (digitalRead(BUTTON_LEFT) == LOW), curR = (digitalRead(BUTTON_RIGHT) == LOW);
  bool anyPressed = curL || curR;
  if (!gameOver) {
    static bool prevAny = false;
    static unsigned long pressStart = 0, windowStart = 0;
    static int tapCount = 0;
    if (anyPressed && !prevAny) pressStart = millis();
    if (!anyPressed && prevAny) {
      unsigned long dur = millis() - pressStart;
      if (dur < 400) {
        unsigned long now = millis();
        if (now - windowStart > 1500) { windowStart = now; tapCount = 1; } else tapCount++;
        if (tapCount >= 4) { tapCount = 0; prevAny = false; currentState = MENU; waitForButtonRelease(); return; }
      } else tapCount = 0;
    }
    prevAny = anyPressed;
  }
  if (!gameOver) {
    if (curL && curR) {
      mazeTryMove(cos(mazeAngle) * MAZE_MOVE_SPEED, sin(mazeAngle) * MAZE_MOVE_SPEED);
    } else if (curL) { mazeAngle -= MAZE_ROT_SPEED; if (mazeAngle < 0) mazeAngle += 2 * PI; }
    else if (curR) { mazeAngle += MAZE_ROT_SPEED; if (mazeAngle > 2 * PI) mazeAngle -= 2 * PI; }
    if ((int)mazePosX == exitGridX && (int)mazePosY == exitGridY) { gameOver = true; mazeWinTime = millis(); }
    display.clearDisplay();
    renderMaze3D();
    renderMinimap();
    static unsigned long hintStart = 0;
    if (hintStart == 0) hintStart = millis();
    if (millis() - hintStart < 5000) {
      display.fillRect(0, SCREEN_HEIGHT - 8, SCREEN_WIDTH, 8, SSD1306_BLACK);
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(1, SCREEN_HEIGHT - 8);
      display.print("L/R=turn BOTH=fwd 4xTap=exit");
    }
  } else {
    if (millis() - mazeWinTime >= 2500) { currentState = MENU; waitForButtonRelease(); return; }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(20, 10);
    display.print("YOU WIN!");
    display.setTextSize(1);
    display.setCursor(30, 34);
    display.print("Found exit!");
    unsigned long elapsed = millis() - mazeWinTime;
    int barW = (int)((elapsed * (SCREEN_WIDTH - 20)) / 2500);
    if (barW > SCREEN_WIDTH - 20) barW = SCREEN_WIDTH - 20;
    display.drawRect(10, 48, SCREEN_WIDTH - 20, 6, SSD1306_WHITE);
    display.fillRect(10, 48, barW, 6, SSD1306_WHITE);
    display.setCursor(28, 57);
    display.print("Returning...");
  }
}

// ================= ИГРЫ 1-4 =================
void updatePillows() {
  if (!gameOver) {
    if (digitalRead(BUTTON_LEFT) == LOW) { playerX -= playerSpeed; if (playerX < 0) playerX = 0; }
    if (digitalRead(BUTTON_RIGHT) == LOW) { playerX += playerSpeed; if (playerX > SCREEN_WIDTH - playerWidth) playerX = SCREEN_WIDTH - playerWidth; }
    pillowY += pillowSpeed;
    if (pillowY + pillowHeight >= playerY && pillowY <= playerY + playerHeight)
      if (pillowX + pillowWidth >= playerX && pillowX <= playerX + playerWidth) { globalScore++; spawnPillow(); }
    if (pillowY > SCREEN_HEIGHT) gameOver = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.drawBitmap(playerX, playerY, android_bmp, playerWidth, playerHeight, SSD1306_WHITE);
    display.fillRoundRect(pillowX, pillowY, pillowWidth, pillowHeight, 2, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Score: "); display.print(globalScore);
  } else {
    drawGameOverScreen();
    if (digitalRead(BUTTON_LEFT) == LOW) resetPillowsGame();
    else if (digitalRead(BUTTON_RIGHT) == LOW) { currentState = MENU; waitForButtonRelease(); }
  }
}

void updateFlappy() {
  if (!gameOver) {
    if (digitalRead(BUTTON_LEFT) == LOW) birdVelocity = jumpStrength;
    birdVelocity += gravity;
    birdY += birdVelocity;
    pipeX -= 2;
    if (pipeX + pipeWidth < birdX && !scoredForThisPipe) { globalScore++; scoredForThisPipe = true; }
    if (pipeX + pipeWidth < 0) {
      pipeX = SCREEN_WIDTH;
      int minH = 10, maxH = SCREEN_HEIGHT - pipeGap - 10;
      if (maxH < minH) maxH = minH + 1;
      pipeUpperHeight = random(minH, maxH);
      scoredForThisPipe = false;
    }
    if (birdY < 0 || birdY + birdHeight > SCREEN_HEIGHT) gameOver = true;
    if (pipeX < birdX + birdWidth && pipeX + pipeWidth > birdX)
      if (birdY < pipeUpperHeight || birdY + birdHeight > pipeUpperHeight + pipeGap) gameOver = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.drawBitmap(birdX, (int)birdY, bird_bmp, birdWidth, birdHeight, SSD1306_WHITE);
    display.fillRect(pipeX, 0, pipeWidth, pipeUpperHeight, SSD1306_WHITE);
    display.fillRect(pipeX, pipeUpperHeight + pipeGap, pipeWidth, SCREEN_HEIGHT - (pipeUpperHeight + pipeGap), SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Score: "); display.print(globalScore);
  } else {
    drawGameOverScreen();
    if (digitalRead(BUTTON_LEFT) == LOW) resetFlappyGame();
    else if (digitalRead(BUTTON_RIGHT) == LOW) { currentState = MENU; waitForButtonRelease(); }
  }
}

void updateBreakout() {
  if (!gameOver) {
    if (digitalRead(BUTTON_LEFT) == LOW) { paddleX -= 3; if (paddleX < 0) paddleX = 0; }
    if (digitalRead(BUTTON_RIGHT) == LOW) { paddleX += 3; if (paddleX > SCREEN_WIDTH - paddleWidth) paddleX = SCREEN_WIDTH - paddleWidth; }
    ballX += ballVx; ballY += ballVy;
    if (ballX < 0 || ballX > SCREEN_WIDTH - ballSize) ballVx = -ballVx;
    if (ballY < 0) ballVy = -ballVy;
    if (ballY > SCREEN_HEIGHT) gameOver = true;
    if (ballY + ballSize >= paddleY && ballY <= paddleY + paddleHeight)
      if (ballX + ballSize >= paddleX && ballX <= paddleX + paddleWidth) {
        ballVy = -ballVy;
        float hitPos = (ballX + ballSize / 2) - (paddleX + paddleWidth / 2);
        ballVx = hitPos / (paddleWidth / 2) * 2.0;
        if (abs(ballVx) < 0.5) ballVx = 0;
        ballY = paddleY - ballSize;
      }
    for (int r = 0; r < brickRows; r++)
      for (int c = 0; c < brickCols; c++) {
        if (bricks[r][c] == 0) continue;
        int bx = c * (brickWidth + 2) + 2, by = r * (brickHeight + 2) + 10;
        if (ballX + ballSize > bx && ballX < bx + brickWidth && ballY + ballSize > by && ballY < by + brickHeight) {
          bricks[r][c] = 0;
          float ovX = min(ballX + ballSize - bx, bx + brickWidth - ballX);
          float ovY = min(ballY + ballSize - by, by + brickHeight - ballY);
          if (ovX < ovY) { ballVx = -ballVx; if (ballX < bx + brickWidth / 2) ballX = bx - ballSize; else ballX = bx + brickWidth; }
          else { ballVy = -ballVy; if (ballY < by + brickHeight / 2) ballY = by - ballSize; else ballY = by + brickHeight; }
          globalScore++;
          break;
        }
      }
    bool allCleared = true;
    for (int r = 0; r < brickRows && allCleared; r++)
      for (int c = 0; c < brickCols; c++)
        if (bricks[r][c] == 1) { allCleared = false; break; }
    if (allCleared) {
      currentLevel++;
      if (currentLevel >= maxLevels) { currentState = MENU; waitForButtonRelease(); return; }
      loadLevel(currentLevel);
      ballX = SCREEN_WIDTH / 2;
      ballY = SCREEN_HEIGHT - paddleHeight - 5;
      ballVx = 1.5 + currentLevel * 0.2;
      ballVy = -2.0 - currentLevel * 0.2;
      paddleX = (SCREEN_WIDTH - paddleWidth) / 2;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.fillRect(paddleX, paddleY, paddleWidth, paddleHeight, SSD1306_WHITE);
    display.fillCircle((int)ballX, (int)ballY, ballSize, SSD1306_WHITE);
    for (int r = 0; r < brickRows; r++)
      for (int c = 0; c < brickCols; c++)
        if (bricks[r][c]) display.fillRect(c * (brickWidth + 2) + 2, r * (brickHeight + 2) + 10, brickWidth, brickHeight, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0); display.print("Score:"); display.print(globalScore);
    display.setCursor(80, 0); display.print("Lvl:"); display.print(currentLevel + 1);
  } else {
    drawGameOverScreen();
    if (digitalRead(BUTTON_LEFT) == LOW) resetBreakoutGame();
    else if (digitalRead(BUTTON_RIGHT) == LOW) { currentState = MENU; waitForButtonRelease(); }
  }
}

void updatePong() {
  if (digitalRead(BUTTON_LEFT) == LOW && digitalRead(BUTTON_RIGHT) == LOW) {
    currentState = MENU; waitForButtonRelease(); return;
  }
  if (!gameOver) {
    if (digitalRead(BUTTON_LEFT) == LOW) { playerPaddleY -= 2; if (playerPaddleY < 0) playerPaddleY = 0; }
    if (digitalRead(BUTTON_RIGHT) == LOW) { playerPaddleY += 2; if (playerPaddleY > SCREEN_HEIGHT - paddlePongH) playerPaddleY = SCREEN_HEIGHT - paddlePongH; }
    float botCenter = botPaddleY + paddlePongH / 2.0;
    float targetY = ballPongY + 1.5 + botTargetOffset;
    bool botActive = (ballPongVx > 0) || (ballPongX > SCREEN_WIDTH * 0.55);
    if (botActive) {
      if (botCenter < targetY - 2.0) botPaddleY += botSpeed;
      else if (botCenter > targetY + 2.0) botPaddleY -= botSpeed;
    }
    if (botPaddleY < 0) botPaddleY = 0;
    if (botPaddleY > SCREEN_HEIGHT - paddlePongH) botPaddleY = SCREEN_HEIGHT - paddlePongH;
    ballPongX += ballPongVx;
    ballPongY += ballPongVy;
    if (fabs(ballPongVx) < 1.2) ballPongVx = (ballPongVx >= 0) ? 1.2 : -1.2;
    if (fabs(ballPongVy) < 0.7) ballPongVy = (ballPongVy >= 0) ? 0.7 : -0.7;
    if (ballPongY < 0) { ballPongY = 0; ballPongVy = fabs(ballPongVy); if (ballPongVy < 0.7) ballPongVy = 0.7; }
    if (ballPongY + 3 > SCREEN_HEIGHT) { ballPongY = SCREEN_HEIGHT - 3; ballPongVy = -fabs(ballPongVy); if (ballPongVy > -0.7) ballPongVy = -0.7; }
    if (ballPongX + 3 < 0) {
      botScore++;
      if (botScore >= winScore) gameOver = true;
      else { ballPongX = SCREEN_WIDTH / 2; ballPongY = SCREEN_HEIGHT / 2;
             ballPongVx = 2.0; ballPongVy = (random(0, 2) == 0) ? 1.4 : -1.4;
             botTargetOffset = (float)random(-4, 5); }
    } else if (ballPongX > SCREEN_WIDTH) {
      playerScore++;
      if (playerScore >= winScore) gameOver = true;
      else { ballPongX = SCREEN_WIDTH / 2; ballPongY = SCREEN_HEIGHT / 2;
             ballPongVx = -2.0; ballPongVy = (random(0, 2) == 0) ? 1.4 : -1.4;
             botTargetOffset = (float)random(-4, 5); }
    }
    if (ballPongVx < 0 && ballPongX < playerPaddleX + paddlePongW && ballPongX + 3 > playerPaddleX &&
        ballPongY + 3 > playerPaddleY && ballPongY < playerPaddleY + paddlePongH) {
      ballPongVx = fabs(ballPongVx);
      float rel = ((ballPongY + 1.5) - (playerPaddleY + paddlePongH / 2.0)) / (paddlePongH / 2.0);
      if (rel > 1.0) rel = 1.0; if (rel < -1.0) rel = -1.0;
      ballPongVy = rel * 2.5;
      if (fabs(ballPongVy) < 0.9) ballPongVy = (rel >= 0) ? 0.9 : -0.9;
      ballPongX = playerPaddleX + paddlePongW + 1;
    }
    if (ballPongVx > 0 && ballPongX + 3 > botPaddleX && ballPongX < botPaddleX + paddlePongW &&
        ballPongY + 3 > botPaddleY && ballPongY < botPaddleY + paddlePongH) {
      ballPongVx = -fabs(ballPongVx);
      float rel = ((ballPongY + 1.5) - (botPaddleY + paddlePongH / 2.0)) / (paddlePongH / 2.0);
      if (rel > 1.0) rel = 1.0; if (rel < -1.0) rel = -1.0;
      ballPongVy = rel * 2.5;
      if (fabs(ballPongVy) < 0.9) ballPongVy = (rel >= 0) ? 0.9 : -0.9;
      ballPongX = botPaddleX - 4;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    for (int y = 0; y < SCREEN_HEIGHT; y += 6) display.drawPixel(SCREEN_WIDTH / 2, y, SSD1306_WHITE);
    display.fillRect(playerPaddleX, playerPaddleY, paddlePongW, paddlePongH, SSD1306_WHITE);
    display.fillRect(botPaddleX, (int)botPaddleY, paddlePongW, paddlePongH, SSD1306_WHITE);
    display.fillRect((int)ballPongX, (int)ballPongY, 3, 3, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(10, 0); display.print(playerScore);
    display.setCursor(SCREEN_WIDTH - 20, 0); display.print(botScore);
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.print(playerScore >= winScore ? "YOU WIN!" : "YOU LOSE");
    display.setTextSize(1);
    display.setCursor(35, 32);
    display.print("Score: "); display.print(playerScore); display.print(" - "); display.print(botScore);
    display.setCursor(5, 48); display.print("L: Restart");
    display.setCursor(5, 56); display.print("R: Main Menu");
    if (digitalRead(BUTTON_LEFT) == LOW) resetPongGame();
    else if (digitalRead(BUTTON_RIGHT) == LOW) { currentState = MENU; waitForButtonRelease(); }
  }
}

// ================= HILL CLIMB =================
void hwApplyGas(float &vx, float &vy, float x, float force) {
  float slope = hillTerrainY(x + 0.5) - hillTerrainY(x - 0.5);
  float len = sqrt(1.0 + slope * slope);
  vx += force * (1.0 / len);
  vy += force * (slope / len);
}
void hwApplyBrake(float &vx, float amount) { if (vx > amount) vx -= amount; else if (vx > 0) vx = 0; }
void hwCollideWheel(float &x, float &y, float &vx, float &vy) {
  float ty = hillTerrainY(x);
  if (y + HW_RADIUS > ty) {
    y = ty - HW_RADIUS;
    float slope = hillTerrainY(x + 0.5) - hillTerrainY(x - 0.5);
    float nl = sqrt(1.0 + slope * slope);
    float Nx = slope / nl, Ny = -1.0 / nl, Tx = 1.0 / nl, Ty = slope / nl;
    float Vn = vx * Nx + vy * Ny, Vt = vx * Tx + vy * Ty;
    if (Vn < 0) Vn = -Vn * HW_REST;
    Vt *= HW_FRICTION;
    vx = Vn * Nx + Vt * Tx; vy = Vn * Ny + Vt * Ty;
  }
}
void hwSolveRod() {
  float dx = hw2x - hw1x, dy = hw2y - hw1y;
  float d = sqrt(dx * dx + dy * dy);
  if (d < 0.01) return;
  float diff = (d - HW_DIST) / d / 2.0;
  hw1x += dx * diff; hw1y += dy * diff;
  hw2x -= dx * diff; hw2y -= dy * diff;
}
void hwClampSpeed(float &vx, float &vy) {
  float s2 = vx * vx + vy * vy;
  if (s2 > HW_MAX_V * HW_MAX_V) { float f = HW_MAX_V / sqrt(s2); vx *= f; vy *= f; }
}
bool hwOnGround(float x, float y) { return (y + HW_RADIUS) >= hillTerrainY(x) - 1.5; }

void updateHillClimb() {
  if (gameOver) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.print(hillGameOverMsg);
    display.setTextSize(1);
    display.setCursor(20, 34); display.print("Dist: "); display.print((int)hillDistance);
    display.setCursor(20, 44); display.print("Best: "); display.print((int)hillMaxDistance);
    display.setCursor(5, 48); display.print("L: Restart");
    display.setCursor(5, 56); display.print("R: Main Menu");
    if (digitalRead(BUTTON_LEFT) == LOW) resetHillClimbGame();
    else if (digitalRead(BUTTON_RIGHT) == LOW) { currentState = MENU; waitForButtonRelease(); }
    return;
  }
  bool curL = digitalRead(BUTTON_LEFT) == LOW, curR = digitalRead(BUTTON_RIGHT) == LOW;
  if (curL && curR) { currentState = MENU; waitForButtonRelease(); return; }
  hw1vy += HW_GRAV; hw2vy += HW_GRAV;
  if (curL) {
    if (hwOnGround(hw1x, hw1y)) hwApplyGas(hw1vx, hw1vy, hw1x, HW_ACCEL);
    if (hwOnGround(hw2x, hw2y)) hwApplyGas(hw2vx, hw2vy, hw2x, HW_ACCEL * 0.55);
  }
  if (curR) {
    if (hwOnGround(hw1x, hw1y)) hwApplyBrake(hw1vx, 0.10);
    if (hwOnGround(hw2x, hw2y)) hwApplyBrake(hw2vx, 0.10);
  }
  hw1x += hw1vx; hw1y += hw1vy;
  hw2x += hw2vx; hw2y += hw2vy;
  hwCollideWheel(hw1x, hw1y, hw1vx, hw1vy);
  hwCollideWheel(hw2x, hw2y, hw2vx, hw2vy);
  hwSolveRod();
  hwCollideWheel(hw1x, hw1y, hw1vx, hw1vy);
  hwCollideWheel(hw2x, hw2y, hw2vx, hw2vy);
  hwClampSpeed(hw1vx, hw1vy); hwClampSpeed(hw2vx, hw2vy);
  float centerX = (hw1x + hw2x) / 2.0, centerY = (hw1y + hw2y) / 2.0;
  if (centerX > hillDistance) hillDistance = centerX;
  if (centerX > hillMaxDistance) hillMaxDistance = centerX;
  float drain = 0.015;
  if (curL) drain += 0.105;
  hillFuel -= drain;
  if (hillFuel <= 0) { hillFuel = 0; gameOver = true; hillGameOverMsg = "NO FUEL"; }
  if (centerX + 6 > hillFuelNextX && centerX - 6 < hillFuelNextX + 6) {
    hillFuel += FUEL_CAN_VALUE;
    if (hillFuel > HILL_FUEL_MAX) hillFuel = HILL_FUEL_MAX;
    hillFuelNextX += (float)random(400, 900);
  }
  float dxr = hw2x - hw1x, dyr = hw2y - hw1y;
  float lenr = sqrt(dxr * dxr + dyr * dyr);
  if (lenr > 0.01) {
    float px = dyr / lenr, py = -dxr / lenr;
    float headX = centerX + px * 9.0, headY = centerY + py * 9.0;
    if (headY > hillTerrainY(headX)) { gameOver = true; hillGameOverMsg = "CRASH!"; }
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  float camX = centerX, camY = centerY;
  for (int sx = 0; sx < SCREEN_WIDTH; sx++) {
    float wx = camX + (sx - 32);
    int sy = (int)(hillTerrainY(wx) - camY + 32);
    if (sy < 0) sy = 0;
    if (sy > SCREEN_HEIGHT) sy = SCREEN_HEIGHT;
    for (int yy = sy; yy < SCREEN_HEIGHT; yy++)
      if (((sx + yy) & 1) == 0) display.drawPixel(sx, yy, SSD1306_WHITE);
  }
  int canSX = (int)(hillFuelNextX - camX + 32);
  int canBaseY = (int)(hillTerrainY(hillFuelNextX) - camY + 32);
  if (canSX > -10 && canSX < SCREEN_WIDTH + 10) {
    bool blink = ((millis() / 250) & 1) == 0;
    int canTop = canBaseY - 9;
    if (blink) {
      display.drawRect(canSX, canTop, 6, 9, SSD1306_WHITE);
      display.drawPixel(canSX + 2, canTop - 1, SSD1306_WHITE);
      display.drawPixel(canSX + 3, canTop - 1, SSD1306_WHITE);
      display.drawLine(canSX + 1, canTop + 1, canSX + 4, canTop + 4, SSD1306_WHITE);
      display.drawLine(canSX + 4, canTop + 1, canSX + 1, canTop + 4, SSD1306_WHITE);
    } else display.drawRect(canSX, canTop, 6, 9, SSD1306_WHITE);
  }
  int s1x = (int)(hw1x - camX + 32), s1y = (int)(hw1y - camY + 32);
  int s2x = (int)(hw2x - camX + 32), s2y = (int)(hw2y - camY + 32);
  int scx = (s1x + s2x) / 2, scy = (s1y + s2y) / 2;
  float ux = (hw2x - hw1x) / HW_DIST, uy = (hw2y - hw1y) / HW_DIST;
  float nx = uy, ny = -ux;
  int b1ax = s1x + (int)(nx * 3), b1ay = s1y + (int)(ny * 3);
  int b1bx = s2x + (int)(nx * 3), b1by = s2y + (int)(ny * 3);
  int b2ax = s1x - (int)(nx * 3), b2ay = s1y - (int)(ny * 3);
  int b2bx = s2x - (int)(nx * 3), b2by = s2y - (int)(ny * 3);
  display.drawLine(b1ax, b1ay, b1bx, b1by, SSD1306_WHITE);
  display.drawLine(b2ax, b2ay, b2bx, b2by, SSD1306_WHITE);
  display.drawLine(b1ax, b1ay, b2ax, b2ay, SSD1306_WHITE);
  display.drawLine(b1bx, b1by, b2bx, b2by, SSD1306_WHITE);
  int hx = scx + (int)(nx * 8), hy = scy + (int)(ny * 8);
  display.drawLine(scx - 3, scy, hx, hy, SSD1306_WHITE);
  display.drawLine(scx + 3, scy, hx, hy, SSD1306_WHITE);
  display.fillCircle(s1x, s1y, HW_RADIUS, SSD1306_WHITE);
  display.fillCircle(s2x, s2y, HW_RADIUS, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0); display.print("D:"); display.print((int)hillDistance);
  bool fuelWarn = (hillFuel < 20) && ((millis() / 200) & 1);
  if (!fuelWarn) { display.setCursor(48, 0); display.print("F:"); display.print((int)hillFuel); }
  int distToNext = (int)(hillFuelNextX - centerX);
  if (distToNext < 0) distToNext = 0;
  display.setCursor(90, 0); display.print("N:"); display.print(distToNext);
  int barFill = (int)((hillFuel / HILL_FUEL_MAX) * 60);
  if (barFill < 0) barFill = 0;
  display.drawRect(0, 10, 62, 5, SSD1306_WHITE);
  if (barFill > 0) display.fillRect(1, 11, barFill, 3, SSD1306_WHITE);
}

// ================= DOOM (ИСПРАВЛЕННЫЙ СПАВН) =================
void spawnDoomEnemies(int n) {
  doomEnemyCount = 0;

  // Основной проход: ищем случайные свободные клетки не слишком близко и не слишком далеко
  int attempts = 0;
  while (doomEnemyCount < n && doomEnemyCount < DOOM_MAX_ENEMIES && attempts < 500) {
    attempts++;
    int gx = random(1, GRID_W - 1);
    int gy = random(1, GRID_H - 1);
    if (mazeGrid[gy][gx] != 0) continue;
    float dx = gx + 0.5 - doomPX;
    float dy = gy + 0.5 - doomPY;
    float d2 = dx * dx + dy * dy;
    if (d2 < 4) continue;      // не вплотную (2 клетки мин)
    if (d2 > 180) continue;    // не в самом дальнем углу
    bool tooClose = false;
    for (int i = 0; i < doomEnemyCount; i++) {
      float ex = doomEnemies[i].x - (gx + 0.5);
      float ey = doomEnemies[i].y - (gy + 0.5);
      if (ex * ex + ey * ey < 2) { tooClose = true; break; }
    }
    if (tooClose) continue;
    doomEnemies[doomEnemyCount].x = gx + 0.5;
    doomEnemies[doomEnemyCount].y = gy + 0.5;
    doomEnemies[doomEnemyCount].hp = 2;
    doomEnemies[doomEnemyCount].state = 0;
    doomEnemies[doomEnemyCount].lastAtk = 0;
    doomEnemies[doomEnemyCount].hitTime = 0;
    doomEnemyCount++;
  }

  // FALLBACK: если враги не заспавнились — идём по всей карте и берём первые свободные клетки
  if (doomEnemyCount < n) {
    for (int gy = 1; gy < GRID_H - 1 && doomEnemyCount < n; gy++) {
      for (int gx = 1; gx < GRID_W - 1 && doomEnemyCount < n; gx++) {
        if (mazeGrid[gy][gx] != 0) continue;
        if (gx == 1 && gy == 1) continue;  // не на игроке
        bool already = false;
        for (int i = 0; i < doomEnemyCount; i++) {
          if ((int)doomEnemies[i].x == gx && (int)doomEnemies[i].y == gy) { already = true; break; }
        }
        if (already) continue;
        doomEnemies[doomEnemyCount].x = gx + 0.5;
        doomEnemies[doomEnemyCount].y = gy + 0.5;
        doomEnemies[doomEnemyCount].hp = 2;
        doomEnemies[doomEnemyCount].state = 0;
        doomEnemies[doomEnemyCount].lastAtk = 0;
        doomEnemies[doomEnemyCount].hitTime = 0;
        doomEnemyCount++;
      }
    }
  }

  doomAlive = doomEnemyCount;
}

void resetDoomGame() {
  waitForButtonRelease();
  generateMaze();
  doomPX = 1.5;
  doomPY = 1.5;

  // Выбираем стартовый угол так, чтобы впереди был открытый коридор
  doomAngle = 0;                       // вправо
  if (mazeGrid[1][2] == 1) {           // справа стена
    if (mazeGrid[2][1] == 0) doomAngle = PI / 2;  // вниз открыто
    else doomAngle = PI;              // fallback — влево
  }

  doomHP = 100;
  doomKills = 0;
  doomWave = 1;
  doomShootTime = 0;
  gameOver = false;
  spawnDoomEnemies(3);
}

void doomTryMove(float dx, float dy) {
  const float r = 0.25;
  float tx = doomPX + dx + (dx > 0 ? r : -r);
  int gx = (int)tx, gy = (int)doomPY;
  if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H && mazeGrid[gy][gx] == 0) doomPX += dx;
  float ty = doomPY + dy + (dy > 0 ? r : -r);
  gx = (int)doomPX; gy = (int)ty;
  if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H && mazeGrid[gy][gx] == 0) doomPY += dy;
}

void doomShoot() {
  doomShootTime = millis();
  float dirX = cos(doomAngle), dirY = sin(doomAngle);
  float bestDist = 100;
  int hitIdx = -1;
  for (int i = 0; i < doomEnemyCount; i++) {
    if (doomEnemies[i].state != 0) continue;
    float dx = doomEnemies[i].x - doomPX;
    float dy = doomEnemies[i].y - doomPY;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist > bestDist) continue;
    float dot = (dx * dirX + dy * dirY) / dist;
    if (dot < 0.93) continue;
    float losDist; bool s;
    castRay(doomPX, doomPY, atan2(dy, dx), losDist, s);
    if (losDist < dist - 0.3) continue;
    bestDist = dist;
    hitIdx = i;
  }
  if (hitIdx >= 0) {
    doomEnemies[hitIdx].hp--;
    doomEnemies[hitIdx].hitTime = millis();
    if (doomEnemies[hitIdx].hp <= 0) {
      doomEnemies[hitIdx].state = 1;
      doomKills++;
    }
  }
}

void updateDoomEnemies() {
  unsigned long now = millis();
  for (int i = 0; i < doomEnemyCount; i++) {
    if (doomEnemies[i].state != 0) continue;
    float dx = doomPX - doomEnemies[i].x;
    float dy = doomPY - doomEnemies[i].y;
    float dist = sqrt(dx * dx + dy * dy);
    if (dist < 0.8) {
      if (now - doomEnemies[i].lastAtk > 1000) {
        doomEnemies[i].lastAtk = now;
        doomHP -= 10;
        if (doomHP <= 0) { doomHP = 0; gameOver = true; }
      }
      continue;
    }
    float a = atan2(dy, dx);
    float losDist; bool s;
    castRay(doomEnemies[i].x, doomEnemies[i].y, a, losDist, s);
    if (losDist < dist - 0.5) continue;
    float spd = 0.025;
    float nx = dx / dist * spd;
    float ny = dy / dist * spd;
    float ox = doomEnemies[i].x, oy = doomEnemies[i].y;
    if (mazeGrid[(int)oy][(int)(ox + nx)] == 0) doomEnemies[i].x = ox + nx;
    if (mazeGrid[(int)(oy + ny)][(int)doomEnemies[i].x] == 0) doomEnemies[i].y = oy + ny;
  }
}

void renderDoomSprites() {
  int order[DOOM_MAX_ENEMIES];
  float dists[DOOM_MAX_ENEMIES];
  int n = 0;
  for (int i = 0; i < doomEnemyCount; i++) {
    if (doomEnemies[i].state != 0) continue;
    float dx = doomEnemies[i].x - doomPX;
    float dy = doomEnemies[i].y - doomPY;
    dists[i] = dx * dx + dy * dy;
    order[n++] = i;
  }
  for (int i = 1; i < n; i++) {
    int key = order[i];
    float kd = dists[key];
    int j = i - 1;
    while (j >= 0 && dists[order[j]] < kd) { order[j + 1] = order[j]; j--; }
    order[j + 1] = key;
  }

  float dirX = cos(doomAngle), dirY = sin(doomAngle);
  float planeMag = 0.577;
  float planeX = -dirY * planeMag, planeY = dirX * planeMag;
  float invDet = 1.0 / (planeX * dirY - dirX * planeY);

  for (int k = 0; k < n; k++) {
    int i = order[k];
    float spriteX = doomEnemies[i].x - doomPX;
    float spriteY = doomEnemies[i].y - doomPY;
    float transformX = invDet * (dirY * spriteX - dirX * spriteY);
    float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
    if (transformY <= 0.2) continue;
    int spriteScreenX = (int)((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
    int spriteHeight = (int)(0.75 * SCREEN_HEIGHT / transformY);
    if (spriteHeight < 1) spriteHeight = 1;
    int spriteWidth = spriteHeight * 8 / 12;
    if (spriteWidth < 1) spriteWidth = 1;
    int bottomY = SCREEN_HEIGHT / 2 + (int)(SCREEN_HEIGHT / transformY) / 2;
    int topY = bottomY - spriteHeight;
    int leftX = spriteScreenX - spriteWidth / 2;
    int rightX = leftX + spriteWidth;
    if (rightX < 0 || leftX >= SCREEN_WIDTH) continue;
    bool flashing = (millis() - doomEnemies[i].hitTime < 100) && doomEnemies[i].hitTime > 0;
    for (int x = leftX; x < rightX; x++) {
      if (x < 0 || x >= SCREEN_WIDTH) continue;
      if (transformY >= zBuffer[x]) continue;
      int texX = (x - leftX) * 8 / spriteWidth;
      if (texX < 0 || texX >= 8) continue;
      for (int y = topY; y < bottomY; y++) {
        if (y < 0 || y >= SCREEN_HEIGHT) continue;
        int texY = (y - topY) * 12 / spriteHeight;
        if (texY < 0 || texY >= 12) continue;
        if (flashing && ((x + y) & 1)) continue;
        byte rowBits = pgm_read_byte(&enemy_sprite[texY]);
        if ((rowBits >> (7 - texX)) & 1) display.drawPixel(x, y, SSD1306_WHITE);
      }
    }
  }
}

void renderDoomHUD() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  // HP полоска
  int hpBarW = 24;
  display.drawRect(0, 0, hpBarW + 2, 5, SSD1306_WHITE);
  int hpFill = (int)((doomHP / 100.0) * hpBarW);
  if (hpFill > 0) display.fillRect(1, 1, hpFill, 3, SSD1306_WHITE);
  // Kills и Alive и Wave
  display.setCursor(30, 0);
  display.print("A:"); display.print(doomAlive);
  display.setCursor(58, 0);
  display.print("K:"); display.print(doomKills);
  display.setCursor(86, 0);
  display.print("W:"); display.print(doomWave);
  // Прицел
  int cx = SCREEN_WIDTH / 2, cy = SCREEN_HEIGHT / 2;
  display.drawPixel(cx, cy, SSD1306_WHITE);
  display.drawPixel(cx - 1, cy, SSD1306_WHITE);
  display.drawPixel(cx + 1, cy, SSD1306_WHITE);
  display.drawPixel(cx, cy - 1, SSD1306_WHITE);
  display.drawPixel(cx, cy + 1, SSD1306_WHITE);
  // Вспышка выстрела
  if (millis() - doomShootTime < 100) {
    for (int y = SCREEN_HEIGHT - 14; y < SCREEN_HEIGHT - 4; y++) {
      int halfW = (y - (SCREEN_HEIGHT - 14)) / 2 + 2;
      display.drawFastHLine(cx - halfW, y, halfW * 2, SSD1306_WHITE);
    }
  }
}

void updateDoom() {
  bool curL = digitalRead(BUTTON_LEFT) == LOW;
  bool curR = digitalRead(BUTTON_RIGHT) == LOW;
  unsigned long now = millis();

  static bool prevL = false, prevR = false;
  static unsigned long leftDownT = 0, rightDownT = 0;
  static unsigned long tapWindow = 0;
  static int tapCount = 0;

  if (curL && !prevL) leftDownT = now;
  if (curR && !prevR) rightDownT = now;

  // Tap = выстрел
  if (!curL && prevL && (now - leftDownT) < 150 && !curR) {
    doomShoot();
    if (now - tapWindow > 1200) { tapWindow = now; tapCount = 1; } else tapCount++;
  }
  if (!curR && prevR && (now - rightDownT) < 150 && !curL) {
    doomShoot();
    if (now - tapWindow > 1200) { tapWindow = now; tapCount = 1; } else tapCount++;
  }

  if (tapCount >= 4) {
    tapCount = 0;
    prevL = false; prevR = false;
    currentState = MENU;
    waitForButtonRelease();
    return;
  }

  prevL = curL;
  prevR = curR;

  if (gameOver) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(15, 10);
    display.print("YOU DIED");
    display.setTextSize(1);
    display.setCursor(20, 34);
    display.print("Kills: "); display.print(doomKills);
    display.setCursor(20, 44);
    display.print("Wave: "); display.print(doomWave);
    display.setCursor(5, 48); display.print("L: Retry");
    display.setCursor(5, 56); display.print("R: Menu");
    if (digitalRead(BUTTON_LEFT) == LOW) resetDoomGame();
    else if (digitalRead(BUTTON_RIGHT) == LOW) { currentState = MENU; waitForButtonRelease(); }
    return;
  }

  bool heldL = curL && (now - leftDownT > 150);
  bool heldR = curR && (now - rightDownT > 150);

  if (heldL && heldR) {
    doomTryMove(cos(doomAngle) * DOOM_MOVE_SPEED, sin(doomAngle) * DOOM_MOVE_SPEED);
  } else if (heldL) {
    doomAngle -= DOOM_ROT_SPEED;
    if (doomAngle < 0) doomAngle += 2 * PI;
  } else if (heldR) {
    doomAngle += DOOM_ROT_SPEED;
    if (doomAngle > 2 * PI) doomAngle -= 2 * PI;
  }

  updateDoomEnemies();

  doomAlive = 0;
  for (int i = 0; i < doomEnemyCount; i++) if (doomEnemies[i].state == 0) doomAlive++;
  if (doomAlive == 0) {
    doomWave++;
    doomHP += 20;
    if (doomHP > 100) doomHP = 100;
    int n = 3 + doomWave;
    if (n > DOOM_MAX_ENEMIES) n = DOOM_MAX_ENEMIES;
    spawnDoomEnemies(n);
  }

  display.clearDisplay();
  renderWallsWithZBuffer(doomPX, doomPY, doomAngle, DOOM_FOV);
  renderDoomSprites();
  renderDoomHUD();
}

// ================= МЕНЮ =================
void drawMenuItem(int idx, int yOffset) {
  const char* names[] = {"PILLOWS", "FLAPPY", "BREAKOUT", "PONG", "3D MAZE", "HILL CLIMB", "DOOM"};
  const char* descs[] = {"catch falling", "tap to fly", "bricks & ball", "vs computer", "find the exit", "gas & brake", "shoot demons"};
  int textLen = strlen(names[idx]);
  int textW = textLen * 12;
  int x = (SCREEN_WIDTH - textW) / 2;
  if (x < 0) x = 0;
  display.setTextSize(2);
  display.setCursor(x, MENU_CENTER_Y + yOffset);
  display.print(names[idx]);
  int dLen = strlen(descs[idx]);
  int dW = dLen * 6;
  int dx = (SCREEN_WIDTH - dW) / 2;
  if (dx < 0) dx = 0;
  display.setTextSize(1);
  display.setCursor(dx, MENU_CENTER_Y + 20 + yOffset);
  display.print(descs[idx]);
}

void updateMenu() {
  static unsigned long lastPressTime = 0;
  unsigned long currentTime = millis();
  float progress = 1.0;
  if (menuAnimating) {
    progress = (float)(currentTime - menuAnimStart) / MENU_ANIM_DURATION;
    if (progress >= 1.0) { progress = 1.0; menuAnimating = false; }
  }
  float p = progress;
  p = p * p * (3.0 - 2.0 * p);
  if (!menuAnimating && digitalRead(BUTTON_LEFT) == LOW && currentTime - lastPressTime > 250) {
    menuOldSelection = menuSelection;
    menuSelection = (menuSelection + 1) % 7;
    menuAnimStart = currentTime;
    menuAnimating = true;
    lastPressTime = currentTime;
  }
  if (!menuAnimating && digitalRead(BUTTON_RIGHT) == LOW && currentTime - lastPressTime > 250) {
    if      (menuSelection == 0) { currentState = GAME_PILLOWS;  resetPillowsGame(); }
    else if (menuSelection == 1) { currentState = GAME_FLAPPY;   resetFlappyGame(); }
    else if (menuSelection == 2) { currentState = GAME_BREAKOUT; resetBreakoutGame(); }
    else if (menuSelection == 3) { currentState = GAME_PONG;     resetPongGame(); }
    else if (menuSelection == 4) { currentState = GAME_MAZE;     resetMazeGame(); }
    else if (menuSelection == 5) { currentState = GAME_HILL;     resetHillClimbGame(); }
    else                         { currentState = GAME_DOOM;     resetDoomGame(); }
    return;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print("ARCADE");
  if ((currentTime / 400) % 2 == 0 && !menuAnimating) {
    display.setCursor(86, 2);
    display.print("> PLAY");
  }
  display.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);
  if (menuAnimating && menuOldSelection != menuSelection) {
    int oldY = -(int)(p * 55);
    drawMenuItem(menuOldSelection, oldY);
  }
  int newY = 0;
  if (menuAnimating) newY = (int)((1.0 - p) * 55);
  drawMenuItem(menuSelection, newY);
  for (int i = 0; i < 7; i++) {
    int dx = SCREEN_WIDTH / 2 - 24 + i * 8;
    if (i == menuSelection) display.fillCircle(dx, 60, 2, SSD1306_WHITE);
    else                    display.drawCircle(dx, 60, 2, SSD1306_WHITE);
  }
}

// ================= SETUP / LOOP =================
void setup() {
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  #if defined(ESP8266) || defined(ESP32)
    Wire.begin(SDA_PIN, SCL_PIN);
  #else
    Wire.begin();
  #endif
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for (;;);
  randomSeed(analogRead(0));
  display.clearDisplay();
  display.display();
}

void loop() {
  switch (currentState) {
    case MENU:           updateMenu();          break;
    case GAME_PILLOWS:   updatePillows();       break;
    case GAME_FLAPPY:    updateFlappy();        break;
    case GAME_BREAKOUT:  updateBreakout();      break;
    case GAME_PONG:      updatePong();          break;
    case GAME_MAZE:      updateMazeGame();      break;
    case GAME_HILL:      updateHillClimb();     break;
    case GAME_DOOM:      updateDoom();          break;
  }
  display.display();
  delay(30);
}