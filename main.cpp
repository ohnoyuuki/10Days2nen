#include <Novice.h>
#include <vector>
#include <cmath>

const char kWindowTitle[] = "境界を守れ！";

// 2Dベクトル
struct Vector2 {
    float x;
    float y;
};

// プレイヤー
struct Player {
    Vector2 pos;
    float radius;
    int speed;
    int shotgunLevel;
    int shotgunTimer; // ▼ 修正: ショットガン効果の残り時間を追加
};

// 弾
struct Bullet {
    Vector2 pos;
    float radius;
    int speed;
    Vector2 direction;
    bool isAlive;
};

// 敵
struct Enemy {
    Vector2 pos;
    float radius;
    int speed;
    bool isAlive;
    int hp;
};

// パワーアップアイテム (連射速度アップ)
struct PowerUp {
    Vector2 pos;
    float radius;
    int speed;
    bool isAlive;
    int hp;
};

// 弾数増加アイテム
struct ShotgunPowerUp {
    Vector2 pos;
    float radius;
    int speed;
    bool isAlive;
    int hp;
};

// シーン
enum Scene {
    TITLE,
    EXPLANATION,
    GAME,
    CLEAR,
    OVER,
};

int scene = TITLE;

// 2つのアイテム間の距離を計算する関数
bool IsTooClose(const Vector2& pos1, float radius1, const Vector2& pos2, float radius2) {
    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    float distance = sqrtf(dx * dx + dy * dy);
    return distance < (radius1 + radius2 + 50.0f);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    const int kWindowWidth = 1280;
    const int kWindowHeight = 720;

    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

    // キー入力
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    // プレイヤー初期化
    Player player = { {kWindowWidth / 2.0f, kWindowHeight - 100.0f}, 20.0f, 8, 0, 0 }; // ▼ 修正: shotgunTimerを0で初期化

    // 弾リスト
    std::vector<Bullet> bullets;
    int shotCooldown = 0;
    int defaultShotCooldown = 15;
    int powerUpLevel = 0;

    // 敵リスト
    std::vector<Enemy> enemies;
    int enemySpawnTimer = 0;
    int lives = 20;

    // アイテムリスト
    std::vector<PowerUp> powerUps;
    std::vector<ShotgunPowerUp> shotgunPowerUps;
    int powerUpSpawnTimer = 0;
    int shotgunPowerUpSpawnTimer = 0;

    int minX = 360;
    int maxX = 920;
    int range = maxX - minX;

    // ゲーム時間管理
    int gameTimer = 0;
    const int framePerSecond = 60;
    const int totalTime = 60 * framePerSecond;

    // メインループ
    while (Novice::ProcessMessage() == 0) {
        Novice::BeginFrame();

        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓ 更新処理 ↓
        ///
        switch (scene) {
        case TITLE:
            if (keys[DIK_SPACE]) {
                scene = EXPLANATION;
            }
            break;

        case EXPLANATION:
            if (keys[DIK_SPACE]) {
                scene = GAME;
                bullets.clear();
                enemies.clear();
                lives = 20;
                player.pos = { kWindowWidth / 2.0f, kWindowHeight - 100.0f };
                gameTimer = 0;
                shotCooldown = 0;
                powerUpSpawnTimer = 0;
                shotgunPowerUpSpawnTimer = 0;
                powerUps.clear();
                shotgunPowerUps.clear();
                defaultShotCooldown = 15;
                powerUpLevel = 0;
                player.shotgunLevel = 0;
                player.shotgunTimer = 0; // ▼ 修正: shotgunTimerも初期化
            }
            break;

        case GAME: {
            gameTimer++;
            if (gameTimer >= totalTime) {
                scene = CLEAR;
            }

            if (keys[DIK_A]) { player.pos.x -= player.speed; }
            if (keys[DIK_D]) { player.pos.x += player.speed; }

            float minPlayerX = minX + player.radius;
            float maxPlayerX = maxX - player.radius;
            if (player.pos.x < minPlayerX) player.pos.x = minPlayerX;
            if (player.pos.x > maxPlayerX) player.pos.x = maxPlayerX;

            if (keys[DIK_SPACE] && shotCooldown == 0) {
                if (player.shotgunLevel == 0) {
                    bullets.push_back({ {player.pos.x, player.pos.y}, 8.0f, 15, {0.0f, -1.0f}, true });
                } else {
                    float angleIncrement = 0.3f;
                    bullets.push_back({ {player.pos.x, player.pos.y}, 8.0f, 15, {0.0f, -1.0f}, true });
                    for (int i = 1; i <= player.shotgunLevel; ++i) {
                        float angle = angleIncrement * i;
                        bullets.push_back({ {player.pos.x, player.pos.y}, 8.0f, 15, {angle, -1.0f}, true });
                        bullets.push_back({ {player.pos.x, player.pos.y}, 8.0f, 15, {-angle, -1.0f}, true });
                    }
                }
                shotCooldown = defaultShotCooldown;
            }

            if (shotCooldown > 0) shotCooldown--;

            for (auto& b : bullets) {
                if (b.isAlive) {
                    b.pos.x += b.direction.x * b.speed;
                    b.pos.y += b.direction.y * b.speed;
                    if (b.pos.y < 0) b.isAlive = false;
                }
            }

            enemySpawnTimer++;
            if (enemySpawnTimer > 60) {
                enemySpawnTimer = 0;
                float enemyRadius = 20.0f;
                int safeMinX = minX + (int)player.radius;
                int safeRange = range - (int)(player.radius * 2);
                int timeInSeconds = gameTimer / framePerSecond;
                int enemyInitialHP = 0;
                if (timeInSeconds <= 10) {
                    enemyInitialHP = 1;
                } else {
                    int hpIncrease = (timeInSeconds / 10) * 5;
                    enemyInitialHP = 1 + hpIncrease;
                }
                Enemy e = { {(float)(safeMinX + rand() % safeRange), 0.0f}, enemyRadius, 2, true, enemyInitialHP };
                enemies.push_back(e);
            }

            for (auto& e : enemies) {
                if (e.isAlive) {
                    e.pos.y += e.speed;
                    if (e.pos.y > kWindowHeight - 200) {
                        e.isAlive = false;
                        lives--;
                        if (lives <= 0) scene = OVER;
                    }
                }
            }

            powerUpSpawnTimer++;
            if (powerUpSpawnTimer >= 5 * framePerSecond) {
                powerUpSpawnTimer = 0;
                PowerUp p = { {0.0f, 0.0f}, 15.0f, 3, true, 3 };
                bool spawnable = false;
                int maxAttempts = 10;
                for (int i = 0; i < maxAttempts; ++i) {
                    float newX = (float)(minX + rand() % range);
                    p.pos = { newX, 0.0f };
                    spawnable = true;
                    for (const auto& existingP : powerUps) {
                        if (IsTooClose(p.pos, p.radius, existingP.pos, existingP.radius)) {
                            spawnable = false;
                            break;
                        }
                    }
                    if (spawnable) {
                        for (const auto& existingS : shotgunPowerUps) {
                            if (IsTooClose(p.pos, p.radius, existingS.pos, existingS.radius)) {
                                spawnable = false;
                                break;
                            }
                        }
                    }
                    if (spawnable) {
                        break;
                    }
                }
                if (spawnable) {
                    powerUps.push_back(p);
                }
            }

            shotgunPowerUpSpawnTimer++;
            if (shotgunPowerUpSpawnTimer >= 10 * framePerSecond) {
                shotgunPowerUpSpawnTimer = 0;
                ShotgunPowerUp s = { {0.0f, 0.0f}, 15.0f, 3, true, 5 };
                bool spawnable = false;
                int maxAttempts = 10;
                for (int i = 0; i < maxAttempts; ++i) {
                    float newX = (float)(minX + rand() % range);
                    s.pos = { newX, 0.0f };
                    spawnable = true;
                    for (const auto& existingP : powerUps) {
                        if (IsTooClose(s.pos, s.radius, existingP.pos, existingP.radius)) {
                            spawnable = false;
                            break;
                        }
                    }
                    if (spawnable) {
                        for (const auto& existingS : shotgunPowerUps) {
                            if (IsTooClose(s.pos, s.radius, existingS.pos, existingS.radius)) {
                                spawnable = false;
                                break;
                            }
                        }
                    }
                    if (spawnable) {
                        break;
                    }
                }
                if (spawnable) {
                    shotgunPowerUps.push_back(s);
                }
            }

            for (auto& p : powerUps) {
                if (p.isAlive) {
                    p.pos.y += p.speed;
                }
            }

            for (auto& s : shotgunPowerUps) {
                if (s.isAlive) {
                    s.pos.y += s.speed;
                }
            }

            for (auto& b : bullets) {
                if (!b.isAlive) continue;
                for (auto& e : enemies) {
                    if (!e.isAlive) continue;
                    float dx = b.pos.x - e.pos.x;
                    float dy = b.pos.y - e.pos.y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < b.radius + e.radius) {
                        b.isAlive = false;
                        e.hp -= 1;
                        if (e.hp <= 0) e.isAlive = false;
                    }
                }
                for (auto& p : powerUps) {
                    if (!p.isAlive) continue;
                    float dx = b.pos.x - p.pos.x;
                    float dy = b.pos.y - p.pos.y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < b.radius + p.radius) {
                        b.isAlive = false;
                        p.hp -= 1;
                        if (p.hp <= 0) {
                            p.isAlive = false;
                            powerUpLevel++;
                            if (powerUpLevel == 1) defaultShotCooldown = 10;
                            else if (powerUpLevel == 2) defaultShotCooldown = 8;
                            else if (powerUpLevel == 3) defaultShotCooldown = 6;
                            else defaultShotCooldown = 4;
                        }
                    }
                }
                for (auto& s : shotgunPowerUps) {
                    if (!s.isAlive) continue;
                    float dx = b.pos.x - s.pos.x;
                    float dy = b.pos.y - s.pos.y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < b.radius + s.radius) {
                        b.isAlive = false;
                        s.hp -= 1;
                        if (s.hp <= 0) {
                            s.isAlive = false;
                            player.shotgunLevel++;
                            player.shotgunTimer = 6 * framePerSecond; // ▼ 修正: ショットガン効果時間を設定
                        }
                    }
                }
            }

            // ▼ 修正: ショットガンタイマーの更新
            if (player.shotgunTimer > 0) {
                player.shotgunTimer--;
                if (player.shotgunTimer <= 0) {
                    player.shotgunLevel = 0;
                }
            }
            // ▲ 修正終わり

        } break;

        case CLEAR:

            break;

        case OVER:
            if (keys[DIK_SPACE]) {
                scene = TITLE;
            }
            break;
        }

        ///
        /// ↓ 描画処理 ↓
        ///
        int elapsedMinutes = gameTimer / framePerSecond / 60;
        int elapsedSeconds = gameTimer / framePerSecond % 60;

        switch (scene) {
        case TITLE:
            Novice::ScreenPrintf(500, 300, "境界を守れ！");
            Novice::ScreenPrintf(500, 350, "Press SPACE to Start");
            break;

        case EXPLANATION:
            Novice::ScreenPrintf(400, 300, "ルール説明:");
            Novice::ScreenPrintf(400, 340, "・←→キーで移動");
            Novice::ScreenPrintf(400, 380, "・SPACEで弾を撃つ");
            Novice::ScreenPrintf(400, 420, "・敵が境界(画面中央の白線)を超えないよう守ろう！");
            Novice::ScreenPrintf(400, 500, "Press SPACE to Play");
            break;

        case GAME:
            Novice::DrawLine(0, kWindowHeight - 200, kWindowWidth, kWindowHeight - 200, WHITE);
            Novice::DrawEllipse((int)player.pos.x, (int)player.pos.y, (int)player.radius, (int)player.radius, 0.0f, BLUE, kFillModeSolid);
            for (auto& b : bullets) {
                if (b.isAlive) {
                    Novice::DrawEllipse((int)b.pos.x, (int)b.pos.y, (int)b.radius, (int)b.radius, 0.0f, RED, kFillModeSolid);
                }
            }
            for (auto& e : enemies) {
                if (e.isAlive) {
                    Novice::DrawEllipse((int)e.pos.x, (int)e.pos.y, (int)e.radius, (int)e.radius, 0.0f, RED, kFillModeSolid);
                    Novice::ScreenPrintf((int)e.pos.x - 10, (int)e.pos.y - 30, "HP:%d", e.hp);
                }
            }
            for (auto& p : powerUps) {
                if (p.isAlive) {
                    Novice::DrawEllipse((int)p.pos.x, (int)p.pos.y, (int)p.radius, (int)p.radius, 0.0f, WHITE, kFillModeSolid);
                    Novice::ScreenPrintf((int)p.pos.x - 10, (int)p.pos.y - 30, "HP:%d", p.hp);
                }
            }
            for (auto& s : shotgunPowerUps) {
                if (s.isAlive) {
                    Novice::DrawEllipse((int)s.pos.x, (int)s.pos.y, (int)s.radius, (int)s.radius, 0.0f, WHITE, kFillModeSolid);
                    Novice::ScreenPrintf((int)s.pos.x - 10, (int)s.pos.y - 30, "HP:%d", s.hp);
                }
            }
            Novice::ScreenPrintf(20, 20, "Lives: %d", lives);
            Novice::ScreenPrintf(20, 40, "Time: %d:%02d", elapsedMinutes, elapsedSeconds);
            Novice::ScreenPrintf(20, 60, "Shot Speed Level: %d", powerUpLevel);
            Novice::ScreenPrintf(20, 80, "Shotgun Level: %d", player.shotgunLevel);
            Novice::ScreenPrintf(20, 100, "Shotgun Timer: %d", player.shotgunTimer / framePerSecond); // ▼ 修正: ショットガン効果時間を表示
            Novice::DrawBox(0, 0, minX, kWindowHeight, 0.0f, BLACK, kFillModeSolid);
            Novice::DrawBox(maxX + 1, 0, kWindowWidth - maxX, kWindowHeight, 0.0f, BLACK, kFillModeSolid);
            break;

        case CLEAR:
            Novice::ScreenPrintf(500, 400, "CLEAR!");
            Novice::ScreenPrintf(500, 450, "Clear Time: %d:%02d", elapsedMinutes, elapsedSeconds);
            break;

        case OVER:
            Novice::ScreenPrintf(500, 400, "GAME OVER");
            Novice::ScreenPrintf(500, 450, "Time: %d:%02d", elapsedMinutes, elapsedSeconds);
            Novice::ScreenPrintf(500, 500, "Press SPACE to Retry");
            break;
        }

        Novice::EndFrame();
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }
    Novice::Finalize();
    return 0;
}