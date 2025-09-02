#include <Novice.h>
#include <vector>

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
};

// 弾
struct Bullet {
    Vector2 pos;
    float radius;
    int speed;
    bool isAlive;
};

// 敵
struct Enemy {
    Vector2 pos;
    float radius;
    int speed;
    bool isAlive;
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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    const int kWindowWidth = 1280;
    const int kWindowHeight = 720;

    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

    // キー入力
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    // プレイヤー初期化
    Player player = { {kWindowWidth / 2.0f, kWindowHeight - 100.0f}, 20.0f, 8 };

    // 弾リスト
    std::vector<Bullet> bullets;

    // 敵リスト
    std::vector<Enemy> enemies;
    int enemySpawnTimer = 0;
    int lives = 5;  // 境界を超えられる回数

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
                // ゲーム開始
                scene = GAME;
                bullets.clear();
                enemies.clear();
                lives = 5;
                player.pos = { kWindowWidth / 2.0f, kWindowHeight - 100.0f };
            }
            break;

        case GAME: {
            // プレイヤー移動
            if (keys[DIK_A]) {
                player.pos.x -= player.speed;
            }
            if (keys[DIK_D]) {
                player.pos.x += player.speed;
            }
            if (player.pos.x < 0) player.pos.x = 0;
            if (player.pos.x > kWindowWidth) player.pos.x = (float)kWindowWidth;

            // 弾発射
            if (keys[DIK_SPACE] && preKeys[DIK_SPACE] == 0) {
                Bullet b = { {player.pos.x, player.pos.y}, 8.0f, 10, true };
                bullets.push_back(b);
            }

            // 弾更新
            for (auto& b : bullets) {
                if (b.isAlive) {
                    b.pos.y -= b.speed;
                    if (b.pos.y < 0) {
                        b.isAlive = false;
                    }
                }
            }

            // 敵出現
            enemySpawnTimer++;
            if (enemySpawnTimer > 60) {
                enemySpawnTimer = 0;
                Enemy e = { {(float)(rand() % kWindowWidth), 0}, 20.0f, 4, true };
                enemies.push_back(e);
            }

            // 敵更新
            for (auto& e : enemies) {
                if (e.isAlive) {
                    e.pos.y += e.speed;
                    if (e.pos.y > kWindowHeight / 2) {
                        e.isAlive = false;
                        lives--;
                        if (lives <= 0) {
                            scene = OVER;
                        }
                    }
                }
            }

            // 衝突判定（弾と敵）
            for (auto& b : bullets) {
                if (!b.isAlive) continue;
                for (auto& e : enemies) {
                    if (!e.isAlive) continue;
                    float dx = b.pos.x - e.pos.x;
                    float dy = b.pos.y - e.pos.y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < b.radius + e.radius) {
                        b.isAlive = false;
                        e.isAlive = false;
                    }
                }
            }
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
            // 境界線
            Novice::DrawLine(0, kWindowHeight / 2, kWindowWidth, kWindowHeight / 2, WHITE);

            // プレイヤー
            Novice::DrawEllipse((int)player.pos.x, (int)player.pos.y, (int)player.radius,
                (int)player.radius, 0.0f, BLUE, kFillModeSolid);

            // 弾
            for (auto& b : bullets) {
                if (b.isAlive) {
                    Novice::DrawEllipse((int)b.pos.x, (int)b.pos.y, (int)b.radius, (int)b.radius,
                        0.0f, RED, kFillModeSolid);
                }
            }

            // 敵
            for (auto& e : enemies) {
                if (e.isAlive) {
                    Novice::DrawEllipse((int)e.pos.x, (int)e.pos.y, (int)e.radius, (int)e.radius,
                        0.0f, RED, kFillModeSolid);
                }
            }

            // 残りライフ
            Novice::ScreenPrintf(20, 20, "Lives: %d", lives);
            break;

        case CLEAR:
            Novice::ScreenPrintf(500, 400, "CLEAR!");
            break;

        case OVER:
            Novice::ScreenPrintf(500, 400, "GAME OVER");
            Novice::ScreenPrintf(500, 450, "Press SPACE to Retry");
            break;
        }

        Novice::EndFrame();

        // ESCキーで終了
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    Novice::Finalize();
    return 0;
}