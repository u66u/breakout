#include <raylib.h>
#include <cmath>
#include <vector>

const int WIDTH = 800;
const int HEIGHT = 600;
const float INITIAL_BALL_SPEED = 600.0f;
const float INITIAL_PADDLE_SPEED = 550.0f;
const float MAX_BALL_SPEED = 800.0f;
const float MAX_PADDLE_SPEED = 750.0f;
const float BALL_SPEED_INCREMENT = 2.0f;
const float PADDLE_SPEED_INCREMENT = 2.0f;
const int BRICK_ROWS = 5;
const int BRICK_COLS = 8;
const float BRICK_PADDING = 4.0f;

float CURRENT_BALL_SPEED = INITIAL_BALL_SPEED;
int WAVE_MULTIPLIER = 1;
int BRICKS_DESTROYED = 0;
Music BACKGROUND_MUSIC;
Sound PADDLE_SOUND;
Sound WALL_SOUND;
Sound BRICK_SOUND;
Sound NEW_BLOCKS_SOUND;
float CURRENT_PADDLE_SPEED = INITIAL_PADDLE_SPEED;

struct Ball
{
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;
};

struct Paddle
{
    Rectangle rect;
    float speed;
    Color color;
};

struct Brick
{
    Rectangle rect;
    Color color;
    int points;
    bool active;
};

struct GameState
{
    std::vector<Brick> bricks;
    int score;
    bool gameOver;
};

void UpdatePaddle(Paddle &paddle, float deltaTime);
void UpdateBall(Ball &ball, Paddle &paddle, GameState &gameState, float deltaTime);
void CreateBricks(GameState &gameState);
void DrawGame(Ball &ball, Paddle &paddle, GameState &gameState);
bool CheckBrickCollision(Ball &ball, Brick &brick);
void NormalizeBallVelocity(Ball &ball);
void LoadGameAudio();
void UnloadGameAudio();

Ball CreateBall()
{
    float randomAngle = (GetRandomValue(-30, 30) * PI) / 180.0f;
    return {
        {WIDTH / 2.0f, HEIGHT - 60.0f},
        {CURRENT_BALL_SPEED * sin(randomAngle), -CURRENT_BALL_SPEED * cos(randomAngle)},
        15.0f,
        RED};
}

Paddle CreatePaddle()
{
    return {
        {WIDTH / 2.0f - 50, HEIGHT - 40, 100, 20},
        CURRENT_PADDLE_SPEED,
        BLUE};
}

void CreateBricks(GameState &gameState)
{
    const float BRICK_WIDTH = (WIDTH - 100) / BRICK_COLS;
    const float BRICK_HEIGHT = 30;
    const float START_X = 50;
    const float START_Y = 50;

    gameState.bricks.clear();

    Color BRICK_COLORS[BRICK_ROWS] = {RED, ORANGE, YELLOW, GREEN, BLUE};

    for (int row = 0; row < BRICK_ROWS; row++)
    {
        for (int col = 0; col < BRICK_COLS; col++)
        {
            Brick brick = {
                {START_X + col * BRICK_WIDTH,
                 START_Y + row * BRICK_HEIGHT,
                 BRICK_WIDTH - BRICK_PADDING,
                 BRICK_HEIGHT - BRICK_PADDING},
                BRICK_COLORS[row],
                (BRICK_ROWS - row) * 100,
                true};
            gameState.bricks.push_back(brick);
        }
    }
}

void UpdatePaddle(Paddle &paddle, float deltaTime)
{
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    {
        paddle.rect.x -= paddle.speed * deltaTime;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
    {
        paddle.rect.x += paddle.speed * deltaTime;
    }

    if (paddle.rect.x < 0)
        paddle.rect.x = 0;
    if (paddle.rect.x + paddle.rect.width > WIDTH)
    {
        paddle.rect.x = WIDTH - paddle.rect.width;
    }
}

void HandleWallCollision(Ball &ball)
{
    if (ball.position.x - ball.radius <= 0 || ball.position.x + ball.radius >= WIDTH)
    {
        ball.velocity.x *= -1;
        ball.position.x = (ball.position.x - ball.radius <= 0) ? ball.radius : WIDTH - ball.radius;
        NormalizeBallVelocity(ball);
        PlaySound(WALL_SOUND);
    }
    if (ball.position.y - ball.radius <= 0)
    {
        ball.velocity.y *= -1;
        ball.position.y = ball.radius;
        NormalizeBallVelocity(ball);
        PlaySound(WALL_SOUND);
    }
}

void HandlePaddleCollision(Ball &ball, const Paddle &paddle)
{
    if (!CheckCollisionCircleRec(ball.position, ball.radius, paddle.rect))
        return;

    PlaySound(PADDLE_SOUND);

    float hitPosition = (ball.position.x - paddle.rect.x) / paddle.rect.width;
    float angle = (hitPosition - 0.5f) * PI / 3.0f;

    float currentSpeed = sqrt(ball.velocity.x * ball.velocity.x + ball.velocity.y * ball.velocity.y);
    ball.velocity = {
        currentSpeed * sin(angle),
        -currentSpeed * cos(angle)};
    ball.position.y = paddle.rect.y - ball.radius - 1;
    NormalizeBallVelocity(ball);
}

bool HandleBrickCollision(Ball &ball, Brick &brick, GameState &gameState)
{
    if (!brick.active || !CheckCollisionCircleRec(ball.position, ball.radius, brick.rect))
        return false;

    PlaySound(BRICK_SOUND);

    brick.active = false;
    gameState.score += brick.points * WAVE_MULTIPLIER;
    BRICKS_DESTROYED++;

    if (CURRENT_PADDLE_SPEED < MAX_PADDLE_SPEED)
    {
        CURRENT_PADDLE_SPEED += PADDLE_SPEED_INCREMENT;
    }
    if (CURRENT_BALL_SPEED < MAX_BALL_SPEED)
    {
        CURRENT_BALL_SPEED += BALL_SPEED_INCREMENT;
        NormalizeBallVelocity(ball);
    }

    Vector2 brickCenter = {
        brick.rect.x + brick.rect.width / 2,
        brick.rect.y + brick.rect.height / 2};

    bool hitVertical = abs(ball.position.x - brickCenter.x) / (brick.rect.width / 2) >
                       abs(ball.position.y - brickCenter.y) / (brick.rect.height / 2);

    if (hitVertical)
    {
        ball.velocity.x *= -1;
        ball.position.x = ball.position.x < brickCenter.x ? brick.rect.x - ball.radius : brick.rect.x + brick.rect.width + ball.radius;
    }
    else
    {
        ball.velocity.y *= -1;
        ball.position.y = ball.position.y < brickCenter.y ? brick.rect.y - ball.radius : brick.rect.y + brick.rect.height + ball.radius;
    }

    return true;
}

bool CheckBrickCollision(Ball &ball, Brick &brick)
{
    if (!brick.active)
        return false;

    if (CheckCollisionCircleRec(ball.position, ball.radius, brick.rect))
    {
        brick.active = false;

        Vector2 brickCenter = {
            brick.rect.x + brick.rect.width / 2,
            brick.rect.y + brick.rect.height / 2};

        float dx = abs(ball.position.x - brickCenter.x);
        float dy = abs(ball.position.y - brickCenter.y);

        if (dx / (brick.rect.width / 2) > dy / (brick.rect.height / 2))
        {
            ball.velocity.x *= -1;
            if (ball.position.x < brick.rect.x)
                ball.position.x = brick.rect.x - ball.radius;
            else
                ball.position.x = brick.rect.x + brick.rect.width + ball.radius;
        }
        else
        {
            ball.velocity.y *= -1;
            if (ball.position.y < brick.rect.y)
                ball.position.y = brick.rect.y - ball.radius;
            else
                ball.position.y = brick.rect.y + brick.rect.height + ball.radius;
        }
        return true;
    }
    return false;
}

void NormalizeBallVelocity(Ball &ball)
{
    float speed = sqrt(ball.velocity.x * ball.velocity.x + ball.velocity.y * ball.velocity.y);
    ball.velocity.x = (ball.velocity.x / speed) * CURRENT_BALL_SPEED;
    ball.velocity.y = (ball.velocity.y / speed) * CURRENT_BALL_SPEED;
}

void UpdateBall(Ball &ball, Paddle &paddle, GameState &gameState, float deltaTime)
{
    ball.position.x += ball.velocity.x * deltaTime;
    ball.position.y += ball.velocity.y * deltaTime;

    HandleWallCollision(ball);
    HandlePaddleCollision(ball, paddle);

    bool allBricksDestroyed = true;
    for (auto &brick : gameState.bricks)
    {
        if (brick.active)
        {
            allBricksDestroyed = false;
            if (HandleBrickCollision(ball, brick, gameState))
            {
                break;
            }
        }
    }

    if (allBricksDestroyed)
    {
        WAVE_MULTIPLIER *= 2;
        CreateBricks(gameState);
        PlaySound(NEW_BLOCKS_SOUND);
    }

    if (ball.position.y + ball.radius >= HEIGHT)
    {
        gameState.gameOver = true;
    }
}

void DrawGame(Ball &ball, Paddle &paddle, GameState &gameState)
{
    BeginDrawing();
    ClearBackground(BLACK);

    for (const auto &brick : gameState.bricks)
    {
        if (brick.active)
        {
            DrawRectangleRec(brick.rect, brick.color);
        }
    }

    DrawCircleV(ball.position, ball.radius, ball.color);
    DrawRectangleRec(paddle.rect, paddle.color);

    DrawText(TextFormat("Score: %d", gameState.score), 10, HEIGHT - 30, 20, WHITE);

    if (gameState.gameOver)
    {
        const char *gameOverText = "Game Over!";
        int textWidth = MeasureText(gameOverText, 60);
        DrawText(gameOverText, WIDTH / 2 - textWidth / 2, HEIGHT / 2 - 30, 60, RED);
    }

    EndDrawing();
}

void LoadGameAudio()
{
    InitAudioDevice();

    BACKGROUND_MUSIC = LoadMusicStream("res/bg.mp3");
    PADDLE_SOUND = LoadSound("res/paddle.wav");
    WALL_SOUND = LoadSound("res/wall.wav");
    BRICK_SOUND = LoadSound("res/brick.wav");
    NEW_BLOCKS_SOUND = LoadSound("res/new_blocks.mp3");

    SetMusicVolume(BACKGROUND_MUSIC, 0.5f);
}

void UnloadGameAudio()
{
    UnloadMusicStream(BACKGROUND_MUSIC);
    UnloadSound(PADDLE_SOUND);
    UnloadSound(WALL_SOUND);
    UnloadSound(BRICK_SOUND);
    UnloadSound(NEW_BLOCKS_SOUND);
    CloseAudioDevice();
}

int main()
{
    InitWindow(WIDTH, HEIGHT, "Breakout Game");
    SetTargetFPS(60);
    LoadGameAudio();

    Ball ball;
    Paddle paddle;
    GameState gameState;
    bool exitGame = false;
    bool gameStarted = false;

    while (!WindowShouldClose() && !exitGame)
    {
        if (!gameStarted)
        {
            StopMusicStream(BACKGROUND_MUSIC);

            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Press ENTER to Start", WIDTH / 2 - 150, HEIGHT / 2 - 50, 30, WHITE);
            DrawText("Press ESC to Exit", WIDTH / 2 - 120, HEIGHT / 2, 30, WHITE);
            EndDrawing();

            if (IsKeyPressed(KEY_ENTER))
            {
                gameStarted = true;
                PlayMusicStream(BACKGROUND_MUSIC);

                ball = CreateBall();
                paddle = CreatePaddle();
                gameState = {std::vector<Brick>(), 0, false};
                CreateBricks(gameState);
            }
            else if (IsKeyPressed(KEY_ESCAPE))
            {
                exitGame = true;
            }
        }
        else if (!gameState.gameOver)
        {
            UpdateMusicStream(BACKGROUND_MUSIC);

            float deltaTime = GetFrameTime();
            UpdatePaddle(paddle, deltaTime);
            UpdateBall(ball, paddle, gameState, deltaTime);
            DrawGame(ball, paddle, gameState);
        }
        else
        {
            StopMusicStream(BACKGROUND_MUSIC);

            BeginDrawing();
            ClearBackground(BLACK);
            DrawText(TextFormat("Final Score: %d", gameState.score), WIDTH / 2 - 100, HEIGHT / 2 - 50, 30, WHITE);
            DrawText("Press ENTER to Restart", WIDTH / 2 - 150, HEIGHT / 2, 30, WHITE);
            DrawText("Press ESC to Exit", WIDTH / 2 - 120, HEIGHT / 2 + 50, 30, WHITE);
            EndDrawing();

            if (IsKeyPressed(KEY_ENTER))
            {
                gameStarted = false;
            }
            else if (IsKeyPressed(KEY_ESCAPE))
            {
                exitGame = true;
            }
        }
    }

    UnloadGameAudio();
    CloseWindow();
    return 0;
}