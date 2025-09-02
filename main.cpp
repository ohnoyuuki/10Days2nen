#include <Novice.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

const char kWindowTitle[] = "境界を守れ！";
const float kPi = 3.14159265f;

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
    int shotLevel;     // 発射弾数レベル（無限増加可）
    float bulletSpeed; // 新しく作る弾の速度（アイテムで増やす）
};

// 弾（角度を持つ）
struct Bullet {
    Vector2 pos;
    float radius;
    float speed;
    bool isAlive;
    float angle; // 発射角度（度数法）
};

// 敵
struct Enemy {
    Vector2 pos;
    float radius;
    int speed;
    bool isAlive;
};

// アイテム
struct Item {
    Vector2 pos;
    float radius;
    int speed;
    int hp;         // 耐久値（弾5発で壊れる）
    bool isAlive;   // 出現中かどうか
    bool isBroken;  // HP0になって取得可能になったか
    int type;       // 0=弾速アップ, 1=弾数アップ(無制限)
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

    // 乱数初期化
    std::srand((unsigned int)std::time(nullptr));

    // キー入力
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    // プレイヤー初期化
    Player player = { {kWindowWidth / 2.0f, kWindowHeight - 100.0f}, 20.0f, 8, 1, 10.0f };

    // 弾リスト
    std::vector<Bullet> bullets;
    // 発射間隔を管理する変数
    int shotCooldown = 0;

    //アイテムリスト
    std::vector<Item> items;
    int itemSpawnTimer = 0;

    // 敵リスト
    std::vector<Enemy> enemies;
    int enemySpawnTimer = 0;
    int lives = 5;  // 境界を超えられる回数

    int minX = 400;   // 出現範囲の左端
    int maxX = 800;   // 出現範囲の右端
    int range = maxX - minX;

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
                items.clear();
                lives = 5;
                player.pos = { kWindowWidth / 2.0f, kWindowHeight - 100.0f };
                player.shotLevel = 1;
                player.bulletSpeed = 10.0f;
            }
            if(keys[DIK_BACKSPACE]){
                scene = TITLE;
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

            // クールタイム減少
            if (shotCooldown > 0) {
                shotCooldown--;
            }

            // 弾発射（扇状）
            if (keys[DIK_SPACE] && shotCooldown == 0) {
                int n = player.shotLevel;  // 発射弾数
                float angleRange = 60.0f;  // 扇の広がり角度（度）
                float baseAngle = -90.0f;  // 上方向を基準（yが下方向の座標系なので -90 = 真上）

                if (n <= 1) {
                    // 1発のときは中央に撃つ
                    bullets.push_back({ {player.pos.x, player.pos.y}, 8.0f, player.bulletSpeed, true, baseAngle });
                } else {
                    for (int i = 0; i < n; i++) {
                        // 弾ごとの角度を計算（等間隔）
                        float angle = baseAngle - angleRange / 2.0f + angleRange * i / (float)(n - 1);
                        bullets.push_back({ {player.pos.x, player.pos.y}, 8.0f, player.bulletSpeed, true, angle });
                    }
                }

                // クールタイム（発射間隔）
                shotCooldown = 10;
            }

            // 弾更新
            for (auto& b : bullets) {
                if (b.isAlive) {
                    float rad = b.angle * kPi / 180.0f;
                    b.pos.x += cosf(rad) * b.speed;
                    b.pos.y += sinf(rad) * b.speed;

                    // 画面外に出たら無効化
                    if (b.pos.x < 0 || b.pos.x > kWindowWidth || b.pos.y < 0 || b.pos.y > kWindowHeight) {
                        b.isAlive = false;
                    }
                }
            }

            // 敵出現
            enemySpawnTimer++;
            if (enemySpawnTimer > 60) {
                enemySpawnTimer = 0;
                Enemy e = { {(float)(minX + rand() % range), 0}, 20.0f, 4, true };
                enemies.push_back(e);
            }

            // 敵更新
            for (auto& e : enemies) {
                if (e.isAlive) {
                    e.pos.y += e.speed;
                    if (e.pos.y > kWindowHeight - 200) {
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

            // アイテム出現
            itemSpawnTimer++;
            if (itemSpawnTimer > 300) { // 5秒に1回くらい
                itemSpawnTimer = 0;
                int itemType = rand() % 2; // 0=弾速アップ,1=弾数アップ
                Item item = { {(float)(minX + rand() % range), 0}, 25.0f, 2, 5, true, false, itemType };
                items.push_back(item);
            }

            // アイテム更新（HP残っていても動く。境界で消える）
            for (auto& it : items) {
                if (it.isAlive) {
                    it.pos.y += it.speed;
                    if (it.pos.y > kWindowHeight - 0) {
                        it.isAlive = false;
                    }
                }
            }

            // 弾とアイテムの当たり判定（壊す）
            for (auto& b : bullets) {
                if (!b.isAlive) continue;
                for (auto& it : items) {
                    if (!it.isAlive || it.isBroken) continue;
                    float dx = b.pos.x - it.pos.x;
                    float dy = b.pos.y - it.pos.y;
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < b.radius + it.radius) {
                        b.isAlive = false;
                        it.hp--;
                        if (it.hp <= 0) {
                            it.isBroken = true; // 取得可能状態へ
                        }
                    }
                }
            }

            // プレイヤーとアイテムの当たり判定（取得）
            for (auto& it : items) {
                if (!it.isAlive || !it.isBroken) continue;
                float dx = player.pos.x - it.pos.x;
                float dy = player.pos.y - it.pos.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist < player.radius + it.radius) {
                    it.isAlive = false; // 消える

                    if (it.type == 0) {
                        // 弾速アップ：既存の弾にも、新しい弾にも効果を与える
                        player.bulletSpeed += 5.0f;
                        for (auto& b : bullets) {
                            if (b.isAlive) b.speed += 5.0f;
                        }
                    } else if (it.type == 1) {
                        // 弾数アップ（無制限で増える）
                        player.shotLevel++;
                    }
                }
            }

        } break;

        case CLEAR:
            break;

        case OVER:
            if (keys[DIK_RETURN]) {
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
            Novice::ScreenPrintf(400, 340, "・A/Dキーで移動");
            Novice::ScreenPrintf(400, 380, "・SPACEで弾を撃つ（長押し可）");
            Novice::ScreenPrintf(400, 420, "・敵が境界(画面下)を超えないよう守ろう！");
            Novice::ScreenPrintf(400, 500, "Press SPACE to Play");
            break;

        case GAME:
            // 境界線
            Novice::DrawLine(0, kWindowHeight - 200, kWindowWidth, kWindowHeight - 200, WHITE);
            Novice::DrawLine(400, 0, 400, kWindowHeight, WHITE);
            Novice::DrawLine(800, 0, 800, kWindowHeight, WHITE);

            // プレイヤー
            Novice::DrawEllipse((int)player.pos.x, (int)player.pos.y, (int)player.radius,
                (int)player.radius, 0.0f, BLUE, kFillModeSolid);

            // 弾描画
            for (auto& b : bullets) {
                if (b.isAlive) {
                    Novice::DrawEllipse((int)b.pos.x, (int)b.pos.y, (int)b.radius, (int)b.radius,
                        0.0f, RED, kFillModeSolid);
                }
            }

            // 敵描画
            for (auto& e : enemies) {
                if (e.isAlive) {
                    Novice::DrawEllipse((int)e.pos.x, (int)e.pos.y, (int)e.radius, (int)e.radius,
                        0.0f, RED, kFillModeSolid);
                }
            }

            // アイテム描画
            for (auto& it : items) {
                if (!it.isAlive) continue;

                unsigned int color;
                if (!it.isBroken) {
                    color = (it.type == 0) ? GREEN : BLACK;
                } else {
                    color = BLUE; // 取得可能
                }
                Novice::DrawEllipse((int)it.pos.x, (int)it.pos.y, (int)it.radius, (int)it.radius,
                    0.0f, color, kFillModeSolid);

                // HP表示
                if (!it.isBroken) {
                    Novice::ScreenPrintf((int)it.pos.x - 10, (int)it.pos.y - 40, "HP:%d", it.hp);
                }
            }

            // UI
            Novice::ScreenPrintf(20, 20, "Lives: %d", lives);
            Novice::ScreenPrintf(20, 40, "ShotLevel: %d", player.shotLevel);
            Novice::ScreenPrintf(20, 60, "BulletSpeed: %.1f", player.bulletSpeed);

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